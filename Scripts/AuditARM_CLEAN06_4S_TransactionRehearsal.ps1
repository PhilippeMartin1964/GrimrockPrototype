<#
 ARM-CLEAN06.4S: immutable 84-file Editor transaction rehearsal in PRIVATE SANDBOX only. Private real asset values stay in Windows StateRoot. No SavePackage.
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
$StagePath=Join-Path $RunDir 'ARMClean064S.stage'
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


Stage 'READ_4R'
$Bridge=Private-Report 'ARM-CLEAN06.4R' 'action-bridge-private.json' 'schema' 'ARM-CLEAN06.4R/nonarm-lossless-action-bridge/v1'
$BridgeGatePath=Join-Path (Split-Path -Parent $Bridge.Path) 'action-bridge-gate-private.json'
if(-not (Test-Path -LiteralPath $BridgeGatePath -PathType Leaf)) {
 throw 'Final 06.4R signed action bridge gate missing.'
}
$BridgeGate=Get-Content -LiteralPath $BridgeGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($BridgeGate.schema -cne 'ARM-CLEAN06.4R/lossless-action-gate/v1' -or
 $BridgeGate.validation_sha -cne $BridgeSha -or
 $BridgeGate.source_ownership_sha -cne $OwnershipSha -or
 $BridgeGate.source_action_inventory_sha -cne $ActionSha -or
 $BridgeGate.source_stack_sha -cne $StackSha -or
 $BridgeGate.protected_backup_sha -cne $SnapshotSha -or
 $BridgeGate.ownership_report_sha256 -cne (Hash $Previous.Path) -or
 $BridgeGate.action_bridge_report_sha256 -cne (Hash $Bridge.Path) -or
 [int]$Bridge.Data.verified_item_ids -ne 11 -or
 [int]$Bridge.Data.world_occurrences_preserved -ne 40 -or
 [int]$Bridge.Data.detailed_actions_projected -ne 4 -or
 [int]$Bridge.Data.quick_actions_projected -ne 3 -or
 [int]$Bridge.Data.detailed_presentations_projected -ne 1 -or
 [int]$Bridge.Data.full_property_exports_compared -ne 176 -or
 [int]$Bridge.Data.nondefault_nested_properties_preserved -ne 54 -or
 [int]$Bridge.Data.candidate_bindings_not_approved -ne 4 -or
 [int]$Bridge.Data.approved_runtime_action_routes -ne 0 -or
 $Bridge.Data.migration_authorized -ne $false -or
 [int]$BridgeGate.approved_runtime_action_routes -ne 0) {
 throw 'Trusted 06.4R data/receipt mismatch.'
}
Assert-Closed $BridgeGate
Stage 'PREPARE_SANDBOX'
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4S'
if(Test-Path -LiteralPath $Folder) {
 throw 'Refuse overwriting an existing private rehearsal.'
}
New-Item -ItemType Directory -Path $Folder -ErrorAction Stop | Out-Null
$Sandbox=Join-Path $Folder 'TransactionSandbox'
New-Item -ItemType Directory -Path $Sandbox -ErrorAction Stop | Out-Null
$SandboxFull=[IO.Path]::GetFullPath($Sandbox).TrimEnd('\','/')
if(-not $SandboxFull.StartsWith(($RunFull+'\'),[StringComparison]::OrdinalIgnoreCase)) {
 throw 'Transaction sandbox must remain in the private run directory.'
}
# The manifest has an EXACT path, SHA-256, length and source class for every
# protected binary. It never authorizes writing to a project source.
Stage 'BUILD_TRANSACTION_MANIFEST'
$ManifestFiles=New-Object System.Collections.ArrayList
$ByRelative=@{}
$LegacyCount=0
$CanonicalCount=0
foreach($Record in @($Snapshot.Data.source_files)) {
 $Rel=[string]$Record.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+\.uasset$' -or
  $Rel.Contains('..') -or $ByRelative.ContainsKey($Rel) -or
  [string]$Record.sha256 -cnotmatch '^[a-f0-9]{64}$' -or
  [int64]$Record.bytes -le 200) {
  throw 'Closed binary transaction scope contains an invalid asset.'
 }
 $ByRelative[$Rel]=$true
 $Category=[string]$Record.category
 if($Category -ceq 'legacy') {++$LegacyCount}
 elseif($Category -ceq 'canonical') {++$CanonicalCount}
 else {throw 'Unexpected source category in signed 06.4G receipt.'}
 [void]$ManifestFiles.Add([ordered]@{
  relative=$Rel
  source_sha256=[string]$Record.sha256
  source_bytes=[int64]$Record.bytes
  source_category=$Category
  planned_operation='REVIEW_ONLY'
  target_path=$null
  write_authorized=$false
 })
}
if($ManifestFiles.Count -ne 84 -or $ByRelative.Count -ne 84 -or
 $LegacyCount -ne 56 -or $CanonicalCount -ne 28) {
 throw 'Expected exact 56 legacy and 28 canonical source assets.'
}
# 11 distinct missing ItemIds across 40 placements are NOT auto-classified
# into a consumable/generic equipment family. Four detailed actions have
# NOT been routed to the player runtime yet. Both block production apply.
$ExplicitBlockers=@(
 'EXACT_ITEM_FAMILY_AND_ACTION_BINDINGS_NOT_APPROVED',
 'REAL_RUNTIME_ACTION_ROUTES_NOT_PROVEN',
 'REAL_SAVEGAME_BACKUP_AND_SCHEMA_REPLAY_NOT_PROVEN',
 'PERIPHERAL_WBP_AND_PRESET_WRITE_SCOPE_NOT_PROTECTED',
 'POST_CONVERSION_WORLD_REFERENCE_AND_PIE_COOK_NOT_PROVEN'
)
$Plan=[ordered]@{
 schema='ARM-CLEAN06.4S/transaction-plan/v1'
 validation_sha=$ExpectedHead
 source_backup_sha=$SnapshotSha
 source_full_action_sha=$BridgeSha
 immutable_source_count=84
 existing_legacy_packages=56
 existing_canonical_packages=28
 existing_canonical_item_ids=23
 missing_nonarm_item_ids=11
 unresolved_world_occurrences=40
 authored_full_action_records=4
 action_execution_routes_approved=0
 savegame_files_read_or_backed_up=0
 application_mode='SANDBOX_REHEARSAL_ONLY'
 migration_authorized=$false
 approved_target_paths=@()
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 blocking_conditions=$ExplicitBlockers
 source_files=@($ManifestFiles.ToArray())
}
$PlanFile=Join-Path $Folder 'transaction-plan-private.json'
$Plan | ConvertTo-Json -Depth 11 |
 Set-Content -LiteralPath $PlanFile -Encoding UTF8
Stage 'REHEARSE_STAGING'
$Staged=0
foreach($Record in @($Snapshot.Data.source_files)) {
 $Rel=[string]$Record.relative
 $Source=Join-Path $WorktreePath $Rel.Replace('/','\')
 $Protected=Join-Path $BackupRoot $Rel.Replace('/','\')
 $Copy=Join-Path $Sandbox $Rel.Replace('/','\')
 $Parent=Split-Path -Parent $Copy
 if(-not (Test-Path -LiteralPath $Parent -PathType Container)) {
  New-Item -ItemType Directory -Path $Parent -Force -ErrorAction Stop | Out-Null
 }
 $SourceItem=Get-Item -LiteralPath $Source -ErrorAction Stop
 $BackupItem=Get-Item -LiteralPath $Protected -ErrorAction Stop
 if((($SourceItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) -or
    (($BackupItem.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)) {
  throw 'Reparse-point sources are not allowed in an immutable transaction.'
 }
 Copy-Item -LiteralPath $Protected -Destination $Copy -ErrorAction Stop
 if((Get-Item -LiteralPath $Copy).Length -ne [int64]$Record.bytes -or
    (Hash $Copy) -cne [string]$Record.sha256 -or
    (Hash $Source) -cne [string]$Record.sha256 -or
    (Hash $Protected) -cne [string]$Record.sha256) {
  throw 'Transaction staging source/backup/staged hash mismatch.'
 }
 ++$Staged
}
if($Staged -ne 84) {throw 'Incomplete immutable staging.'}
Stage 'REHEARSE_DETECTION'
# Alter ONE PRIVATE SANDBOX copy by appending a single byte, then verify
# the transaction detects corruption. The UE source and backup are untouched.
$CanaryRecord=@($Snapshot.Data.source_files)[0]
$CanaryRel=[string]$CanaryRecord.relative
$Canary=Join-Path $Sandbox $CanaryRel.Replace('/','\')
$ProtectedCanary=Join-Path $BackupRoot $CanaryRel.Replace('/','\')
$Stream=[IO.File]::Open($Canary,[IO.FileMode]::Append,
 [IO.FileAccess]::Write,[IO.FileShare]::None)
try {$Stream.WriteByte(90);$Stream.Flush()} finally {$Stream.Dispose()}
if((Hash $Canary) -ceq [string]$CanaryRecord.sha256 -or
 (Get-Item -LiteralPath $Canary).Length -ne (1+[int64]$CanaryRecord.bytes)) {
 throw 'Sandbox mutation detector failed.'
}
Stage 'REHEARSE_ROLLBACK'
Copy-Item -LiteralPath $ProtectedCanary -Destination $Canary -Force -ErrorAction Stop
if((Hash $Canary) -cne [string]$CanaryRecord.sha256 -or
 (Get-Item -LiteralPath $Canary).Length -ne [int64]$CanaryRecord.bytes) {
 throw 'Sandbox rollback failed.'
}
Stage 'VERIFY_ALL_AFTER_ROLLBACK'
$Verified=0
foreach($Record in @($Snapshot.Data.source_files)) {
 $Rel=[string]$Record.relative
 $Source=Join-Path $WorktreePath $Rel.Replace('/','\')
 $Protected=Join-Path $BackupRoot $Rel.Replace('/','\')
 $Copy=Join-Path $Sandbox $Rel.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Source -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Protected -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Copy -PathType Leaf) -or
    (Get-Item -LiteralPath $Source).Length -ne [int64]$Record.bytes -or
    (Get-Item -LiteralPath $Protected).Length -ne [int64]$Record.bytes -or
    (Get-Item -LiteralPath $Copy).Length -ne [int64]$Record.bytes -or
    (Hash $Source) -cne [string]$Record.sha256 -or
    (Hash $Protected) -cne [string]$Record.sha256 -or
    (Hash $Copy) -cne [string]$Record.sha256) {
  throw 'Immutable source, backup or sandbox still differs after rollback.'
 }
 ++$Verified
}
if($Verified -ne 84) {throw 'Not all sandbox assets reverified.'}
# This directory was created by this run; delete ONLY it, preserving the
# immutable 06.4G snapshot and all user-owned UE/SaveGame files.
Stage 'CLEAN_PRIVATE_SANDBOX'
if(-not (Test-Path -LiteralPath $Sandbox -PathType Container) -or
 $SandboxFull -cne [IO.Path]::GetFullPath($Sandbox).TrimEnd('\','/')) {
 throw 'Sandbox location changed before private cleanup.'
}
Remove-Item -LiteralPath $Sandbox -Recurse -Force -ErrorAction Stop
if(Test-Path -LiteralPath $Sandbox) {
 throw 'Temporary staged copies not cleared.'
}
Stage 'WRITE_CLOSED_GATE'
$GateOut=[ordered]@{
 schema='ARM-CLEAN06.4S/transaction-rehearsal-gate/v1'
 validation_sha=$ExpectedHead
 source_binary_snapshot_sha=$SnapshotSha
 source_action_bridge_sha=$BridgeSha
 snapshot_receipt_sha256=(Hash $Snapshot.Path)
 full_action_report_sha256=(Hash $Bridge.Path)
 transaction_plan_sha256=(Hash $PlanFile)
 protected_binary_sources_verified=84
 sandbox_staged_files=84
 sandbox_tamper_detected=$true
 sandbox_rollback_verified=$true
 sandbox_full_rehash_verified=84
 private_sandbox_removed=$true
 source_worktree_writes=0
 real_savegames_touched=0
 production_rollback_executed=$false
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 blocking_conditions=$ExplicitBlockers
 next='06.4T_AUTHOR_EXPLICIT_EDITOR_CONVERSION_AND_REAL_SAVEGAME_BACKUP_GUARD'
}
$GateFile=Join-Path $Folder 'transaction-rehearsal-gate-private.json'
$GateOut | ConvertTo-Json -Depth 9 |
 Set-Content -LiteralPath $GateFile -Encoding UTF8
Stage 'VERIFY_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Public=Join-Path $Folder 'github-issue.md'
$Lines=@(
 '# ARM-CLEAN06.4S - closed UE asset transaction sandbox rehearsal'
 ''
 ('Validated commit: '+$ExpectedHead)
 ('Signed immutable backup SHA: '+$SnapshotSha)
 ('Signed full action bridge SHA: '+$BridgeSha)
 ''
 '| Real protection and sandbox action | Count |'
 '|---|---:|'
 '| Historical legacy binaries rehashed and staged privately | 56 |'
 '| Canonical binaries rehashed and staged privately | 28 |'
 '| Full immutable source/backup pairs reverified | 84 |'
 '| Private staged files intentionally tested by one-byte corruption | 1 |'
 '| Verified sandbox restoration and full rehash | 84 |'
 '| Distinct non-ARM ItemIds still missing from live canonical root | 11 |'
 '| Active world/palette/receptacle references awaiting conversion | 40 |'
 '| Full action profiles conserved by previous stage | 4 |'
 '| Action routes approved in actual runtime | 0 |'
 '| Real personal SaveGame files backed up or opened | 0 |'
 '| Binary writes/deletions/SaveGame writes authorized | 0 |'
 ''
 'The manifest contains exact binary file SHA-256, sizes, categories and paths PRIVATE on Windows only.'
 'All 84 sandbox copies were removed after verified rollback; permanent 06.4G backups remain untouched.'
 'This is NOT a UE SavePackage dry-run nor a production rollback or conversion.'
 'Actual cutover remains BLOCKED: family/action semantics, SaveGame safety and peripheral assets require exact authoring.'
)
Set-Content -LiteralPath $Public -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4S] Closed transaction manifest and sandbox rollback '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Public)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
 [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Aggregate GitHub transaction receipt not published.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4S] CLOSED_TRANSACTION_REHEARSAL_OK '+$Url[0])
