<#
 ARM-CLEAN06.4C — qualify the private 06.4B comparison.
 Zero Unreal writes. All exact asset names and values remain in Runs.
#>
[CmdletBinding()]
param(
 [Parameter(Mandatory=$true)][string]$WorktreePath,
 [Parameter(Mandatory=$true)][string]$StateRoot,
 [Parameter(Mandatory=$true)][string]$RunDir,
 [Parameter(Mandatory=$true)][string]$ExpectedHead,
 [string]$GitHubRepository='PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$CompareSha='e2381de8b516bf826d7568c20bed943fca929413'
$OriginalSha='c66bf68aa1a42eb8d9c47eb9b9336214d0bccf95'
$RecaptureSha='27563be7bee162e86b5f5f9ac29704c1b50aa35c'
$OldManifestHash='03c3b09c1430fc8938e4c4bf1cde6fb229334aeda2348ed902c359c4edca6940'
$NewManifestHash='c00c395cc19645e9f798df5c4a7c38d94aa9d1ecb1e8487ca27d07f864c2b1f9'
$StagePath=Join-Path $RunDir 'ARMClean064C.stage'
function Stage([string]$Name) {
 Set-Content -LiteralPath $StagePath -Value $Name -Encoding ASCII
}
function PropertyDomain([string]$Name) {
 # The categories are TRIAGE HINTS, never authorization to copy a value.
 if($Name -match '(?i)^(ItemDefinitionId|ItemId|WeaponArchetypeId|ArchetypeId|DefaultWeaponItemLevel|ItemLevel|RuntimeObjectId|OwnerCharacterIndex|ItemKind|ItemType|HandUsage|CompatibleEquipmentSlots)') {
  return 'IDENTITY_LEVEL'
 }
 if($Name -match '(?i)(Throw|Projectile|Ballistic|Flight|Arc|Tumbling|Spin|ImpactOffset)') {
  return 'PHYSICS_PROJECTILE'
 }
 if($Name -match '(?i)(Action|Attack|Combat|Damage|Dice|Ammo|Cooldown|Range|Block|Parry|StatBonus|Armor|Health|Mana|Strength|Dexterity)') {
  return 'GAMEPLAY'
 }
 if($Name -match '(?i)(Icon|Mesh|Visual|Material|Texture|Niagara|Sound|SFX|VFX|Audio|Animation|Particle)') {
  return 'VISUAL_AUDIO'
 }
 if($Name -match '(?i)(Contained|GridCell|GridWall|Placed|Spawn|Dungeon|Palette|LevelAsset|CellData|ObjectData|GridObject)') {
  return 'LEVEL_CONTENT'
 }
 if($Name -match '(?i)(DisplayName|Description|Weight|Tag|Stack|Category|Tooltip|DisplayText)') {
  return 'ITEM_METADATA'
 }
 return 'UNKNOWN_REVIEW'
}
function AssetDomain([string]$Id) {
 if($Id -match '^/Game/GrimrockPrototype/Core/DataAssets/Weapons/') { return 'WEAPONS_AND_CATALOGUES' }
 if($Id -match '^/Game/GrimrockPrototype/Core/DataAssets/Items/') { return 'ITEM_DEFINITIONS' }
 if($Id -match '^/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/') { return 'GRID_LEVELS' }
 if($Id -match '^/Game/GrimrockPrototype/Core/DataAssets/GridDungeons/') { return 'DUNGEONS' }
 if($Id -match '^/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default$') { return 'OBJECT_PALETTE' }
 return 'OTHER_REVIEW'
}
function VerifyClean {
 $Dirty=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Dirty.Count -ne 0) {
  Stage 'WORKTREE_CHANGED'
  throw 'Validation worktree modified or untracked files present.'
 }
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$CompareSha,$OriginalSha,$RecaptureSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') { throw 'Invalid commit SHA.' }
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH'
 throw 'Worktree HEAD not equal to the requested commit.'
}
VerifyClean
Stage 'READ_COMPARISON'
$Root=Join-Path $StateRoot 'Runs'
if(-not (Test-Path -LiteralPath $Root -PathType Container)) {
 throw 'Private validation runs unavailable.'
}
$Found=New-Object System.Collections.ArrayList
foreach($Run in @(Get-ChildItem -LiteralPath $Root -Directory -ErrorAction Stop)) {
 $Path=Join-Path (Join-Path $Run.FullName 'ARM-CLEAN06.4B') 'comparison-private.json'
 if(-not (Test-Path -LiteralPath $Path -PathType Leaf)) { continue }
 $Candidate=Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
 if($Candidate.comparison_sha -cne $CompareSha) { continue }
 [void]$Found.Add([pscustomobject]@{ Path=$Path; Report=$Candidate })
}
if($Found.Count -ne 1) { throw 'Expected exactly one original private comparison.' }
$Report=$Found[0].Report
$ReportHash=(Get-FileHash -LiteralPath $Found[0].Path -Algorithm SHA256).Hash.ToLowerInvariant()
if($Report.schema -cne 'ARM-CLEAN06.4B/compare/v1' -or
 $Report.original_sha -cne $OriginalSha -or
 $Report.current_sha -cne $RecaptureSha -or
 $Report.original_manifest_sha256 -cne $OldManifestHash -or
 $Report.current_manifest_sha256 -cne $NewManifestHash -or
 [bool]$Report.migration_authorized) {
 throw 'Comparison provenance or read-only status does not match 06.4B.'
}
Stage 'VERIFY_TOTALS'
$Verified=@{}
foreach($Field in @($Report.counts.PSObject.Properties)) {
 $Verified[[string]$Field.Name]=[int]$Field.Value
}
$Expected=[ordered]@{
 package_set=0
 class_or_kind=0
 property_values=8
 dependencies=4
 referencers=0
 binary_hashes=4
 expanded_profile_fields=0
 changed_packages=4
 changed_profile_rows=0
}
foreach($Key in @($Expected.Keys)) {
 if(-not $Verified.ContainsKey($Key) -or $Verified[$Key] -ne $Expected[$Key]) {
  throw 'Comparison totals no longer match the published evidence.'
 }
}
$Actual=@{ property_values=0; dependencies=0; binary_hashes=0 }
$Assets=@{}
$Categories=[ordered]@{
 IDENTITY_LEVEL=0
 PHYSICS_PROJECTILE=0
 GAMEPLAY=0
 VISUAL_AUDIO=0
 LEVEL_CONTENT=0
 ITEM_METADATA=0
 UNKNOWN_REVIEW=0
}
$AssetKinds=[ordered]@{
 WEAPONS_AND_CATALOGUES=0
 ITEM_DEFINITIONS=0
 GRID_LEVELS=0
 DUNGEONS=0
 OBJECT_PALETTE=0
 OTHER_REVIEW=0
}
Stage 'CLASSIFY'
foreach($Change in @($Report.changes)) {
 $Kind=[string]$Change.category
 if(-not $Actual.ContainsKey($Kind)) {
  throw 'Unrecognized change category in private comparison.'
 }
 $Id=[string]$Change.owner
 if($Id -cnotmatch '^/Game/GrimrockPrototype/Core/DataAssets/' -or $Id.Length -gt 512) {
  throw 'Source package outside legacy audited scope.'
 }
 if([string]::IsNullOrWhiteSpace([string]$Change.field)) {
  throw 'Difference has no serialized field or reference type.'
 }
 if(-not $Assets.ContainsKey($Id)) {
  $AssetKind=AssetDomain $Id
  $AssetKinds[$AssetKind]=[int]$AssetKinds[$AssetKind]+1
  $Assets.Add($Id,[pscustomobject]@{
    package=$Id
    domain=$AssetKind
    approval='REVIEW_REQUIRED'
    migration_authorized=$false
    property_differences=@()
    dependency_differences=@()
    binary_differences=@()
  })
 }
 $Entry=$Assets[$Id]
 if($Kind -eq 'property_values') {
  $Field=[string]$Change.field
  $Domain=PropertyDomain $Field
  $Categories[$Domain]=[int]$Categories[$Domain]+1
  $Entry.property_differences+=@([ordered]@{
   property=$Field; triage_domain=$Domain
   old_export=[string]$Change.before
   new_export=[string]$Change.after
   status='VALUE_REVIEW_REQUIRED'
  })
 }
 elseif($Kind -eq 'dependencies') {
  $Entry.dependency_differences+=@([ordered]@{
   previous_edges=[string]$Change.before
   current_edges=[string]$Change.after
   status='REFERENCE_REVIEW_REQUIRED'
  })
 }
 elseif($Kind -eq 'binary_hashes') {
  $Entry.binary_differences+=@([ordered]@{
   old_sha256=[string]$Change.before
   new_sha256=[string]$Change.after
   status='BINARY_REVIEW_REQUIRED'
  })
 }
 $Actual[$Kind]++
}
foreach($Kind in @($Actual.Keys)) {
 if($Actual[$Kind] -ne $Expected[$Kind]) {
  throw 'Detailed changes disagree with published 06.4B counters.'
 }
}
if($Assets.Count -ne $Expected.changed_packages) {
 throw 'Number of distinct altered packages has changed.'
}
Stage 'WRITE_PRIVATE'
$Output=Join-Path $RunDir 'ARM-CLEAN06.4C'
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$Rows=@($Assets.Values | Sort-Object -Property package)
$Classification=[ordered]@{
 schema='ARM-CLEAN06.4C/qualification/v1'
 source_comparison_sha=$CompareSha
 validation_sha=$ExpectedHead
 source_report_sha256=$ReportHash
 baseline_sha=$OriginalSha
 recapture_sha=$RecaptureSha
 verified_totals=$Verified
 property_domains=$Categories
 changed_asset_domains=$AssetKinds
 changed_packages=$Rows
 qualified_by_policy=$false
 every_difference_requires_review=$true
 migration_authorized=$false
}
$ClassificationPath=Join-Path $Output 'qualification-private.json'
$Classification | ConvertTo-Json -Depth 12 |
 Set-Content -LiteralPath $ClassificationPath -Encoding UTF8
$Plan=[ordered]@{
 schema='ARM-CLEAN06.4C/conversion-gate/v1'
 comparison_sha=$CompareSha
 validation_sha=$ExpectedHead
 source_report_sha256=$ReportHash
 status='BLOCKED_PENDING_EXACT_VALUE_AND_REFERENCE_REVIEW'
 mode='DRY_RUN_PLANNING_ONLY'
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 rollback_required=$true
 editor_only_converter_required=$true
 exact_item_id_level_guid_parity_required=$true
 exact_ballistics_and_projectile_visual_parity_required=$true
 asset_registry_legacy_zero_references_required=$true
 save_continue_and_pie_required=$true
 affected_assets_unapproved=$Rows.Count
}
$Plan | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Output 'conversion-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
VerifyClean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4C - qualification prudente des ecarts prives'
 ''
 ('Commit de qualification : '+$ExpectedHead)
 ('Source comparee : '+$CompareSha)
 ''
 '| Fait confirme | Quantite |'
 '|---|---:|'
 '| Packages avec differences | 4 |'
 '| Proprietes UPROPERTY a examiner | 8 |'
 '| Graphes de dependances a examiner | 4 |'
 '| Empreintes binaires modifiees | 4 |'
 '| Profils ARM numeriques modifies | 0 |'
 ''
 '| Domaine de tri des UPROPERTY (indicatif) | Nombre |'
 '|---|---:|'
)
foreach($Key in @($Categories.Keys)) {
 $Lines+=('| '+$Key+' | '+$Categories[$Key]+' |')
}
$Lines+=@(
 ''
 '| Domaine des packages modifies | Nombre |'
 '|---|---:|'
)
foreach($Key in @($AssetKinds.Keys)) {
 $Lines+=('| '+$Key+' | '+$AssetKinds[$Key]+' |')
}
$Lines+=@(
 ''
 'Chaque difference reste en revue obligatoire: classification NON approuvee.'
 'Les noms, valeurs avant/apres et references exactes restent PRIVES.'
 'Le plan de conversion est bloque. Zero package en autorisation d ecriture.'
 'Aucun fichier de jeu, niveau ou sauvegarde modifie.'
)
$Body=Join-Path $Output 'github-issue.md'
Set-Content -LiteralPath $Body -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4C] Read-only delta qualification '+$ExpectedHead.Substring(0,12)
$Url=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Body)
if($LASTEXITCODE -ne 0 -or $Url.Count -ne 1 -or
 [string]$Url[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Could not publish sanitized qualification issue.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4C] QUALIFICATION_REVIEW_ONLY '+$Url[0])
