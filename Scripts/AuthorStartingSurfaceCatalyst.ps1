# ARMEMENT01.17.2A — ONE native catalyst asset authoring after sole ProductionCatalyst failure.
# Never changes the developer checkout; whitelist only the new DataAsset.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ReportRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Asset = 'Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.uasset'
$OnlyTest = 'Grimrock.RPG.ARMEMENT01.17.2.ProductionCatalyst'
$StageFile = Join-Path (Split-Path -Parent $ReportRoot) 'CatalystAuthoring.stage'
function Stage([string]$Value) { Set-Content -LiteralPath $StageFile -Value $Value -Encoding ASCII }
function Invoke-GitChecked {
    param([Parameter(Mandatory=$true)][string[]]$GitArgs)
    $Lines = @(& git -C $WorktreePath @GitArgs)
    if ($LASTEXITCODE -ne 0) { throw ('Git failed: ' + $GitArgs[0]) }
    return @($Lines)
}
Stage 'PREFLIGHT'
if ($ExpectedHead -notmatch '^[0-9a-f]{40}$') { throw 'Invalid expected SHA.' }
Stage 'WORKTREE'
if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) { throw 'Missing validation worktree.' }
Stage 'CLEAN'
if (@(Invoke-GitChecked -GitArgs @('status','--porcelain','--untracked-files=all')).Count -ne 0) { throw 'Dirty worktree. No authoring.' }
Stage 'HEAD'
if (@(Invoke-GitChecked -GitArgs @('rev-parse','HEAD'))[0].Trim() -cne $ExpectedHead) { throw 'Wrong checked out revision.' }
Stage 'REMOTE'
$Remote = @(Invoke-GitChecked -GitArgs @('remote','get-url','origin'))[0].Trim()
if ($Remote -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') { throw 'Unexpected GitHub origin.' }
Stage 'TARGET_ABSENT'
if (Test-Path -LiteralPath (Join-Path $WorktreePath $Asset)) { exit 2 }
Stage 'LFS'
$Attr = @(Invoke-GitChecked -GitArgs @('check-attr','filter','--',$Asset))
if ($Attr.Count -ne 1 -or $Attr[0] -notmatch ': filter: lfs$') { throw 'Missing LFS filter.' }
Stage 'REPORT_FILE'
$Indexes = @(Get-ChildItem -LiteralPath $ReportRoot -Recurse -File -Filter index.json -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)
if ($Indexes.Count -eq 0) { exit 2 }
Stage 'REPORT_PARSE'
$Report = Get-Content -LiteralPath $Indexes[0].FullName -Raw | ConvertFrom-Json
Stage 'REPORT_FILTER'
$Failed = @($Report.tests | Where-Object { [string]$_.state -match '^(?i:fail|failed|error)$' })
if ([int]$Report.failed -ne 1 -or $Failed.Count -ne 1 -or
    [string]$Failed[0].fullTestPath -cne $OnlyTest) { exit 2 }
Stage 'REPORT_ACCEPTED'
$Exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if (-not (Test-Path -LiteralPath $Exe -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'UE or project unavailable.' }
$Dir = Split-Path -Parent $ReportRoot
$Output = Join-Path $Dir 'CatalystCommandlet.stdout.log'
$Err = Join-Path $Dir 'CatalystCommandlet.stderr.log'
$Arguments = '"' + $Project + '" -run=RPGAlchemistAuthoring -StartingSurfaceCatalyst -Apply -Unattended -NoSplash -NoP4 -NullRHI'
Stage 'COMMANDLET_STARTED'
$Process = Start-Process -FilePath $Exe -ArgumentList $Arguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Output -RedirectStandardError $Err -PassThru -Wait -ErrorAction Stop
if ($Process.ExitCode -ne 0) { Stage 'COMMANDLET_FAILED'; throw 'UE catalyst commandlet failed (private logs).' }
Stage 'COMMANDLET_COMPLETED'
$Changes = @(Invoke-GitChecked -GitArgs @('status','--porcelain','--untracked-files=all'))
if ($Changes.Count -ne 1 -or $Changes[0] -cne ('?? ' + $Asset)) {
    Stage 'FILESET';
    throw 'Unexpected changed files/status; no commit.'
}
$Path = Join-Path $WorktreePath $Asset
if (-not (Test-Path -LiteralPath $Path -PathType Leaf) -or
    (Get-Item -LiteralPath $Path).Length -lt 1024) { throw 'Invalid Unreal asset size.' }
Stage 'ASSET_VERIFIED'
Invoke-GitChecked -GitArgs @('fetch','--no-tags','origin','master') | Out-Null
if (@(Invoke-GitChecked -GitArgs @('rev-parse','refs/remotes/origin/master'))[0].Trim() -cne $ExpectedHead) {
    throw 'master moved; no commit.'
}
Invoke-GitChecked -GitArgs @('add','--',$Asset) | Out-Null
$Staged = @(Invoke-GitChecked -GitArgs @('diff','--cached','--name-only','--diff-filter=ACMR'))
if ($Staged.Count -ne 1 -or $Staged[0] -cne $Asset) { throw 'Unexpected index.' }
Invoke-GitChecked -GitArgs @('-c','user.name=Grimrock UE Automation',
    '-c','user.email=grimrock-ue-automation@users.noreply.github.com',
    'commit','-m','ARMEMENT01.17.2A: materialize N1 surface catalyst DataAsset') | Out-Null
$NewHead = @(Invoke-GitChecked -GitArgs @('rev-parse','HEAD'))[0].Trim()
Stage 'COMMIT_CREATED'
$Done = $false
for ($Try=1; $Try -le 3; $Try++) {
    & git -C $WorktreePath push origin 'HEAD:refs/heads/master' | Out-Null
    if ($LASTEXITCODE -eq 0) { $Done = $true; break }
    if ($Try -lt 3) { Start-Sleep -Seconds (2 * $Try) }
}
if (-not $Done) { throw 'Push failed; see private logs.' }
Stage 'SUCCESS'
Write-Host ('[ARMEMENT01.17.2] Native catalyst published: ' + $NewHead)
exit 0
