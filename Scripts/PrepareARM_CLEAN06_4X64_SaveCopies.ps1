[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$StateRoot,
    [Parameter(Mandatory=$true)][string]$SaveDirectory,
    [Parameter(Mandatory=$true)][ValidatePattern('^[0-9a-f]{40}$')][string]$ExpectedHead,
    [switch]$PassThru,
    [switch]$RepairReadables
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Full([string]$P) { return [IO.Path]::GetFullPath($P).TrimEnd('\','/') }
function IsUnder([string]$Child,[string]$Parent) {
    return $Child.Equals($Parent,[StringComparison]::OrdinalIgnoreCase) -or
        $Child.StartsWith($Parent+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)
}
function AssertOrdinaryPath([string]$P) {
    $Walk = Get-Item -LiteralPath $P -Force
    while ($null -ne $Walk) {
        if (($Walk.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
            throw ('Reparse point refused: '+$Walk.FullName)
        }
        if ($Walk -is [IO.FileInfo]) { $Walk = $Walk.Directory }
        else { $Walk = $Walk.Parent }
    }
}
function Sha([string]$P) { return (Get-FileHash -LiteralPath $P -Algorithm SHA256).Hash.ToLowerInvariant() }
function Git([string[]]$ArgsList) {
    $Lines = @(& git.exe -C $RepoRoot @ArgsList)
    if ($LASTEXITCODE -ne 0) { throw ('Git failure: '+($ArgsList -join ' ')) }
    return $Lines
}
function AssertGit {
    if ((Git @('branch','--show-current')) -cne 'master' -or
        (Git @('rev-parse','HEAD')) -cne $ExpectedHead -or
        @(Git @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
        throw 'Expected exact clean master checkout.'
    }
    $Remote = @(Git @('ls-remote','origin','refs/heads/master'))
    if ($Remote.Count -ne 1 -or ($Remote[0] -split '\s+')[0] -cne $ExpectedHead) {
        throw 'Remote master changed.'
    }
}
function InventorySaves {
    $Entries = @(Get-ChildItem -LiteralPath $SaveDirectory -Force)
    foreach ($Entry in $Entries) {
        if (($Entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0 -or $Entry.PSIsContainer) {
            throw ('Nested directory or reparse point refused: '+$Entry.FullName)
        }
    }
    $Files = @($Entries | Where-Object { $_.Extension -ieq '.sav' } | Sort-Object Name)
    foreach ($File in $Files) {
        if ($File.Name -notmatch '^[A-Za-z0-9_-]+\.sav$' -or $File.BaseName -match '^(CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])$') {
            throw ('Unsupported slot filename: '+$File.Name)
        }
        if ($File.Length -le 0) { throw ('Empty save: '+$File.Name) }
        [pscustomobject]@{ name=$File.Name; bytes=$File.Length; sha256=(Sha $File.FullName) }
    }
}
function ReceiptString($Rows) {
    return (ConvertTo-Json -InputObject @($Rows) -Depth 4 -Compress)
}

$RepoRoot = Full $RepoRoot
$EngineRoot = Full $EngineRoot
$StateRoot = Full $StateRoot
$SaveDirectory = Full $SaveDirectory
foreach ($Dir in @($RepoRoot,$StateRoot,$SaveDirectory)) {
    if (-not (Test-Path -LiteralPath $Dir -PathType Container)) { throw ('Missing directory: '+$Dir) }
    AssertOrdinaryPath $Dir
}
if ((IsUnder $StateRoot $RepoRoot) -or (IsUnder $RepoRoot $StateRoot) -or
    (IsUnder $SaveDirectory $StateRoot) -or (IsUnder $StateRoot $SaveDirectory)) {
    throw 'Snapshot directory must be separate from repository and source saves.'
}
$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $RepoRoot 'GrimrockPrototype.uproject'
$Build = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
foreach ($File in @($Editor,$Project,$Build)) {
    if (-not (Test-Path -LiteralPath $File -PathType Leaf)) { throw ('Missing: '+$File) }
}
AssertGit
$Raw = @(& gh.exe api --method GET ('repos/PhilippeMartin1964/GrimrockPrototype/commits/'+$ExpectedHead+'/status'))
if ($LASTEXITCODE -ne 0) { throw 'Could not retrieve exact-SHA Windows validation.' }
$Status = ($Raw -join [Environment]::NewLine) | ConvertFrom-Json
foreach ($Context in @('UE5.5.4/SilentTask','UE5.5.4/LocalAuto')) {
    if (@($Status.statuses | Where-Object { $_.context -ceq $Context -and $_.state -ceq 'success' }).Count -ne 1) {
        throw ('Windows gate not acquired: '+$Context)
    }
}
$Busy = @(Get-Process -Name 'UnrealEditor','UnrealEditor-Cmd','GrimrockPrototype','GrimrockPrototype-Win64-Shipping' -ErrorAction SilentlyContinue)
if ($Busy.Count -ne 0) { throw 'Close the editor and game before capturing save copies.' }
# The validation agent compiled a separate worktree, not this checkout.
& $Build GrimrockPrototypeEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE | Out-Host
if ($LASTEXITCODE -ne 0) { throw 'Local commandlet build failed.' }
AssertGit
$Before = @(InventorySaves)
$Prefix = if ($RepairReadables) { 'ARM_CLEAN06_5B_' } else { 'ARM_CLEAN06_4X64_' }
$Run = Join-Path $StateRoot ($Prefix+$ExpectedHead.Substring(0,12)+'_'+[guid]::NewGuid().ToString('N'))
if (Test-Path -LiteralPath $Run) { throw 'Snapshot collision.' }
[void](New-Item -ItemType Directory -Path $Run)
$Original = Join-Path $Run 'Original'
$Converted = Join-Path $Run 'Converted'
[void](New-Item -ItemType Directory -Path $Original)
[void](New-Item -ItemType Directory -Path $Converted)
$Rows = @()
foreach ($Row in $Before) {
    $Source = Join-Path $SaveDirectory $Row.name
    $Copy = Join-Path $Original $Row.name
    Copy-Item -LiteralPath $Source -Destination $Copy
    if ((Sha $Copy) -cne $Row.sha256 -or (Sha $Source) -cne $Row.sha256) {
        throw ('Source changed during snapshot: '+$Row.name)
    }
}
foreach ($Row in $Before) {
    $Copy = Join-Path $Original $Row.name
    $Target = Join-Path $Converted $Row.name
    $ModeArgs = @()
    if ($RepairReadables) { $ModeArgs += '-RepairReadables' }
    & $Editor $Project '-run=GridArmClean064X64SaveCopy' '-ConvertCopy' "-SnapshotRoot=$Run" "-File=$($Row.name)" '-Unattended' '-NoP4' '-NoSplash' '-NoSound' '-NullRHI' @ModeArgs | Out-Host
    if ($LASTEXITCODE -ne 0) { throw ('Copy conversion failed: '+$Row.name) }
    if (-not (Test-Path -LiteralPath $Target -PathType Leaf) -or (Sha $Copy) -cne $Row.sha256) {
        throw ('Copy receipt invalid: '+$Row.name)
    }
    $Rows += [pscustomobject]@{ name=$Row.name; original_sha256=$Row.sha256; converted_sha256=(Sha $Target); original_bytes=$Row.bytes; converted_bytes=(Get-Item -LiteralPath $Target).Length }
}
$End = @(InventorySaves)
if ((ReceiptString $Before) -cne (ReceiptString $End)) { throw 'Live save set changed during conversion. Do not install these candidates.' }
AssertGit
$Report = [pscustomobject]@{
    schema=$(if ($RepairReadables) { 'ARM-CLEAN06.5B/readable-copy-receipt/v1' } else { 'ARM-CLEAN06.4X64/save-copy-receipt/v1' })
    head=$ExpectedHead
    source_directory=$SaveDirectory
    snapshot=$Run
    save_count=$Before.Count
    live_saves_unchanged=$true
    installed=$false
    files=$Rows
}
$Receipt = Join-Path $Run 'verified-copies.json'
$Report | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $Receipt -Encoding UTF8
$Label = if ($RepairReadables) { 'ARM-CLEAN06.5B' } else { 'ARM-CLEAN06.4X64' }
Write-Host ('['+$Label+'] VERIFIED OFFLINE COPIES '+$Before.Count+' '+$Receipt)

if ($PassThru) { Write-Output $Receipt }
