# ARMEMENT01.17.2C: materialize 3 native materials, one dedicated mesh,
# one source-painted icon, and update the ONLY original catalyst DataAsset.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ReportRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead
)
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Base='Content/GrimrockPrototype/Art/'
$New=@(
    ($Base+'Materials/Catalyst/M_SurfaceCatalyst_Flask.uasset'),
    ($Base+'Materials/Catalyst/M_SurfaceCatalyst_Ember.uasset'),
    ($Base+'Materials/Catalyst/M_SurfaceCatalyst_RunePulse.uasset'),
    ($Base+'Meshes/Catalyst/SM_SurfaceCatalyst_01.uasset'),
    ($Base+'Textures/Catalyst/T_UI_Item_SurfaceCatalyst_01.uasset')
)
$Item='Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_SurfaceCatalyst_01.uasset'
$Only='Grimrock.RPG.ARMEMENT01.17.2C.ProductionVisualAssets'
$RunDir=Split-Path -Parent $ReportRoot
$StageFile=Join-Path $RunDir 'CatalystVisuals.stage'
function Stage([string]$S) { Set-Content -LiteralPath $StageFile -Value $S -Encoding ASCII }
function Invoke-Git {
    param([Parameter(Mandatory=$true)][string[]]$GitArgs)
    $Value=@(& git -C $WorktreePath @GitArgs)
    if($LASTEXITCODE -ne 0) { throw ('git failure: '+$GitArgs[0]) }
    return @($Value)
}
Stage 'PREFLIGHT'
if($ExpectedHead -notmatch '^[a-f0-9]{40}$') { throw 'Invalid SHA.' }
if(-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) { throw 'Missing worktree.' }
if(@(Invoke-Git -GitArgs @('status','--porcelain','--untracked-files=all')).Count -ne 0) { throw 'Worktree not clean.' }
if(@(Invoke-Git -GitArgs @('rev-parse','HEAD'))[0].Trim() -cne $ExpectedHead) { throw 'Wrong head.' }
$Remote=@(Invoke-Git -GitArgs @('remote','get-url','origin'))[0].Trim()
if($Remote -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') { throw 'Wrong remote.' }
foreach($Name in $New) {
    if(Test-Path -LiteralPath (Join-Path $WorktreePath $Name)) { exit 2 }
}
Stage 'SOURCE_READY'
if(-not (Test-Path -LiteralPath (Join-Path $WorktreePath $Item) -PathType Leaf)) { throw 'Missing source item.' }
foreach($Name in @($New)+@($Item)) {
    $Attr=@(Invoke-Git -GitArgs @('check-attr','filter','--',$Name))
    if($Attr.Count -ne 1 -or $Attr[0] -notmatch ': filter: lfs$') { throw 'Missing LFS filter.' }
}
Stage 'REPORT'
$Index=@(Get-ChildItem -LiteralPath $ReportRoot -Recurse -File -Filter index.json -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending)
if($Index.Count -ne 1) { exit 2 }
$Report=Get-Content -LiteralPath $Index[0].FullName -Raw | ConvertFrom-Json
$Failed=@($Report.tests | Where-Object { [string]$_.state -match '^(?i:fail|failed|error)$' })
if([int]$Report.failed -ne 1 -or $Failed.Count -ne 1 -or [string]$Failed[0].fullTestPath -cne $Only) { exit 2 }
Stage 'REPORT_ACCEPTED'
$Exe=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Exe -PathType Leaf) -or -not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'UE unavailable.' }
$Output=Join-Path $RunDir 'CatalystVisualsCommandlet.stdout.log'
$Err=Join-Path $RunDir 'CatalystVisualsCommandlet.stderr.log'
$Args='"'+$Project+'" -run=RPGAlchemistAuthoring -SurfaceCatalystVisuals -Apply -Unattended -NoSplash -NoP4 -NullRHI'
Stage 'COMMANDLET_STARTED'
$Process=Start-Process -FilePath $Exe -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput $Output -RedirectStandardError $Err -PassThru -Wait -ErrorAction Stop
if($Process.ExitCode -ne 0) { Stage 'COMMANDLET_FAILED'; throw 'Unreal commandlet failed; see private logs.' }
Stage 'COMMANDLET_COMPLETED'
$Changes=@(Invoke-Git -GitArgs @('status','--porcelain','--untracked-files=all'))
if($Changes.Count -ne ($New.Count+1)) { Stage 'FILESET'; throw 'Incorrect number of files.' }
$Expected=@($New | ForEach-Object {'?? '+$_})+@(' M '+$Item)
if(@($Changes | Where-Object { $_ -cnotin $Expected }).Count -ne 0) {
    Stage 'FILESET'; throw 'Unexpected changed file.'
}
foreach($Name in $New) {
    $Full=Join-Path $WorktreePath $Name
    if(-not (Test-Path -LiteralPath $Full -PathType Leaf) -or (Get-Item -LiteralPath $Full).Length -lt 1024) {
        throw 'Invalid Unreal binary asset.'
    }
}
Stage 'ASSETS_VERIFIED'
Invoke-Git -GitArgs @('fetch','--no-tags','origin','master') | Out-Null
if(@(Invoke-Git -GitArgs @('rev-parse','refs/remotes/origin/master'))[0].Trim() -cne $ExpectedHead) {
    throw 'master moved.'
}
Invoke-Git -GitArgs (@('add','--')+@($New)+@($Item)) | Out-Null
$Staged=@(Invoke-Git -GitArgs @('diff','--cached','--name-only','--diff-filter=ACMR'))
if($Staged.Count -ne ($New.Count+1) -or @($Staged | Where-Object { $_ -cnotin (@($New)+@($Item)) }).Count -ne 0) {
    throw 'Index outside allowlist.'
}
Invoke-Git -GitArgs @('-c','user.name=Grimrock UE Automation',
    '-c','user.email=grimrock-ue-automation@users.noreply.github.com',
    'commit','-m','ARMEMENT01.17.2C.2: materialize native catalyst mesh/icon/VFX materials') | Out-Null
Stage 'COMMIT_CREATED'
$Pushed=$false
for($Try=0;$Try -lt 3;$Try++) {
    & git -C $WorktreePath push origin 'HEAD:refs/heads/master' | Out-Null
    if($LASTEXITCODE -eq 0) { $Pushed=$true; break }
    Start-Sleep -Seconds 2
}
if(-not $Pushed) { throw 'Push failed; local commit retained for recovery.' }
Stage 'SUCCESS'
exit 0
