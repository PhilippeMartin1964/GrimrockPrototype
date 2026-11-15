<#
 ARM-CLEAN06.4J — real UE transient-only non-armament authoring plan.
 Source = 06.4I signed private evidence + 06.4G immutable binary backup.
 Nothing written in the project; all item identities and fields stay private.
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
$IdentitySha='d3fc4e10de5b82e5669c4dd03c309c30abeab9c6'
$SnapshotSha='e0f877d17f39d913c585b55ba18110745c5385b6'
$StageFile=Join-Path $RunDir 'ARMClean064J.stage'
function Stage([string]$Value) {Set-Content -LiteralPath $StageFile -Value $Value -Encoding ASCII}
function Hash([string]$Path) {
 return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Clean {
 $Check=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Check.Count -ne 0) {
  Stage 'WORKTREE_CHANGED';throw 'Validation source is not clean.'
 }
}
function Find-Report([string]$Label,[string]$Name,[string]$PropertyName,[string]$ExpectedValue) {
 $Root=Join-Path $StateRoot 'Runs'
 if(-not (Test-Path -LiteralPath $Root -PathType Container)) {
  throw 'Windows private validation run archive absent.'
 }
 $Results=New-Object System.Collections.ArrayList
 foreach($Folder in @(Get-ChildItem -LiteralPath $Root -Directory -ErrorAction Stop)) {
  $Path=Join-Path (Join-Path $Folder.FullName $Label) $Name
  if(-not (Test-Path -LiteralPath $Path -PathType Leaf)) {continue}
  $Data=Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
  $Proof=$Data.PSObject.Properties[$PropertyName]
  if($null -ne $Proof -and [string]$Proof.Value -ceq $ExpectedValue) {
   [void]$Results.Add([pscustomobject]@{Path=$Path;Data=$Data})
  }
 }
 if($Results.Count -ne 1) {throw 'Expected one exact private proof, found another count.'}
 return $Results[0]
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$IdentitySha,$SnapshotSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') {throw 'Invalid pinned commit SHA.'}
}
$Current=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Current.Count -ne 1 -or
 ([string]$Current[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH';throw 'Validation HEAD mismatch.'
}
$Branch=@(& git -C $WorktreePath symbolic-ref --quiet --short HEAD 2>$null)
if($LASTEXITCODE -eq 0 -and $Branch.Count -ne 0) {
 throw 'Validation requires detached worktree.'
}
$RunFull=[IO.Path]::GetFullPath($RunDir).TrimEnd('\','/')
$StateFull=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
$RepoFull=[IO.Path]::GetFullPath($WorktreePath).TrimEnd('\','/')
if(-not $RunFull.StartsWith(($StateFull+'\'),[StringComparison]::OrdinalIgnoreCase) -or
 $RunFull.StartsWith(($RepoFull+'\'),[StringComparison]::OrdinalIgnoreCase)) {
 throw 'Private report would be outside validation state.'
}
Clean

Stage 'READ_06_4I'
$Source=Find-Report 'ARM-CLEAN06.4I' 'identity-gap-private.json' 'schema' 'ARM-CLEAN06.4I/distinct-identity-gap/v1'
$IdentityGatePath=Join-Path (Split-Path -Parent $Source.Path) 'identity-gate-private.json'
if(-not (Test-Path -LiteralPath $IdentityGatePath -PathType Leaf)) {
 throw 'Identity gate absent.'
}
$Gate=Get-Content -LiteralPath $IdentityGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Gate.schema -cne 'ARM-CLEAN06.4I/distinct-identity-gate/v1' -or
 $Gate.validation_sha -cne $IdentitySha -or
 $Gate.source_binary_snapshot_sha -cne $SnapshotSha -or
 $Gate.gap_report_sha256 -cne (Hash $Source.Path) -or
 [int]$Gate.references -ne 95 -or
 [int]$Gate.unique_missing_item_ids -ne 11 -or
 [int]$Source.Data.unique_missing_item_ids -ne 11 -or
 [int]$Source.Data.missing_canonical_occurrences -ne 40 -or
 [int]$Source.Data.missing_weapon_occurrences -ne 0 -or
 [int]$Source.Data.ids_with_conflicting_legacy_properties -ne 0 -or
 [int]$Source.Data.N0_occurrences -ne 95 -or
 @($Source.Data.missing_canonical_id_details).Count -ne 11 -or
 $Gate.migration_authorized -ne $false -or
 @($Gate.allowed_binary_writes).Count -ne 0 -or
 @($Gate.allowed_package_deletions).Count -ne 0 -or
 @($Gate.allowed_savegame_writes).Count -ne 0) {
 throw '06.4I strict source or zero-write gate invalid.'
}
$Count=0
$Unique=@{}
foreach($Item in @($Source.Data.missing_canonical_id_details)) {
 $Id=[string]$Item.item_id
 if([string]::IsNullOrWhiteSpace($Id) -or $Unique.ContainsKey($Id) -or
 [int]$Item.distinct_legacy_assets -ne 1 -or
 [bool]$Item.source_conflict -or
 $null -eq $Item.source_fields_exact) {
  throw 'Missing identity source is ambiguous.'
 }
 $Unique[$Id]=$true
 $Count+=[int]$Item.world_occurrences
}
if($Unique.Count -ne 11 -or $Count -ne 40) {
 throw 'Private distinct ItemIds do not cover 40 references.'
}

Stage 'VERIFY_BINARY_SNAPSHOT'
$Receipt=Find-Report 'ARM-CLEAN06.4G' 'binary-snapshot-receipt.json' 'validation_sha' $SnapshotSha
$SnapshotGatePath=Join-Path (Split-Path -Parent $Receipt.Path) 'transaction-gate-private.json'
if(-not (Test-Path -LiteralPath $SnapshotGatePath -PathType Leaf)) {
 throw 'Protected 06.4G transaction gate absent.'
}
$SnapshotGate=Get-Content -LiteralPath $SnapshotGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($SnapshotGate.schema -cne 'ARM-CLEAN06.4G/transaction-gate/v1' -or
 $SnapshotGate.validation_sha -cne $SnapshotSha -or
 $SnapshotGate.snapshot_receipt_sha256 -cne (Hash $Receipt.Path) -or
 $SnapshotGate.migration_authorized -ne $false -or
 $Receipt.Data.copy_verified -ne $true -or
 $Receipt.Data.sandbox_restore_drill_verified -ne $true -or
 @($Receipt.Data.source_files).Count -ne 84) {
 throw 'Original immutable binary backup cannot be verified.'
}
$BackupRoot=Join-Path (Split-Path -Parent $Receipt.Path) 'SourceSnapshot'
$Seen=@{}
foreach($Bin in @($Receipt.Data.source_files)) {
 $Rel=[string]$Bin.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+\.uasset$' -or
 $Rel.Contains('..') -or $Seen.ContainsKey($Rel) -or
 [string]$Bin.sha256 -cnotmatch '^[a-f0-9]{64}$') {
  throw 'Protected binary allowlist invalid.'
 }
 $Seen[$Rel]=$true
 $Origin=Join-Path $WorktreePath $Rel.Replace('/','\')
 $Backup=Join-Path $BackupRoot $Rel.Replace('/','\')
 if(-not (Test-Path -LiteralPath $Origin -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Backup -PathType Leaf) -or
 (Get-Item -LiteralPath $Origin).Length -ne [int64]$Bin.bytes -or
 (Get-Item -LiteralPath $Backup).Length -ne [int64]$Bin.bytes -or
 (Hash $Origin) -cne [string]$Bin.sha256 -or
 (Hash $Backup) -cne [string]$Bin.sha256) {
  throw 'Protected UE binary changed or backup no longer matches.'
 }
}
if($Seen.Count -ne 84) {throw 'Expected exactly 84 protected UE binaries.'}
Stage 'UE_TRANSIENT_PROJECTION'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 commandlet or .uproject absent.'
}
$Out=Join-Path $RunDir 'ARM-CLEAN06.4J'
if(Test-Path -LiteralPath $Out) {throw 'Refuse to overwrite former private report.'}
New-Item -ItemType Directory -Path $Out -ErrorAction Stop | Out-Null
$ResultPath=Join-Path $Out 'nonarm-projection-private.json'
& $Editor $Project '-run=GridArmClean064JNonArmProjection' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Input=$($Source.Path)" "-Output=$ResultPath" | Out-Null
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
 Stage 'UE_PROJECTION_FAILED';throw 'UE transient projection did not succeed.'
}

Stage 'VERIFY_RESULT'
$R=Get-Content -LiteralPath $ResultPath -Raw -Encoding UTF8 | ConvertFrom-Json
if($R.schema -cne 'ARM-CLEAN06.4J/nonarm-dryrun/v1' -or
 [int]$R.unique_missing_item_ids -ne 11 -or
 [int]$R.missing_world_occurrences -ne 40 -or
 [int]$R.transient_common_field_projections_equal -ne 11 -or
 [int]$R.ids_with_remaining_behavioral_gates -ne 11 -or
 @($R.candidate_details_private).Count -ne 11 -or
 [int]$R.original_package_dirty_state_changes -ne 0 -or
 $R.transient_only -ne $true -or
 $R.migration_authorized -ne $false -or
 $R.real_savegames_opened -ne $false) {
 throw 'Unreal canonical transient projection integrity invalid.'
}
$TotalFamilies=[int]$R.consumable_candidates+
 [int]$R.equipment_candidates+[int]$R.generic_review_candidates+
 [int]$R.unclassified_candidates
if($TotalFamilies -ne 11) {throw 'Unaccounted canonical candidate family.'}
$CheckIds=@{}
$CheckOccurrences=0
$AllNondefault=0
foreach($Row in @($R.candidate_details_private)) {
 $Id=[string]$Row.item_id
 if(-not $Unique.ContainsKey($Id) -or $CheckIds.ContainsKey($Id) -or
 $Row.common_fields_exact -ne $true -or
 $Row.authoring_approved -ne $false -or
 $Row.binary_writes_allowed -ne $false -or
 @($Row.required_reviews).Count -lt 4) {
  throw 'Transient preview omits an item or a mandatory safety gate.'
 }
 $CheckIds[$Id]=$true
 $CheckOccurrences+=[int]$Row.world_occurrences
 $AllNondefault+=@($Row.nondefault_unmapped_legacy_fields).Count
}
if($CheckIds.Count -ne 11 -or $CheckOccurrences -ne 40 -or
 $AllNondefault -ne [int]$R.unmapped_nondefault_property_occurrences) {
 throw 'Projection candidates do not fully cover private source records.'
}

Stage 'WRITE_PRIVATE_GATE'
$Review=[ordered]@{
 schema='ARM-CLEAN06.4J/nonarm-transaction-gate/v1'
 validation_sha=$ExpectedHead
 prior_identity_sha=$IdentitySha
 protected_snapshot_sha=$SnapshotSha
 source_identity_report_sha256=(Hash $Source.Path)
 projection_report_sha256=(Hash $ResultPath)
 unique_missing_id_count=11
 legacy_occurrence_count=40
 transient_core_parity_count=11
 candidate_family_counts=[ordered]@{
  consumable=[int]$R.consumable_candidates
  equipment=[int]$R.equipment_candidates
  generic_review=[int]$R.generic_review_candidates
  unclassified_review=[int]$R.unclassified_candidates
 }
 unmapped_nondefault_field_occurrences=[int]$R.unmapped_nondefault_property_occurrences
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 next_required='PREPARE_EXPLICIT_BEHAVIOR_AND_PHYSICS_MAPPING_PER_ITEM_BEFORE_ANY_SAVEPACKAGE'
}
$Review | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Out 'nonarm-transaction-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4J - Unreal transient non-weapon definition authoring dry-run'
 ''
 ('Commit : '+$ExpectedHead)
 ('Identity evidence : '+$IdentitySha)
 ('Protected 84-bin snapshot : '+$SnapshotSha)
 ''
 '| Live UE result (aggregate only) | Count |'
 '|---|---:|'
 '| Distinct non-weapon missing ItemIds | 11 |'
 '| Original physical-world references | 40 |'
 '| Transient core metadata projections equal | 11 |'
 ('| Consumable class candidates | '+$R.consumable_candidates+' |')
 ('| Other-equipment class candidates | '+$R.equipment_candidates+' |')
 ('| Generic / specialized policy review candidates | '+$R.generic_review_candidates+' |')
 ('| Unclassified candidates needing explicit decision | '+$R.unclassified_candidates+' |')
 ('| Identities with unmapped nondefault old properties | '+$R.ids_with_unmapped_nondefault_properties+' |')
 ('| Unmapped nondefault property occurrences | '+$R.unmapped_nondefault_property_occurrences+' |')
 ('| Identities still blocked by gameplay/physics/save policy | '+$R.ids_with_remaining_behavioral_gates+' |')
 '| Source and private backup binaries reverified by SHA-256 | 84 |'
 '| Actual user SaveGames opened | 0 |'
 '| .uasset / .umap / .sav writes permitted | 0 |'
 ''
 'Class assignment is a review CANDIDATE based on reflected old ItemType and explicit action flags.'
 'The 11 transient projections prove only named core metadata copy, not gameplay/physics or SaveGame parity.'
 'Private source item IDs, complete UPROPERTY values, per-ID blockers, and all previews stay on Windows.'
 'No transient UObject is saved. No package or game save is written.'
 'Conversion/asset authoring remains BLOCKED pending explicit per-family semantics and rollback.'
)
$Body=Join-Path $Out 'github-issue.md'
Set-Content -LiteralPath $Body -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4J] Transient non-armament projection '+$ExpectedHead.Substring(0,12)
$Link=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Body)
if($LASTEXITCODE -ne 0 -or $Link.Count -ne 1 -or
 [string]$Link[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Public sanitized projection receipt failed.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4J] TRANSIENT_AUTHORING_DRYRUN_OK '+$Link[0])
