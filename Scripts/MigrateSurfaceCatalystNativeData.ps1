# ARMEMENT01.17.3D: isolated Windows migration of ONE existing artist DataAsset.
# Requires precise Automation failure; never edits user checkout or unrelated assets.
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
$RequiredFailure = 'Grimrock.RPG.ARMEMENT01.17.3D.SerializedCatalystContract'
$OptionalFailure = 'Grimrock.Items.VFX01_4A.ProductionAssetAudit'
$Run = Split-Path -Parent $ReportRoot
$StageFile = Join-Path $Run 'CatalystNative.stage'
function Stage([string]$S) { Set-Content -LiteralPath $StageFile -Value $S -Encoding ASCII }
function GitChecked {
    param([Parameter(Mandatory=$true)][string[]]$GitArgs)
    $Lines = @(& git -C $WorktreePath @GitArgs)
    if ($LASTEXITCODE -ne 0) { throw ('git failed: ' + $GitArgs[0]) }
    return @($Lines)
}
Stage 'PREFLIGHT'
if ($ExpectedHead -notmatch '^[0-9a-f]{40}$') { throw 'Invalid expected SHA.' }
if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) { throw 'Worktree unavailable.' }
if (@(GitChecked -GitArgs @('status','--porcelain','--untracked-files=all')).Count -ne 0) { throw 'Dirty worktree.' }
if (@(GitChecked -GitArgs @('rev-parse','HEAD'))[0].Trim() -cne $ExpectedHead) { throw 'Wrong revision.' }
$Remote = @(GitChecked -GitArgs @('remote','get-url','origin'))[0].Trim()
if ($Remote -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') { throw 'Unexpected origin.' }
$Path = Join-Path $WorktreePath $Asset
if (-not (Test-Path -LiteralPath $Path -PathType Leaf) -or (Get-Item -LiteralPath $Path).Length -lt 1024) {
    throw 'Missing or invalid original catalyst.'
}
$Attr = @(GitChecked -GitArgs @('check-attr','filter','--',$Asset))
if ($Attr.Count -ne 1 -or $Attr[0] -notmatch ': filter: lfs$') { throw 'Missing LFS filter.' }
foreach ($Art in @(
    'Content/GrimrockPrototype/Meshes/Items/SM_Catalyst_01.uasset',
    'Content/GrimrockPrototype/UI/Icons/Items/T_Catalyst_Icon.uasset')) {
    if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath $Art) -PathType Leaf)) {
        throw 'Artist visual assets missing.'
    }
}
$Indexes = @(Get-ChildItem -LiteralPath $ReportRoot -Recurse -File -Filter index.json -ErrorAction SilentlyContinue)
if ($Indexes.Count -ne 1) { Stage 'REPORT_REJECTED'; exit 2 }
$Report = Get-Content -LiteralPath $Indexes[0].FullName -Raw | ConvertFrom-Json
$Failed = @($Report.tests | Where-Object { [string]$_.state -match '^(?i:fail|failed|error)$' })
if ([int]$Report.failed -lt 1 -or [int]$Report.failed -gt 2 -or
    $Failed.Count -ne [int]$Report.failed -or
    @($Failed | Where-Object { [string]$_.fullTestPath -ceq $RequiredFailure }).Count -ne 1 -or
    @($Failed | Where-Object {
        [string]$_.fullTestPath -cne $RequiredFailure -and
        [string]$_.fullTestPath -cne $OptionalFailure
    }).Count -gt 0) {
    Stage 'REPORT_REJECTED'
    exit 2
}
Stage 'REPORT_ACCEPTED'
$Backup = Join-Path $Run 'CatalystNative.original.uasset'
Copy-Item -LiteralPath $Path -Destination $Backup -ErrorAction Stop
$OriginalHash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
if ($OriginalHash -ne (Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash) {
    throw 'Backup hash mismatch.'
}
Stage 'BACKUP_VERIFIED'
$Exe = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if (-not (Test-Path -LiteralPath $Exe -PathType Leaf) -or -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
    throw 'UE commandlet unavailable.'
}
$Args = '"' + $Project + '" -run=RPGAlchemistAuthoring -UpgradeSurfaceCatalystNative -Apply -Unattended -NoSplash -NoP4 -NullRHI -stdout -FullStdOutLogOutput'
$Out = Join-Path $Run 'CatalystNativeCommandlet.stdout.log'
$Err = Join-Path $Run 'CatalystNativeCommandlet.stderr.log'
Stage 'COMMANDLET_STARTED'
$Proc = Start-Process -FilePath $Exe -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Out -RedirectStandardError $Err -PassThru -Wait -ErrorAction Stop
if ($Proc.ExitCode -ne 0) { Stage 'COMMANDLET_FAILED'; throw 'UE commandlet error; private logs only.' }
Stage 'COMMANDLET_COMPLETED'
$Changes = @(GitChecked -GitArgs @('status','--porcelain','--untracked-files=all'))
if ($Changes.Count -ne 1 -or $Changes[0] -cne (' M ' + $Asset)) {
    Stage 'FILESET'
    throw 'Unexpected file changes; nothing will be published.'
}
if ((Get-Item -LiteralPath $Path).Length -lt 1024 -or
    $OriginalHash -ceq (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash) {
    throw 'Binary did not change or is invalid.'
}
Stage 'ASSET_VERIFIED'
GitChecked -GitArgs @('fetch','--no-tags','origin','master') | Out-Null
if (@(GitChecked -GitArgs @('rev-parse','refs/remotes/origin/master'))[0].Trim() -cne $ExpectedHead) {
    Stage 'MASTER_MOVED'
    throw 'master moved; no commit.'
}
GitChecked -GitArgs @('add','--',$Asset) | Out-Null
$Staged = @(GitChecked -GitArgs @('diff','--cached','--name-only','--diff-filter=ACMR'))
if ($Staged.Count -ne 1 -or $Staged[0] -cne $Asset) { throw 'Index outside allowlist.' }
GitChecked -GitArgs @('-c','user.name=Grimrock UE Automation',
    '-c','user.email=grimrock-ue-automation@users.noreply.github.com',
    'commit','-m','ARMEMENT01.17.3D.2: persist catalyst native action and VFX policy') | Out-Null
$NewHead = @(GitChecked -GitArgs @('rev-parse','HEAD'))[0].Trim()
Stage 'COMMIT_CREATED'
& git -C $WorktreePath push origin 'HEAD:refs/heads/master' | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Push failed; local commit retained.' }
Stage 'SUCCESS'
Write-Host ('[ARMEMENT01.17.3D] Published ' + $NewHead)
exit 0
