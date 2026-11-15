<#
 ARM-CLEAN06.4L: STRICT read-only UE projection of non-arm light / persistent
 visuals. Private real asset values stay in Windows StateRoot. No SavePackage.
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
$StagePath=Join-Path $RunDir 'ARMClean064L.stage'
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
foreach($Sha in @($ExpectedHead,$OwnershipSha,$SnapshotSha,$ProjectionSha,$IdentitySha)) {
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
Stage 'UNREAL_PRESENTATION_PARITY'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 project or commandlet unavailable.'
}
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4L'
if(Test-Path -LiteralPath $Folder) {throw 'Refuse to overwrite any existing report.'}
New-Item -ItemType Directory -Path $Folder -ErrorAction Stop | Out-Null
$Out=Join-Path $Folder 'presentation-parity-private.json'
& $Editor $Project '-run=GridArmClean064LNonArmPresentationParity' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Ownership=$($Previous.Path)" "-Output=$Out" | Out-Null
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Out -PathType Leaf)) {
 Stage 'UNREAL_PARITY_FAILED'
 throw 'Native real UE light/visual projection failed.'
}
Stage 'VERIFY_RESULT'
$R=Get-Content -LiteralPath $Out -Raw -Encoding UTF8 | ConvertFrom-Json
if($R.schema -cne 'ARM-CLEAN06.4L/nonarm-presentation-data-parity/v1' -or
 [int]$R.verified_item_ids -ne 11 -or
 [int]$R.preserved_world_occurrences -ne 40 -or
 [int]$R.source_active_fields_reviewed -ne 50 -or
 [int]$R.active_light_fields -ne 1 -or
 [int]$R.active_persistent_visual_fields -ne 11 -or
 [int]$R.exact_reflected_copies_including_defaults -ne 88 -or
 [int]$R.source_dirty_state_changes -ne 0 -or
 [int]$R.runtime_semantic_parity_approved -ne 0 -or
 $R.binary_writes_performed -ne $false -or
 $R.savegame_files_opened -ne $false -or
 $R.migration_authorized -ne $false -or
 @($R.private_item_projections).Count -ne 11) {
 throw 'Native UE scope, parity or no-write invariant failed.'
}
$Seen=@{}
$PerItemCopies=0
$PerItemActive=0
foreach($Row in @($R.private_item_projections)) {
 $Id=[string]$Row.item_id
 if([string]::IsNullOrWhiteSpace($Id) -or $Seen.ContainsKey($Id) -or
  $Row.data_parity -ne $true -or $Row.runtime_parity -ne $false -or
  [int]$Row.exact_reflected_copies -ne 8 -or
  [int]$Row.active_presentation_properties -lt 0) {
  throw 'Private UE item projection incomplete.'
 }
 $Seen[$Id]=$true
 $PerItemCopies+=[int]$Row.exact_reflected_copies
 $PerItemActive+=[int]$Row.active_presentation_properties
}
if($Seen.Count -ne 11 -or $PerItemCopies -ne 88 -or
 $PerItemActive -ne 12) {
 throw 'Private cross-item parity totals disagree.'
}
Stage 'WRITE_PRIVATE_GATE'
$GateOut=[ordered]@{
 schema='ARM-CLEAN06.4L/nonarm-presentation-gate/v1'
 validation_sha=$ExpectedHead
 source_ownership_sha=$OwnershipSha
 protected_backup_sha=$SnapshotSha
 ownership_report_sha256=(Hash $Previous.Path)
 presentation_report_sha256=(Hash $Out)
 physical_source_snapshots_verified=84
 active_data_parity_fields=12
 reflected_copy_checks=88
 runtime_semantic_parity_approved=0
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 next='REVIEW_NONARM_BEHAVIOR_AND_REMAINING_38_FIELDS_BEFORE_BINARY_CUTOVER'
}
$GateOut | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Folder 'presentation-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4L - canonical non-arm light / persistent visual data parity'
 ''
 ('Validation commit: '+$ExpectedHead)
 ('Protected 06.4K ownership: '+$OwnershipSha)
 ('Protected 06.4G binary backup: '+$SnapshotSha)
 ''
 '| Verified property | Count |'
 '|---|---:|'
 '| Real UE missing non-arm ItemIds | 11 |'
 '| Preserved level/palette/receptacle occurrences | 40 |'
 '| Original active legacy fields still tracked | 50 |'
 '| Active light/persistent visual properties copied exactly | 12 |'
 '| Strict reflected copies including default-valued properties | 88 |'
 '| Remaining active properties without data projection in 06.4L | 38 |'
 '| Runtime/PIE/Chaos semantic migrations approved | 0 |'
 '| Asset/SaveGame writes authorized | 0 |'
 ''
 'Exact field values, identities and source paths remain in private Windows evidence.'
 'This proves in-memory DATA parity only; it does not prove runtime Niagara, torch state, PIE, SaveGame or a binary conversion.'
 'The migration gate remains CLOSED.'
)
$Public=Join-Path $Folder 'github-issue.md'
Set-Content -LiteralPath $Public -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4L] Non-arm presentation data parity '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Public)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
 [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Public aggregate receipt could not be published.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4L] NONARM_PRESENTATION_DATA_PARITY_OK '+$Url[0])
