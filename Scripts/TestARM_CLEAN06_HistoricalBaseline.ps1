[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$ManifestPath,
    [Parameter(Mandatory)][string]$ReceiptPath,
    [string]$NativeAuditPath,
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$SourceCommit = 'c66bf68aa1a42eb8d9c47eb9b9336214d0bccf95'
$ManifestSHA256 = '03c3b09c1430fc8938e4c4bf1cde6fb229334aeda2348ed902c359c4edca6940'

function Read-VerifiedBaseline([string]$Manifest, [string]$Receipt) {
    if (-not (Test-Path -LiteralPath $Manifest -PathType Leaf)) { throw 'Historical manifest missing.' }
    if (-not (Test-Path -LiteralPath $Receipt -PathType Leaf)) { throw 'Historical receipt missing.' }
    $Proof = Get-Content -LiteralPath $Receipt -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($Proof.schema -cne 'ARM-CLEAN06.0/asset-hashes/v1' -or $Proof.git_sha -cne $SourceCommit -or
        $Proof.manifest_sha256 -cne $ManifestSHA256) { throw 'Historical provenance mismatch.' }

    # Verify and decode the same byte array.
    $Bytes = [IO.File]::ReadAllBytes((Resolve-Path -LiteralPath $Manifest).ProviderPath)
    $Hasher = [Security.Cryptography.SHA256]::Create()
    try {
        $Hash = [BitConverter]::ToString($Hasher.ComputeHash($Bytes)).Replace('-', '').ToLowerInvariant()
    } finally { $Hasher.Dispose() }
    if ($Hash -cne $ManifestSHA256) { throw 'Historical manifest SHA-256 mismatch.' }

    $Data = [Text.Encoding]::UTF8.GetString($Bytes).TrimStart([char]0xFEFF) | ConvertFrom-Json
    if ($Data.schema -cne 'ARM-CLEAN06.0/read-only/v1' -or @($Data.assets).Count -ne 56 -or
        @($Proof.assets).Count -ne 56 -or @($Data.expanded_profiles).Count -ne 1640 -or
        $Data.counts.profiles -ne 1640 -or $Data.counts.passive_profiles -ne 260) {
        throw 'Historical manifest schema or cardinalities invalid.'
    }
    $Passive = 0
    for ($Index = 0; $Index -lt 1640; $Index++) {
        $Row = $Data.expanded_profiles[$Index]
        $Id = 'ARM{0:D3}' -f (1 + [int][Math]::Floor($Index / 20))
        if ($Row.ArchetypeId -cne $Id -or [int]$Row.ItemLevel -ne (1 + $Index % 20)) {
            throw 'Historical manifest profile coverage invalid.'
        }
        if ($Row.DamageDice -ceq '()') { $Passive++ }
    }
    if ($Passive -ne 260) { throw 'Historical manifest passive coverage invalid.' }
    return $Data
}

function Assert-Rejected([string]$Manifest, [string]$Receipt, [string]$Expected) {
    try { $null = Read-VerifiedBaseline $Manifest $Receipt }
    catch {
        if ($_.Exception.Message -eq $Expected) { return }
        throw
    }
    throw "Invalid baseline accepted. Expected $Expected"
}

$Baseline = Read-VerifiedBaseline $ManifestPath $ReceiptPath
if ($SelfTest) {
    $Temporary = Join-Path ([IO.Path]::GetTempPath()) ('GrimrockHistoricalBaseline-' + [guid]::NewGuid())
    try {
        New-Item -ItemType Directory -Path $Temporary | Out-Null
        $Tampered = Join-Path $Temporary 'tampered.json'
        $WrongReceipt = Join-Path $Temporary 'wrong-receipt.json'
        Assert-Rejected (Join-Path $Temporary 'missing.json') $ReceiptPath 'Historical manifest missing.'
        Assert-Rejected $ManifestPath (Join-Path $Temporary 'missing.json') 'Historical receipt missing.'
        $Source = (Resolve-Path -LiteralPath $ManifestPath).ProviderPath
        $Bytes = [IO.File]::ReadAllBytes($Source)
        $Bytes[$Bytes.Length - 1] = $Bytes[$Bytes.Length - 1] -bxor 1
        [IO.File]::WriteAllBytes($Tampered, $Bytes)
        Assert-Rejected $Tampered $ReceiptPath 'Historical manifest SHA-256 mismatch.'
        (Get-Content -LiteralPath $ReceiptPath -Raw).Replace($SourceCommit, ('0' * 40)) |
            Set-Content -LiteralPath $WrongReceipt -Encoding UTF8
        Assert-Rejected $ManifestPath $WrongReceipt 'Historical provenance mismatch.'
    } finally {
        foreach ($Name in @('tampered.json', 'wrong-receipt.json')) {
            $File = Join-Path $Temporary $Name
            if ([IO.File]::Exists($File)) { [IO.File]::Delete($File) }
        }
        if ([IO.Directory]::Exists($Temporary)) { [IO.Directory]::Delete($Temporary) }
    }
}

if ($NativeAuditPath) {
    $Current = Get-Content -LiteralPath $NativeAuditPath -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($Current.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or
        @($Current.native.weapon_profiles).Count -ne 1640) { throw 'Current profile evidence is incomplete.' }
    $Passive = 0
    $Failures = [Collections.Generic.List[string]]::new()
    for ($Index = 0; $Index -lt 1640; $Index++) {
        $Old = $Baseline.expanded_profiles[$Index]
        $New = $Current.native.weapon_profiles[$Index]
        foreach ($Field in @('ArchetypeId', 'ItemLevel', 'DamageType', 'PhysicalSubtype', 'ScalingAttribute',
            'AccuracyBonus', 'ActionPointCost', 'RangeCells', 'PhysicalArmor', 'MagicalArmor')) {
            if ([string]$Old.$Field -cne [string]$New.$Field) {
                $Failures.Add("$($Old.ArchetypeId)/N$($Old.ItemLevel) $Field ($($Old.$Field) -> $($New.$Field))")
            }
        }
        $OldDice = @{ bEnabled = 'False'; DiceCount = '0'; DiceSides = '0'; FlatBonus = '0' }
        foreach ($Part in $Old.DamageDice.Trim('(', ')').Split(',')) {
            if ($Part) { $Key, $Value = $Part.Split('=', 2); $OldDice[$Key] = $Value }
        }
        foreach ($Field in $OldDice.Keys) {
            if ($OldDice[$Field] -cne [string]$New.$Field) {
                $Failures.Add("$($Old.ArchetypeId)/N$($Old.ItemLevel) $Field ($($OldDice[$Field]) -> $($New.$Field))")
            }
        }
        if (-not $New.bEnabled) { $Passive++ }
    }
    if ($Passive -ne 260) { throw "Expected 260 active passive profiles, observed $Passive." }
    if ($Failures.Count) { throw ("$($Failures.Count) profile differences" + [Environment]::NewLine + ($Failures -join [Environment]::NewLine)) }
    Write-Output 'Numerical parity verified for 1,640 profiles, including 260 passive profiles.'
}

Write-Output "Historical manifest verified at $SourceCommit ($ManifestSHA256)."
