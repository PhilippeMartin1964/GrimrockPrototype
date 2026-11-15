<#
 ARM-CLEAN06.4R: full non-ARM action/presentation staging in the four canonical catalogues. Private real asset values stay in Windows StateRoot. No SavePackage.
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
$StagePath=Join-Path $RunDir 'ARMClean064R.stage'
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
foreach($Sha in @($ExpectedHead,$OwnershipSha,$SnapshotSha,$ProjectionSha,$IdentitySha,$PresentationSha,$ReadingWorldSha,$ThrowSha,$ActionSha,$EquipmentSha,$StackSha)) {
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


Stage 'READ_4O'
$Old=Private-Report 'ARM-CLEAN06.4O' 'action-semantics-private.json' 'schema' 'ARM-CLEAN06.4O/nonarm-action-semantic-inventory/v1'
$OldGatePath=Join-Path (Split-Path -Parent $Old.Path) 'action-audit-gate-private.json'
if(-not (Test-Path -LiteralPath $OldGatePath -PathType Leaf)) {
 throw '06.4O action provenance receipt missing.'
}
$OldGate=Get-Content -LiteralPath $OldGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($OldGate.schema -cne 'ARM-CLEAN06.4O/action-semantic-audit-gate/v1' -or
 $OldGate.validation_sha -cne $ActionSha -or
 $OldGate.source_ownership_sha -cne $OwnershipSha -or
 $OldGate.source_throw_sha -cne $ThrowSha -or
 $OldGate.protected_backup_sha -cne $SnapshotSha -or
 $OldGate.ownership_report_sha256 -cne (Hash $Previous.Path) -or
 $OldGate.action_report_sha256 -cne (Hash $Old.Path) -or
 [int]$Old.Data.verified_item_ids -ne 11 -or
 [int]$Old.Data.legacy_equipped_action_records -ne 1 -or
 [int]$Old.Data.quick_item_normalized_valid -ne 3 -or
 [int]$Old.Data.presentations_valid -ne 1 -or
 [int]$Old.Data.nested_nondefault_action_fields -ne 54 -or
 [int]$Old.Data.semantic_parity_approved -ne 0 -or
 $Old.Data.migration_authorized -ne $false) {
 throw '06.4O private action evidence mismatch.'
}
Assert-Closed $OldGate
Stage 'READ_4Q'
$Stack=Private-Report 'ARM-CLEAN06.4Q' 'stack-parity-private.json' 'schema' 'ARM-CLEAN06.4Q/nonarm-stack-data-parity/v1'
$StackGatePath=Join-Path (Split-Path -Parent $Stack.Path) 'stack-gate-private.json'
if(-not (Test-Path -LiteralPath $StackGatePath -PathType Leaf)) {
 throw '06.4Q stack receipt missing.'
}
$StackGate=Get-Content -LiteralPath $StackGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($StackGate.schema -cne 'ARM-CLEAN06.4Q/nonarm-stack-gate/v1' -or
 $StackGate.validation_sha -cne $StackSha -or
 $StackGate.source_ownership_sha -cne $OwnershipSha -or
 $StackGate.source_equipment_sha -cne $EquipmentSha -or
 $StackGate.protected_backup_sha -cne $SnapshotSha -or
 $StackGate.ownership_report_sha256 -cne (Hash $Previous.Path) -or
 $StackGate.stack_report_sha256 -cne (Hash $Stack.Path) -or
 [int]$Stack.Data.verified_item_ids -ne 11 -or
 [int]$Stack.Data.transient_registry_definitions -ne 34 -or
 [int]$Stack.Data.active_nonarm_stack_fields -ne 6 -or
 [int]$StackGate.true_registry_level_probes -ne 44 -or
 $Stack.Data.migration_authorized -ne $false) {
 throw 'Pinned 06.4Q stack receipt does not match.'
}
Assert-Closed $StackGate
Stage 'UNREAL_ACTION_BRIDGE'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 executable or project missing.'
}
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4R'
if(Test-Path -LiteralPath $Folder) {
 throw 'Refuse overwrite of an existing action bridge report.'
}
New-Item -ItemType Directory -Path $Folder -ErrorAction Stop | Out-Null
$Out=Join-Path $Folder 'action-bridge-private.json'
$UeLog=Join-Path $Folder 'ue-action-bridge-private.log'
# Keep native Unreal diagnostic text solely on the private Windows agent.
& $Editor $Project '-run=GridArmClean064RNonArmFullActionBridge' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' "-Ownership=$($Previous.Path)" "-ActionInventory=$($Old.Path)" "-Stack=$($Stack.Path)" "-Output=$Out" *> $UeLog
$NativeResult=$LASTEXITCODE
if($NativeResult -ne 0 -or -not (Test-Path -LiteralPath $Out -PathType Leaf)) {
 Stage 'UNREAL_ACTION_BRIDGE_FAILED'
 throw 'Native UE lossless action bridge failed; full log is private.'
}
Stage 'VERIFY_RESULT'
$R=Get-Content -LiteralPath $Out -Raw -Encoding UTF8 | ConvertFrom-Json
if($R.schema -cne 'ARM-CLEAN06.4R/nonarm-lossless-action-bridge/v1' -or
 [int]$R.verified_item_ids -ne 11 -or
 [int]$R.world_occurrences_preserved -ne 40 -or
 [int]$R.legacy_active_fields_reverified -ne 50 -or
 [int]$R.active_action_fields -ne 11 -or
 [int]$R.detailed_actions_projected -ne 4 -or
 [int]$R.quick_actions_projected -ne 3 -or
 [int]$R.detailed_presentations_projected -ne 1 -or
 [int]$R.full_property_exports_compared -lt 100 -or
 [int]$R.nondefault_nested_properties_preserved -ne 54 -or
 [int]$R.candidate_bindings_not_approved -ne 4 -or
 [int]$R.transient_registry_definitions -ne 34 -or
 [int]$R.package_dirty_state_changes -ne 0 -or
 [int]$R.approved_runtime_action_routes -ne 0 -or
 $R.binary_writes_performed -ne $false -or
 $R.savegame_files_opened -ne $false -or
 $R.migration_authorized -ne $false -or
 @($R.private_item_action_projections).Count -ne 11) {
 throw 'Native action staging output violates scope or closed gate.'
}
$Ids=@{}
$Bindings=0
foreach($Row in @($R.private_item_action_projections)) {
 $ItemId=[string]$Row.item_id
 if([string]::IsNullOrWhiteSpace($ItemId) -or $Ids.ContainsKey($ItemId) -or
  $Row.semantic_parity_approved -ne $false -or
  $Row.binary_migration_authorized -ne $false) {
  throw 'Private action row invalid.'
 }
 $Ids[$ItemId]=$true
 $Bindings+=@($Row.candidate_unapproved_bindings).Count
}
if($Ids.Count -ne 11 -or $Bindings -ne 4) {
 throw 'Per-item full action records incomplete.'
}
Stage 'WRITE_PRIVATE_GATE'
$Gate=[ordered]@{
 schema='ARM-CLEAN06.4R/lossless-action-gate/v1'
 validation_sha=$ExpectedHead
 source_ownership_sha=$OwnershipSha
 source_action_inventory_sha=$ActionSha
 source_stack_sha=$StackSha
 protected_backup_sha=$SnapshotSha
 ownership_report_sha256=(Hash $Previous.Path)
 action_inventory_sha256=(Hash $Old.Path)
 stack_report_sha256=(Hash $Stack.Path)
 action_bridge_report_sha256=(Hash $Out)
 immutable_binary_sources_verified=84
 action_records_projected=4
 raw_presentation_profiles_projected=1
 nested_nondefault_fields_retained=54
 approved_runtime_action_routes=0
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 next='ONE_TIME_EDITOR_MIGRATION_TRANSACTION_WITH_SEMANTIC_ACTION_ROUTING_AND_SAVE_REPLAY'
}
$Gate | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Folder 'action-bridge-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4R - full source action and presentation projection'
 ''
 ('Validation commit: '+$ExpectedHead)
 ('Protected 06.4K ownership: '+$OwnershipSha)
 ('Protected 06.4O action inventory: '+$ActionSha)
 ('Protected 06.4Q stack proof: '+$StackSha)
 ('Protected 06.4G binary backup: '+$SnapshotSha)
 ''
 '| Real data verified in UE | Count |'
 '|---|---:|'
 '| Non-armament ItemIds examined | 11 |'
 '| References in levels, palettes and receptacles preserved | 40 |'
 '| Active historical action fields verified | 11 |'
 '| Full authored actions in same action catalogue, transient | 4 |'
 '| Of which normalized QuickItem definitions | 3 |'
 '| Full attack presentations in same presentation catalogue, transient | 1 |'
 ('| Reflected combat property exports compared | '+$R.full_property_exports_compared+' |')
 '| Authored nondefault nested combat fields preserved | 54 |'
 '| Candidate ItemActionBinding routes NOT yet approved | 4 |'
 '| Transient root item definitions validated (23 + 11) | 34 |'
 '| Runtime execution routes approved | 0 |'
 '| Binary writes or SaveGame migration authorized | 0 |'
 ''
 'Exact ItemId, ActionId, original field values and private comparison reports stay on Windows.'
 'The canonical catalogues now have opt-in full runtime-compatible execution payload slots.'
 'No fifth catalogue, no compact-action loss, no invented attack; 0 executable routes activated.'
 'The 06.4 binary migration gate remains CLOSED.'
)
$Public=Join-Path $Folder 'github-issue.md'
Set-Content -LiteralPath $Public -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4R] Non-arm full action catalogue bridge '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Public)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
 [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Aggregate public action bridge receipt could not be published.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4R] FULL_NONARM_ACTION_BRIDGE_OK '+$Url[0])
