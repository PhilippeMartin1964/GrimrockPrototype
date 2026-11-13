# ARM-CLEAN06.2E — read-only native semantic inventory in a detached worktree.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$WorktreePath,
    [Parameter(Mandatory=$true)][string]$EngineRoot,
    [Parameter(Mandatory=$true)][string]$ExpectedHead,
    [Parameter(Mandatory=$true)][string]$RunDir,
    [string]$GitHubRepository='PhilippeMartin1964/GrimrockPrototype'
)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$Subject='fix(ARM-CLEAN06.2E.4.1): normalize semantic qualification and classify failures'
$Stage=Join-Path $RunDir 'ARMClean062E.stage'
$OutDir=Join-Path $RunDir 'ARM-CLEAN06.2E'
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
function Set-Stage([string]$Value) {
    Set-Content -LiteralPath $Stage -Value $Value -Encoding ASCII
}
function Git-One([string[]]$GitArgs) {
    $Lines=@(& git -C $WorktreePath @GitArgs)
    if ($LASTEXITCODE -ne 0 -or $Lines.Count -ne 1) { throw 'Git lookup failed.' }
    return ([string]$Lines[0]).Trim()
}
Set-Stage 'PREFLIGHT'
if ($ExpectedHead -notmatch '^[a-f0-9]{40}$' -or
    (Git-One -GitArgs @('rev-parse','HEAD')) -cne $ExpectedHead -or
    (Git-One -GitArgs @('log','-1','--format=%s')) -cne $Subject) {
    throw 'SHA / exact semantic audit trigger mismatch.'
}
if ((Git-One -GitArgs @('remote','get-url','origin')) -notmatch
    '(?i)github\.com[:/]PhilippeMartin1964/GrimrockPrototype(\.git)?$') {
    throw 'Unexpected origin.'
}
$Branch=@(& git -C $WorktreePath branch --show-current)
if ($LASTEXITCODE -ne 0 -or
    @($Branch | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }).Count -gt 0) {
    throw 'Detached validation worktree is mandatory.'
}
$Dirty=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $Dirty.Count -ne 0) { throw 'Read-only worktree not clean.' }
$Editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Project=Join-Path $WorktreePath 'GrimrockPrototype.uproject'
if (-not (Test-Path -LiteralPath $Editor -PathType Leaf) -or
    -not (Test-Path -LiteralPath $Project -PathType Leaf)) {
    throw 'Unreal Editor or project not found.'
}
foreach ($Value in @($Editor,$Project,$OutDir)) {
    if ($Value.Contains('"')) { throw 'Unsupported quoted path.' }
}
Set-Stage 'COMMANDLET'
$ResultPath=Join-Path $OutDir 'hand-semantics-private.json'
$Args='"'+$Project+'" -run=GridArmClean06HandSemantics -unattended -NoSplash -NoP4 -NullRHI "-Output='+$ResultPath+'"'
$Stdout=Join-Path $OutDir 'Unreal.stdout.log'
$Stderr=Join-Path $OutDir 'Unreal.stderr.log'
$Proc=Start-Process -FilePath $Editor -ArgumentList $Args -WorkingDirectory $WorktreePath -WindowStyle Hidden -PassThru -Wait -RedirectStandardOutput $Stdout -RedirectStandardError $Stderr -ErrorAction Stop
if ($Proc.ExitCode -ne 0) {
    # Strict whitelist of diagnostics inferred from the PRIVATE native logs.
    # No source lines, user paths, asset IDs or stack traces are ever exported.
    $Category='NATIVE_PROCESS'
    foreach ($LogPath in @($Stdout,$Stderr)) {
        if (-not (Test-Path -LiteralPath $LogPath -PathType Leaf)) { continue }
        foreach ($Line in @(Get-Content -LiteralPath $LogPath -Tail 3000 -ErrorAction SilentlyContinue)) {
            if ($Line -match '\[ARM-CLEAN06\.2E\] Explicit -Output=') { $Category='CMD_ARGUMENT'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Source catalogue invalid') { $Category='SOURCE_CATALOGUE'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Invalid source ARM identity') { $Category='SOURCE_ARCHETYPE'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Missing/duplicate real ItemDefinitionId') { $Category='ITEM_IDS'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Concrete source count changed') { $Category='ASSET_COUNT'; break }
            if ($Line -match '\[ARM-CLEAN06\.2E\] Private JSON output failed') { $Category='OUTPUT_JSON'; break }
        }
        if ($Category -ne 'NATIVE_PROCESS') { break }
    }
    if ($Category -eq 'NATIVE_PROCESS' -and $Proc.ExitCode -eq 255) {
        $Category='UNREAL_EXIT_255'
    }
    Set-Stage $Category
    throw 'Native UE semantic audit failed; sanitized stage recorded.'
}
if (-not (Test-Path -LiteralPath $ResultPath -PathType Leaf)) {
    Set-Stage 'REPORT_ABSENT'
    throw 'Private UE semantic audit absent.'
}
Set-Stage 'REPORT'
$Report=Get-Content -LiteralPath $ResultPath -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Report.schema -cne 'ARM-CLEAN06.2E/semantic-census/read-only/v1' -or
    [int]$Report.archetypes -ne 82 -or
    [int]$Report.concrete_weapon_assets -ne 24 -or
    [int]$Report.weapon_packages -ne 24 -or
    [int]$Report.unique_weapon_packages -ne 24 -or
    [int]$Report.duplicate_weapon_packages -ne 0 -or
    [int]$Report.count_expected_class -lt 0 -or
    [int]$Report.count_other_class -lt 0 -or
    [int]$Report.count_unloadable -lt 0 -or
    ([int]$Report.count_expected_class + [int]$Report.count_other_class + [int]$Report.count_unloadable) -ne 24 -or
    [int]$Report.unique_nonempty_item_ids -lt 0 -or
    [int]$Report.missing_item_ids -lt 0 -or
    [int]$Report.duplicate_item_ids -lt 0 -or
    ([int]$Report.unique_nonempty_item_ids + [int]$Report.missing_item_ids + [int]$Report.duplicate_item_ids) -ne [int]$Report.count_expected_class -or
    [int]$Report.source_hand_label_variants -ne 3 -or
    @($Report.source_archetype_rows).Count -ne 82 -or
    @($Report.concrete_weapon_rows).Count -ne 24 -or
    [int]$Report.concrete_joined -lt 0 -or
    [int]$Report.concrete_joined -gt [int]$Report.count_expected_class -or
    [int]$Report.concrete_hand_policy_proven -gt [int]$Report.concrete_joined -or
    [int]$Report.concrete_armament_kind_proven -gt [int]$Report.concrete_joined -or
    @($Report.concrete_weapon_rows | Where-Object {
        $_.load_classification -notin @('expected_class','other_class','unloadable') -or
        [string]::IsNullOrWhiteSpace([string]$_.asset_package) -or
        [string]::IsNullOrWhiteSpace([string]$_.registry_class) -or
        ($_.load_classification -ne 'unloadable' -and
            [string]::IsNullOrWhiteSpace([string]$_.loaded_class))
    }).Count -ne 0 -or
    @($Report.concrete_weapon_rows | Select-Object -ExpandProperty asset_package -Unique).Count -ne 24 -or
    [bool]$Report.binary_migration_authorized -or
    [bool]$Report.any_uasset_saved) {
    throw 'Semantic census schema or safety invariants invalid.'
}
# Compare with the exact, user-authored production manifest. Run the helper
# in this process so it can consume the validated PRIVATE native UE report.
# The helper contains no Git writes, and its private rows never leave RunDir.
Set-Stage 'RECONCILE'
$ReconcileHelper=Join-Path $PSScriptRoot 'ARM_CLEAN06_2E3_Reconcile.ps1'
if (-not (Test-Path -LiteralPath $ReconcileHelper -PathType Leaf)) {
    throw 'ARM-CLEAN06.2E.3 helper missing.'
}
$Tokens=$null
$ParseErrors=$null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    $ReconcileHelper, [ref]$Tokens, [ref]$ParseErrors)
if ($ParseErrors -and $ParseErrors.Count -gt 0) {
    throw 'ARM-CLEAN06.2E.3 helper failed syntax validation.'
}
. $ReconcileHelper
if (-not (Test-Path -LiteralPath (Join-Path $OutDir 'reconciliation-private.json') -PathType Leaf)) {
    throw 'ARM-CLEAN06.2E.3 private cross-check absent.'
}
# ARM-CLEAN06.2E.4.1: classify parser, schema, helper and output
# failures separately, without ever publishing UE properties or file paths.
Set-Stage 'QUALIFY_HELPER'
$QualificationHelper=Join-Path $PSScriptRoot 'ARM_CLEAN06_2E4_Qualify.ps1'
if (-not (Test-Path -LiteralPath $QualificationHelper -PathType Leaf)) {
    throw 'ARM-CLEAN06.2E.4.1 qualification helper missing.'
}
Set-Stage 'QUALIFY_PARSE'
$QualificationTokens=$null
$QualificationErrors=$null
[void][System.Management.Automation.Language.Parser]::ParseFile(
    $QualificationHelper, [ref]$QualificationTokens, [ref]$QualificationErrors)
if ($QualificationErrors -and $QualificationErrors.Count -gt 0) {
    throw 'ARM-CLEAN06.2E.4.1 helper syntax rejected.'
}
Set-Stage 'QUALIFY_SCHEMA'
foreach ($Row in @($Report.concrete_weapon_rows)) {
    foreach ($RequiredField in @('is_data_asset','is_primary_data_asset',
        'class_compiled_from_blueprint')) {
        if (@($Row.PSObject.Properties.Name) -notcontains $RequiredField) {
            throw 'ARM-CLEAN06.2E.4.1 native class properties not present.'
        }
    }
    if ([string]$Row.load_classification -eq 'expected_class') {
        foreach ($RequiredField in @('declared_hand_usage_enum','declared_slot_main',
            'declared_slot_off','combat_action_count','combat_throw_weapon',
            'source_n1_has_damage_dice')) {
            if (@($Row.PSObject.Properties.Name) -notcontains $RequiredField) {
                throw 'ARM-CLEAN06.2E.4.1 native item properties not present.'
            }
        }
    }
}
Set-Stage 'QUALIFY_EXEC'
try {
    . $QualificationHelper
}
catch {
    $SafeCodes=@(
        'E4_BAD_PACKAGE_COUNT','E4_CLASS_COUNTS','E4_OPTOUT_COUNTS',
        'E4_HAND_COUNTS','E4_KIND_COUNTS','E4_FOCUS_COUNTS'
    )
    $ErrorCode=[string]$_.Exception.Message
    if ($SafeCodes -contains $ErrorCode) {
        Set-Stage $ErrorCode
    }
    elseif ($ErrorCode -ceq 'Inconsistent native class hierarchy.') {
        Set-Stage 'QUALIFY_CLASS_HIERARCHY'
    }
    elseif ($ErrorCode -ceq 'Expected UGridItemDefinitionAsset is not a primary data asset.') {
        Set-Stage 'QUALIFY_EXPECTED_CLASS'
    }
    else {
        Set-Stage 'QUALIFY_EXEC'
    }
    throw 'ARM-CLEAN06.2E.4.1 native qualification rejected; see sanitized stage.'
}
Set-Stage 'QUALIFY_REPORT'
$QualificationPath=Join-Path $OutDir 'qualification-private.json'
if (-not (Test-Path -LiteralPath $QualificationPath -PathType Leaf)) {
    throw 'ARM-CLEAN06.2E.4.1 private qualification not produced.'
}
Set-Stage 'CHECK_CLEAN'
$After=@(& git -C $WorktreePath status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $After.Count -ne 0) {
    throw 'Native semantic audit modified tracked or untracked project files.'
}
Set-Stage 'ISSUE'
$Markdown=@(
    '# ARM-CLEAN06.2E — audit natif des mains et familles',
    '',
    ('SHA verifie : '+$ExpectedHead),
    '',
    '| Controle | Resultat |',
    '|---|---:|',
    '| Archetypes source | 82 |',
    ('| Objet autre classe : DataAsset | '+$Details['other_class_data_assets']+' |'),
    ('| Objet autre classe : PrimaryDataAsset | '+$Details['other_class_primary_data_assets']+' |'),
    ('| Objet autre classe : hors DataAsset | '+$Details['other_class_non_data_assets']+' |'),
    ('| Objet autre classe : derive Blueprint compile | '+$Details['other_class_blueprint_compiled']+' |'),
    ('| Opt-out catalogue dans manifeste | '+$Details['legacy_opt_out_manifest']+' |'),
    ('| Opt-out catalogue hors manifeste | '+$Details['legacy_opt_out_unlisted']+' |'),
    ('| Opt-out disposant d actions locales | '+$Details['legacy_opt_out_with_combat_actions']+' |'),
    ('| Opt-out lancer de combat active | '+$Details['legacy_opt_out_throw_enabled']+' |'),
    ('| Main indeterminee : NotHandHeld effectif | '+$Details['unresolved_hand_not_handheld']+' |'),
    ('| Main indeterminee : Auto non resolu | '+$Details['unresolved_hand_effective_auto']+' |'),
    ('| Main indeterminee : permissions une main | '+$Details['unresolved_hand_one_hand_permission']+' |'),
    ('| Main indeterminee : permissions deux mains | '+$Details['unresolved_hand_two_hand_permission']+' |'),
    ('| Main indeterminee : autre | '+$Details['unresolved_hand_other']+' |'),
    ('| Type indetermine : arme passive / focus candidat | '+$Details['unresolved_kind_passive_weapon_focus_candidate']+' |'),
    ('| Type indetermine : bouclier offensif | '+$Details['unresolved_kind_offensive_shield_conflict']+' |'),
    ('| Type indetermine : autres divergences | '+$Details['unresolved_kind_other']+' |'),
    ('| Profils joints : permissions runtime plus larges que slots declares | '+$Details['explicit_slot_mismatch_on_joined']+' |'),
    ('| Profils FocusPassive du manifeste joints | '+$Details['listed_focus_passive_profiles']+' |'),
    ('| FocusPassive joints avec type encore indetermine | '+$Details['listed_focus_passive_unresolved_kind']+' |'),

    '| Lignes du manifeste de production | 22 |',
    '| Packages hors manifeste | 2 |',
    ('| Def. du manifeste, classe attendue | '+$Counts['manifest_expected']+' |'),
    ('| Def. du manifeste, autre classe | '+$Counts['manifest_other']+' |'),
    ('| Def. hors manifeste, classe attendue | '+$Counts['extra_expected']+' |'),
    ('| Def. hors manifeste, autre classe | '+$Counts['extra_other']+' |'),
    ('| Autres classes de module Engine | '+$Counts['other_engine']+' |'),
    ('| Autres classes de module jeu natif | '+$Counts['other_project_native']+' |'),
    ('| Autres classes Blueprint / editeur / module divers | '+($Counts['other_blueprint']+$Counts['other_editor']+$Counts['other_module'])+' |'),
    ('| Identites non conformes au manifeste | '+$Counts['manifest_identity_mismatch']+' |'),
    ('| ARM non resolu : opt-out historique | '+$Counts['join_opt_out']+' |'),
    ('| ARM non resolu : identifiant vide | '+$Counts['join_missing_arm']+' |'),
    ('| ARM non resolu : identifiant inconnu | '+$Counts['join_unknown_arm']+' |'),
    ('| ARM non resolu : profil inexistant | '+$Counts['join_missing_profile']+' |'),
    ('| Profils lies, regles de mains non prouvees | '+$Counts['unresolved_joined_hand']+' |'),
    ('| Profils lies, type d armement non prouve | '+$Counts['unresolved_joined_kind']+' |'),

    '| Packages DA_Weapon_* inventoriés | 24 |',
    ('| Classe UGridItemDefinitionAsset | '+[int]$Report.count_expected_class+' |'),
    ('| Autres classes chargeables | '+[int]$Report.count_other_class+' |'),
    ('| Packages non chargeables | '+[int]$Report.count_unloadable+' |'),
    ('| ItemId uniques et presents parmi les classes attendues | '+[int]$Report.unique_nonempty_item_ids+' |'),
    ('| ItemId absents | '+[int]$Report.missing_item_ids+' |'),
    ('| ItemId dupliques | '+[int]$Report.duplicate_item_ids+' |'),
    ('| Jointures catalogue valides | '+[int]$Report.concrete_joined+' |'),
    ('| Regles de mains prouvables | '+[int]$Report.concrete_hand_policy_proven+' |'),
    ('| Types offensif/bouclier prouvables | '+[int]$Report.concrete_armament_kind_proven+' |'),
    ('| Conflits semantiques releves | '+[int]$Report.semantic_conflicts+' |'),
    ('| Types de familles source | '+[int]$Report.source_family_variants+' |'),
    '| Libelles HandUsage source differents | 3 |',
    ('| Archetypes sans definition concrete | '+[int]$Report.source_archetypes_without_any_concrete_definition+' |'),
    '| .uasset modifies | 0 |',
    '',
    '**Inventaire seulement : conversion binaire NON autorisee.**',
    'Qualification structurelle seulement : un objet chargeable sous une autre classe peut etre valide.',
    'Le type Focus n est jamais deduit automatiquement du seul flag de degats.',
    'CanEquipToSlot peut accepter davantage de mains que les slots declares : le cutover exige une preuve comportementale.',
    'Les identites et classes exactes restent dans qualification-private.json ; ni conversion ni suppression d assets.',
    'Deux packages hors du manifeste ne signifient pas deux fichiers obsoletes.',
    'La classe exacte et les identites de chaque package restent dans le rapport prive.',
    'Une reconciliation exacte ne justifie jamais une suppression automatique.',
    'Chaque package DA_Weapon_* reste dans le rapport, meme si sa classe est inattendue.',
    'Les autres classes, packages non chargeables et ItemId invalides sont des BLOQUEURS de conversion.',
    'Les ItemId absents et dupliques ne justifient aucune fusion ni suppression.',
    'La conversion des 82 archetypes ne peut etre deduite des seules 24 definitions.',
    'Les libelles sources exacts, identites, slots et chemins restent dans le JSON prive.',
    'Les valeurs HandUsage du classeur sont informatives dans le catalogue historique.',
    'Aucune modification de Sword/RatGiant, sauvegarde ou lecteur runtime.'
)
$BodyPath=Join-Path $OutDir 'issue.md'
Set-Content -LiteralPath $BodyPath -Value $Markdown -Encoding UTF8
$Title='[ARM-CLEAN06.2E.4] Native semantic qualification '+$ExpectedHead.Substring(0,12)
$Issue=@(& gh issue create --repo $GitHubRepository --title $Title --body-file $BodyPath)
if ($LASTEXITCODE -ne 0 -or $Issue.Count -ne 1) {
    throw 'Filtered GitHub issue publication failed.'
}
Set-Stage 'SUCCESS'
Write-Host ('[ARM-CLEAN06.2E] SEMANTIC_CENSUS_OK '+$Issue[0])
