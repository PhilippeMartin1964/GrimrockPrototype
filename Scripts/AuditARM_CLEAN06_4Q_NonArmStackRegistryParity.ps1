<#
 ARM-CLEAN06.4Q: non-ARM generic stacking and true transient registry parity. Private real asset values stay in Windows StateRoot. No SavePackage.
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
$StagePath=Join-Path $RunDir 'ARMClean064Q.stage'
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
foreach($Sha in @($ExpectedHead,$OwnershipSha,$SnapshotSha,$ProjectionSha,$IdentitySha,$PresentationSha,$ReadingWorldSha,$ThrowSha,$ActionSha,$EquipmentSha)) {
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


Stage 'READ_4P'
$Prior=Private-Report 'ARM-CLEAN06.4P' 'nonarm-equipment-private.json' 'schema' 'ARM-CLEAN06.4P/nonarm-equipment-parity/v1'
$GatePath=Join-Path (Split-Path -Parent $Prior.Path) 'equipment-gate-private.json'
if(-not (Test-Path -LiteralPath $GatePath -PathType Leaf)) {
 throw '06.4P pinned private equipment gate missing.'
}
$Gate=Get-Content -LiteralPath $GatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Gate.schema -cne 'ARM-CLEAN06.4P/nonarm-equipment-gate/v1' -or
 $Gate.validation_sha -cne $EquipmentSha -or
 $Gate.source_ownership_sha -cne $OwnershipSha -or
 $Gate.source_action_sha -cne $ActionSha -or
 $Gate.protected_backup_sha -cne $SnapshotSha -or
 $Gate.ownership_report_sha256 -cne (Hash $Previous.Path) -or
 $Gate.equipment_report_sha256 -cne (Hash $Prior.Path) -or
 [int]$Prior.Data.verified_item_ids -ne 11 -or
 [int]$Prior.Data.world_occurrences_preserved -ne 40 -or
 [int]$Prior.Data.active_nonarm_equipment_fields -ne 15 -or
 [int]$Prior.Data.exact_reflected_copies -ne 44 -or
 [int]$Prior.Data.direct_slot_probes -ne 209 -or
 [int]$Prior.Data.fixed_level_stat_bonus_probes -ne 33 -or
 [int]$Gate.runtime_semantic_parity_approved -ne 0 -or
 $Prior.Data.migration_authorized -ne $false) {
 throw '06.4P source evidence drift or invalid receipt SHA.'
}
Assert-Closed $Gate
Stage 'UNREAL_GENERIC_STACK'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 commandlet or project missing.'
}
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4Q'
if(Test-Path -LiteralPath $Folder) {
 throw 'Refuse to overwrite existing private stack audit.'
}
New-Item -ItemType Directory -Path $Folder -ErrorAction Stop | Out-Null
$Out=Join-Path $Folder 'stack-parity-private.json'
$Diag=Join-Path $Folder 'ue-return-code-private.txt'
$UeLog=Join-Path $Folder 'ue-commandlet-output-private.log'
# All native logging stays private; only a bounded diagnostic code is
# published through the Windows validation status.
& $Editor $Project '-run=GridArmClean064QNonArmStackParity' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' '-stdout' '-FullStdOutLogOutput' "-Ownership=$($Previous.Path)" "-Equipment=$($Prior.Path)" "-Output=$Out" "-Diagnostic=$Diag" *> $UeLog
$UeExit=$LASTEXITCODE
if($UeExit -ne 0 -or -not (Test-Path -LiteralPath $Out -PathType Leaf)) {
 $Safe='PROCESS'
 if(Test-Path -LiteralPath $Diag -PathType Leaf) {
  $Receipt=([string](Get-Content -LiteralPath $Diag -Raw -Encoding UTF8)).Trim()
  if($Receipt -cmatch '^ARM-CLEAN06\.4Q/CODE_([1-9])$') {
   $Safe='CODE_' + $Matches[1]
  }
 }
 Stage ('UNREAL_Q_' + $Safe)
 throw 'UE stack parity incomplete; private diagnostic receipt produced.'
}
Stage 'VERIFY_RESULT'
$R=Get-Content -LiteralPath $Out -Raw -Encoding UTF8 | ConvertFrom-Json
if($R.schema -cne 'ARM-CLEAN06.4Q/nonarm-stack-data-parity/v1' -or
 [int]$R.verified_item_ids -ne 11 -or
 [int]$R.world_occurrences_preserved -ne 40 -or
 [int]$R.legacy_active_fields_reverified -ne 50 -or
 [int]$R.active_nonarm_stack_fields -ne 6 -or
 [int]$R.source_stack_values_verified -ne 22 -or
 [int]$R.transient_registry_definitions -ne 34 -or
 [int]$R.stackable_preview_items -lt 0 -or
 [int]$R.stackable_preview_items -gt 11 -or
 [int]$R.unit_capacity_preview_items -lt 0 -or
 [int]$R.unit_capacity_preview_items -gt 11 -or
 ([int]$R.stackable_preview_items + [int]$R.unit_capacity_preview_items) -ne 11 -or
 [int]$R.registry_level_probes -ne 44 -or
 [int]$R.quantity_policy_probes -ne 33 -or
 [int]$R.source_dirty_state_changes -ne 0 -or
 [int]$R.runtime_savegame_parity_approved -ne 0 -or
 $R.binary_writes_performed -ne $false -or
 $R.savegame_files_opened -ne $false -or
 $R.migration_authorized -ne $false -or
 @($R.private_item_projections).Count -ne 11) {
 throw 'Native non-ARM stack report does not satisfy exact scope or write gate.'
}
$Seen=@{}
$TotalActive=0
$Stacking=0
foreach($Item in @($R.private_item_projections)) {
 $Id=[string]$Item.item_id
 if([string]::IsNullOrWhiteSpace($Id) -or $Seen.ContainsKey($Id) -or
  [int]$Item.active_stack_source_fields -lt 0 -or
  [int]$Item.effective_canonical_stack_capacity -lt 1 -or
  $Item.registry_family_selection_approved -ne $false -or
  $Item.runtime_savegame_parity_approved -ne $false) {
  throw 'Private per-ItemId stack output invalid.'
 }
 $Seen[$Id]=$true
 $TotalActive += [int]$Item.active_stack_source_fields
 if([int]$Item.effective_canonical_stack_capacity -gt 1) {++$Stacking}
}
if($Seen.Count -ne 11 -or $TotalActive -ne 6 -or
 $Stacking -ne [int]$R.stackable_preview_items) {
 throw 'Per-ItemId verified source stack totals disagree.'
}
Stage 'WRITE_PRIVATE_GATE'
$OutGate=[ordered]@{
 schema='ARM-CLEAN06.4Q/nonarm-stack-gate/v1'
 validation_sha=$ExpectedHead
 source_ownership_sha=$OwnershipSha
 source_equipment_sha=$EquipmentSha
 protected_backup_sha=$SnapshotSha
 ownership_report_sha256=(Hash $Previous.Path)
 equipment_report_sha256=(Hash $Prior.Path)
 stack_report_sha256=(Hash $Out)
 private_ue_source_snapshots_verified=84
 active_stack_fields_projected=6
 actual_registry_validated=$true
 transient_registry_definition_count=34
 true_registry_level_probes=44
 quantity_policy_probes=33
 approved_instance_family_cutover=0
 runtime_savegame_parity_approved=0
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 next='NONARM_ACTION_SEMANTIC_MAPPING_AND_TRANSITION_PLAN'
}
$OutGate | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Folder 'stack-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4Q - non-arm generic stacking parity in real UE registry'
 ''
 ('Validation commit: '+$ExpectedHead)
 ('Protected 06.4K source: '+$OwnershipSha)
 ('Protected 06.4P equipment: '+$EquipmentSha)
 ('Protected 06.4G source/backups: '+$SnapshotSha)
 ''
 '| Verified evidence | Count |'
 '|---|---:|'
 '| Real UE legacy non-ARM ItemIds | 11 |'
 '| Unchanged world/level/palette references | 40 |'
 '| All earlier active legacy fields revalidated | 50 |'
 '| Active stack field occurrences projected | 6 |'
 '| Authored flags/capacities checked (including defaults) | 22 |'
 '| Transient DA_ItemSystem definitions (23 + 11) | 34 |'
 ('| Stackable generic previews | '+$R.stackable_preview_items+' |')
 ('| Unstackable capacity-one previews | '+$R.unit_capacity_preview_items+' |')
 '| Real UGridItemSubsystem N0/N1/N20/N21 probes | 44 |'
 '| Pure quantity/stack capacity probes | 33 |'
 '| Remaining unapproved legacy action fields | 11 |'
 '| Actual binary migration / SaveGame rights granted | 0 |'
 ''
 'All real ItemIds and serialized source values remain only in private Windows evidence.'
 'Generic stackable objects have a separate canonical class; Ammo and Consumable keep their own single MaxStack.'
 'This is a transient data/policy registry proof, not a binary conversion, PIE or SaveGame replay.'
 'The migration gate remains CLOSED.'
)
$Public=Join-Path $Folder 'github-issue.md'
Set-Content -LiteralPath $Public -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4Q] Generic non-arm stack registry parity '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Public)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
 [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Generic stack aggregate issue could not be published.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4Q] NONARM_STACK_REGISTRY_PARITY_OK '+$Url[0])
