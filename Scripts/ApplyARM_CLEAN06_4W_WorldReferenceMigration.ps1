<#
 ARM-CLEAN06.4W: actual four level + one palette UE .uasset conversion.
 Exactly one Git commit with UE5.5.4 Apply, independent Verify, automation
 and Git LFS. No SaveGame, map, WBP or legacy definition modification.
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
$StagePath=Join-Path $RunDir 'ARMClean064W.stage'
function Stage([string]$Text) {Set-Content -LiteralPath $StagePath -Value $Text -Encoding ASCII}
Stage 'SCRIPT_STARTED'
function RunGit([string[]]$Arguments) {
 $Text=@(& git -C $WorktreePath @Arguments)
 if($LASTEXITCODE -ne 0) {throw 'Git command failed; no uncontrolled recovery.'}
 return $Text
}
function Ensure-Clean {
 $Dirty=@(RunGit -Arguments @('status','--porcelain','--untracked-files=all'))
 if($Dirty.Count -ne 0) {Stage 'WORKTREE_DIRTY';throw 'Validation worktree changed.'}
}
function Validate-Report([object]$Data,[bool]$Saved,[string[]]$Files) {
 if($null -eq $Data -or
  [string]$Data.schema -cne 'ARM-CLEAN06.4W/world-canonical-reference-materialization/v1' -or
  [int]$Data.levels -ne 4 -or [int]$Data.loose_items -ne 44 -or
  [int]$Data.spawns -ne 0 -or [int]$Data.receptacle_items -ne 18 -or
  [int]$Data.palette_items -ne 33 -or
  [int]$Data.world_definition_default_items -ne 0 -or
  [int]$Data.canonical_references -ne 95 -or
  [int]$Data.old_unmapped_ids_now_resolved -ne 40 -or
  [int]$Data.new_real_asset_files -ne 5 -or
  $Data.assets_saved -ne $Saved -or
  $Data.verified_after_reopen -ne (-not $Saved) -or
  $Data.legacy_runtime_fallback_added -ne $false -or
  $Data.legacy_source_pointers_retained_until_cutover -ne $true -or
  $Data.savegame_files_opened -ne $false) {
  throw 'World reference authoring report not identical to signed scope.'
 }
 $Reported=@($Data.asset_paths_private)
 if($Reported.Count -ne 5 -or
  @($Reported | Where-Object {$_ -cnotin $Files}).Count -ne 0 -or
  (@($Reported | Sort-Object -Unique)).Count -ne 5) {
  throw 'Actual UE output files diverge from exact five-file allowlist.'
 }
}
$RootParent='5b7131fe55d7cd770e9ae2bcc10901db7d7dfd51'
$RepairParent='7da02306d1926362fb491b4560553fec990872e8'
$InitialWParent='6126d3825a1767692a41324407ac81245da9c22d'
$FirstWParent='4f3f6577e3cee260ed5cbacb6059ab8669eb6c04'
$Subject='feat(ARM-CLEAN06.4W): migrate real level and palette item references to canonical ItemIds'
# Closed allowlist: one literal per line, no commas or additions.
# PowerShell 5.1 must produce exactly five repository-relative paths.
$Files=@(
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.uasset'
 'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'
)
if($Files.Count -ne 5 -or @($Files | Sort-Object -Unique).Count -ne 5) {
 Stage 'LFS_FILE_COUNT_MISMATCH'
 throw 'Native UE asset allowlist must contain exactly five distinct paths.'
}
Stage 'CHECK_GIT_PARENT'
$ResolvedHead=(@(RunGit -Arguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
$ResolvedParent=(@(RunGit -Arguments @('rev-parse','HEAD^')) | Select-Object -First 1).Trim()
$ActualSubject=(@(RunGit -Arguments @('log','-1','--format=%s')) | Select-Object -First 1).Trim()
$IsRepairCandidate=$ResolvedParent -ceq $RepairParent
$IsFinalSingleCommit=$ResolvedParent -ceq $RootParent
if($ResolvedHead -cne $ExpectedHead -or $ActualSubject -cne $Subject -or
 (-not $IsRepairCandidate -and -not $IsFinalSingleCommit)) {
 Stage 'HEAD_MISMATCH'
 throw 'Only own W repair or final one-commit W migration is permitted.'
}
if($IsRepairCandidate) {
 $GrandParent=(@(RunGit -Arguments @('rev-parse','HEAD^^')) | Select-Object -First 1).Trim()
 $GreatGrandParent=(@(RunGit -Arguments @('rev-parse','HEAD^^^')) | Select-Object -First 1).Trim()
 $GreatGreatGrandParent=(@(RunGit -Arguments @('rev-parse','HEAD^^^^')) | Select-Object -First 1).Trim()
 if($GrandParent -cne $InitialWParent -or
    $GreatGrandParent -cne $FirstWParent -or
    $GreatGreatGrandParent -cne $RootParent) {
  Stage 'HEAD_MISMATCH'
  throw 'W repair ancestry diverges from signed handoff.'
 }
}
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
$Validate=Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
$PsExe=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
foreach($File in @($Editor,$Project,$Validate,$PsExe)) {
 if(-not (Test-Path -LiteralPath $File -PathType Leaf)) {throw 'UE5.5.4 validation executable missing.'}
}
Stage 'CHECK_SOURCE_FILES_LFS'
Ensure-Clean
foreach($Relative in $Files) {
 $Lfs=@(& git -C $WorktreePath check-attr filter -- $Relative)
 if($LASTEXITCODE -ne 0 -or $Lfs.Count -ne 1 -or
  [string]$Lfs[0] -notmatch ': filter: lfs$') {
  throw 'Expected real native UE binary not tracked by Git LFS.'
 }
}
# Detached validation worktrees may contain only Git LFS pointers.
# Hydrate exactly the five signed source files, not the entire repository.
Stage 'HYDRATE_SOURCE_FILES_LFS'
$LfsLog=Join-Path $RunDir 'ARM064W.lfs-private.log'
$LfsErrorLog=Join-Path $RunDir 'ARM064W.lfs-private.stderr.log'
$Include=($Files -join ',')
# Windows PowerShell 5.1 promotes native stderr to ErrorRecords
# with ErrorActionPreference Stop. Normal Git LFS progress may abort
# the command before its exit status can be checked.
$LfsArguments='-C "'+$WorktreePath+'" lfs pull origin "--include='+$Include+'" "--exclude="'
$LfsProcess=Start-Process -FilePath 'git.exe' -ArgumentList $LfsArguments -WorkingDirectory $WorktreePath -NoNewWindow -PassThru -Wait -RedirectStandardOutput $LfsLog -RedirectStandardError $LfsErrorLog -ErrorAction Stop
if($LfsProcess.ExitCode -ne 0) {
 Stage 'LFS_DOWNLOAD_FAILED'
 throw 'Cannot download the five native UE binaries from Git LFS.'
}
# git lfs pull can return success after fetching without replacing a pointer
# in a detached Windows validation worktree. Enforce explicit checkout.
Stage 'LFS_CHECKOUT_SOURCE_FILES'
$CheckoutArguments='-C "'+$WorktreePath+'" lfs checkout'
foreach($Relative in $Files) { $CheckoutArguments+=' "'+$Relative+'"' }
$CheckoutStdout=Join-Path $RunDir 'ARM064W.checkout-private.log'
$CheckoutStderr=Join-Path $RunDir 'ARM064W.checkout-private.stderr.log'
$Checkout=Start-Process -FilePath 'git.exe' -ArgumentList $CheckoutArguments -WorkingDirectory $WorktreePath -NoNewWindow -PassThru -Wait -RedirectStandardOutput $CheckoutStdout -RedirectStandardError $CheckoutStderr -ErrorAction Stop
if($Checkout.ExitCode -ne 0) {
 Stage 'LFS_CHECKOUT_FAILED'
 throw 'Exact five Git LFS pointers could not be checked out.'
}
# Only a completely clean detached worktree may replace its LFS placeholders.
Ensure-Clean
# The protected 06.4G backup is used only in the temporary validation
# checkout, and only with exact proof against HEAD's Git LFS OID+size.
$Unhydrated=New-Object System.Collections.ArrayList
foreach($Relative in $Files) {
 $Candidate=Join-Path $WorktreePath $Relative.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Candidate -PathType Leaf)) {
  # Permit absent LFS path only after signed Git pointer / backup proof.
  [void]$Unhydrated.Add($Relative)
 } elseif((Get-Item -LiteralPath $Candidate).Length -le 200) {
  [void]$Unhydrated.Add($Relative)
 }
}
if($Unhydrated.Count -gt 0) {
 if(-not $IsRepairCandidate) {
  Stage 'LFS_FINAL_BINARY_UNAVAILABLE'
  throw 'Final migrated UE binaries must come from their own Git LFS OIDs.'
 }
 Stage 'LFS_SIGNED_BACKUP_PREFLIGHT'
 $SnapshotSha='e0f877d17f39d913c585b55ba18110745c5385b6'
 Stage 'LFS_BACKUP_SCAN_RUNS'
 $Archive=Join-Path $StateRoot 'Runs'
 $Snapshots=New-Object System.Collections.ArrayList
 if(Test-Path -LiteralPath $Archive -PathType Container) {
  foreach($Run in @(Get-ChildItem -LiteralPath $Archive -Directory -ErrorAction Stop)) {
   $Place=Join-Path $Run.FullName 'ARM-CLEAN06.4G'
   $ReceiptPath=Join-Path $Place 'binary-snapshot-receipt.json'
   $GatePath=Join-Path $Place 'transaction-gate-private.json'
   if(-not (Test-Path -LiteralPath $ReceiptPath -PathType Leaf) -or
      -not (Test-Path -LiteralPath $GatePath -PathType Leaf)) {continue}
   Stage 'LFS_BACKUP_READ_RECEIPT'
   $Receipt=Get-Content -LiteralPath $ReceiptPath -Raw -Encoding UTF8 | ConvertFrom-Json
   if([string]$Receipt.validation_sha -cne $SnapshotSha) {continue}
   [void]$Snapshots.Add([pscustomobject]@{
    Root=$Place; ReceiptPath=$ReceiptPath; GatePath=$GatePath; Receipt=$Receipt
   })
  }
 }
 Stage 'LFS_BACKUP_CHECK_UNIQUE'
 if($Snapshots.Count -ne 1) {
  Stage 'LFS_SIGNED_BACKUP_MISSING'
  throw 'Exactly one protected snapshot must exist.'
 }
 $Snapshot=$Snapshots[0]
 Stage 'LFS_BACKUP_READ_GATE'
 $Gate=Get-Content -LiteralPath $Snapshot.GatePath -Raw -Encoding UTF8 | ConvertFrom-Json
 Stage 'LFS_BACKUP_REHASH_RECEIPT'
 $ReceiptHash=(Get-FileHash -LiteralPath $Snapshot.ReceiptPath -Algorithm SHA256).Hash.ToLowerInvariant()
 Stage 'LFS_BACKUP_VERIFY_GATE'
 if($Snapshot.Receipt.schema -cne 'ARM-CLEAN06.4G/private-binary-snapshot/v1' -or
    [int]$Snapshot.Receipt.binary_file_count -ne 84 -or
    [int]$Snapshot.Receipt.legacy_file_count -ne 56 -or
    [int]$Snapshot.Receipt.canonical_file_count -ne 28 -or
    $Snapshot.Receipt.copy_verified -ne $true -or
    $Snapshot.Receipt.sandbox_restore_drill_verified -ne $true -or
    @($Snapshot.Receipt.source_files).Count -ne 84 -or
    $Gate.schema -cne 'ARM-CLEAN06.4G/transaction-gate/v1' -or
    $Gate.validation_sha -cne $SnapshotSha -or
    $Gate.snapshot_receipt_sha256 -cne $ReceiptHash -or
    $Gate.permitted_restore_path -cne 'PRIVATE_SANDBOX_ONLY' -or
    $Gate.migration_authorized -ne $false -or
    @($Gate.allowed_binary_writes).Count -ne 0 -or
    @($Gate.allowed_savegame_writes).Count -ne 0) {
  Stage 'LFS_SIGNED_BACKUP_INVALID'
  throw 'Protected snapshot receipt, gate or SHA-256 proof is invalid.'
 }
 $BackupRoot=Join-Path $Snapshot.Root 'SourceSnapshot'
 # Prove ALL missing packages first. No partial copy when later OIDs differ.
 $Proven=New-Object System.Collections.ArrayList
 Stage 'LFS_BACKUP_PROVE_OIDS'
 # Immutable OIDs published by Git at exact handoff commit 5b7131fe.
 # Their order must be identical to the closed five-file list above.
 $Pinned=@{
  'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset'=[pscustomobject]@{Hash='481b68d7caa337254ab1ad50be5e7f39ca3483b48892d63aa7fe7392c141e750';Bytes=732114}
  'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.uasset'=[pscustomobject]@{Hash='b0c6346cdfed94097e70fbe09e15faac6b62beeb0f0e78345848a911cc695333';Bytes=450769}
  'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_New_Level_03.uasset'=[pscustomobject]@{Hash='9b35a5956ad685e800a4a5ac4e11199d5f6169c470246929162c134c88b50ff1';Bytes=414789}
  'Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_Pillars_of_Light.uasset'=[pscustomobject]@{Hash='d1bcf3157fb65cb4bd95558cecf3cfaedd55ec98c97f114c061c5626a939c867';Bytes=414878}
  'Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset'=[pscustomobject]@{Hash='d01876ac01161fb64c4aeddcf6e2b61658c411a6ca5093935f5de547d1cbe6a3';Bytes=40623}
 }
 if($Pinned.Count -ne 5 -or $Files.Count -ne 5) {
  Stage 'LFS_PIN_COUNT_MISMATCH'
  throw 'Closed signed Git LFS reference manifest has unexpected count.'
 }
 foreach($ExpectedPath in $Files) {
  if(-not $Pinned.ContainsKey([string]$ExpectedPath)) {
   Stage 'LFS_PIN_PATH_MISMATCH'
   throw 'Signed Git LFS reference does not cover an exact native UE path.'
  }
 }
 # No W candidate commit may modify an original level or palette file
 # before the exact five signed UE binaries are saved by the Editor.
 Stage 'LFS_PINNED_HEAD_DIVERGED'
 $PriorAssetChanges=@(RunGit -Arguments (@('diff','--name-only',$RootParent,'HEAD','--')+$Files))
 if($PriorAssetChanges.Count -ne 0) {
  throw 'Tracked UE assets changed after the signed original baseline.'
 }
 foreach($Relative in $Unhydrated.ToArray()) {
  Stage 'LFS_BACKUP_PREPARE_ENTRY'
  if(-not $Pinned.ContainsKey([string]$Relative)) {
   Stage 'LFS_PIN_PATH_MISMATCH'
   throw 'Unapproved UE asset in protected hydration allowlist.'
  }
  $Destination=Join-Path $WorktreePath ([string]$Relative).Replace('/','\')
  $ExpectedHash=[string]$Pinned[[string]$Relative].Hash
  $ExpectedBytes=[int64]$Pinned[[string]$Relative].Bytes
  Stage 'LFS_BACKUP_VALIDATE_POINTER'
  # Clean Git state + unchanged baseline tree + signed OIDs are authoritative.
  # A <=200-byte local placeholder has no authority over the verified backup.
  if(Test-Path -LiteralPath $Destination -PathType Leaf) {
   if((Get-Item -LiteralPath $Destination).Length -gt 200) {
    Stage 'LFS_LOCAL_PLACEHOLDER_DIRTY'
    throw 'Refuse overwriting a non-placeholder Unreal package.'
   }
  }
  Stage 'LFS_BACKUP_LOOKUP_RECORD'
  $Records=@($Snapshot.Receipt.source_files | Where-Object {
   [string]$_.relative -ceq $Relative
  })
  if($Records.Count -ne 1) {
   Stage 'LFS_SIGNED_BACKUP_INVALID'
   throw 'Tracked binary absent from signed source inventory.'
  }
  $Backup=Join-Path $BackupRoot $Relative.Replace('/','\')
  if(-not (Test-Path -LiteralPath $Backup -PathType Leaf)) {
   Stage 'LFS_SIGNED_BACKUP_MISSING'
   throw 'Tracked binary absent from private source snapshot.'
  }
  Stage 'LFS_BACKUP_REHASH_FILE'
  $BackupItem=Get-Item -LiteralPath $Backup
  $BackupHash=(Get-FileHash -LiteralPath $Backup -Algorithm SHA256).Hash.ToLowerInvariant()
  if($BackupItem.Length -ne $ExpectedBytes -or
     $BackupItem.Length -ne [int64]$Records[0].bytes -or
     $BackupHash -cne $ExpectedHash -or
     $BackupHash -cne [string]$Records[0].sha256) {
   Stage 'LFS_SNAPSHOT_OID_MISMATCH'
   throw 'Protected backup differs from Git-tracked LFS SHA-256 or byte size.'
  }
  [void]$Proven.Add([pscustomobject]@{
   Source=$Backup;Destination=$Destination;Hash=$ExpectedHash
  })
 }
 # Only after complete preflight, hydrate exact originals in isolated worktree.
 Stage 'LFS_BACKUP_COPY_VERIFIED'
 foreach($Item in $Proven.ToArray()) {
  $ParentDir=Split-Path -Parent $Item.Destination
  if(-not (Test-Path -LiteralPath $ParentDir -PathType Container)) {
   New-Item -Path $ParentDir -ItemType Directory -Force -ErrorAction Stop | Out-Null
  }
  Copy-Item -LiteralPath $Item.Source -Destination $Item.Destination -Force -ErrorAction Stop
  if((Get-FileHash -LiteralPath $Item.Destination -Algorithm SHA256).Hash.ToLowerInvariant() -cne $Item.Hash) {
   Stage 'LFS_BACKUP_COPY_HASH_MISMATCH'
   throw 'Sandbox hydration did not preserve binary SHA-256.'
  }
 }
 Stage 'LFS_SIGNED_BACKUP_HYDRATED'
}
foreach($Relative in $Files) {
 $Absolute=Join-Path $WorktreePath $Relative.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Absolute -PathType Leaf) -or
  (Get-Item -LiteralPath $Absolute).Length -le 200) {
  Stage 'LFS_POST_RESTORE_POINTER'
  throw 'Expected UE asset is absent or remains a Git LFS pointer.'
 }
 $Stream=[System.IO.File]::OpenRead($Absolute)
 try {
  [byte[]]$Header=@(0,0,0,0)
  if($Stream.Read($Header,0,4) -ne 4 -or
   $Header[0] -ne 0xC1 -or $Header[1] -ne 0x83 -or
   $Header[2] -ne 0x2A -or $Header[3] -ne 0x9E) {
   Stage 'LFS_INVALID_PACKAGE_HEADER'
   throw 'Hydrated source does not have Unreal package signature.'
  }
 } finally {$Stream.Dispose()}
}
Ensure-Clean
Stage 'CHECK_COMMIT_FILESET'
$Diff=@(RunGit -Arguments @('diff-tree','--no-commit-id','--name-only','-r','HEAD'))
$Committed=@($Diff | Where-Object {$Files -ccontains $_})
$AlreadyMigrated=$Committed.Count -eq 5
if($Committed.Count -ne 0 -and -not $AlreadyMigrated) {
 throw 'Mixed binary/software W commit: fail closed.'
}
if(($AlreadyMigrated -and -not $IsFinalSingleCommit) -or
 (-not $AlreadyMigrated -and -not $IsRepairCandidate)) {
 Stage 'HEAD_MISMATCH'
 throw 'Materialization allowed only on W repair; verification only on final single W commit.'
}
if($AlreadyMigrated) {
 $Binaries=@($Diff | Where-Object {$_.EndsWith('.uasset',[StringComparison]::OrdinalIgnoreCase)})
 if($Binaries.Count -ne 5 -or
  @($Binaries | Where-Object {$Files -cnotcontains $_}).Count -ne 0) {
  throw 'Extra binary content in final W commit.'
 }
}
Stage 'START'
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4W'
New-Item -ItemType Directory -Path $Folder -ErrorAction Stop | Out-Null
$Report=Join-Path $Folder 'materialized-private.json'
$UELog=Join-Path $Folder 'unreal-editor-private.log'
$Mode=if($AlreadyMigrated){'-Verify'}else{'-Apply'}
$Recovery=Join-Path $StateRoot ('Recovery\ARM4W-'+
 $ExpectedHead.Substring(0,12)+'-'+(Get-Date -Format 'yyyyMMddHHmmssfff'))
$BackedUp=$false
try {
 if(-not $AlreadyMigrated) {
  Stage 'SAVE_PRIVATE_BACKUPS'
  Ensure-Clean
  foreach($Relative in $Files) {
   $From=Join-Path $WorktreePath $Relative.Replace('/','\')
   $To=Join-Path $Recovery $Relative.Replace('/','\')
   New-Item -ItemType Directory -Path (Split-Path -Parent $To) -Force -ErrorAction Stop | Out-Null
   Copy-Item -LiteralPath $From -Destination $To -ErrorAction Stop
   if((Get-FileHash -LiteralPath $From -Algorithm SHA256).Hash -cne
      (Get-FileHash -LiteralPath $To -Algorithm SHA256).Hash) {
     throw 'Private pre-migration SHA256 backup mismatch.'
   }
  }
  $BackedUp=$true
 }
 Stage $(if($AlreadyMigrated){'VERIFY_COMMITTED'}else{'APPLY_UNREAL'})
 & $Editor $Project '-run=GridArmClean064WWorldItemReferences' '-Unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' $Mode "-Output=$Report" *> $UELog
 if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Report -PathType Leaf)) {
  Stage 'UNREAL_MATERIALIZATION_FAILED'
  throw 'Actual UE asset write/verify failed; see local private commandlet log.'
 }
 $R=Get-Content -LiteralPath $Report -Raw -Encoding UTF8 | ConvertFrom-Json
 Validate-Report $R (-not $AlreadyMigrated) $Files
 if($AlreadyMigrated) {
  Stage 'FINAL_BINARY_SET'
  Ensure-Clean
  $Body=Join-Path $Folder 'github-issue.md'
  @(
    '# ARM-CLEAN06.4W - real world item reference migration'
    ''
    ('Verified final commit: '+$ExpectedHead)
    ''
    '| Verifiable fact | Count |'
    '|---|---:|'
    '| Real grid levels saved and reopened | 4 |'
    '| Real palette assets saved and reopened | 1 |'
    '| Loose item references with canonical ItemId | 44 |'
    '| Receptacle content references with canonical ItemId | 18 |'
    '| Direct palette entries with canonical ItemId | 33 |'
    '| Total actual persisted ItemIds | 95 |'
    '| Newly resolvable former missing identities | 40 |'
    '| Native item definitions in central registry | 34 |'
    '| SaveGames/UMaps/Blueprints modified | 0 |'
    '| New runtime legacy fallback paths | 0 |'
    ''
    'The old pointer properties are retained solely until the immediate runtime/schema cutover; W writes the new ItemId values into the real .uasset packages.'
    'All original item names, placement GUIDs and source paths remain in the private UE reports.'
  ) | Set-Content -LiteralPath $Body -Encoding UTF8
  Stage 'PUBLISH_FINAL_RECEIPT'
  $Issue=@(& gh issue create --repo $GitHubRepository --title ('[ARM-CLEAN06.4W] Real 95 world ItemIds '+$ExpectedHead.Substring(0,12)) --body-file $Body)
  if($LASTEXITCODE -ne 0 -or $Issue.Count -ne 1) {
   throw 'Could not publish aggregate-only final native migration receipt.'
  }
  Stage 'SUCCESS'
  Write-Host ('[ARM-CLEAN06.4W] VERIFIED_FINAL_WORLD_ITEMIDS '+$Issue[0])
  exit 0
 }
 Stage 'EXACT_FIVE_CHANGED'
 $Dirty=@(RunGit -Arguments @('status','--porcelain','--untracked-files=all'))
 if($Dirty.Count -ne 5) {throw 'Not exactly five intended generated UE assets.'}
 foreach($Entry in $Dirty) {
  $Relative=([string]$Entry).Substring(3).Replace('\','/')
  if(([string]$Entry).Substring(0,2) -cne ' M' -or
     $Files -cnotcontains $Relative) {
   throw 'Unapproved generated or modified file; publication refused.'
  }
 }
 Stage 'VERIFY_RELOADED_UNREAL'
 $Verify=Join-Path $Folder 'reloaded-private.json'
 $VerifyLog=Join-Path $Folder 'unreal-verify-private.log'
 & $Editor $Project '-run=GridArmClean064WWorldItemReferences' '-Unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' '-Verify' "-Output=$Verify" *> $VerifyLog
 if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Verify -PathType Leaf)) {
  Stage 'UNREAL_REOPEN_FAILED'
  throw 'Independent UE Editor verification failed.'
 }
 $V=Get-Content -LiteralPath $Verify -Raw -Encoding UTF8 | ConvertFrom-Json
 Validate-Report $V $false $Files
 Stage 'POST_MIGRATION_AUTOMATION'
 $PostDir=Join-Path $Folder 'post-automation'
 $OutLog=Join-Path $Folder 'post-automation.stdout.log'
 $ErrLog=Join-Path $Folder 'post-automation.stderr.log'
 $Args='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "'+
   $Validate+'" -EngineRoot "'+$EngineRoot+
   '" -SkipBuild -AutomationFilter "Grimrock" -ReportRoot "'+$PostDir+'"'
 $Run=Start-Process -FilePath $PsExe -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $OutLog -RedirectStandardError $ErrLog -ErrorAction Stop
 if($Run.ExitCode -ne 0) {
  Stage 'POST_AUTOMATION_FAILED'
  throw 'Real converted native assets failed the full Grimrock suite.'
 }
 $After=@(RunGit -Arguments @('status','--porcelain','--untracked-files=all'))
 if((@($After) -join '|') -cne (@($Dirty) -join '|')) {
  throw 'Automation unexpectedly modified the generated asset file set.'
 }
 Stage 'REMOTE_LEASE'
 [void](RunGit -Arguments @('fetch','--no-tags','origin','master'))
 $Remote=(@(RunGit -Arguments @('rev-parse','refs/remotes/origin/master')) | Select-Object -First 1).Trim()
 if($Remote -cne $ExpectedHead) {throw 'Remote master moved during migration.'}
 Stage 'STAGE_FIVE_REAL_FILES'
 [void](RunGit -Arguments (@('add','--')+$Files))
 $Staged=@(RunGit -Arguments @('diff','--cached','--name-only','--diff-filter=ACMR'))
 if($Staged.Count -ne 5 -or @($Staged | Where-Object {$Files -cnotcontains $_}).Count -ne 0) {
  throw 'Git staged files outside the five-file native allowlist.'
 }
 Stage 'AMEND_ONLY_OWN_COMMIT'
 # Fold W source, LFS hydration and native-signature guard into one W commit.
 [void](RunGit -Arguments @('reset','--soft',$RootParent))
 $CodeFiles=@(
  'Scripts/ApplyARM_CLEAN06_4W_WorldReferenceMigration.ps1',
  'Scripts/UEValidationAgent.ps1',
  'Source/GrimrockPrototype/Public/Core/GridLevelPlacementTypes.h',
  'Source/GrimrockPrototype/Public/Core/GridObjectBehavior.h',
  'Source/GrimrockPrototype/Public/Core/GridObjectPaletteAsset.h',
  'Source/GrimrockPrototypeEditor/Private/ARMEMENT/GridArmClean064WWorldItemReferencesCommandlet.cpp',
  'Source/GrimrockPrototypeEditor/Private/Tests/GridEditorARMCLEAN064WWorldReferencesTests.cpp',
  'Source/GrimrockPrototypeEditor/Public/ARMEMENT/GridArmClean064WWorldItemReferencesCommandlet.h',
  'docs/Architecture/ARM_CLEAN06_4W_REAL_WORLD_ITEM_REFERENCES.md'
 )
 $Combined=@(RunGit -Arguments @('diff','--cached','--name-only','--diff-filter=ACMR'))
 $Allowed=@($CodeFiles)+@($Files)
 if($Combined.Count -ne $Allowed.Count -or
   @($Combined | Where-Object {$Allowed -cnotcontains $_}).Count -ne 0 -or
   @($Allowed | Where-Object {$Combined -cnotcontains $_}).Count -ne 0) {
  throw 'Single W commit must include exactly nine source files and five native UAssets.'
 }
 & git -C $WorktreePath -c 'user.name=Grimrock UE Automation' -c 'user.email=grimrock-ue-automation@users.noreply.github.com' commit -m $Subject | Out-Null
 if($LASTEXITCODE -ne 0) {throw 'Cannot create single final W commit.'}
 $NewHead=(@(RunGit -Arguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
 $NewParent=(@(RunGit -Arguments @('rev-parse','HEAD^')) | Select-Object -First 1).Trim()
 if($NewHead -ceq $ExpectedHead -or $NewParent -cne $RootParent) {
  throw 'Final W commit must preserve exactly the validated handoff parent.'
 }
 Ensure-Clean
 Stage 'PUSH_WITH_LEASE'
 & git -C $WorktreePath push ('--force-with-lease=refs/heads/master:'+$ExpectedHead) origin HEAD:refs/heads/master | Out-Null
 if($LASTEXITCODE -ne 0) {throw 'Failed to publish real W assets with Git lease.'}
 $BackedUp=$false
 Stage 'SUCCESS'
 Write-Host ('[ARM-CLEAN06.4W] FIVE_REAL_WORLD_ASSETS_PUSHED '+$NewHead)
}
catch {
 # A failure before commit/push is recoverable only when this exact W
 # operation caused no other changes. Otherwise fail closed untouched.
 if($BackedUp) {
  try {
   # Never restore parent bytes over a newly amended Git commit when a
   # network failure interrupts the lease push. In that case keep its
   # index/worktree intact for targeted retry; restore only our own
   # original in-progress binary authoring against the original HEAD.
   $RecoveryHead=(@(RunGit -Arguments @('rev-parse','HEAD')) | Select-Object -First 1).Trim()
   if($RecoveryHead -cne $ExpectedHead) {
    throw 'Committed W SHA changed; private restore forbidden.'
   }
   $DirtyNow=@(RunGit -Arguments @('status','--porcelain','--untracked-files=all'))
   if($DirtyNow.Count -le 5 -and
     @($DirtyNow | Where-Object {
       ([string]$_).Length -lt 4 -or
       ([string]$_).Substring(0,2) -cne ' M' -or
       $Files -cnotcontains ([string]$_).Substring(3).Replace('\','/')
     }).Count -eq 0) {
    foreach($Relative in $Files) {
     $Saved=Join-Path $Recovery $Relative.Replace('/','\')
     $Dest=Join-Path $WorktreePath $Relative.Replace('/','\')
     if(-not (Test-Path -LiteralPath $Saved -PathType Leaf)) {
      throw 'Private immutable backup missing.'
     }
     Copy-Item -LiteralPath $Saved -Destination $Dest -Force -ErrorAction Stop
     if((Get-FileHash -LiteralPath $Saved -Algorithm SHA256).Hash -cne
        (Get-FileHash -LiteralPath $Dest -Algorithm SHA256).Hash) {
      throw 'Private restoration did not match original SHA256.'
     }
    }
    $Remain=@(RunGit -Arguments @('status','--porcelain','--untracked-files=all'))
    if($Remain.Count -ne 0) {throw 'Residual dirty worktree after private restore.'}
    Write-Host '[ARM-CLEAN06.4W] Five source UAssets restored from verified private backup.'
   }
  }
  catch {Write-Warning '[ARM-CLEAN06.4W] Exact safe backup recovery was not possible.'}
 }
 throw
}
