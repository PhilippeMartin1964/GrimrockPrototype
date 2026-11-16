<#
 ARM-CLEAN06.4X.5 — guarded materialization of physical ItemLevel for
 four real grid levels and one direct item palette. Exactly one commit.
 Never opens personal SaveGames, maps, WBP or legacy definition packages.
#>
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$WorktreePath,
 [Parameter(Mandatory=$true)][string]$EngineRoot,
 [Parameter(Mandatory=$true)][string]$StateRoot,
 [Parameter(Mandatory=$true)][string]$RunDir,
 [Parameter(Mandatory=$true)][string]$ExpectedHead,
 [string]$GitHubRepository='PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$StagePath=Join-Path $RunDir 'ARMClean064X5.stage'
function Stage([string]$Code) {Set-Content -LiteralPath $StagePath -Encoding ASCII -Value $Code}
Stage 'SCRIPT_STARTED'
function RunGit([string[]]$Arguments) {
 $Lines=@(& git -C $WorktreePath @Arguments)
 if($LASTEXITCODE -ne 0) {throw 'Git command failed.'}
 return $Lines
}
function Ensure-Clean {
 $Dirty=@(RunGit -Arguments @('status','--porcelain','--untracked-files=all'))
 if($Dirty.Count -ne 0) {Stage 'WORKTREE_DIRTY';throw 'Validation worktree must stay clean.'}
}
function Validate-Report($Report,[bool]$Saved) {
 if($null -eq $Report -or
  [string]$Report.schema -cne 'ARM-CLEAN06.4X5/real-authored-physical-levels/v1' -or
  [int]$Report.levels -ne 4 -or [int]$Report.loose -ne 44 -or
  [int]$Report.spawns -ne 0 -or [int]$Report.receptacle -ne 18 -or
  [int]$Report.palette -ne 33 -or [int]$Report.total -ne 95 -or
  [int]$Report.armament -le 0 -or [int]$Report.nonarm -le 0 -or
  [int]$Report.armament + [int]$Report.nonarm -ne 95 -or
  [int]$Report.inherited_world_armament -le 0 -or
  $Report.assets_saved -ne $Saved -or
  $Report.verified_after_reopen -ne (-not $Saved) -or
  $Report.legacy_source_pointers_retained -ne $true -or
  $Report.savegames_opened_or_written -ne $false) {
  Stage 'REPORT_INVALID'
  throw 'Physical-level conversion report violates the 95-item gate.'
 }
 if($Saved -and
  [int]$Report.world_levels_written -ne [int]$Report.inherited_world_armament) {
  Stage 'REPORT_INVALID'
  throw 'Every world ARM authoring N0 must be materialized exactly once.'
 }
 if(-not $Saved -and
  ([int]$Report.world_levels_written -ne 0 -or
   [int]$Report.palette_levels_written -ne 0)) {
  Stage 'REPORT_INVALID'
  throw 'Verification is read-only.'
 }
}
$Parent='0a79e8a11368d8c68d0cfadc17fce4e23ac872e7'
$Subject='feat(ARM-CLEAN06.4X.5): materialize authored physical levels in real UE assets'
$Files=@(
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'
)
$CodeFiles=@(
 'Scripts/ApplyARM_CLEAN06_4X5_PhysicalLevelMigration.ps1'
 'Scripts/UEValidationAgent.ps1'
 'Source/GrimrockPrototype/Public/Core/GridObjectPaletteAsset.h'
 'Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064X5PhysicalLevelsCommandlet.h'
 'Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064X5PhysicalLevelsCommandlet.cpp'
 'Source/GrimrockPrototypeEditor/Private/Tests/GridEditorARMCLEAN064X2AuthoredSourceEvidenceTests.cpp'
 'Source/GrimrockPrototypeEditor/Private/Tests/GridEditorARMCLEAN064X4RealAuthoredLevelsTests.cpp'
 'docs/Architecture/ARM_CLEAN06_4X5_NATIVE_PHYSICAL_LEVELS.md'
)
# Original W binaries, independently retrieved from GitHub's LFS OIDs.
$Pinned=@{
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset'=@{Hash='5df6b36e0044914b49c40f7508395913a00f029281c2412b873740eba3e5344c';Bytes=757688}
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.uasset'=@{Hash='ccd7bc235ed5a8bf1ed5d452b22b1fd1263c02c67f74b6b31be5f878cf6fad4c';Bytes=463390}
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.uasset'=@{Hash='7f86821fafd426bb0dff03b5a923b0b012985c88e9a6cb8d66a1322ef79ac7c6';Bytes=414789}
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.uasset'=@{Hash='8e4744998d6d638d4e3b9404eb6c68dde722f74be95bc1853b2dcfe0362cc5b4';Bytes=414809}
 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'=@{Hash='05b638f62f362bea3dbd451989b2ec52e6f0dfdf01f5369fb495c34e4f7c23a1';Bytes=43337}
}
Stage 'CHECK_PARENT'
if($Files.Count -ne 5 -or $Pinned.Count -ne 5 -or $CodeFiles.Count -ne 8) {
 throw 'Closed source allowlists changed.'
}
foreach($File in $Files) {
 if(-not $Pinned.ContainsKey([string]$File)) {throw 'Missing signed W source OID.'}
}
$Head=(@(RunGit -Arguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
$ActualParent=(@(RunGit -Arguments @('rev-parse','HEAD^')) | Select-Object -First 1).Trim()
$ActualSubject=(@(RunGit -Arguments @('log','-1','--format=%s')) | Select-Object -First 1).Trim()
if($Head -cne $ExpectedHead -or $ActualParent -cne $Parent -or
 $ActualSubject -cne $Subject) {
 Stage 'HEAD_MISMATCH'
 throw 'Only the exact self-owned X5 commit directly after validated X4 may write.'
}
$Changed=@(RunGit -Arguments @('diff-tree','--no-commit-id','--name-only','-r','HEAD'))
$BinaryChanged=@($Changed | Where-Object {$Files -ccontains $_})
$CodeChanged=@($Changed | Where-Object {$CodeFiles -ccontains $_})
if($CodeChanged.Count -ne 8 -or
 @($Changed | Where-Object {$CodeFiles -cnotcontains $_ -and $Files -cnotcontains $_}).Count -ne 0 -or
 @($CodeFiles | Where-Object {$CodeChanged -cnotcontains $_}).Count -ne 0 -or
 @($BinaryChanged | Sort-Object -Unique).Count -ne $BinaryChanged.Count) {
 Stage 'COMMIT_FILESET_MISMATCH'
 throw 'Unexpected code or binary changes in dedicated X5 commit.'
}
$Final=$BinaryChanged.Count -gt 0
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Validator=Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
$PsExe=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
foreach($File in @($Editor,$Project,$Validator,$PsExe)) {
 if(-not (Test-Path -LiteralPath $File -PathType Leaf)) {
  Stage 'EXECUTABLE_MISSING'
  throw 'UE 5.5.4 validation executable missing.'
 }
}
Ensure-Clean
Stage 'HYDRATE_GIT_LFS'
foreach($Relative in $Files) {
 $Attr=@(& git -C $WorktreePath check-attr filter -- $Relative)
 if($LASTEXITCODE -ne 0 -or $Attr.Count -ne 1 -or
  [string]$Attr[0] -notmatch ': filter: lfs$') {
  Stage 'LFS_ATTRIBUTE_MISSING'
  throw 'Native UE package must be tracked by Git LFS.'
 }
}
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4X5'
New-Item -ItemType Directory -Path $Folder -Force -ErrorAction Stop | Out-Null
$Include=($Files -join ',')
$LfsOut=Join-Path $Folder 'lfs-pull.stdout-private.log'
$LfsErr=Join-Path $Folder 'lfs-pull.stderr-private.log'
$PullArgs='-C "'+$WorktreePath+'" lfs pull origin "--include='+$Include+'" "--exclude="'
$Pull=Start-Process -FilePath 'git.exe' -ArgumentList $PullArgs -WorkingDirectory $WorktreePath -NoNewWindow -Wait -PassThru -RedirectStandardOutput $LfsOut -RedirectStandardError $LfsErr -ErrorAction Stop
if($Pull.ExitCode -ne 0) {Stage 'LFS_PULL_FAILED';throw 'Git LFS source fetch refused.'}
$CheckoutArgs='-C "'+$WorktreePath+'" lfs checkout'
foreach($Relative in $Files) {$CheckoutArgs+=' "'+$Relative+'"'}
$CO=Start-Process -FilePath 'git.exe' -ArgumentList $CheckoutArgs -WorkingDirectory $WorktreePath -NoNewWindow -Wait -PassThru -RedirectStandardOutput (Join-Path $Folder 'lfs-checkout.stdout-private.log') -RedirectStandardError (Join-Path $Folder 'lfs-checkout.stderr-private.log') -ErrorAction Stop
if($CO.ExitCode -ne 0) {Stage 'LFS_CHECKOUT_FAILED';throw 'Git LFS exact source checkout refused.'}
foreach($Relative in $Files) {
 $Absolute=Join-Path $WorktreePath $Relative.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Absolute -PathType Leaf) -or
  (Get-Item -LiteralPath $Absolute).Length -le 200) {
  Stage 'LFS_POINTER_REMAINS'
  throw 'Real native UE package is absent or is an LFS pointer.'
 }
 if(-not $Final) {
  if((Get-Item -LiteralPath $Absolute).Length -ne [int64]$Pinned[$Relative].Bytes -or
   (Get-FileHash -LiteralPath $Absolute -Algorithm SHA256).Hash.ToLowerInvariant() -cne
    [string]$Pinned[$Relative].Hash) {
   Stage 'SOURCE_W_OID_MISMATCH'
   throw 'Input package differs from the fully validated 06.4W LFS object.'
  }
 }
 $Stream=[System.IO.File]::OpenRead($Absolute)
 try {
  [byte[]]$Header=@(0,0,0,0)
  if($Stream.Read($Header,0,4) -ne 4 -or
   $Header[0] -ne 0xC1 -or $Header[1] -ne 0x83 -or
   $Header[2] -ne 0x2A -or $Header[3] -ne 0x9E) {
   Stage 'INVALID_UE_PACKAGE'
   throw 'LFS source is not an Unreal .uasset package.'
  }
 } finally {$Stream.Dispose()}
}
Ensure-Clean
$Mode=if($Final){'-Verify'}else{'-Apply'}
$ApplyReport=Join-Path $Folder 'native-levels-private.json'
$BackupsReady=$false
$BackupDir=Join-Path $StateRoot ('Recovery\ARM4X5-'+$ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff'))
try {
 if(-not $Final) {
  Stage 'SAVE_PRIVATE_SOURCE_BACKUPS'
  foreach($Relative in $Files) {
   $Source=Join-Path $WorktreePath $Relative.Replace('/','\')
   $Backup=Join-Path $BackupDir $Relative.Replace('/','\')
   New-Item -ItemType Directory -Path (Split-Path -Parent $Backup) -Force -ErrorAction Stop | Out-Null
   Copy-Item -LiteralPath $Source -Destination $Backup -ErrorAction Stop
   if((Get-FileHash -LiteralPath $Source -Algorithm SHA256).Hash -cne
    (Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash) {
    throw 'Source backup differs from original W package.'
   }
  }
  $BackupsReady=$true
 }
 Stage $(if($Final){'VERIFY_COMMITTED'}else{'APPLY_UNREAL'})
 & $Editor $Project '-run=GridArmClean064X5PhysicalLevels' '-Unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' $Mode "-Output=$ApplyReport" *> (Join-Path $Folder 'editor-private.log')
 if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $ApplyReport -PathType Leaf)) {
  Stage 'UNREAL_COMMANDLET_FAILED'
  throw 'Native Editor source-level materialization or verification failed.'
 }
 $Report=Get-Content -LiteralPath $ApplyReport -Raw -Encoding UTF8 | ConvertFrom-Json
 Validate-Report $Report (-not $Final)
 if($Final) {
  Stage 'FINAL_REOPEN_CLEAN'
  Ensure-Clean
  Stage 'PUBLISH_RECEIPT'
  $Body=Join-Path $Folder 'issue-aggregate.md'
  @(
   '# ARM-CLEAN06.4X.5 - final native physical levels verified'
   ''
   ('Verified commit: '+$ExpectedHead)
   ''
   '| Fact | Value |'
   '|---|---:|'
   '| Real levels reopened | 4 |'
   '| Real direct palette reopened | 1 |'
   '| Canonical item references revalidated | 95 |'
   ('| ARM identity references | '+[int]$Report.armament+' |')
   ('| World ARM references with physical level | '+[int]$Report.inherited_world_armament+' |')
   ('| Unlevelled items preserved at N0 | '+[int]$Report.nonarm+' |')
   '| Personal SaveGames, maps and Blueprints modified | 0 |'
   '| Historical pointer deletion | 0 |'
   ''
   'Native UE assets were reopened independently. No PIE, Chaos, Save/Continue or Shipping assertion.'
  ) | Set-Content -LiteralPath $Body -Encoding UTF8
  $Issue=@(& gh issue create --repo $GitHubRepository --title ('[ARM-CLEAN06.4X.5] Physical world levels '+$ExpectedHead.Substring(0,12)) --body-file $Body)
  if($LASTEXITCODE -ne 0 -or $Issue.Count -ne 1) {
   Stage 'ISSUE_PUBLICATION_FAILED'
   throw 'Cannot publish aggregate-only verification receipt.'
  }
  Stage 'SUCCESS'
  exit 0
 }
 Stage 'CHECK_EXACT_NATIVE_DIFF'
 $Dirty=@(RunGit -Arguments @('diff','--name-only'))
 if($Dirty.Count -lt 1 -or $Dirty.Count -gt 5 -or
  @($Dirty | Where-Object {$Files -cnotcontains $_}).Count -ne 0) {
  throw 'Native conversion modified files outside five allowed UE packages.'
 }
 $Status=@(RunGit -Arguments @('status','--porcelain','--untracked-files=all'))
 if($Status.Count -ne $Dirty.Count -or
  @($Status | Where-Object {
   ([string]$_).Length -lt 4 -or
   ([string]$_).Substring(0,2) -cne ' M' -or
   $Dirty -cnotcontains ([string]$_).Substring(3).Replace('\','/')
  }).Count -ne 0) {
  throw 'Unexpected user or automation files appeared during UE materialization.'
 }
 Stage 'REOPEN_IN_SECOND_EDITOR'
 $ReopenReport=Join-Path $Folder 'native-levels-reopened-private.json'
 & $Editor $Project '-run=GridArmClean064X5PhysicalLevels' '-Unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' '-Verify' "-Output=$ReopenReport" *> (Join-Path $Folder 'reopen-private.log')
 if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $ReopenReport -PathType Leaf)) {
  Stage 'REOPEN_VERIFICATION_FAILED'
  throw 'Independent Unreal Editor reread rejected physical levels.'
 }
 $Reopened=Get-Content -LiteralPath $ReopenReport -Raw -Encoding UTF8 | ConvertFrom-Json
 Validate-Report $Reopened $false
 if([int]$Report.armament -ne [int]$Reopened.armament -or
  [int]$Report.nonarm -ne [int]$Reopened.nonarm -or
  [int]$Report.inherited_world_armament -ne [int]$Reopened.inherited_world_armament) {
  Stage 'REPORT_PARITY_FAILED'
  throw 'Editor materialization and independent reread disagree.'
 }
 Stage 'POST_MIGRATION_AUTOMATION'
 $ValidateArgs='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "'+
  $Validator+'" -EngineRoot "'+$EngineRoot+'" -SkipBuild -AutomationFilter "Grimrock" -ReportRoot "'+
  (Join-Path $Folder 'post-automation')+'"'
 $Validation=Start-Process -FilePath $PsExe -ArgumentList $ValidateArgs -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput (Join-Path $Folder 'automation.stdout-private.log') -RedirectStandardError (Join-Path $Folder 'automation.stderr-private.log') -ErrorAction Stop
 if($Validation.ExitCode -ne 0) {
  Stage 'POST_AUTOMATION_FAILED'
  throw 'Full Grimrock suite failed on materially changed native UE binaries.'
 }
 $After=@(RunGit -Arguments @('diff','--name-only'))
 if((@($After) -join '|') -cne (@($Dirty) -join '|')) {
  Stage 'POST_AUTOMATION_FILESET_CHANGED'
  throw 'Full automation changed native file allowlist.'
 }
 Stage 'REMOTE_LEASE_CHECK'
 [void](RunGit -Arguments @('fetch','--no-tags','origin','master'))
 $Remote=(@(RunGit -Arguments @('rev-parse','refs/remotes/origin/master')) | Select-Object -First 1).Trim()
 if($Remote -cne $ExpectedHead) {throw 'Master changed; refuse overwrite.'}
 Stage 'STAGE_NATIVE_ASSETS'
 [void](RunGit -Arguments (@('add','--')+$Dirty))
 $Staged=@(RunGit -Arguments @('diff','--cached','--name-only'))
 if($Staged.Count -ne $Dirty.Count -or
  @($Staged | Where-Object {$Dirty -cnotcontains $_}).Count -ne 0) {
  throw 'Git staging changed an unapproved file.'
 }
 Stage 'AMEND_SINGLE_COMMIT'
 & git -C $WorktreePath -c 'user.name=Grimrock UE Automation' -c 'user.email=grimrock-ue-automation@users.noreply.github.com' commit --amend --no-edit | Out-Null
 if($LASTEXITCODE -ne 0) {throw 'Could not fold real UAssets into one X5 commit.'}
 $NewHead=(@(RunGit -Arguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
 $NewParent=(@(RunGit -Arguments @('rev-parse','HEAD^')) | Select-Object -First 1).Trim()
 if($NewHead -ceq $ExpectedHead -or $NewParent -cne $Parent) {
  throw 'Final commit does not preserve exact validated X4 parent.'
 }
 $FinalDiff=@(RunGit -Arguments @('diff-tree','--no-commit-id','--name-only','-r','HEAD'))
 $AllExpected=@($CodeFiles)+@($Dirty)
 if($FinalDiff.Count -ne $AllExpected.Count -or
  @($FinalDiff | Where-Object {$AllExpected -cnotcontains $_}).Count -ne 0 -or
  @($AllExpected | Where-Object {$FinalDiff -cnotcontains $_}).Count -ne 0) {
  throw 'Final single X5 commit differs from exact code and native allowlists.'
 }
 Ensure-Clean
 Stage 'PUSH_WITH_LEASE'
 & git -C $WorktreePath push ('--force-with-lease=refs/heads/master:'+$ExpectedHead) origin HEAD:refs/heads/master | Out-Null
 if($LASTEXITCODE -ne 0) {throw 'Failed to publish one exact native X5 commit.'}
 $BackupsReady=$false
 Stage 'SUCCESS'
 Write-Host ('[ARM-CLEAN06.4X5] REAL_NATIVE_LEVELS_PUSHED '+$NewHead)
}
catch {
 if($BackupsReady) {
  try {
   $Current=(@(RunGit -Arguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
   if($Current -cne $ExpectedHead) {throw 'Refuse restoration after a commit amend.'}
   $CurrentDirty=@(RunGit -Arguments @('diff','--name-only'))
   if(@($CurrentDirty | Where-Object {$Files -cnotcontains $_}).Count -ne 0) {
    throw 'Non-native files modified; no automatic restore.'
   }
   foreach($Relative in $Files) {
    $Backup=Join-Path $BackupDir $Relative.Replace('/','\')
    $Destination=Join-Path $WorktreePath $Relative.Replace('/','\')
    if(-not (Test-Path -LiteralPath $Backup -PathType Leaf)) {throw 'Private backup missing.'}
    Copy-Item -LiteralPath $Backup -Destination $Destination -Force -ErrorAction Stop
    if((Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash -cne
     (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash) {
     throw 'Private restore hash mismatch.'
    }
   }
   Ensure-Clean
  }
  catch {Write-Warning '[ARM-CLEAN06.4X5] Private rollback did not pass all safety gates.'}
 }
 throw
}
