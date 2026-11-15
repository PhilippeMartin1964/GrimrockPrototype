<#
 ARM-CLEAN06.4T: one-time true UE Editor asset materialization with atomic amend-and-push. Private real asset values stay in Windows StateRoot. No SavePackage.
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
$OwnershipSha='4d5e7eb8b1406d29eeb9b0805f82507aa8a1bfc7'
$SnapshotSha='e0f877d17f39d913c585b55ba18110745c5385b6'
$ProjectionSha='d1548ca7ad69162dcba12bc8f8570b6d6004135c'
$IdentitySha='d3fc4e10de5b82e5669c4dd03c309c30abeab9c6'
$PresentationSha='a279761d52fba8d8a5280104e009bd336cb6c2a1'
$ReadingWorldSha='8d925f186e02f419c3b5087f7df8f8f848548552'
$ThrowSha='99bd3afc3207830c76542078eaba417dcbbc9b63'
$ActionSha='20f641621cff0f2db0c12a94f9ff38176f0ca666'
$EquipmentSha='29955df612e1478b94f30a706bac009df3c8d531'
$StackSha='608a83c76205cfc126088fc19f36d5f0e2e173aa'
$BridgeSha='37cf07dc21359a9762c6dd244eebe74bba02bc41'
$StagePath=Join-Path $RunDir 'ARMClean064T.stage'
function Stage([string]$Value) {
 Set-Content -LiteralPath $StagePath -Value $Value -Encoding ASCII
}
function Hash([string]$Path) {
 (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Clean {
 $Lines=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Lines.Count -ne 0) {
  Stage 'WORKTREE_CHANGED'
  throw 'Validation worktree must remain clean.'
 }
}
function Private-Report([string]$StageName,[string]$File,[string]$Selector,[string]$Match) {
 $Root=Join-Path $StateRoot 'Runs'
 if(-not (Test-Path -LiteralPath $Root -PathType Container)) {
  throw 'Private Windows validation history unavailable.'
 }
 $Results=New-Object System.Collections.ArrayList
 foreach($Run in @(Get-ChildItem -LiteralPath $Root -Directory -ErrorAction Stop)) {
  $Path=Join-Path (Join-Path $Run.FullName $StageName) $File
  if(-not (Test-Path -LiteralPath $Path -PathType Leaf)) {continue}
  $Data=Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
  $Value=$Data.PSObject.Properties[$Selector]
  if($null -ne $Value -and [string]$Value.Value -ceq $Match) {
   [void]$Results.Add([pscustomobject]@{Path=$Path;Data=$Data})
  }
 }
 if($Results.Count -eq 0) {
  if($StageName -eq 'ARM-CLEAN06.4K') {Stage 'READ_4K_MISSING'}
  throw 'Exact historical report missing.'
 }
 if($Results.Count -eq 1) {return $Results[0]}
 # R may have harmless reruns. Only a report signed by the final R commit
 # is eligible. A report from an abandoned commit or failed run is ignored.
 if($StageName -ceq 'ARM-CLEAN06.4R') {
  Stage 'READ_4R_DUPLICATE_PROVENANCE'
  $Trusted=New-Object System.Collections.ArrayList
  foreach($Candidate in $Results) {
   $GateFile=Join-Path (Split-Path -Parent $Candidate.Path) 'action-bridge-gate-private.json'
   if(-not (Test-Path -LiteralPath $GateFile -PathType Leaf)) {continue}
   try {
    $Proof=Get-Content -LiteralPath $GateFile -Raw -Encoding UTF8 | ConvertFrom-Json
    if($Proof.schema -cne 'ARM-CLEAN06.4R/lossless-action-gate/v1' -or
     $Proof.validation_sha -cne $BridgeSha -or
     $Proof.source_ownership_sha -cne $OwnershipSha -or
     $Proof.source_action_inventory_sha -cne $ActionSha -or
     $Proof.source_stack_sha -cne $StackSha -or
     $Proof.protected_backup_sha -cne $SnapshotSha -or
     $Proof.action_bridge_report_sha256 -cne (Hash $Candidate.Path) -or
     [int]$Candidate.Data.detailed_actions_projected -ne 4 -or
     [int]$Candidate.Data.detailed_presentations_projected -ne 1 -or
     [int]$Candidate.Data.nondefault_nested_properties_preserved -ne 54 -or
     [int]$Proof.approved_runtime_action_routes -ne 0 -or
     [bool]$Proof.migration_authorized -or
     @($Proof.allowed_binary_writes).Count -ne 0 -or
     @($Proof.allowed_package_deletions).Count -ne 0 -or
     @($Proof.allowed_savegame_writes).Count -ne 0) {continue}
    [void]$Trusted.Add($Candidate)
   } catch {continue}
  }
  if($Trusted.Count -eq 0) {
   Stage 'READ_4R_NO_TRUSTED_COPY'
   throw 'No signed final 06.4R report found among retries.'
  }
  Stage 'READ_4R_COMPARE_COPIES'
  $Reference=Hash $Trusted[0].Path
  foreach($Candidate in $Trusted) {
   if((Hash $Candidate.Path) -cne $Reference) {
    Stage 'READ_4R_CONFLICTING_COPIES'
    throw 'Different approved 06.4R reports: transaction blocked.'
   }
  }
  Stage 'READ_4R_DUPLICATES_RECONCILED'
  $Selected=@($Trusted | Sort-Object -Property Path)[0]
  return $Selected
 }
 # Earlier Q retries returned valid JSON but failed due to a transient
 # GameInstance Outer ensure; only the FINAL successful Q has a trusted
 # SHA-pinned gate. Never select an unapproved failed retry by timestamp.
 if($StageName -ceq 'ARM-CLEAN06.4Q') {
  Stage 'READ_4Q_DUPLICATE_PROVENANCE'
  $Trusted=New-Object System.Collections.ArrayList
  foreach($Candidate in $Results) {
   $P=Join-Path (Split-Path -Parent $Candidate.Path) 'stack-gate-private.json'
   if(-not (Test-Path -LiteralPath $P -PathType Leaf)) {continue}
   try {
    $G=Get-Content -LiteralPath $P -Raw -Encoding UTF8 | ConvertFrom-Json
    if($G.schema -cne 'ARM-CLEAN06.4Q/nonarm-stack-gate/v1' -or
     $G.validation_sha -cne $StackSha -or
     $G.source_ownership_sha -cne $OwnershipSha -or
     $G.source_equipment_sha -cne $EquipmentSha -or
     $G.protected_backup_sha -cne $SnapshotSha -or
     $G.stack_report_sha256 -cne (Hash $Candidate.Path) -or
     [int]$Candidate.Data.verified_item_ids -ne 11 -or
     [int]$Candidate.Data.active_nonarm_stack_fields -ne 6 -or
     [int]$Candidate.Data.transient_registry_definitions -ne 34 -or
     [int]$G.true_registry_level_probes -ne 44 -or
     $Candidate.Data.migration_authorized -ne $false -or
     [bool]$G.migration_authorized -or
     @($G.allowed_binary_writes).Count -ne 0 -or
     @($G.allowed_package_deletions).Count -ne 0 -or
     @($G.allowed_savegame_writes).Count -ne 0) {
     continue
    }
    [void]$Trusted.Add($Candidate)
   }
   catch {continue}
  }
  if($Trusted.Count -eq 0) {
   Stage 'READ_4Q_NO_TRUSTED_COPY'
   throw 'No pinned successful 06.4Q result+gate exists.'
  }
  Stage 'READ_4Q_COMPARE_TRUSTED_COPIES'
  $ReferenceHash=Hash $Trusted[0].Path
  foreach($Candidate in $Trusted) {
   if((Hash $Candidate.Path) -cne $ReferenceHash) {
    Stage 'READ_4Q_CONFLICTING_COPIES'
    throw 'Multiple mutually inconsistent 06.4Q approved reports.'
   }
  }
  Stage 'READ_4Q_DUPLICATES_RECONCILED'
  $Selected=@($Trusted | Sort-Object -Property Path)[0]
  return $Selected
 }
 # Windows validation retries may leave several 06.4K reports. Never
 # accept a random one: every selected candidate needs the exact pinned
 # provenance receipt, and all eligible semantic inventories must agree.
 if($StageName -cne 'ARM-CLEAN06.4K') {
  throw 'Ambiguous private historical reports.'
 }
 Stage 'READ_4K_DUPLICATE_PROVENANCE'
 $Trusted=New-Object System.Collections.ArrayList
 foreach($Candidate in $Results) {
  $P=Join-Path (Split-Path -Parent $Candidate.Path) 'field-owner-gate-private.json'
  if(-not (Test-Path -LiteralPath $P -PathType Leaf)) {continue}
  try {
   $G=Get-Content -LiteralPath $P -Raw -Encoding UTF8 | ConvertFrom-Json
   if($G.schema -cne 'ARM-CLEAN06.4K/field-owner-gate/v1' -or
    $G.validation_sha -cne $OwnershipSha -or
    $G.source_projection_sha -cne $ProjectionSha -or
    $G.source_identity_sha -cne $IdentitySha -or
    $G.protected_backup_sha -cne $SnapshotSha -or
    $G.ownership_report_sha256 -cne (Hash $Candidate.Path) -or
    [int]$G.parity_approved_fields -ne 0 -or
    [bool]$G.migration_authorized -or
    @($G.allowed_binary_writes).Count -ne 0 -or
    @($G.allowed_package_deletions).Count -ne 0 -or
    @($G.allowed_savegame_writes).Count -ne 0 -or
    [int]$Candidate.Data.verified_nonarm_item_ids -ne 11 -or
    [int]$Candidate.Data.verified_world_occurrences -ne 40 -or
    [int]$Candidate.Data.active_unmapped_property_occurrences -ne 50 -or
    [int]$Candidate.Data.distinct_active_unmapped_property_names -ne 26 -or
    @($Candidate.Data.private_item_ownership).Count -ne 11) {
    continue
   }
   [void]$Trusted.Add($Candidate)
  }
  catch {continue}
 }
 if($Trusted.Count -eq 0) {
  Stage 'READ_4K_NO_TRUSTED_COPY'
  throw 'No valid pinned 06.4K report+receipt among historical retries.'
 }
 Stage 'READ_4K_COMPARE_COPIES'
 function Semantic-Fingerprint([object]$Report) {
  $Inventory=@()
  foreach($Item in @($Report.private_item_ownership | Sort-Object item_id)) {
   $Fields=@()
   foreach($Field in @($Item.field_reviews | Sort-Object property)) {
    $Fields+=([ordered]@{
     property=[string]$Field.property
     source_value=[string]$Field.source_value_exact
     domain=[string]$Field.domain
     required_contract=[string]$Field.required_contract
     status=[string]$Field.status
    })
   }
   $Inventory+=([ordered]@{
    item_id=[string]$Item.item_id
    source_asset=[string]$Item.source_asset
    candidate_family=[string]$Item.candidate_family
    affected_world_occurrences=[int]$Item.affected_world_occurrences
    active_unmapped_fields=[int]$Item.active_unmapped_fields
    field_reviews=$Fields
   })
  }
  return (ConvertTo-Json -InputObject $Inventory -Compress -Depth 16)
 }
 $Reference=Semantic-Fingerprint $Trusted[0].Data
 foreach($Candidate in $Trusted) {
  if((Semantic-Fingerprint $Candidate.Data) -cne $Reference) {
   Stage 'READ_4K_CONFLICTING_COPIES'
   throw 'Different pinned 06.4K semantic inventories; fail closed.'
  }
 }
 # Same verified content, independent receipts. A stable path order avoids
 # depending on Get-ChildItem enumeration order. No files are changed.
 $Selected=@($Trusted | Sort-Object -Property Path)[0]
 Stage 'READ_4K_DUPLICATES_RECONCILED'
 return $Selected
}
function Assert-Closed([object]$Gate) {
 if([bool]$Gate.migration_authorized -or
  @($Gate.allowed_binary_writes).Count -ne 0 -or
  @($Gate.allowed_package_deletions).Count -ne 0 -or
  @($Gate.allowed_savegame_writes).Count -ne 0) {
  throw 'A protected previous gate unexpectedly authorized writes.'
 }
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$OwnershipSha,$SnapshotSha,$ProjectionSha,$IdentitySha,$PresentationSha,$ReadingWorldSha,$ThrowSha,$ActionSha,$EquipmentSha,$StackSha,$BridgeSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') {throw 'Invalid pinned SHA.'}
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH'
 throw 'Detached validation worktree is not the expected revision.'
}
$Branch=@(& git -C $WorktreePath symbolic-ref --quiet --short HEAD 2>$null)
if($LASTEXITCODE -eq 0 -and $Branch.Count -gt 0) {
 throw 'Only detached validation worktrees allowed.'
}
$RunFull=[IO.Path]::GetFullPath($RunDir).TrimEnd('\','/')
$StateFull=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
$RepoFull=[IO.Path]::GetFullPath($WorktreePath).TrimEnd('\','/')
if(-not $RunFull.StartsWith(($StateFull+'\'),[StringComparison]::OrdinalIgnoreCase) -or
 $RunFull.StartsWith(($RepoFull+'\'),[StringComparison]::OrdinalIgnoreCase)) {
 throw 'Private report path must remain outside the project.'
}
Clean
Stage 'READ_4K_LOCATE'
$Previous=Private-Report 'ARM-CLEAN06.4K' 'ownership-private.json' 'schema' 'ARM-CLEAN06.4K/field-ownership/v1'
Stage 'READ_4K_GATE_EXISTS'
$GatePath=Join-Path (Split-Path -Parent $Previous.Path) 'field-owner-gate-private.json'
if(-not (Test-Path -LiteralPath $GatePath -PathType Leaf)) {
 throw '06.4K proof missing.'
}
Stage 'READ_4K_GATE_READ'
$Gate=Get-Content -LiteralPath $GatePath -Raw -Encoding UTF8 | ConvertFrom-Json
Stage 'READ_4K_GATE_SCHEMA'
if($Gate.schema -cne 'ARM-CLEAN06.4K/field-owner-gate/v1' -or
 $Gate.validation_sha -cne $OwnershipSha -or
 $Gate.source_projection_sha -cne $ProjectionSha -or
 $Gate.source_identity_sha -cne $IdentitySha -or
 $Gate.protected_backup_sha -cne $SnapshotSha -or
 [int]$Gate.parity_approved_fields -ne 0) {
 throw '06.4K validation or provenance does not match.'
}
Stage 'READ_4K_REPORT_HASH'
if($Gate.ownership_report_sha256 -cne (Hash $Previous.Path)) {
 throw '06.4K private report SHA-256 mismatch.'
}
Stage 'READ_4K_COUNTERS'
if([int]$Previous.Data.verified_nonarm_item_ids -ne 11 -or
 [int]$Previous.Data.verified_world_occurrences -ne 40 -or
 [int]$Previous.Data.active_unmapped_property_occurrences -ne 50 -or
 [int]$Previous.Data.distinct_active_unmapped_property_names -ne 26 -or
 @($Previous.Data.private_item_ownership).Count -ne 11) {
 throw '06.4K report counts mismatch.'
}
Stage 'READ_4K_GATE_CLOSED'
Assert-Closed $Gate
Stage 'VERIFY_4K_DOMAINS'
$ExpectedCounts=[ordered]@{
 READING=1;LIGHT=1;PERSISTENT_VISUAL=11;WORLD_CHAOS=2;
 GENERIC_THROW=3;ITEM_ACTION=11;NONARM_EQUIPMENT=15;
 NONARM_STACK=6;LEGACY_ARMAMENT_CATALOGUE=0;UNCLASSIFIED=0
}
$Sum=0
foreach($Domain in $ExpectedCounts.Keys) {
 $P=$Previous.Data.domain_counts.PSObject.Properties[$Domain]
 $V=if($null -eq $P) {0} else {[int]$P.Value}
 if($V -ne $ExpectedCounts[$Domain]) {throw '06.4K domain distribution changed.'}
 $Sum+=$V
}
if($Sum -ne 50) {throw 'Unreconciled 06.4K domains.'}

# The user's opt-in is strictly the public commit subject below. Only our
# freshly published code commit may be amended; user commits are immutable.
$BaseSha='07c757485a929707bcaed9832ed5933ab409950d'
$Trigger='feat(ARM-CLEAN06.4T): materialize real canonical nonarm items in UE Editor'
$ItemRoot='Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/'
$RootAsset=$ItemRoot+'DA_ItemSystem.uasset'
$Definitions=$ItemRoot+'Definitions/'
if((@(& git -C $WorktreePath rev-parse --verify HEAD^) | Select-Object -First 1).Trim() -cne $BaseSha -or
   $LASTEXITCODE -ne 0) {
 throw 'Refuse migration from a parent other than validated 06.4S.'
}
$Subject=([string](@(& git -C $WorktreePath log -1 --format=%s)[0])).Trim()
if($LASTEXITCODE -ne 0 -or $Subject -cne $Trigger) {
 throw 'Only exact one-shot opt-in migration commit is allowed.'
}
$Diff=@(& git -C $WorktreePath diff-tree --no-commit-id --name-only -r HEAD)
if($LASTEXITCODE -ne 0) {throw 'Cannot inspect atomic migration commit.'}
$AlreadyMigrated=@($Diff | Where-Object {$_ -ceq $RootAsset}).Count -eq 1
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
$Validate=Join-Path $WorktreePath 'Scripts\ValidateUE.ps1'
$PsExe=Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
foreach($F in @($Editor,$Project,$Validate,$PsExe)) {
 if(-not (Test-Path -LiteralPath $F -PathType Leaf)) {
  throw 'Unreal validation environment unavailable.'
 }
}
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4T'
if(Test-Path -LiteralPath $Folder) {
 throw 'Do not overwrite a previous private materialization report.'
}
New-Item -ItemType Directory -Path $Folder -ErrorAction Stop | Out-Null
if(-not $AlreadyMigrated) {
 Stage 'PROVE_SIGNED_SOURCE'
Stage 'REHASH_84'
$Snapshot=Private-Report 'ARM-CLEAN06.4G' 'binary-snapshot-receipt.json' 'validation_sha' $SnapshotSha
$SnapshotGatePath=Join-Path (Split-Path -Parent $Snapshot.Path) 'transaction-gate-private.json'
if(-not (Test-Path -LiteralPath $SnapshotGatePath -PathType Leaf)) {
 throw '06.4G protected snapshot receipt missing.'
}
$SnapshotGate=Get-Content -LiteralPath $SnapshotGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($SnapshotGate.schema -cne 'ARM-CLEAN06.4G/transaction-gate/v1' -or
 $SnapshotGate.validation_sha -cne $SnapshotSha -or
 $SnapshotGate.snapshot_receipt_sha256 -cne (Hash $Snapshot.Path) -or
 $Snapshot.Data.copy_verified -ne $true -or
 $Snapshot.Data.sandbox_restore_drill_verified -ne $true -or
 @($Snapshot.Data.source_files).Count -ne 84) {
 throw '06.4G backup proof has changed.'
}
Assert-Closed $SnapshotGate
$BackupRoot=Join-Path (Split-Path -Parent $Snapshot.Path) 'SourceSnapshot'
$Unique=@{}
foreach($Record in @($Snapshot.Data.source_files)) {
 $Rel=[string]$Record.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+\.uasset$' -or
  $Rel.Contains('..') -or $Unique.ContainsKey($Rel) -or
  [string]$Record.sha256 -cnotmatch '^[a-f0-9]{64}$') {
  throw 'Unexpected protected source file path or SHA.'
 }
 $Unique[$Rel]=$true
 $Original=Join-Path $WorktreePath $Rel.Replace('/','\')
 $Backup=Join-Path $BackupRoot $Rel.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Original -PathType Leaf) -or
  -not (Test-Path -LiteralPath $Backup -PathType Leaf) -or
  (Get-Item -LiteralPath $Original).Length -ne [int64]$Record.bytes -or
  (Get-Item -LiteralPath $Backup).Length -ne [int64]$Record.bytes -or
  (Hash $Original) -cne [string]$Record.sha256 -or
  (Hash $Backup) -cne [string]$Record.sha256) {
  throw 'A source or immutable backup diverged from 06.4G.'
 }
}
if($Unique.Count -ne 84) {throw 'Not all 84 protected sources were checked.'}



$SPlan=Private-Report 'ARM-CLEAN06.4S' 'transaction-plan-private.json' 'schema' 'ARM-CLEAN06.4S/transaction-plan/v1'
$SGatePath=Join-Path (Split-Path -Parent $SPlan.Path) 'transaction-rehearsal-gate-private.json'
if(-not (Test-Path -LiteralPath $SGatePath -PathType Leaf)) {
 throw 'Validated 06.4S transaction receipt missing.'
}
$SGate=Get-Content -LiteralPath $SGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($SGate.schema -cne 'ARM-CLEAN06.4S/transaction-rehearsal-gate/v1' -or
 $SGate.validation_sha -cne $BaseSha -or
 $SPlan.Data.validation_sha -cne $BaseSha -or
 $SGate.transaction_plan_sha256 -cne (Hash $SPlan.Path) -or
 $SGate.sandbox_rollback_verified -ne $true -or
 [int]$SPlan.Data.immutable_source_count -ne 84 -or
 $SPlan.Data.migration_authorized -ne $false -or
 [int]$SPlan.Data.action_execution_routes_approved -ne 0) {
 throw 'Last known signed transaction evidence differs; abort.'
}
Assert-Closed $SGate
}

$Report=Join-Path $Folder 'materialized-private.json'
$UeLog=Join-Path $Folder 'UE-ApplyOrVerify-private.log'
$Mode=if($AlreadyMigrated){'-Verify'}else{'-Apply'}
Stage $(if($AlreadyMigrated){'VERIFY_COMMITTED'}else{'APPLY_UNREAL'})
& $Editor $Project '-run=GridArmClean064TNonArmMaterialize' '-Unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' $Mode "-Ownership=$($Previous.Path)" "-Output=$Report" *> $UeLog
$UeExit=$LASTEXITCODE
if($UeExit -ne 0 -or -not (Test-Path -LiteralPath $Report -PathType Leaf)) {
 Stage 'UNREAL_MATERIALIZATION_FAILED'
 throw 'Actual UE Editor materialization or reopened-asset verification failed (private logs only).'
}
Stage 'VALIDATE_REPORT'
$R=Get-Content -LiteralPath $Report -Raw -Encoding UTF8 | ConvertFrom-Json
$N=[int]$R.new_real_assets
$Deferred=[int]$R.deferred_with_unproven_semantics
if($R.schema -cne 'ARM-CLEAN06.4T/nonarm-real-asset-materialization/v1' -or
 [int]$R.source_item_ids -ne 11 -or
 [int]$R.world_references_unchanged -ne 40 -or
 [int]$R.old_registry_size -ne 23 -or
 $N -lt 1 -or $N -gt 11 -or $N+$Deferred -ne 11 -or
 [int]$R.new_registry_size -ne 23+$N -or
 $R.runtime_cutover_performed -ne $false -or
 $R.savegame_files_opened -ne $false -or
 $R.saved_binaries -ne (-not $AlreadyMigrated) -or
 $R.verified_after_reopen -ne $AlreadyMigrated -or
 @($R.private_new_assets).Count -ne $N) {
 throw 'Actual materialization counts/guard flags do not match expected scope.'
}
$Targets=New-Object System.Collections.ArrayList
[void]$Targets.Add($RootAsset)
$Used=@{$RootAsset=$true}
foreach($Row in @($R.private_new_assets)) {
 $Rel=[string]$Row.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/CanonicalStage/Definitions/DA_Canonical_[A-Za-z0-9_]+\.uasset$' -or
  $Used.ContainsKey($Rel) -or
  -not (Test-Path -LiteralPath (Join-Path $WorktreePath $Rel.Replace('/','\')) -PathType Leaf) -or
  (Get-Item -LiteralPath (Join-Path $WorktreePath $Rel.Replace('/','\'))).Length -le 200) {
  throw 'Actual converted UE asset is outside the exact commandlet allowlist.'
 }
 $Used[$Rel]=$true
 [void]$Targets.Add($Rel)
}
if($Targets.Count -ne $N+1) { throw 'Bad actual asset allowlist count.' }
# Ensure EVERY expected materialized path is tracked by Git LFS.
foreach($Rel in @($Targets)) {
 $Attr=@(& git -C $WorktreePath check-attr filter -- $Rel)
 if($LASTEXITCODE -ne 0 -or $Attr.Count -ne 1 -or
    [string]$Attr[0] -notmatch ': filter: lfs$') {
  throw 'Converted binary not governed by Git LFS.'
 }
}
if($AlreadyMigrated) {
 Stage 'VERIFY_COMMIT_FILESET'
 $BinaryDiff=@($Diff | Where-Object {$_.EndsWith('.uasset',[StringComparison]::OrdinalIgnoreCase)})
 if($BinaryDiff.Count -ne $Targets.Count -or
  @($BinaryDiff | Where-Object {-not $Used.ContainsKey($_)}).Count -ne 0) {
  throw 'Final migration commit has unexpected binary files.'
 }
 Stage 'CHECK_CLEAN'
 Clean
 Stage 'PUBLISH_FINAL_RECEIPT'
 $Body=Join-Path $Folder 'github-issue.md'
 @(
  '# ARM-CLEAN06.4T — real Unreal Editor canonical asset materialization'
  ''
  ('Final Git commit: '+$ExpectedHead)
  ('Previously validated binary snapshot: '+$SnapshotSha)
  ''
  '| Proof on saved real .uasset | Count |'
  '|---|---:|'
  ('| New real canonical item definitions | '+$N+' |')
  '| DA_ItemSystem binary updated and reopened | 1 |'
  ('| True final registry definitions | '+(23+$N)+' |')
  ('| Action-dependent / ambiguous identities deferred | '+$Deferred+' |')
  '| Old world/palette/receptacle references unchanged | 40 |'
  '| Legacy runtime fallback branches introduced | 0 |'
  '| Personal SaveGames accessed/converted | 0 |'
  '| Assets modified outside exact allowlist | 0 |'
  ''
  'All ItemIds, real paths and source field values remain solely in private Windows evidence.'
  'This is a real UAsset content migration, NOT the world-reference or action-runtime cutover.'
 ) | Set-Content -LiteralPath $Body -Encoding UTF8
 $Issue=@(& gh issue create --repo $GitHubRepository --title ('[ARM-CLEAN06.4T] Native asset materialization '+$ExpectedHead.Substring(0,12)) --body-file $Body)
 if($LASTEXITCODE -ne 0 -or $Issue.Count -ne 1) {
  throw 'Failed to publish aggregate-only migration receipt.'
 }
 Stage 'SUCCESS'
 Write-Host ('[ARM-CLEAN06.4T] VERIFIED_COMMITTED_REAL_ASSETS '+$Issue[0])
 exit 0
}
Stage 'CHECK_EXACT_UNCOMMITTED_FILESET'
$Changes=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if($LASTEXITCODE -ne 0 -or $Changes.Count -ne $Targets.Count) {
 throw 'Unexpected migration working tree file count; do not publish.'
}
foreach($Change in $Changes) {
 $Path=$Change.Substring(3).Replace('\','/')
 $Code=$Change.Substring(0,2)
 if(-not $Used.ContainsKey($Path) -or
  ($Path -ceq $RootAsset -and $Code -cne ' M') -or
  ($Path -cne $RootAsset -and $Code -cne '??')) {
  throw 'Unexpected asset or Git operation outside the exact migration file set.'
 }
}
Stage 'VERIFY_RELOADED_UNREAL'
$Verify=Join-Path $Folder 'reloaded-private.json'
$VerifyLog=Join-Path $Folder 'UE-Verify-private.log'
& $Editor $Project '-run=GridArmClean064TNonArmMaterialize' '-Unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' '-Verify' "-Ownership=$($Previous.Path)" "-Output=$Verify" *> $VerifyLog
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Verify -PathType Leaf)) {
 Stage 'UNREAL_REOPEN_FAILED'
 throw 'Persisted UAssets did not match independent source data in a second UE process.'
}
$V=Get-Content -LiteralPath $Verify -Raw -Encoding UTF8 | ConvertFrom-Json
if($V.schema -cne $R.schema -or $V.verified_after_reopen -ne $true -or
 [int]$V.new_real_assets -ne $N -or
 [int]$V.new_registry_size -ne 23+$N -or
 @($V.private_new_assets).Count -ne $N) {
 throw 'Saved asset verification differs from the authoring manifest.'
}
Stage 'POST_MIGRATION_AUTOMATION'
$PostReport=Join-Path $Folder 'post-automation'
$PostOut=Join-Path $Folder 'post-automation.stdout.log'
$PostErr=Join-Path $Folder 'post-automation.stderr.log'
$Args='-NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File "'+
 $Validate+'" -EngineRoot "'+$EngineRoot+
 '" -SkipBuild -AutomationFilter "Grimrock" -ReportRoot "'+$PostReport+'"'
$Proc=Start-Process -FilePath $PsExe -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -Wait -PassThru -RedirectStandardOutput $PostOut -RedirectStandardError $PostErr -ErrorAction Stop
if($Proc.ExitCode -ne 0) {
 Stage 'POST_AUTOMATION_FAILED'
 throw 'Real converted UAssets failed full Grimrock Automation; publication blocked.'
}
$Again=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if($LASTEXITCODE -ne 0 -or (@($Again) -join '|') -cne (@($Changes) -join '|')) {
 throw 'Post-asset Automation wrote unexpected files.'
}
Stage 'REMOTE_LEASE'
& git -C $WorktreePath fetch --no-tags origin master | Out-Null
if($LASTEXITCODE -ne 0) {throw 'Could not fetch final upstream SHA.'}
$Remote=([string](@(& git -C $WorktreePath rev-parse refs/remotes/origin/master)[0])).Trim()
if($LASTEXITCODE -ne 0 -or $Remote -cne $ExpectedHead) {
 throw 'Someone advanced origin/master during migration; do not rewrite their commit.'
}
Stage 'STAGE_REAL_UNREAL_ASSETS'
& git -C $WorktreePath add -- @($Targets.ToArray())
if($LASTEXITCODE -ne 0) {throw 'Cannot stage exact binary allowlist.'}
$Staged=@(& git -C $WorktreePath diff --cached --name-only --diff-filter=ACMR)
if($LASTEXITCODE -ne 0 -or $Staged.Count -ne $Targets.Count -or
 @($Staged | Where-Object {-not $Used.ContainsKey($_)}).Count -ne 0) {
 throw 'Git index diverges from exact native asset allowlist.'
}
# GitHub contains only ONE final commit for this step: amend only the
# ChatGPT-created staging commit, preserving its parent and subject.
Stage 'AMEND_ONLY_OWN_COMMIT'
& git -C $WorktreePath -c 'user.name=Grimrock UE Automation' -c 'user.email=grimrock-ue-automation@users.noreply.github.com' commit --amend --no-edit | Out-Null
if($LASTEXITCODE -ne 0) {throw 'Could not amend one-shot own commit.'}
$FinalHead=([string](@(& git -C $WorktreePath rev-parse HEAD)[0])).Trim()
$FinalParent=([string](@(& git -C $WorktreePath rev-parse HEAD^)[0])).Trim()
if($LASTEXITCODE -ne 0 -or $FinalParent -cne $BaseSha -or
 $FinalHead -ceq $ExpectedHead) {
 throw 'Amended commit parent or identity invalid; do not publish.'
}
Stage 'PUSH_WITH_LEASE'
& git -C $WorktreePath push ('--force-with-lease=refs/heads/master:'+$ExpectedHead) origin HEAD:refs/heads/master | Out-Null
if($LASTEXITCODE -ne 0) {throw 'One-shot migrated asset commit push failed.'}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4T] REAL_UASSET_AMENDED_AND_PUSHED '+$FinalHead+' assets='+$N)
