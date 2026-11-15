<#
 ARM-CLEAN06.4D: Unreal Editor live read-only dry-run, not asset conversion.
 Exact changed item data stays in private Windows Runs; GitHub sees counts only.
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
$StagePath=Join-Path $RunDir 'ARMClean064D.stage'
$QualificationSha='5da0a702310fa05505d0eb8571d788f9f22d219e'
$RecaptureSha='27563be7bee162e86b5f5f9ac29704c1b50aa35c'
$BaselineHash='c00c395cc19645e9f798df5c4a7c38d94aa9d1ecb1e8487ca27d07f864c2b1f9'
$RootPackage='/Game/GrimrockPrototype/Core/DataAssets/'
function Set-Stage([string]$Name) {
 Set-Content -LiteralPath $StagePath -Value $Name -Encoding ASCII
}
function Is-Same([object]$A,[object]$B) {
 return [string]::Equals([string]$A,[string]$B,[StringComparison]::Ordinal)
}
function Assert-Clean {
 $Lines=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $Lines.Count -ne 0) {
  Set-Stage 'WORKTREE_CHANGED'
  throw 'Validation worktree is not clean.'
 }
}
function Read-Private([string]$Label,[string]$FileName,[string]$ExpectedProperty,[string]$Sha) {
 $Runs=Join-Path $StateRoot 'Runs'
 if(-not (Test-Path -LiteralPath $Runs -PathType Container)) {
  throw 'Windows private validation history is missing.'
 }
 $Found=New-Object System.Collections.ArrayList
 foreach($Run in @(Get-ChildItem -LiteralPath $Runs -Directory -ErrorAction Stop)) {
  $Path=Join-Path (Join-Path $Run.FullName $Label) $FileName
  if(-not (Test-Path -LiteralPath $Path -PathType Leaf)) { continue }
  $Candidate=Get-Content -LiteralPath $Path -Raw -Encoding UTF8 | ConvertFrom-Json
  $Id=$Candidate.PSObject.Properties[$ExpectedProperty]
  if($null -eq $Id -or [string]$Id.Value -cne $Sha) { continue }
  [void]$Found.Add([pscustomobject]@{ Path=$Path; Report=$Candidate })
 }
 if($Found.Count -ne 1) { throw ('Expected exactly one private proof: '+$Label) }
 return $Found[0]
}
function Get-Map([object]$Source) {
 if($null -eq $Source) { throw 'Missing serialized properties.' }
 $Result=@{}
 foreach($Property in @($Source.PSObject.Properties)) {
  $Key=[string]$Property.Name
  if($Result.ContainsKey($Key)) { throw 'Duplicated source field.' }
  $Result.Add($Key,[string]$Property.Value)
 }
 return $Result
}
function Assert-MapEqual([object]$Before,[object]$After) {
 $A=Get-Map $Before
 $B=Get-Map $After
 if($A.Count -ne $B.Count) { throw 'UPROPERTY cardinality drift.' }
 foreach($Name in @($A.Keys)) {
  if(-not $B.ContainsKey($Name) -or -not (Is-Same $A[$Name] $B[$Name])) {
   throw 'Serialized UPROPERTY drift since 06.4A.'
  }
 }
}
function Edges([object[]]$Names) {
 $Ordered=@($Names | ForEach-Object { [string]$_ } | Sort-Object -CaseSensitive)
 return [string]::Join([string][char]31,[string[]]$Ordered)
}
function Target([string]$Field) {
 # Candidate ownership ONLY. Never infer a value or automatically write it.
 if($Field -match '^(WeaponCatalogue|bUseWeaponCatalogue)$') {return 'DA_ItemSystem.WeaponCatalogue'}
 if($Field -match '^(WeaponVisualCatalogue|bUseWeaponVisualCatalogue)$') {return 'DA_ItemSystem.WeaponVisualCatalogue'}
 if($Field -match '(?i)(Presentation|SFX|VFX|Niagara|AttackSound|AttackVisual)') {return 'DA_ActionPresentationCatalogue'}
 if($Field -match '(?i)(Throw|Projectile|Ballistic|Flight|Arc|Spin|Tumble)') {return 'DA_WeaponCatalogue.ProjectilePolicies'}
 if($Field -match '(?i)(CombatActions|QuickItemCombatAction|ActionId|Ability|AttackAction|ActionBinding)') {return 'DA_ActionCatalogue+ItemActionBinding'}
 if($Field -match '(?i)(Ammo|Attack|Damage|Dice|Accuracy|Armor|Parry|Block|HandUsage|EquipmentStatBonus|RangeCells|ActionPointCost)') {return 'DA_WeaponCatalogue'}
 if($Field -match '(?i)(Icon|Mesh|Visual|Texture|Material)') {return 'DA_WeaponVisualCatalogue.VisualProfiles'}
 if($Field -match '(?i)(ItemDefinitionId|ArchetypeId|ItemLevel)') {return 'ConcreteDefinition_or_Instance'}
 if($Field -match '(?i)(DisplayName|Description|Weight|Tags)') {return 'ConcreteDefinition'}
 return 'UNMAPPED_REVIEW'
}

Set-Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$QualificationSha,$RecaptureSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') { throw 'Invalid pinned SHA.' }
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Set-Stage 'HEAD_MISMATCH'
 throw 'Source worktree HEAD not the requested SHA.'
}
Assert-Clean
Set-Stage 'READ_PRIVATE'
$Proof=Read-Private 'ARM-CLEAN06.4C' 'qualification-private.json' 'validation_sha' $QualificationSha
$Qual=$Proof.Report
if($Qual.schema -cne 'ARM-CLEAN06.4C/qualification/v1' -or
 [bool]$Qual.migration_authorized -or [bool]$Qual.qualified_by_policy -or
 @($Qual.changed_packages).Count -ne 4 -or
 [int]$Qual.verified_totals.property_values -ne 8 -or
 [int]$Qual.verified_totals.dependencies -ne 4 -or
 [int]$Qual.verified_totals.binary_hashes -ne 4 -or
 [int]$Qual.verified_totals.changed_profile_rows -ne 0) {
 throw 'Qualification provenance or counts invalid.'
}
$Base=Read-Private 'ARM-CLEAN06.4A' 'asset-hashes.json' 'git_sha' $RecaptureSha
$Receipt=$Base.Report
$BaselinePath=Join-Path (Split-Path -Parent $Base.Path) 'asset-baseline.json'
if(-not (Test-Path -LiteralPath $BaselinePath -PathType Leaf)) {
 throw 'UE 06.4A manifest missing.'
}
$Hash=(Get-FileHash -LiteralPath $BaselinePath -Algorithm SHA256).Hash.ToLowerInvariant()
if($Receipt.schema -cne 'ARM-CLEAN06.4A/asset-hashes/v1' -or
 $Hash -cne $BaselineHash -or $Receipt.manifest_sha256 -cne $Hash -or
 @($Receipt.assets).Count -ne 56) {
 throw 'Previously signed live UE baseline invalid.'
}
$Baseline=Get-Content -LiteralPath $BaselinePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Baseline.schema -cne 'ARM-CLEAN06.0/read-only/v1' -or
 @($Baseline.assets).Count -ne 56 -or
 @($Baseline.expanded_profiles).Count -ne 1640) {
 throw 'Baseline content counts invalid.'
}
$Gate=Read-Private 'ARM-CLEAN06.4C' 'conversion-gate-private.json' 'validation_sha' $QualificationSha
if($Gate.Report.schema -cne 'ARM-CLEAN06.4C/conversion-gate/v1' -or
 [string]$Gate.Report.status -cne 'BLOCKED_PENDING_EXACT_VALUE_AND_REFERENCE_REVIEW' -or
 @($Gate.Report.allowed_binary_writes).Count -ne 0 -or
 @($Gate.Report.allowed_package_deletions).Count -ne 0 -or
 @($Gate.Report.allowed_savegame_writes).Count -ne 0) {
 throw '06.4C closed conversion gate is missing.'
}
Set-Stage 'UE_CAPTURE'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'Local UE5.5.4 editor or project absent.'
}
$Output=Join-Path $RunDir 'ARM-CLEAN06.4D'
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$LivePath=Join-Path $Output 'live-unreal-manifest.json'
# Use the EXISTING Editor-only read commandlet. This commandlet does not
# call SavePackage; the output file is private and outside the Git worktree.
& $Editor $Project '-run=GridArmClean06Baseline' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Output=$LivePath" | Out-Null
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $LivePath -PathType Leaf)) {
 Set-Stage 'UE_CAPTURE_FAILED'
 throw 'UE read-only inventory commandlet failed.'
}
Set-Stage 'CHECK_LIVE_ALL'
$Live=Get-Content -LiteralPath $LivePath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Live.schema -cne $Baseline.schema -or
 @($Live.assets).Count -ne 56 -or @($Live.expanded_profiles).Count -ne 1640 -or
 [int]$Live.counts.passive_profiles -ne 260) {
 throw 'Real UE current inventory cardinality drift.'
}
$OldByName=@{}; $LiveByName=@{}; $ReceiptByName=@{}
foreach($A in @($Baseline.assets)) { $OldByName.Add([string]$A.package,$A) }
foreach($A in @($Live.assets)) { $LiveByName.Add([string]$A.package,$A) }
foreach($A in @($Receipt.assets)) { $ReceiptByName.Add([string]$A.package,$A) }
if($OldByName.Count -ne 56 -or $LiveByName.Count -ne 56 -or $ReceiptByName.Count -ne 56) {
 throw 'Duplicate or missing assets in live UE audit.'
}
foreach($Package in @($OldByName.Keys)) {
 if(-not $LiveByName.ContainsKey($Package) -or -not $ReceiptByName.ContainsKey($Package)) {
  throw 'Package set has changed since 06.4A.'
 }
 $A=$OldByName[$Package]; $B=$LiveByName[$Package]
 foreach($Field in @('class','kind','object')) {
  if(-not (Is-Same $A.$Field $B.$Field)) { throw 'Package class/object drift.' }
 }
 Assert-MapEqual $A.properties $B.properties
 if(-not (Is-Same (Edges @($A.dependencies)) (Edges @($B.dependencies))) -or
    -not (Is-Same (Edges @($A.referencers)) (Edges @($B.referencers)))) {
  throw 'AssetRegistry dependencies/referencers changed since 06.4A.'
 }
 $Relative=$Package.Substring('/Game/'.Length).Replace('/','\')+'.uasset'
 $Binary=Join-Path (Join-Path $WorktreePath 'Content') $Relative
 if(-not (Test-Path -LiteralPath $Binary -PathType Leaf)) { throw 'Binary missing.' }
 $Size=(Get-Item -LiteralPath $Binary).Length
 if($Size -le 200 -or $Size -ne [int64]$ReceiptByName[$Package].bytes) {
  throw 'Current binary invalid, missing or LFS pointer.'
 }
 $BinaryHash=(Get-FileHash -LiteralPath $Binary -Algorithm SHA256).Hash.ToLowerInvariant()
 if(-not (Is-Same $BinaryHash $ReceiptByName[$Package].sha256)) {
  throw 'Current binary changed since 06.4A.'
 }
}
Set-Stage 'CHECK_LIVE_PROFILES'
for($I=0;$I -lt 1640;$I++) {
 Assert-MapEqual $Baseline.expanded_profiles[$I] $Live.expanded_profiles[$I]
}
Set-Stage 'VERIFY_CHANGED_FOUR'
$PrivateRows=New-Object System.Collections.ArrayList
$DestinationCounts=@{}
$TotalProperty=0; $TotalDependency=0; $TotalBinary=0
foreach($Changed in @($Qual.changed_packages)) {
 $Package=[string]$Changed.package
 if(-not $Package.StartsWith($RootPackage,[StringComparison]::Ordinal) -or
    -not $LiveByName.ContainsKey($Package)) { throw 'Changed package invalid.' }
 $Asset=$LiveByName[$Package]
 $Properties=Get-Map $Asset.properties
 $Proposals=New-Object System.Collections.ArrayList
 foreach($Diff in @($Changed.property_differences)) {
  $Name=[string]$Diff.property
  if(-not $Properties.ContainsKey($Name) -or
     -not (Is-Same $Properties[$Name] $Diff.new_export) -or
     (Is-Same $Diff.old_export $Diff.new_export)) {
   throw 'Live UE property does not match signed 06.4C delta.'
  }
  $Candidate=Target $Name
  if(-not $DestinationCounts.ContainsKey($Candidate)) { $DestinationCounts[$Candidate]=0 }
  $DestinationCounts[$Candidate]++
  [void]$Proposals.Add([ordered]@{
   field=$Name
   old_export=[string]$Diff.old_export
   current_export=[string]$Diff.new_export
   candidate_destination=$Candidate
   status='UNAPPROVED_DRY_RUN_PROPOSAL'
  })
  $TotalProperty++
 }
 foreach($Diff in @($Changed.dependency_differences)) {
  if(-not (Is-Same (Edges @($Asset.dependencies)) $Diff.current_edges)) {
   throw 'Live UE dependency graph no longer matches 06.4C.'
  }
  $TotalDependency++
 }
 foreach($Diff in @($Changed.binary_differences)) {
  if(-not (Is-Same $ReceiptByName[$Package].sha256 $Diff.new_sha256)) {
   throw 'Live UE binary does not match 06.4C.'
  }
  $TotalBinary++
 }
 [void]$PrivateRows.Add([ordered]@{
  package=$Package
  domain=[string]$Changed.domain
  property_proposals=@($Proposals.ToArray())
  dependency_differences=@($Changed.dependency_differences)
  binary_differences=@($Changed.binary_differences)
  conversion_status='BLOCKED_PENDING_EXACT_MAPPING'
 })
}
if($PrivateRows.Count -ne 4 -or $TotalProperty -ne 8 -or
 $TotalDependency -ne 4 -or $TotalBinary -ne 4) {
 throw 'Current four-package real UE mapping count mismatch.'
}
Set-Stage 'WRITE_PRIVATE'
$Private=[ordered]@{
 schema='ARM-CLEAN06.4D/live-dry-run/v1'
 validation_sha=$ExpectedHead
 origin_qualification_sha=$QualificationSha
 origin_recapture_sha=$RecaptureSha
 live_manifest_sha256=(Get-FileHash -LiteralPath $LivePath -Algorithm SHA256).Hash.ToLowerInvariant()
 verified_asset_count=56
 verified_profile_count=1640
 verified_changed_packages=4
 verified_property_differences=$TotalProperty
 verified_dependency_differences=$TotalDependency
 verified_binary_differences=$TotalBinary
 proposed_owner_counts=$DestinationCounts
 affected_assets=@($PrivateRows.ToArray())
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 required_next_step='EXPLICIT_MAPPING_AND_REFERENCE_PARITY'
}
$PrivatePath=Join-Path $Output 'dry-run-private.json'
$Private | ConvertTo-Json -Depth 12 |
 Set-Content -LiteralPath $PrivatePath -Encoding UTF8
Set-Stage 'CHECK_CLEAN'
Assert-Clean
Set-Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4D - dry-run Unreal Editor en lecture seule'
 ''
 ('Commit source : '+$ExpectedHead)
 ('Qualification source : '+$QualificationSha)
 ('Capture source : '+$RecaptureSha)
 ''
 '| Verification | Resultat |'
 '|---|---:|'
 '| Assets recharges par Unreal | 56 |'
 '| Profils ARM compares | 1640 |'
 '| Packages divergents revalides | 4 |'
 '| UPROPERTY divergentes revues | 8 |'
 '| Graphes divergents revus | 4 |'
 '| Binaires divergents revus | 4 |'
 '| Ecritures autorisees | 0 |'
 ''
 '| Destination candidate, indicative | Champs |'
 '|---|---:|'
)
foreach($K in @($DestinationCounts.Keys | Sort-Object)) {
 $Lines+=('| '+$K+' | '+$DestinationCounts[$K]+' |')
}
$Lines+=@(
 ''
 'Candidats de destination : classification indicative et NON approuvee.'
 'Aucun nom ou chemin d asset divergent, valeur UPROPERTY, ou SaveGame publie.'
 'Tous les fichiers binaires verifies immuables contre la capture 06.4A.'
 'Conversion editor : BLOQUEE, liste blanche d ecriture vide.'
)
$IssueBody=Join-Path $Output 'github-issue.md'
Set-Content -LiteralPath $IssueBody -Value $Lines -Encoding UTF8
Set-Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4D] Live UE dry-run mapping '+$ExpectedHead.Substring(0,12)
$Link=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $IssueBody)
if($LASTEXITCODE -ne 0 -or $Link.Count -ne 1 -or
 [string]$Link[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Sanitized report publication failed.'
}
Set-Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4D] LIVE_READ_ONLY_OK '+$Link[0])
