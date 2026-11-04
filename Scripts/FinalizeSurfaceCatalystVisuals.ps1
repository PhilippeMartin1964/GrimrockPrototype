# ARMEMENT01.17.2C - one-shot native UE 5.5.4 completion.
# Uses a new isolated worktree, never changes the development working tree.
# No retries, polling or destructive recovery. On failure preserve logs/assets.
[CmdletBinding()]
param([string]$EngineRoot='D:\UE_5.5')
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$Repo=Split-Path -Parent $PSScriptRoot
$Worktree=Join-Path (Split-Path -Parent $Repo) 'GrimrockPrototype_CatalystFinalization'
$Build=Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
function Invoke-RepoGit {
    param([string[]]$Arguments)
    $Lines=@(& git -C $Repo @Arguments)
    if($LASTEXITCODE -ne 0) { throw ('Git failed: '+$Arguments[0]) }
    return @($Lines)
}
if(-not (Test-Path -LiteralPath $Build -PathType Leaf) -or
   -not (Test-Path -LiteralPath $Editor -PathType Leaf)) { throw 'UE 5.5.4 Editor not found.' }
if(Test-Path -LiteralPath $Worktree) { throw 'Finalization worktree already exists; refusing overwrite.' }
$Remote=([string]@(Invoke-RepoGit -Arguments @('remote','get-url','origin'))[0]).Trim()
if($Remote -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
    throw 'Unexpected origin.'
}
Invoke-RepoGit -Arguments @('fetch','--no-tags','origin','master') | Out-Null
$Head=([string]@(Invoke-RepoGit -Arguments @('rev-parse','refs/remotes/origin/master'))[0]).Trim()
if($Head -notmatch '^[a-f0-9]{40}$') { throw 'Invalid remote HEAD.' }
Write-Host ('[Catalyst] Preparing '+$Head.Substring(0,12))
Invoke-RepoGit -Arguments @('worktree','add','--detach',$Worktree,$Head) | Out-Null
& git -C $Worktree submodule update --init --recursive
if($LASTEXITCODE -ne 0) { throw 'Lua submodule unavailable.' }
& git -C $Worktree lfs pull
if($LASTEXITCODE -ne 0) { throw 'Git LFS unavailable.' }
$Project=Join-Path $Worktree 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'Project missing.' }
$Run=Join-Path $Worktree 'Saved\CatalystFinalization'
$Reports=Join-Path $Run 'Reports'
New-Item -ItemType Directory -Force -Path $Reports | Out-Null
Write-Host '[Catalyst] Compiling Development Editor once...'
& $Build GrimrockPrototypeEditor Win64 Development "-Project=$Project" -WaitMutex -NoHotReloadFromIDE
if($LASTEXITCODE -ne 0) { throw 'UE build failed; no asset commit.' }
Set-Content -LiteralPath (Join-Path $Run 'CatalystBuildVerified.sha') -Value $Head -Encoding ASCII
$Helper=Join-Path $Worktree 'Scripts\AuthorSurfaceCatalystVisuals.ps1'
$PsExe=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
$Out=Join-Path $Run 'ManualAuthoring.stdout.log'
$Err=Join-Path $Run 'ManualAuthoring.stderr.log'
$Arg='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "'+$Helper+
     '" -WorktreePath "'+$Worktree+'" -EngineRoot "'+$EngineRoot+
     '" -ReportRoot "'+$Reports+'" -ExpectedHead "'+$Head+'" -ManualValidatedBuild'
Write-Host '[Catalyst] Authoring and publishing one allowlisted asset commit...'
$Proc=Start-Process -FilePath $PsExe -ArgumentList $Arg -WorkingDirectory $Worktree -RedirectStandardOutput $Out -RedirectStandardError $Err -PassThru -Wait
if($Proc.ExitCode -ne 0) { throw ('Authoring failed; private logs: '+$Out+' / '+$Err) }
$NewHead=([string]@(& git -C $Worktree rev-parse HEAD)[0]).Trim()
if($LASTEXITCODE -ne 0 -or $NewHead -ceq $Head -or $NewHead -notmatch '^[a-f0-9]{40}$') {
    throw 'Asset commit not present.'
}
Write-Host ('[Catalyst] Native assets published at '+$NewHead)
Write-Host '[Catalyst] Running full Grimrock Automation once...'
$Validate=Join-Path $Worktree 'Scripts\ValidateUE.ps1'
& $Validate -EngineRoot $EngineRoot -SkipBuild -AutomationFilter 'Grimrock' -ReportRoot $Reports
if($LASTEXITCODE -ne 0) { throw 'Full UE Automation failed; inspect local report.' }
Write-Host ('[Catalyst] COMPLETE '+$NewHead)
