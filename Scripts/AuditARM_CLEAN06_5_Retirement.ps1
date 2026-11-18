[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [string]$EngineRoot='D:\UE_5.5',
    [string]$StateRoot='D:\GrimrockNativeSnapshots'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$RepoRoot=Split-Path -Parent $PSScriptRoot
$Live=Join-Path $RepoRoot 'Saved\SaveGames'
$StateRoot=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
$Project=[IO.Path]::GetFullPath($RepoRoot).TrimEnd('\','/')
if ($StateRoot -ieq $Project -or $StateRoot.StartsWith($Project+'\',[StringComparison]::OrdinalIgnoreCase)) { throw 'Private state must be outside project.' }
function Git([string[]]$Arguments) {
    $Result=@(& git -C $RepoRoot @Arguments)
    if ($LASTEXITCODE -ne 0) { throw 'Git read failed.' }
    $Result
}
function CheckClean {
    if ((Git @('rev-parse','HEAD')) -cne $ExpectedHead -or (Git @('branch','--show-current')) -cne 'master' -or
        @(Git @('status','--porcelain','--untracked-files=all')).Count -ne 0) { throw 'Expected clean master at exact SHA.' }
    if ((@(Git @('ls-remote','origin','refs/heads/master'))[0] -split '\s+')[0] -cne $ExpectedHead) { throw 'Origin master changed.' }
}
function CheckClosed {
    if (@(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,GrimrockPrototype,GrimrockPrototype-Win64-Shipping -ErrorAction SilentlyContinue).Count -ne 0) {
        throw 'Close UE and the game before collecting personal save copies.'
    }
}
function Inventory([string]$Directory) {
    Assert-ArmX66OrdinaryPath $Directory
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { throw 'Save directory absent.' }
    foreach ($File in @(Get-ChildItem -LiteralPath $Directory -Force | Sort-Object Name)) {
        Assert-ArmX66OrdinaryPath $File.FullName
        if ($File.PSIsContainer -or $File.Name -notmatch '^[a-zA-Z0-9_-]+\.sav$') { throw 'Unexpected save directory entry; audit refused.' }
        [pscustomobject]@{ name=$File.Name; bytes=$File.Length; sha256=(Get-ArmX66FileState $File.FullName) }
    }
}
CheckClean
CheckClosed
Assert-ArmX66OrdinaryPath $StateRoot
$Before=@(Inventory $Live)
if ($Before.Count -eq 0) { throw 'No live saves to audit.' }
$Run=Join-Path $StateRoot ('ARM_CLEAN06_5A_'+$ExpectedHead.Substring(0,12)+'_'+[guid]::NewGuid().ToString('N'))
$Copies=Join-Path $Run 'AuditCopies'
if (Test-Path -LiteralPath $Run) { throw 'Private run directory collision.' }
[void](New-Item -ItemType Directory -Path $Copies -Force)
foreach ($File in $Before) { Copy-Item -LiteralPath (Join-Path $Live $File.name) -Destination (Join-Path $Copies $File.name) }
$InventoryJson=ConvertTo-Json -InputObject $Before -Compress
if ((ConvertTo-Json -InputObject @(Inventory $Copies) -Compress) -cne $InventoryJson -or
    (ConvertTo-Json -InputObject @(Inventory $Live) -Compress) -cne $InventoryJson) { throw 'Save copy mismatch.' }
[pscustomobject]@{schema='ARM-CLEAN06.5A/source-copy-receipt/v1'; head=$ExpectedHead; files=$Before} |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $Run 'copy-receipt.json') -Encoding UTF8
$Build=Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$ProjectFile=Join-Path $RepoRoot 'GrimrockPrototype.uproject'
& $Build GrimrockPrototypeEditor Win64 Development "-Project=$ProjectFile" -WaitMutex -NoHotReloadFromIDE
if ($LASTEXITCODE -ne 0) { throw 'Build failed before read-only audit.' }
CheckClean
CheckClosed
$Report=Join-Path $Run 'retirement-evidence.json'
$Log=Join-Path $Run 'audit.log'
& $Editor $ProjectFile '-run=GridArmClean065RetirementAudit' '-ReadOnly' "-Copies=$Copies" "-Output=$Report" '-Unattended' '-NoP4' '-NoSplash' '-NoSound' '-NullRHI' "-abslog=$Log"
$UEExit=$LASTEXITCODE
CheckClean
if ((ConvertTo-Json -InputObject @(Inventory $Live) -Compress) -cne $InventoryJson -or
    (ConvertTo-Json -InputObject @(Inventory $Copies) -Compress) -cne $InventoryJson) { throw 'Save inventory changed; preserve private evidence.' }
Write-Host '[OK] All live saves and audit copies are byte-identical; clean master.'
if (-not (Test-Path -LiteralPath $Report -PathType Leaf)) { throw 'Native audit did not produce a report.' }
$Result=Get-Content -LiteralPath $Report -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Result.schema -cne 'ARM-CLEAN06.5A/retirement-evidence/v1' -or $Result.deletion_authorized -or
    [int]$Result.save_count -ne $Before.Count) { throw 'Unexpected evidence report.' }
Write-Host ('[ARM-CLEAN06.5A] REPORT '+$Report)
Write-Host ('Canonical='+$Result.native.canonical_count+' Legacy='+$Result.native.legacy_definition_count+' ExternalEdges='+$Result.native.external_referencer_edges)
Write-Host ('Saves='+$Result.save_count+' DecodeFailures='+$Result.decode_failures)
if ($UEExit -ne 0 -or $Result.decode_failures -ne 0) { throw 'Read-only report contains incomplete decoding evidence; retain it for review.' }
Write-Host '[OK] Read-only audit completed. No deletion authorized.'
