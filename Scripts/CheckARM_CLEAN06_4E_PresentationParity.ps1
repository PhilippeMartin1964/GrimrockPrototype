<#
 ARM-CLEAN06.4E — Real UE legacy/canonical presentation parity. Read-only.
 Only aggregate counters may go to a public GitHub issue.
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
$StagePath=Join-Path $RunDir 'ARMClean064E.stage'
$SourceSha='046d7850006476925710a06e50033db4da1c0d8e'
$QualificationSha='5da0a702310fa05505d0eb8571d788f9f22d219e'
function Stage([string]$Code) { Set-Content -LiteralPath $StagePath -Value $Code -Encoding ASCII }
function Assert-Clean {
 $State=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
 if($LASTEXITCODE -ne 0 -or $State.Count -ne 0) {
  Stage 'WORKTREE_CHANGED'; throw 'Validation checkout not clean.'
 }
}
Stage 'PREFLIGHT'
foreach($Sha in @($ExpectedHead,$SourceSha,$QualificationSha)) {
 if($Sha -cnotmatch '^[a-f0-9]{40}$') { throw 'Malformed validation SHA.' }
}
$Head=@(& git -C $WorktreePath rev-parse --verify HEAD)
if($LASTEXITCODE -ne 0 -or $Head.Count -ne 1 -or
 ([string]$Head[0]).Trim() -cne $ExpectedHead) {
 Stage 'HEAD_MISMATCH'; throw 'Unrecognized UE validation worktree HEAD.'
}
Assert-Clean
Stage 'READ_4D'
$RunsRoot=Join-Path $StateRoot 'Runs'
if(-not (Test-Path -LiteralPath $RunsRoot -PathType Container)) {
 throw 'Private UE validation runs missing.'
}
$Found=New-Object System.Collections.ArrayList
foreach($Run in @(Get-ChildItem -LiteralPath $RunsRoot -Directory -ErrorAction Stop)) {
 $CandidatePath=Join-Path (Join-Path $Run.FullName 'ARM-CLEAN06.4D') 'dry-run-private.json'
 if(-not (Test-Path -LiteralPath $CandidatePath -PathType Leaf)) { continue }
 $Candidate=Get-Content -LiteralPath $CandidatePath -Raw -Encoding UTF8 | ConvertFrom-Json
 if($Candidate.validation_sha -cne $SourceSha) { continue }
 [void]$Found.Add([pscustomobject]@{Path=$CandidatePath; Data=$Candidate})
}
if($Found.Count -ne 1) { throw 'Exactly one private validated 06.4D report required.' }
$Source=$Found[0].Data
if($Source.schema -cne 'ARM-CLEAN06.4D/live-dry-run/v1' -or
 $Source.origin_qualification_sha -cne $QualificationSha -or
 [int]$Source.verified_asset_count -ne 56 -or
 [int]$Source.verified_profile_count -ne 1640 -or
 [int]$Source.verified_changed_packages -ne 4 -or
 [int]$Source.verified_property_differences -ne 8 -or
 [int]$Source.verified_dependency_differences -ne 4 -or
 [int]$Source.verified_binary_differences -ne 4 -or
 @($Source.affected_assets).Count -ne 4 -or
 [bool]$Source.migration_authorized -or
 @($Source.allowed_binary_writes).Count -ne 0 -or
 @($Source.allowed_package_deletions).Count -ne 0 -or
 @($Source.allowed_savegame_writes).Count -ne 0) {
 throw '06.4D signed dry-run or closed conversion gate invalid.'
}
$FieldCount=0
$Seen=@{}
foreach($Asset in @($Source.affected_assets)) {
 $Package=[string]$Asset.package
 if(-not $Package.StartsWith('/Game/GrimrockPrototype/Core/DataAssets/Weapons/',
     [StringComparison]::Ordinal) -or $Seen.ContainsKey($Package)) {
  throw 'Unexpected private source package.'
 }
 $Seen[$Package]=$true
 foreach($Field in @($Asset.property_proposals)) {
  if($Field.candidate_destination -cne 'DA_ActionPresentationCatalogue' -or
     $Field.status -cne 'UNAPPROVED_DRY_RUN_PROPOSAL') {
   throw 'Unexpected value mapping or prematurely approved migration.'
  }
  $FieldCount++
 }
}
if($FieldCount -ne 8) { throw 'Unexpected private property count.' }
Stage 'UNREAL_PRESENTATION_PARITY'
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if(-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
 -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
 throw 'UE5.5.4 runtime not available.'
}
$Output=Join-Path $RunDir 'ARM-CLEAN06.4E'
New-Item -ItemType Directory -Path $Output -Force | Out-Null
$ParityPath=Join-Path $Output 'presentation-parity-private.json'
& $Editor $Project '-run=GridArmClean064EPresentationParity' '-unattended' '-NoSplash' '-NoP4' '-NullRHI' "-Input=$($Found[0].Path)" "-Output=$ParityPath" | Out-Null
if($LASTEXITCODE -ne 0 -or -not (Test-Path -LiteralPath $ParityPath -PathType Leaf)) {
 Stage 'UNREAL_PARITY_FAILED'
 throw 'Editor-only presentation parity commandlet failed.'
}
Stage 'VERIFY_RESULT'
$Parity=Get-Content -LiteralPath $ParityPath -Raw -Encoding UTF8 | ConvertFrom-Json
if($Parity.schema -cne 'ARM-CLEAN06.4E/presentation-parity/v1' -or
 [int]$Parity.verified_source_assets -ne 4 -or
 [int]$Parity.verified_changed_properties -ne 8 -or
 @($Parity.sources).Count -ne 4 -or
 [bool]$Parity.migration_authorized) {
 throw 'Live presentation parity source/counter mismatch.'
}
$TotalSources=[int]$Parity.fully_matching_assets +
 [int]$Parity.review_required_assets + [int]$Parity.invalid_source_assets
if($TotalSources -ne 4) { throw 'Four historical assets not accounted for.' }
$AllLinks=[int]$Parity.equal_attack_bindings + [int]$Parity.different_attack_bindings +
 [int]$Parity.unresolved_attack_bindings
if($AllLinks -ne [int]$Parity.checked_attack_bindings) {
 throw 'Action binding partition invalid.'
}
foreach($Value in @(
 $Parity.checked_attack_bindings,$Parity.equal_attack_bindings,
 $Parity.different_attack_bindings,$Parity.unresolved_attack_bindings,
 $Parity.fully_matching_assets,$Parity.review_required_assets,$Parity.invalid_source_assets)) {
 if([int]$Value -lt 0) { throw 'Negative read-only parity count.' }
}
# Verify no report row invented, skipped or duplicated an old package.
$SeenResults=@{}
foreach($Row in @($Parity.sources)) {
 $Package=[string]$Row.package
 if(-not $Seen.ContainsKey($Package) -or $SeenResults.ContainsKey($Package)) {
  throw 'Source row lost, invented or repeated.'
 }
 $SeenResults[$Package]=$true
}
Stage 'WRITE_PRIVATE_GATE'
$Gate=[ordered]@{
 schema='ARM-CLEAN06.4E/presentation-gate/v1'
 validation_sha=$ExpectedHead
 input_report_sha=$SourceSha
 parity_report_sha256=(Get-FileHash -LiteralPath $ParityPath -Algorithm SHA256).Hash.ToLowerInvariant()
 exact_parity_known_for=$Parity.fully_matching_assets
 unresolved_or_different=([int]$Parity.review_required_assets+[int]$Parity.invalid_source_assets)
 migration_authorized=$false
 allowed_binary_writes=@()
 allowed_package_deletions=@()
 allowed_savegame_writes=@()
 next='REVIEW_CANONICAL_BINDINGS_AND_FIELD_DELTAS'
}
$Gate | ConvertTo-Json -Depth 8 |
 Set-Content -LiteralPath (Join-Path $Output 'presentation-gate-private.json') -Encoding UTF8
Stage 'CHECK_CLEAN'
Assert-Clean
Stage 'ISSUE_BUILD'
$Lines=@(
 '# ARM-CLEAN06.4E - presentation historique vs catalogue canonique UE'
 ''
 ('Commit de validation : '+$ExpectedHead)
 ('Dry-run source : '+$SourceSha)
 ''
 '| Controle read-only | Resultat |'
 '|---|---:|'
 ('| Assets historiques examines | '+$Parity.verified_source_assets+' |')
 ('| UPROPERTY exactement reverifiees | '+$Parity.verified_changed_properties+' |')
 ('| Liaisons attaque evaluees | '+$Parity.checked_attack_bindings+' |')
 ('| Liaisons rigoureusement identiques | '+$Parity.equal_attack_bindings+' |')
 ('| Liaisons avec differences | '+$Parity.different_attack_bindings+' |')
 ('| Liaisons absentes / non prouvees | '+$Parity.unresolved_attack_bindings+' |')
 ('| Assets pleinement correspondants | '+$Parity.fully_matching_assets+' |')
 ('| Assets a revoir | '+$Parity.review_required_assets+' |')
 ('| Sources non identifiables | '+$Parity.invalid_source_assets+' |')
 '| Packages ou SaveGames autorises a l ecriture | 0 |'
 ''
 'La comparaison des profils canoniques inclut TOUS les champs UE :'
 'animation, timings, chemins audio/Niagara et canaux supplementaires.'
 'Les noms, champs et valeurs exacts restent dans les rapports prives Windows.'
 'Statut SUCCESS = audit execute; PAS validation du cutover ni permission de migration.'
)
$Issue=Join-Path $Output 'github-issue.md'
Set-Content -LiteralPath $Issue -Value $Lines -Encoding UTF8
Stage 'ISSUE_WRITE'
$Title='[ARM-CLEAN06.4E] Presentation parity read-only '+$ExpectedHead.Substring(0,12)
$Link=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $Issue)
if($LASTEXITCODE -ne 0 -or $Link.Count -ne 1 -or
 [string]$Link[0] -notmatch '^https://github\.com/[^/]+/[^/]+/issues/[0-9]+$') {
 throw 'Sanitized GitHub issue publication failed.'
}
Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.4E] PRESENTATION_AUDIT_DONE '+$Link[0])
