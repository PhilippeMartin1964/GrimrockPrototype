# EFFECTS-ORG01.2B: strictly scoped, Unreal-native migration in agent worktree.
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$WorktreePath,
 [Parameter(Mandatory=$true)][string]$EngineRoot,
 [Parameter(Mandatory=$true)][string]$ExpectedHead,
 [Parameter(Mandatory=$true)][string]$StateRoot
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$Old = 'Content/GrimrockPrototype/Effects/Alchimie/M_Teleporter_Spark_02.uasset'
$New = 'Content/GrimrockPrototype/Effects/Alchimie/M_Potion_Spark.uasset'
$Niagara = 'Content/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.uasset'
$Allowed = @($Old, $New, $Niagara)
$Prepared = $false
$Committed = $false
$Phase = 'PREFLIGHT'
$Private = Join-Path $StateRoot ('EffectsOrg\Potion-' + $ExpectedHead.Substring(0,12))

function GitLines {
 param([string[]]$Arguments)
 $Values = @(& git -C $WorktreePath @Arguments)
 if ($LASTEXITCODE -ne 0) { throw ('GIT_' + $Arguments[0]) }
 return @($Values)
}
function GitOne {
 param([string[]]$Arguments)
 $Values = @(GitLines -Arguments $Arguments)
 if ($Values.Count -ne 1) { throw 'GIT_OUTPUT_UNEXPECTED' }
 return ([string]$Values[0]).Trim()
}
function RecoverKnown {
 # Never touch the developer checkout. No broad git clean/restore.
 $Changes = @(GitLines -Arguments @('status','--porcelain','--untracked-files=all'))
 foreach ($Entry in $Changes) {
  if ($Entry.Length -lt 4 -or $Entry.Substring(3) -cnotin $Allowed) {
   Write-Warning 'UNEXPECTED_WORKTREE_CHANGES_NO_AUTORECOVERY'
   return
  }
 }
 $NewFile = Join-Path $WorktreePath $New
 if (Test-Path -LiteralPath $NewFile -PathType Leaf) {
  $Backup = Join-Path $Private 'M_Potion_Spark.generated.uasset'
  Copy-Item -LiteralPath $NewFile -Destination $Backup -Force
  if ((Get-FileHash -LiteralPath $NewFile -Algorithm SHA256).Hash -ne
      (Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash) {
    throw 'BACKUP_HASH_MISMATCH'
  }
 }
 [void](GitLines -Arguments @('reset','--quiet','HEAD','--',$New))
 [void](GitLines -Arguments @('restore','--source=HEAD','--staged','--worktree','--',$Old,$Niagara))
 if (Test-Path -LiteralPath $NewFile -PathType Leaf) {
  Remove-Item -LiteralPath $NewFile -Force
 }
 if (@(GitLines -Arguments @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
  throw 'RECOVERY_NOT_CLEAN'
 }
}

try {
 if ($ExpectedHead -notmatch '^[a-f0-9]{40}$') { throw 'INVALID_SHA' }
 if (-not (Test-Path -LiteralPath (Join-Path $WorktreePath '.git'))) { throw 'WORKTREE_ABSENT' }
 if ((GitOne -Arguments @('rev-parse','HEAD')) -cne $ExpectedHead) { throw 'HEAD_MISMATCH' }
 $Subject = GitOne -Arguments @('log','-1','--format=%s')
 if ($Subject -cnotmatch '^EFFECTS-ORG01[.]2B:') { exit 2 }
 if ((GitOne -Arguments @('remote','get-url','origin')) -notmatch '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
  throw 'ORIGIN_MISMATCH'
 }
 if (@(GitLines -Arguments @('status','--porcelain','--untracked-files=all')).Count -ne 0) {
  throw 'WORKTREE_DIRTY'
 }
 foreach ($Path in @($Old,$Niagara)) {
  $File = Join-Path $WorktreePath $Path
  if (-not (Test-Path -LiteralPath $File -PathType Leaf) -or
      (Get-Item -LiteralPath $File).Length -le 1024) { throw 'LFS_SOURCE_MISSING' }
 }
 if (Test-Path -LiteralPath (Join-Path $WorktreePath $New)) { throw 'TARGET_EXISTS' }
 foreach ($Path in $Allowed) {
  if ((GitOne -Arguments @('check-attr','filter','--',$Path)) -notmatch ': filter: lfs$') {
   throw 'GIT_LFS_ATTRIBUTE_MISSING'
  }
 }
 [void](GitLines -Arguments @('fetch','--no-tags','origin','master'))
 if ((GitOne -Arguments @('rev-parse','refs/remotes/origin/master')) -cne $ExpectedHead) {
  throw 'REMOTE_MOVED'
 }
 New-Item -Path $Private -ItemType Directory -Force | Out-Null
 foreach ($Path in @($Old,$Niagara)) {
  $Source = Join-Path $WorktreePath $Path
  $Backup = Join-Path $Private ([IO.Path]::GetFileName($Path) + '.before')
  Copy-Item -LiteralPath $Source -Destination $Backup -Force
  if ((Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash -ne
      (Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash) {
   throw 'BACKUP_HASH_MISMATCH'
  }
 }
 $Prepared = $true
 $Editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
 $Project = Join-Path $WorktreePath 'GrimrockPrototype.uproject'
 if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
     -not (Test-Path -LiteralPath $Project -PathType Leaf)) { throw 'UE_MISSING' }
 $Arguments = '"' + $Project + '" -run=GridEffectsOrgPotionRename -Apply -Unattended -NoSplash -NoP4 -NullRHI -stdout -FullStdOutLogOutput -log'
 $Phase = 'COMMANDLET'
 # Windows PowerShell 5.1 does not reliably populate ExitCode after a
 # timed WaitForExit() on a Start-Process -PassThru wrapper. The known-working
 # engine authoring scripts use -Wait -PassThru (same UE executable).
 $Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -WorkingDirectory $WorktreePath -WindowStyle Hidden -RedirectStandardOutput (Join-Path $Private 'Rename.stdout.log') -RedirectStandardError (Join-Path $Private 'Rename.stderr.log') -PassThru -Wait
 $Exit = $Process.ExitCode
 if ($null -eq $Exit) { throw 'COMMANDLET_EXITCODE_UNAVAILABLE' }
 if ([int]$Exit -ne 0) {
  # Only fixed, project-owned diagnostic markers are permitted in a GitHub status.
  # No raw UE log line, Windows path or vendor data is ever emitted.
  $Markers = @(
   @('Exactly one old potion material', 'PREFLIGHT_NAMES'),
   @('Potion material referencers differ', 'PREFLIGHT_REFERENCERS'),
   @('NS_LiquideSparkle no longer depends', 'PREFLIGHT_NIAGARA'),
   @('Healing potion no longer references', 'PREFLIGHT_POTION'),
   @('Required Unreal packages could not be loaded', 'ASSET_LOAD'),
   @('AssetTools failed to rename', 'RENAME'),
   @('Saving the renamed material', 'SAVE'),
   @('Postflight failed', 'REDIRECTOR'),
   @('Failed to find commandlet', 'COMMANDLET_ENTRYPOINT'),
   @('Looked for a commandlet named', 'COMMANDLET_ENTRYPOINT'),
   @('Could not find a commandlet', 'COMMANDLET_ENTRYPOINT'),
   @('Missing cached shader map', 'SHADER_SETUP'),
   @('Failed to load package', 'ASSET_LOAD'),
   @('EXCEPTION_ACCESS_VIOLATION', 'CRASH'),
   @('Fatal error', 'FATAL')
  )
  $LogsToScan = @((Join-Path $Private 'Rename.stdout.log'), (Join-Path $Private 'Rename.stderr.log'),
    (Join-Path $WorktreePath 'Saved\Logs\GrimrockPrototype.log'))
  foreach ($Log in $LogsToScan) {
   if (-not (Test-Path -LiteralPath $Log -PathType Leaf)) { continue }
   $Lines = @(Get-Content -LiteralPath $Log -Tail 1500 -ErrorAction SilentlyContinue)
   foreach ($Pair in $Markers) {
    if (@($Lines | Where-Object { ([string]$_).Contains([string]$Pair[0]) }).Count -gt 0) {
     throw ('COMMANDLET_REASON_' + [string]$Pair[1])
    }
   }
  }
  throw ('COMMANDLET_FAILED_' + ([string][int]$Exit))
 }
 $Phase = 'VERIFY'
 $Changes = @(GitLines -Arguments @('status','--porcelain','--untracked-files=all'))
 $ExpectedStatus = @((" D " + $Old), ("?? " + $New), (" M " + $Niagara))
 if ($Changes.Count -ne 3 -or
     @($Changes | Where-Object { $_ -cnotin $ExpectedStatus }).Count -ne 0) {
  throw 'FILESET_UNEXPECTED'
 }
 if ((Test-Path -LiteralPath (Join-Path $WorktreePath $Old)) -or
     -not (Test-Path -LiteralPath (Join-Path $WorktreePath $New))) {
  throw 'OLD_REDIRECTOR_OR_NEW_MISSING'
 }
 if ((Get-Item -LiteralPath (Join-Path $WorktreePath $New)).Length -le 1024) {
  throw 'NEW_ASSET_LFS_POINTER'
 }
 [void](GitLines -Arguments @('fetch','--no-tags','origin','master'))
 if ((GitOne -Arguments @('rev-parse','refs/remotes/origin/master')) -cne $ExpectedHead) {
  throw 'REMOTE_MOVED'
 }
 $Phase = 'COMMIT'
 [void](GitLines -Arguments (@('add','--') + $Allowed))
 $Staged = @(GitLines -Arguments @('diff','--cached','--name-only'))
 if ($Staged.Count -ne 3 -or @($Staged | Where-Object { $_ -cnotin $Allowed }).Count -ne 0) {
  throw 'STAGED_FILESET_UNEXPECTED'
 }
 [void](GitLines -Arguments @('-c','user.name=Grimrock UE Automation',
  '-c','user.email=grimrock-ue-automation@users.noreply.github.com',
  'commit','-m','EFFECTS-ORG01.2B: rename potion material and its Niagara dependency'))
 $Committed = $true
 $NewHead = GitOne -Arguments @('rev-parse','HEAD')
 $Phase = 'PUSH'
 $Published = $false
 for ($Attempt=1;$Attempt -le 3;++$Attempt) {
  & git -C $WorktreePath push origin 'HEAD:refs/heads/master' | Out-Null
  if ($LASTEXITCODE -eq 0) { $Published=$true; break }
  if ($Attempt -lt 3) { Start-Sleep -Seconds (2*$Attempt) }
 }
 if (-not $Published) { throw 'GIT_PUSH_FAILED' }
 Write-Host ('[EFFECTS-ORG01.2B] New commit pushed: ' + $NewHead)
 exit 0
}
catch {
 # Only allowlisted stages and error codes may leave the Windows workstation.
 $ErrorText = [string]$_.Exception.Message
 $AllowedCauses = @(
  'INVALID_SHA','WORKTREE_ABSENT','HEAD_MISMATCH','ORIGIN_MISMATCH',
  'WORKTREE_DIRTY','LFS_SOURCE_MISSING','TARGET_EXISTS',
  'GIT_LFS_ATTRIBUTE_MISSING','REMOTE_MOVED','BACKUP_HASH_MISMATCH',
  'UE_MISSING','COMMANDLET_TIMEOUT','COMMANDLET_EXITCODE_UNAVAILABLE','FILESET_UNEXPECTED',
  'OLD_REDIRECTOR_OR_NEW_MISSING','NEW_ASSET_LFS_POINTER',
  'STAGED_FILESET_UNEXPECTED','GIT_PUSH_FAILED'
 )
 $Cause = 'OTHER'
 if ($ErrorText -cin $AllowedCauses) {
  $Cause = $ErrorText
 }
 elseif ($ErrorText -match '^COMMANDLET_REASON_([A-Z0-9_]{1,35})$') {
  $Cause = 'UE_REASON_' + $Matches[1]
 }
 elseif ($ErrorText -match '^COMMANDLET_FAILED_(-?[0-9]{1,12})$') {
  $Exit = [long]$Matches[1]
  $SafeNumber = ([string]$Exit).Replace('-', 'NEG')
  $Cause = 'UE_EXIT_' + $SafeNumber
 }
 elseif ($ErrorText -like 'COMMANDLET_FAILED_*') {
  $Cause = 'UE_EXIT_OTHER'
 }
 elseif ($ErrorText -like 'GIT_*') {
  $Cause = 'GIT_OPERATION'
 }
 $StageDir = Join-Path $StateRoot 'EffectsOrg'
 New-Item -ItemType Directory -Force -Path $StageDir | Out-Null
 $StagePath = Join-Path $StageDir ('Migration-' + $ExpectedHead.Substring(0,12) + '.stage')
 Set-Content -LiteralPath $StagePath -Value ($Phase + '|' + $Cause) -Encoding ASCII
 Write-Warning ('[EFFECTS-ORG01.2B] Failed ' + $Phase + '/' + $Cause)
 if ($Prepared -and -not $Committed) {
  try { RecoverKnown }
  catch { Write-Warning '[EFFECTS-ORG01.2B] Private backups retained; worktree requires review.' }
 }
 exit 1
}
