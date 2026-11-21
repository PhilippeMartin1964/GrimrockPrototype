[CmdletBinding()]
param(
    [string]$ExpectedHead,
    [string]$StateRoot = 'D:\GrimrockNativeSnapshots',
    [switch]$SelfTest
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

. (Join-Path $PSScriptRoot 'ARM_CLEAN06_4X66_Rollback.ps1')
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Relative = 'Content/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.uasset'
$Package = '/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons'

function Get-ArmH14Hash([string]$Path) {
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

function Assert-ArmH14NativeAsset([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw 'Source Unreal palette is absent; no snapshot created.'
    }
    Assert-ArmX66OrdinaryPath $Path
    $File = Get-Item -LiteralPath $Path -Force -ErrorAction Stop
    if ($File.Length -le 200 -or
        ($File.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
        throw 'Native palette binary absent or tiny (Git LFS pointer refused).'
    }
    $Stream = [IO.File]::OpenRead($Path)
    try {
        $Buffer = New-Object byte[] 96
        $Read = $Stream.Read($Buffer, 0, $Buffer.Length)
        $Prefix = [Text.Encoding]::ASCII.GetString($Buffer, 0, $Read)
        if ($Prefix.StartsWith('version https://git-lfs.github.com/spec/v1',
            [StringComparison]::Ordinal)) {
            throw 'Git LFS pointer is not the Unreal source asset.'
        }
    } finally { $Stream.Dispose() }
    [pscustomobject]@{
        bytes = [int64]$File.Length
        sha256 = (Get-ArmH14Hash $Path)
    }
}

if ($SelfTest) {
    $Fixture = Join-Path ([IO.Path]::GetTempPath()) ('ArmH14-'+[guid]::NewGuid().ToString('N'))
    if (Test-Path -LiteralPath $Fixture) { throw 'Fixture collision.' }
    [void](New-Item -ItemType Directory -Path $Fixture)
    try {
        $From = Join-Path $Fixture 'Fixture.uasset'
        $To = Join-Path $Fixture 'Copy.uasset'
        [IO.File]::WriteAllBytes($From, [byte[]]@(0..255))
        $Before = Assert-ArmH14NativeAsset $From
        Copy-Item -LiteralPath $From -Destination $To -ErrorAction Stop
        $After = Assert-ArmH14NativeAsset $To
        if ($Before.sha256 -cne $After.sha256 -or $Before.bytes -ne $After.bytes) {
            throw 'Synthetic source/copy hashes differed.'
        }
        [IO.File]::WriteAllBytes($To, [byte[]]@(0..254))
        if ((Get-ArmH14Hash $To) -ceq $Before.sha256) {
            throw 'Tampered private copy not detected.'
        }
        $Pointer = Join-Path $Fixture 'Pointer.uasset'
        [IO.File]::WriteAllLines($Pointer, @(
            'version https://git-lfs.github.com/spec/v1',
            'oid sha256:fixture',
            'size 999'
        ), [Text.Encoding]::ASCII)
        $Rejected = $false
        try { [void](Assert-ArmH14NativeAsset $Pointer) } catch { $Rejected = $true }
        if (-not $Rejected) { throw 'Git LFS pointer accepted as native binary.' }
    } finally {
        Remove-Item -LiteralPath $Fixture -Recurse -Force
    }
    Write-Host '[OK] ARM-CLEAN06.5H.14 copy, hash and LFS pointer rejection self-test.'
    return
}
if ($ExpectedHead -cnotmatch '^[0-9a-f]{40}$') {
    throw 'A complete 40-hex -ExpectedHead is mandatory.'
}
$Git = (Get-Command git.exe -CommandType Application -ErrorAction Stop |
    Select-Object -First 1).Source
function Invoke-ArmH14Git([string[]]$Arguments) {
    $Output = @(& $Git -C $RepoRoot @Arguments)
    if ($LASTEXITCODE -ne 0) { throw 'Git verification failed.' }
    $Output
}
$LocalHead = @(Invoke-ArmH14Git @('rev-parse','HEAD'))
$Branch = @(Invoke-ArmH14Git @('branch','--show-current'))
$Remote = @(Invoke-ArmH14Git @('ls-remote','origin','refs/heads/master'))
$Dirty = @(Invoke-ArmH14Git @('status','--porcelain','--untracked-files=all'))
$Tracked = @(Invoke-ArmH14Git @('ls-files','--error-unmatch','--',$Relative))
if ($LocalHead.Count -ne 1 -or $LocalHead[0] -cne $ExpectedHead -or
    $Branch.Count -ne 1 -or $Branch[0] -cne 'master' -or
    $Remote.Count -ne 1 -or
    ($Remote[0] -split '\s+')[0] -cne $ExpectedHead -or
    $Dirty.Count -ne 0 -or $Tracked.Count -ne 1) {
    throw 'Require clean tracked palette on exact origin/master before snapshot.'
}
$Processes = @(Get-Process -Name UnrealEditor,UnrealEditor-Cmd,GrimrockPrototype,
    GrimrockPrototype-Win64-Shipping -ErrorAction SilentlyContinue)
if ($Processes.Count -ne 0) {
    throw 'Close Unreal Editor, commandlets and game before private snapshot.'
}
$State = [IO.Path]::GetFullPath($StateRoot).TrimEnd('\', '/')
$Project = [IO.Path]::GetFullPath($RepoRoot).TrimEnd('\', '/')
if ($State.Equals($Project, [StringComparison]::OrdinalIgnoreCase) -or
    $State.StartsWith($Project + '\', [StringComparison]::OrdinalIgnoreCase) -or
    -not (Test-Path -LiteralPath $State -PathType Container)) {
    throw 'Private state must already exist outside the project.'
}
Assert-ArmX66OrdinaryPath $State
$Source = Join-Path $Project $Relative.Replace('/', '\')
$Before = Assert-ArmH14NativeAsset $Source
$Run = Join-Path $State ('ARM_CLEAN06_5H14_'+$ExpectedHead.Substring(0,12)+'_'+
    [guid]::NewGuid().ToString('N'))
if (Test-Path -LiteralPath $Run) { throw 'Private run directory collision.' }
[void](New-Item -ItemType Directory -Path $Run -ErrorAction Stop)
$Backup = Join-Path $Run 'DA_EquippedWeaponActionIcons.uasset'
Copy-Item -LiteralPath $Source -Destination $Backup -ErrorAction Stop
$After = Assert-ArmH14NativeAsset $Backup
$Unchanged = Assert-ArmH14NativeAsset $Source
if ($Before.sha256 -cne $After.sha256 -or
    $Before.sha256 -cne $Unchanged.sha256 -or
    $Before.bytes -ne $After.bytes -or
    $Before.bytes -ne $Unchanged.bytes) {
    throw 'Source or private palette copy changed; retain evidence and stop.'
}
$Receipt = [ordered]@{
    schema = 'ARM-CLEAN06.5H.14/native-palette-copy/v1'
    head = $ExpectedHead
    package = $Package
    relative_path = $Relative
    bytes = $Before.bytes
    sha256 = $Before.sha256
    deletion_authorized = $false
    source_unchanged = $true
    backup_byte_identical = $true
}
$Receipt | ConvertTo-Json -Depth 5 |
    Set-Content -LiteralPath (Join-Path $Run 'palette-copy-receipt.json') -Encoding UTF8
if (@(Invoke-ArmH14Git @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
    throw 'Git checkout changed while creating private snapshot.'
}
Write-Host ('[OK] True Unreal palette backed up byte-identically: '+$Run)
Write-Host ('[OK] SHA256='+$Before.sha256+' bytes='+$Before.bytes)
Write-Host '[BLOCKED] Read-only snapshot only; no native deletion authorized.'
