<#
 ARM-CLEAN06.4K — private, read-only semantic owner triage of 50 live
 non-armament legacy field occurrences. No SavePackage / no .sav reads.
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
$ProjectionSha='d1548ca7ad69162dcba12bc8f8570b6d6004135c'
$IdentitySha='d3fc4e10de5b82e5669c4dd03c309c30abeab9c6'
$SnapshotSha='e0f877d17f39d913c585b55ba18110745c5385b6'
$StagePath=Join-Path $RunDir 'ARMClean064K.stage'
function Stage([string]$Name) {Set-Content -LiteralPath $StagePath -Value $Name -Encoding ASCII}
function Hash([string]$Path) {(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
function Clean {
 $Lines=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Lines.Count -ne 0) {
  Stage 'WORKTREE_CHANGED';throw 'Validation worktree must be clean.'
 }
}
function Private-Report([string]$StageName,[string]$File,[string]$Selector,[string]$Match) {
 $Root=Join-Path $StateRoot 'Runs'
 if(-not (Test-Path -LiteralPath $Root -PathType Container)) {
  throw 'Private Windows validation history not available.'
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
 if($Results.Count -ne 1) {throw 'Expected exactly one historical proof.'}
 return $Results[0]
}
function Assert-GateClosed([object]$Gate) {
 if([bool]$Gate.migration_authorized -or
  @($Gate.allowed_binary_writes).Count -ne 0 -or
  @($Gate.allowed_package_deletions).Count -ne 0 -or
  @($Gate.allowed_savegame_writes).Count -ne 0) {
  throw 'A historical gate permitted writes unexpectedly.'
 }
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$ProjectionSha,$IdentitySha,$SnapshotSha)) {
 if($Sha -cnotmatch '^[0-9a-f]{40}$') {throw 'Malformed pinned SHA.'}
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH';throw 'Unexpected validation SHA.'
}
$Branch=@(& git -C $WorktreePath symbolic-ref --quiet --short HEAD 2>$null)
if($LASTEXITCODE -eq 0 -and $Branch.Count -gt 0) {
 throw 'Only detached validation worktrees permitted.'
}
$RunFull=[IO.Path]::GetFullPath($RunDir).TrimEnd('\','/')
$StateFull=[IO.Path]::GetFullPath($StateRoot).TrimEnd('\','/')
$RepoFull=[IO.Path]::GetFullPath($WorktreePath).TrimEnd('\','/')
if(-not $RunFull.StartsWith(($StateFull+'\'),[StringComparison]::OrdinalIgnoreCase) -or
  $RunFull.StartsWith(($RepoFull+'\'),[StringComparison]::OrdinalIgnoreCase)) {
 throw 'Private report location may not touch original project.'
}
Clean
Stage 'READ_4J'
$Prior=Private-Report 'ARM-CLEAN06.4J' 'nonarm-projection-private.json' 'schema' 'ARM-CLEAN06.4J/nonarm-dryrun/v1'
$PriorGatePath=Join-Path (Split-Path -Parent $Prior.Path) 'nonarm-transaction-gate-private.json'
if(-not (Test-Path -LiteralPath $PriorGatePath -PathType Leaf)) {
 throw '06.4J canonical projection gate absent.'
}
$PriorGate=Get-Content -LiteralPath $PriorGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($PriorGate.schema -cne 'ARM-CLEAN06.4J/nonarm-transaction-gate/v1' -or
 $PriorGate.validation_sha -cne $ProjectionSha -or
 $PriorGate.prior_identity_sha -cne $IdentitySha -or
 $PriorGate.protected_snapshot_sha -cne $SnapshotSha -or
 $PriorGate.projection_report_sha256 -cne (Hash $Prior.Path) -or
 [int]$Prior.Data.unique_missing_item_ids -ne 11 -or
 [int]$Prior.Data.missing_world_occurrences -ne 40 -or
 [int]$Prior.Data.unmapped_nondefault_property_occurrences -ne 50 -or
 [int]$Prior.Data.ids_with_remaining_behavioral_gates -ne 11 -or
 @($Prior.Data.candidate_details_private).Count -ne 11 -or
 $Prior.Data.transient_only -ne $true) {
 throw '06.4J private provenance incomplete or wrong counters.'
}
Assert-GateClosed $PriorGate
Stage 'READ_4I'
$Original=Private-Report 'ARM-CLEAN06.4I' 'identity-gap-private.json' 'schema' 'ARM-CLEAN06.4I/distinct-identity-gap/v1'
$IdentityGatePath=Join-Path (Split-Path -Parent $Original.Path) 'identity-gate-private.json'
if(-not (Test-Path -LiteralPath $IdentityGatePath -PathType Leaf)) {
 throw '06.4I original identity gate absent.'
}
$IdentityGate=Get-Content -LiteralPath $IdentityGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($IdentityGate.schema -cne 'ARM-CLEAN06.4I/distinct-identity-gate/v1' -or
 $IdentityGate.validation_sha -cne $IdentitySha -or
 $IdentityGate.gap_report_sha256 -cne (Hash $Original.Path) -or
 $IdentityGate.source_binary_snapshot_sha -cne $SnapshotSha -or
 [int]$Original.Data.unique_missing_item_ids -ne 11 -or
 [int]$Original.Data.missing_canonical_occurrences -ne 40) {
 throw '06.4I original identity report invalid.'
}
Assert-GateClosed $IdentityGate
Stage 'REHASH_84'
$Snapshot=Private-Report 'ARM-CLEAN06.4G' 'binary-snapshot-receipt.json' 'validation_sha' $SnapshotSha
$SnapshotGatePath=Join-Path (Split-Path -Parent $Snapshot.Path) 'transaction-gate-private.json'
if(-not (Test-Path -LiteralPath $SnapshotGatePath -PathType Leaf)) {
 throw '06.4G private snapshot gate missing.'
}
$SnapshotGate=Get-Content -LiteralPath $SnapshotGatePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($SnapshotGate.schema -cne 'ARM-CLEAN06.4G/transaction-gate/v1' -or
 $SnapshotGate.validation_sha -cne $SnapshotSha -or
 $SnapshotGate.snapshot_receipt_sha256 -cne (Hash $Snapshot.Path) -or
 $Snapshot.Data.copy_verified -ne $true -or
 $Snapshot.Data.sandbox_restore_drill_verified -ne $true -or
 @($Snapshot.Data.source_files).Count -ne 84) {
 throw '06.4G source material is no longer trusted.'
}
Assert-GateClosed $SnapshotGate
$OriginalBackupRoot=Join-Path (Split-Path -Parent $Snapshot.Path) 'SourceSnapshot'
$Unique=@{}
foreach($Record in @($Snapshot.Data.source_files)) {
 $Rel=[string]$Record.relative
 if($Rel -cnotmatch '^Content/GrimrockPrototype/Core/DataAssets/[A-Za-z0-9_/-]+\.uasset$' -or
  $Rel.Contains('..') -or $Unique.ContainsKey($Rel) -or
  [string]$Record.sha256 -cnotmatch '^[a-f0-9]{64}$') {
  throw 'Private binary source inventory has invalid path/hash.'
 }
 $Unique[$Rel]=$true
 $OriginalFile=Join-Path $WorktreePath $Rel.Replace('/','\')
 $BackupFile=Join-Path $OriginalBackupRoot $Rel.Replace('/','\')
 if(-not (Test-Path -LiteralPath $OriginalFile -PathType Leaf) -or
  -not (Test-Path -LiteralPath $BackupFile -PathType Leaf) -or
  (Get-Item -LiteralPath $OriginalFile).Length -ne [int64]$Record.bytes -or
  (Get-Item -LiteralPath $BackupFile).Length -ne [int64]$Record.bytes -or
  (Hash $OriginalFile) -cne [string]$Record.sha256 -or
  (Hash $BackupFile) -cne [string]$Record.sha256) {
  throw 'Canonical/legacy source or backup changed since 06.4G.'
 }
}
if($Unique.Count -ne 84) {throw 'Expected exactly 84 protected binary sources.'}
Stage 'UNREAL_OWNER_AUDIT'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 commandlet or project unavailable.'
}
$Folder=Join-Path $RunDir 'ARM-CLEAN06.4K'
if(Test-Path -LiteralPath $Folder) {throw 'Refuse overwriting old private ownership report.'}
New-Item -ItemType Directory -Path $Folder -ErrorAction Stop | Out-Null
$Output=Join-Path $Folder 'ownership-private.json'
& $Editor $Project '-run=GridArmClean064KNonArmFieldOwnership' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Projection=$($Prior.Path)" "-Identity=$($Original.Path)" "-Output=$Output" | Out-Null
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $Output -PathType Leaf)) {
 Stage 'UNREAL_OWNER_FAILED';throw 'Editor-only field ownership audit failed.'
}
Stage 'VERIFY_HEADER'
$R=Get-Content -LiteralPath $Output -Raw -Encoding UTF8 | ConvertFrom-Json
if($R.schema -cne 'ARM-CLEAN06.4K/field-ownership/v1' -or
 [int]$R.verified_nonarm_item_ids -ne 11 -or
 [int]$R.verified_world_occurrences -ne 40 -or
 [int]$R.active_unmapped_property_occurrences -ne 50 -or
 [int]$R.field_semantic_parity_approved -ne 0 -or
 [int]$R.source_dirty_state_changes -ne 0 -or
 @($R.private_item_ownership).Count -ne 11 -or
 $R.savegame_files_opened -ne $false -or
 $R.migration_authorized -ne $false) {
 throw 'Real UE field audit header violates strict validation.'
}
Stage 'VERIFY_DOMAIN_KEYS'
$Domains=@('READING','LIGHT','PERSISTENT_VISUAL','WORLD_CHAOS','GENERIC_THROW',
 'ITEM_ACTION','NONARM_EQUIPMENT','NONARM_STACK',
 'LEGACY_ARMAMENT_CATALOGUE','UNCLASSIFIED')
$DomainProperties=@($R.domain_counts.PSObject.Properties | Where-Object {
 $_.MemberType -eq [System.Management.Automation.PSMemberTypes]::NoteProperty
})
$ByDomain=[ordered]@{}
$Sum=0
foreach($Domain in $Domains) {
 $P=$R.domain_counts.PSObject.Properties[$Domain]
 $Value=if($null -eq $P) {0} else {[int]$P.Value}
 if($Value -lt 0) {throw 'Negative ownership group count.'}
 $ByDomain[$Domain]=$Value
 $Sum+=$Value
}
foreach($P in $DomainProperties) {
 if($Domains -cnotcontains [string]$P.Name) {
  throw 'An unrecognized report domain was not classified.'
 }
}
Stage 'VERIFY_DOMAIN_TOTALS'
$Nonempty=@($ByDomain.Keys | Where-Object {$ByDomain[$_] -gt 0})
if($Sum -ne 50 -or $DomainProperties.Count -ne $Nonempty.Count) {
 throw 'Some nondefault legacy fields escaped the ownership matrix.'
}
Stage 'VERIFY_ITEM_ROWS'
$Seen=@{}
$PerItemFields=0
$PerItemOccurrences=0
foreach($Row in @($R.private_item_ownership)) {
 $Id=[string]$Row.item_id
 if([string]::IsNullOrWhiteSpace($Id) -or $Seen.ContainsKey($Id) -or
  [bool]$Row.conversion_authorized -or
  @($Row.field_reviews).Count -ne [int]$Row.active_unmapped_fields) {
  throw 'Per-item ownership report inconsistent.'
 }
 $Seen[$Id]=$true
 $PerItemFields+=@($Row.field_reviews).Count
 $PerItemOccurrences+=[int]$Row.affected_world_occurrences
}
if($Seen.Count -ne 11 -or $PerItemFields -ne 50 -or
 $PerItemOccurrences -ne 40) {
 throw 'Identity count or field list could not be reconciled.'
}
Stage 'WRITE_PRIVATE_GATE'
$GateOut=[ordered]@{
 schema='ARM-CLEAN06.4K/field-owner-gate/v1'
 validation_sha=$ExpectedHead
 source_projection_sha=$ProjectionSha
 source_identity_sha=$IdentitySha
 protected_backup_sha=$SnapshotSha
 exact_projection_sha256=(Hash $Prior.Path)
 exact_identity_sha256=(Hash $Original.Path)
 ownership_report_sha256=(Hash $Output)
 verified_item_ids=11
 active_legacy_field_occurrences=50
 domains=$ByDomain
 parity_approved_fields=0
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 next='AUTHOR_EXPLICIT_NONARM_LIGHT_READ_THROW_CHAOS_ACTION_POLICIES_AND_PROVE_PARITY'
}
$GateOut | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Folder 'field-owner-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4K - exact non-arm legacy property owner gaps'
 ''
 ('Validation commit : '+$ExpectedHead)
 ('Original 11-item projection : '+$ProjectionSha)
 ('Protected 84-source snapshot : '+$SnapshotSha)
 ''
 '| Verified source | Count |'
 '|---|---:|'
 '| Legacy non-arm ItemIds inspected in UE | 11 |'
 '| World references preserved | 40 |'
 '| Active legacy UPROPERTY occurrences reviewed | 50 |'
 ('| Distinct legacy property names in scope | '+$R.distinct_active_unmapped_property_names+' |')
 ('| IDs with unknown property owner domain | '+$R.item_ids_with_unknown_field_domains+' |')
 '| Fully approved semantic migrations | 0 |'
 '| Files / SaveGames authorized for writing | 0 |'
 ''
 '| Strict owner REVIEW category | Active legacy fields |'
 '|---|---:|'
)
foreach($Domain in $Domains) {
 $Lines+=('| '+$Domain+' | '+$ByDomain[$Domain]+' |')
}
$Lines+=@(
 ''
 'An owner category is not an approved conversion and does not prove behavior parity.'
 'Generic non-armament throws must NOT be mapped to ARM-only WeaponCatalogue.ProjectilePolicies.'
 'All ItemIds, exact property names, values and per-asset routes remain private Windows records.'
 'The 84 original files and 84 backup copies were independently rehashed.'
 'SaveGame, UMG, Chaos/PIE and actual .uasset conversion are NOT performed.'
 'Migration gate remains strictly closed.'
)
$PublicPath=Join-Path $Folder 'github-issue.md'
Set-Content -LiteralPath $PublicPath -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4K] Non-arm legacy field ownership '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $PublicPath)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
 [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Aggregate-only GitHub receipt not published.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4K] OWNER_GAP_AUDIT_OK '+$Url[0])
