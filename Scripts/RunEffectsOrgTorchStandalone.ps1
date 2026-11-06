# EFFECTS-ORG01.2D: isolated one-shot Windows executor, NOT the UE validation agent.
# Nothing is changed unless the caller explicitly supplies -Apply.
[CmdletBinding()]
param(
    [string]$RepositoryRoot = 'D:\Development\GrimrockPrototype',
    [string]$EngineRoot = 'D:\UE_5.5',
    [string]$StateRoot = (Join-Path $env:LOCALAPPDATA 'GrimrockPrototype\EffectsOrgStandalone'),
    [switch]$Apply
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Assert-Exit {
    param([string]$Operation)
    if ($LASTEXITCODE -ne 0) { throw ('Independent migration failed: ' + $Operation) }
}
function Read-GitOne {
    param([string]$Root,[string[]]$Arguments)
    $Items = @(& git -C $Root @Arguments)
    Assert-Exit -Operation 'Git read'
    if ($Items.Count -ne 1) { throw 'Unexpected Git read result.' }
    return ([string]$Items[0]).Trim()
}

if (-not $Apply) {
    Write-Host '[EFFECTS-ORG01.2D] No -Apply: read-only launcher. Nothing changed.'
    return
}
if ([string]::IsNullOrWhiteSpace($StateRoot)) { throw 'A private state path is required.' }
$RepositoryRoot = (Resolve-Path -LiteralPath $RepositoryRoot -ErrorAction Stop).Path
$EngineRoot = (Resolve-Path -LiteralPath $EngineRoot -ErrorAction Stop).Path
$StateRoot = [IO.Path]::GetFullPath($StateRoot)
$Worktree = Join-Path (Split-Path -Parent $RepositoryRoot) 'GrimrockPrototype_EffectsOrgTorch'

# The agent's validation worktree and the developer checkout are off limits.
if ($Worktree.TrimEnd('\') -ieq $RepositoryRoot.TrimEnd('\') -or
    $Worktree.StartsWith($RepositoryRoot.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Independent worktree cannot be inside the developer checkout.'
}
if (Test-Path -LiteralPath $Worktree) {
    throw 'Independent worktree already exists: refusing overwrite or cleanup.'
}
if (-not (Test-Path -LiteralPath (Join-Path $RepositoryRoot 'GrimrockPrototype.uproject') -PathType Leaf)) {
    throw 'Unreal project file not found.'
}
$RepoOrigin = Read-GitOne -Root $RepositoryRoot -Arguments @('remote','get-url','origin')
if ($RepoOrigin -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
    throw 'Git remote does not match expected repository.'
}
$ActualRoot = Read-GitOne -Root $RepositoryRoot -Arguments @('rev-parse','--show-toplevel')
if ([IO.Path]::GetFullPath($ActualRoot).TrimEnd('\') -ine $RepositoryRoot.TrimEnd('\')) {
    throw 'RepositoryRoot is not the expected Git checkout root.'
}
& git -C $RepositoryRoot fetch --no-tags origin master | Out-Null
Assert-Exit -Operation 'fetch master'
$ExpectedSha = Read-GitOne -Root $RepositoryRoot -Arguments @('rev-parse','refs/remotes/origin/master')
if ($ExpectedSha -notmatch '^[a-f0-9]{40}$') { throw 'Invalid master SHA.' }
$Subject = Read-GitOne -Root $RepositoryRoot -Arguments @('show','-s','--format=%s',$ExpectedSha)
if ($Subject -notmatch '^EFFECTS-ORG01[.]2D-(PREP|PREFLIGHT|RUNNER):') {
    throw 'HEAD is not an approved EFFECTS-ORG01.2D preparation commit.'
}
New-Item -ItemType Directory -Path $StateRoot -Force | Out-Null

$PreviousSmudge = [Environment]::GetEnvironmentVariable('GIT_LFS_SKIP_SMUDGE','Process')
try {
    $env:GIT_LFS_SKIP_SMUDGE = '1'
    & git -C $RepositoryRoot worktree add --detach $Worktree $ExpectedSha | Out-Null
    Assert-Exit -Operation 'create detached worktree'
}
finally {
    if ($null -eq $PreviousSmudge) {
        Remove-Item Env:\GIT_LFS_SKIP_SMUDGE -ErrorAction SilentlyContinue
    }
    else {
        $env:GIT_LFS_SKIP_SMUDGE = $PreviousSmudge
    }
}
if ((Read-GitOne -Root $Worktree -Arguments @('rev-parse','HEAD')) -cne $ExpectedSha) {
    throw 'New worktree HEAD mismatch.'
}
& git -C $Worktree submodule update --init -- ThirdParty/Lua54 | Out-Null
Assert-Exit -Operation 'initialise Lua54'
& git -C $Worktree lfs pull | Out-Null
Assert-Exit -Operation 'hydrate UE content via Git LFS'

$Validation = Join-Path $Worktree 'Scripts\ValidateUE.ps1'
if (-not (Test-Path -LiteralPath $Validation -PathType Leaf)) { throw 'ValidateUE.ps1 missing.' }
$Report = Join-Path $StateRoot 'Preflight'
& $Validation -EngineRoot $EngineRoot -AutomationFilter 'Grimrock.Editor.EffectsOrg01.TorchMigrationPreflight' -ReportRoot $Report
if (-not $?) { throw 'Standalone UE preflight failed.' }

$Dirty = @(& git -C $Worktree status --porcelain --untracked-files=all)
Assert-Exit -Operation 'check isolated worktree'
if ($Dirty.Count -gt 0) { throw 'UE preflight dirtied the isolated worktree: no migration.' }

$Migration = Join-Path $Worktree 'Scripts\MigrateEffectsOrgTorch.ps1'
if (-not (Test-Path -LiteralPath $Migration -PathType Leaf)) { throw 'Migration script missing.' }
$PowerShellExe = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
if (-not (Test-Path -LiteralPath $PowerShellExe -PathType Leaf)) { throw 'PowerShell missing.' }
& $PowerShellExe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File $Migration -WorktreePath $Worktree -EngineRoot $EngineRoot -ExpectedHead $ExpectedSha -StateRoot $StateRoot
Assert-Exit -Operation 'native Unreal AssetTools migration'

$AssetCommit = Read-GitOne -Root $Worktree -Arguments @('rev-parse','HEAD')
if ($AssetCommit -ceq $ExpectedSha) { throw 'Migration made no new commit.' }
$RemoteHead = Read-GitOne -Root $Worktree -Arguments @('ls-remote','origin','refs/heads/master')
$RemoteHash = ($RemoteHead -split '\s+')[0]
if ($RemoteHash -cne $AssetCommit) { throw 'Published asset commit is not remote HEAD.' }
Write-Host ('[EFFECTS-ORG01.2D] Torch Niagara asset commit published: ' + $AssetCommit)
Write-Host '[EFFECTS-ORG01.2D] The existing Windows validation agent validates the asset commit separately.'
Write-Host '[EFFECTS-ORG01.2D] Isolated worktree and private backups are retained for audit.'
