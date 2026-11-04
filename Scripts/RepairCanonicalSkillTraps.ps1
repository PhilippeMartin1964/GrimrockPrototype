# VALIDATION-AUTO01.7 : reparation limitee a DA_Skill_Traps.uasset, uniquement apres echec exact du gate Skills.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ReportRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Target = 'Content/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps.uasset'
$ExpectedTest = 'Grimrock.UI.RPG06.Skills.ProductionCatalog'
function Invoke-GitChecked {
    param([string[]]$Arguments)
    $Output = @(& git -C $WorktreePath @Arguments)
    if ($LASTEXITCODE -ne 0) { throw ('Git failed: ' + $Arguments[0]) }
    return @($Output)
}
function Get-Changes {
    return @(Invoke-GitChecked -Arguments @('status', '--porcelain', '--untracked-files=normal'))
}
if ($ExpectedHead -notmatch '^[0-9a-f]{40}$') { throw 'Invalid expected SHA.' }
if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git') -PathType Leaf)) { throw 'Detached worktree is missing.' }
if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath $Target) -PathType Leaf)) { throw 'Canonical asset is absent.' }
$LocalHead = @(Invoke-GitChecked -Arguments @('rev-parse', 'HEAD'))[0].Trim()
if ($LocalHead -ne $ExpectedHead) { throw 'Worktree commit changed.' }
$Origin = @(Invoke-GitChecked -Arguments @('remote', 'get-url', 'origin'))[0].Trim()
if ($Origin -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') { throw 'Remote repository mismatch.' }
$Attribute = @(Invoke-GitChecked -Arguments @('check-attr', 'filter', '--', $Target))
if ($Attribute.Count -ne 1 -or $Attribute[0] -notmatch ': filter: lfs$') { throw 'Target is not Git LFS tracked.' }
if (@(Get-Changes).Count -ne 0) { throw 'Worktree contains other changes before repair.' }

$Reports = @(Get-ChildItem -LiteralPath $ReportRoot -Filter 'index.json' -File -Recurse -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)
if ($Reports.Count -eq 0) { Write-Host '[AUTO-REPAIR] Not applicable: missing Automation report.'; exit 2 }
$Report = Get-Content -LiteralPath $Reports[0].FullName -Raw | ConvertFrom-Json
$Failures = @($Report.tests | Where-Object { [string]$_.state -match '^(?i:fail|failed|error)$' })
if ([int]$Report.failed -ne 1 -or $Failures.Count -ne 1 -or [string]$Failures[0].fullTestPath -cne $ExpectedTest) {
    Write-Host '[AUTO-REPAIR] Not applicable: errors are not limited to Skills.ProductionCatalog.'
    exit 2
}

$Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or -not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'Editor/project not found.' }
$ParentReport = Split-Path -Parent $ReportRoot
$StdOut = Join-Path $ParentReport 'SkillTrapsAuthoring.stdout.log'
$StdErr = Join-Path $ParentReport 'SkillTrapsAuthoring.stderr.log'
$RunArgs = '"' + $Project + '" -run=RPGSkillCatalogAuthoring -SkillId=Skill_Traps -Unattended -NoSplash -NoP4 -NullRHI'
$Process = Start-Process -FilePath $Editor -ArgumentList $RunArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $StdOut -RedirectStandardError $StdErr -PassThru -Wait -ErrorAction Stop
if ($Process.ExitCode -ne 0) { throw 'Targeted Skill_Traps authoring failed (local logs only).' }
$Changes = @(Get-Changes)
if ($Changes.Count -ne 1 -or $Changes[0] -cne (' M ' + $Target)) {
    throw 'Unexpected asset modification set; no commit created.'
}
Invoke-GitChecked -Arguments @('fetch', '--no-tags', 'origin', 'master') | Out-Null
$RemoteHead = @(Invoke-GitChecked -Arguments @('rev-parse', 'refs/remotes/origin/master'))[0].Trim()
if ($RemoteHead -ne $ExpectedHead) { throw 'master changed during authoring; refusing to publish.' }
Invoke-GitChecked -Arguments @('add', '--', $Target) | Out-Null
$Staged = @(Invoke-GitChecked -Arguments @('diff', '--cached', '--name-only', '--diff-filter=ACMR'))
if ($Staged.Count -ne 1 -or $Staged[0] -cne $Target) { throw 'Unexpected Git index; refusing to publish.' }
Invoke-GitChecked -Arguments @('-c','user.name=Grimrock UE Automation','-c','user.email=grimrock-ue-automation@users.noreply.github.com','commit','-m','UI-RPG06-SKILLTRAPS01: canonicalize DA_Skill_Traps') | Out-Null
$GeneratedCommit = @(Invoke-GitChecked -Arguments @('rev-parse', 'HEAD'))[0].Trim()
Invoke-GitChecked -Arguments @('push', 'origin', 'HEAD:refs/heads/master') | Out-Null
Write-Host "[AUTO-REPAIR] Canonical Skill_Traps asset published: $GeneratedCommit"
exit 0
