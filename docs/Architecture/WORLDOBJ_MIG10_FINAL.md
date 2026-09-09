# WORLDOBJ-MIG10 — clôture

Date : 2026-09-09

**WORLDOBJ MIG00 → MIG10 ✅ CLOSED.** Clôture technique du modèle et du vocabulaire ; aucune nouvelle migration, aucun changement de modèle, de Source, de Config ou d'asset dans MIG10-C. La publication porte uniquement sur la documentation.

## Commits

| Étape | Référence |
|---|---|
| MIG10-A | `3860a5f4c3ff97f804f4ec5cb6055020e257fa34` |
| MIG10-B | `e863a667d4599a5a9ac98d658dfabdbf3439de0e` |
| MIG10-C | Titre du commit de clôture : `WORLDOBJ-MIG10 C close definition terminology` |

Le titre identifie C sans tentative d'inscrire son propre SHA dans son contenu. Aucun micro-commit de métadonnées n'est nécessaire. Garde-fou initial satisfait : `master`, arbre propre, HEAD = `origin/master` = MIG10-B après fetch. La clôture est publiée en un seul commit enfant de B ; les vérifications de publication sont exécutées après création du commit.

## Vocabulaire final

Le tableau cite explicitement les anciens noms pour documenter le renommage réalisé en A/B.

| Ancien nom | Nom final |
|---|---|
| `UGridObjectArchetypeAsset` | `UGridWorldObjectDefinitionAsset` |
| `ArchetypeId` sur la définition | `DefinitionId` |
| Référence de définition du placement | `WorldObjectDefinitionId` |
| `DefaultArchetype` | `DefaultWorldObjectDefinition` |
| `ObjectArchetypes` | `WorldObjectDefinitions` |
| `FindObjectArchetypeById` | `FindWorldObjectDefinitionById` |
| `GetEffectiveArchetypeId` | `GetEffectiveWorldObjectDefinitionId` |

La définition ne devient pas une instance et le placement ne devient pas une copie de définition. Aucun alias, wrapper ou classe C++ de compatibilité MIG10 n'existe.

## Audit Source et exceptions exactes

Recherches effectuées avec `git grep -n "<symbole>" -- Source` :

| Symbole recherché | Lignes trouvées |
|---|---:|
| `UGridObjectArchetypeAsset` | 0 |
| `GridObjectArchetypeAsset` | 5 |
| `FindObjectArchetypeById` | 0 |
| `DefaultArchetype` | 0 |
| `GetEffectiveArchetypeId` | 0 |
| `ObjectArchetypes` | 0 |
| `ArchetypeId` | 16 |

**Le zéro brut demandé pour le deuxième grep n'est pas atteint.** Les cinq lignes sont des chemins historiques ou des sondes négatives nécessaires aux tests ; elles ne constituent pas un symbole actif. Les conserver respecte l'interdiction de déplacer les packages ou de modifier artificiellement les tests pour obtenir zéro.

L'audit `git grep -n -i "archetype" -- Source` donne 21 lignes, toutes classifiées ci-dessous (chemins relatifs à Source) :

| Fichier | Lignes | Classification |
|---|---|---|
| `GrimrockPrototype/Private/Tests/GridWorldObjectMIG05CollectibleDefinitionTests.cpp` | 97, 99 | Sondes de l'ancienne identité/API item, dont l'absence est exigée. |
| `GrimrockPrototype/Private/Tests/GridWorldObjectMIG09ItemIdentityTests.cpp` | 20–25, 43–44, 52–53 | Noms et messages de tests négatifs des anciennes identités item, réceptacle et sauvegarde. |
| `GrimrockPrototype/Private/Tests/GridWorldObjectMIG10DefinitionNamingTests.cpp` | 23, 25, 27 | Absence de l'ancienne propriété/classe et contrôle du Core Redirect. |
| `GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/CoreDungeon/GridLevelEditorActor_CoreDungeon_07.inl` | 26, 28, 148 | Trois chemins de packages historiques, volontairement inchangés. |
| `GrimrockPrototypeEditor/Private/Tests/GridEditorTD0731CurrentSchemaAssetAuditTests.cpp` | 284 | Ancien AssetClassPath recherché pour garantir zéro ancien tag sérialisé. |
| `GrimrockPrototypeEditor/Private/Tests/GridEditorWorldObjectMIG09ItemBlueprintReferenceTests.cpp` | 41, 90 | Détection et rejet des références Blueprint à l'ancienne API item. |

Aucune classe, propriété, méthode, variable, catégorie UPROPERTY, diagnostic de production ou commentaire définissant le concept actif ne conserve l'ancien vocabulaire. Les chaînes de tests décrivent explicitement le système supprimé. Aucun renommage cosmétique du Source n'a été fait dans C.

## Core Redirects conservés

`Config/DefaultEngine.ini`, lignes 85–99, contient les 15 redirects MIG10. Toute occurrence insensible à la casse de l'ancien vocabulaire dans Config est limitée aux valeurs `OldName`.

- Class : `GridObjectArchetypeAsset` → `GridWorldObjectDefinitionAsset`.
- Enum/Struct de validation : noms finaux `GridWorldObjectDefinitionValidation…`.
- Property : ancienne identité → `GridWorldObjectDefinitionAsset.DefinitionId`.
- Property complémentaire requis au chargement : `GridWorldObjectDefinitionAsset.ArchetypeId` → `DefinitionId`.
- Palette et runtime : `DefaultArchetype` → `DefaultWorldObjectDefinition`, `ObjectArchetypes` → `WorldObjectDefinitions`, `SourceArchetype` → `SourceWorldObjectDefinition`.
- Propriétés, fonctions et argument Editor : références sélectionnées, création stairs/pit, reset de comportement et setter utilisent les noms finaux.

Ces redirects sont temporaires mais restent nécessaires pendant la période où des Blueprints/assets externes éventuels peuvent encore contenir l'ancien nom. Le resave des 38 assets du dépôt n'autorise pas à présumer que tous les contenus externes ont été migrés. Aucun redirect n'est supprimé.

## MIG10-B confirmé

Avant resave : **38 anciens class tags**, 38 assets chargés via redirect. Le premier chargement avait exposé le problème de préservation de `DefinitionId` ; le redirect complémentaire après renommage de classe l'a corrigé avant resave.

Les 38 `DefinitionId` ont été préservés, non vides et uniques. Après resave : **OldClassTagged = 0, NewClassTagged = 38**. Les champs contrôlés (`SupportedType`, `DefaultBehavior`, `StaticPart`, `MovingParts`, audio et classe runtime), la palette, les placements et les liens sont préservés. B a chargé 92 DataAssets et 3 maps. `DA_GridLevel_01` est resté inchangé.

Preuves locales de B relues : `Saved/Automation/MIG10/B/preflight-before-redirect.json`, `preflight.json`, `postflight.json`, `before-resave-content.json` et `after-resave-levels.json`. Le compte rendu local de B avait été rédigé avant sa publication ; la référence publiée fait foi (`e863a667…`).

## Chemins Content historiques, non normatifs

**« GridObjectArchetypeAsset » peut subsister dans certains chemins de packages historiques. Ce chemin ne représente plus une classe ni un concept architectural actif.**

Le chemin concerné est `Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/` (parfois abrégé historiquement en `Content/GridObjectArchetypeAsset`). Les assets eux-mêmes sont désormais des `UGridWorldObjectDefinitionAsset`. Certains noms de packages `DA_Archetype_*` sont également historiques.

MIG10-C ne renomme ni ce dossier ni les packages : un déplacement impose une migration distincte de packages/références et n'apporte rien au modèle MIG10. Une réorganisation éventuelle de Content pourra être faite dans une tâche dédiée avec AssetTools.

## Rebuild complet : BUILD GREEN

UE **5.5.4**, Visual Studio 2022, Development Editor Win64 ; les trois modules déclarent `bUseUnity = false`.

```powershell
& 'D:\UE_5.5\Engine\Build\BatchFiles\Build.bat' GrimrockPrototypeEditor Win64 Development `
  '-Project=D:\Development\GrimrockPrototype\GrimrockPrototype.uproject' -WaitMutex -NoHotReloadFromIDE `
  -Rebuild -NoUBA -NoUBALocal '-Log=D:\Development\GrimrockPrototype\Saved\Logs\MIG10-C-Rebuild.log'
```

**562 actions, code 0, 136,22 secondes** : recompilation effective, pas un simple « up to date ». Warnings identiques à FINAL-D : MSVC 14.44.35227 non préféré par UE, 54 diagnostics C du code tiers Lua. Aucun échec de compilation.

## Automation finale

**350 tests : 338 succeeded, 12 succeeded with warnings, 0 failed, 0 not run, code processus 0.**

Les 349 tests de FINAL-D sont couverts : deux ont été renommés en A (`MIG03.ArchetypeVisualContract` → `MIG03.DefinitionVisualContract`, `CustomRecruiterArchetypeContract` → `CustomRecruiterDefinitionContract`) ; le test supplémentaire est **`Grimrock.WorldObjects.MIG10.DefinitionNaming`**, passé sans warning.

| Filtre réel vérifié dans Source | Succeeded | Avec warnings | Failed | Not run |
|---|---:|---:|---:|---:|
| Grimrock.WorldObjects | 37 | 0 | 0 | 0 |
| Grimrock.MON19.2 | 18 | 2 | 0 | 0 |
| Grimrock.MON19.3 | 6 | 0 | 0 | 0 |
| Grimrock.MON19.4 | 5 | 1 | 0 | 0 |
| Grimrock.MON19.5 | 3 | 1 | 0 | 0 |
| Grimrock.MON19.6 | 4 | 0 | 0 | 0 |
| Grimrock.MON19.7 | 9 | 1 | 0 | 0 |
| Grimrock.MON19.8 | 4 | 0 | 0 | 0 |
| Grimrock.Monsters.MON13 | 14 | 4 | 0 | 0 |
| Grimrock.Monsters.MON14 | 20 | 0 | 0 | 0 |
| Grimrock.Pit | 5 | 3 | 0 | 0 |
| Grimrock.Runtime | 8 | 0 | 0 | 0 |
| Grimrock.TechnicalDebt | 159 | 0 | 0 | 0 |
| Grimrock.Editor | 3 | 0 | 0 | 0 |
| Grimrock.Items | 2 | 0 | 0 | 0 |
| Grimrock.MON20.4 | 18 | 0 | 0 | 0 |
| Grimrock.MON20.5 | 23 | 0 | 0 | 0 |
| **Total** | **338** | **12** | **0** | **0** |

Les mêmes 17 filtres que FINAL-D sont joints par `+` dans `Automation RunTests …;Quit`. Exécutable `D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`, options `-Unattended -NoSplash -NoP4 -NoSound -NullRHI -DDC=NoZenLocalFallback`. Variable de processus `UE-LocalDataCachePath` sur le `DerivedDataCache/` du dépôt.

Le rapport compte **119 warnings sur les mêmes 12 tests** (133 dans FINAL-D), dont 100 sur `MON13.5.RealPIEIntegration`. Les catégories runtime sont inchangées : refus attendus de cycles/variables/budgets Lua, fixture Lua invalide, fixtures Monster sans Party, validations de définitions du contenu réel et fixtures Pit. Les logs comptent 107 warnings `LogTemp`, 6 `LogGridActivation`, 6 `LogGridMonsterAI`, comme FINAL-D ; les avertissements d'environnement EOS/USD/certificats présents dans FINAL-D ne réapparaissent pas dans cette session. Aucun `Error:` ni `LoadErrors` dans le log Automation.

Preuves : `Saved/Automation/MIG10/C/FinalAutomation/index.json`, `Automation.log`, `console.log`, `exit-code.txt` et `Saved/Automation/MIG10/C/filters.txt`.

## Audit final des assets, sans sauvegarde

Le test `Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit` et le script Python Editor de C confirment :

- **92 DataAssets chargeables**, zéro échec et zéro finding de schéma ;
- **38 `UGridWorldObjectDefinitionAsset`**, zéro ancien AssetClassPath, 38 nouveaux ;
- **38 identifiants non vides et uniques**, champs de définitions identiques à B ;
- palette et cinq collections de placements/liens identiques à B ;
- **3 maps chargeables** : `L_MainMenu`, `L_GrimrockRuntime`, `L_GrimrockEditor`.

| LevelAsset | World | Items libres | Monstres | ItemSpawns | Logic | Liens | Erreurs Editor | Warnings |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| DA_GridLevel_00 | 56 | 7 | 7 | 0 | 0 | 16 | 0 | 59 |
| DA_GridLevel_01 | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 28 |
| DA_GridLevel_Pillars_of_Light | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 27 |

Le niveau principal totalise **70 placements typés, 16 liens et 0 erreur Editor**. Tous les messages Editor sont identiques à B, y compris les warnings de visuels, palette, ancrages et réceptacles.

`DA_GridLevel_01` garde le défaut préexistant : `Start cell X=28 Y=23 Facing=North is invalid. It must be inside the grid, non-empty and not block occupancy.` Le diff de cet asset depuis MIG08 (`1fdae24e`) est vide. Il n'est pas corrigé ni déclaré jouable. La map chargée et les tests PIE automatisés ne remplacent pas un parcours visuel manuel, non effectué dans C.

Preuves : `Saved/Automation/MIG10/C/assets.py`, `assets.json`, `levels.json`, `assets.log`, `assets-exit-code.txt` (0), et `Saved/Diagnostics/TD07/TD07_3_1_CurrentSchemaAssetAudit.txt`. Le script reprend les audits B/FINAL-D en lecture seule et compare leurs instantanés, sans modifier les preuves B.

## Invariants confirmés dans le code courant

| Invariant | Preuve de code et protection |
|---|---|
| Items : définition unique | `FGridLooseItemInstance.ItemDefinition`, palette `DefaultItemDefinition`, tests MIG05 et MIG09 ItemIdentity. |
| World objects : définition unique | `FGridWorldObjectInstance.WorldObjectDefinitionId` → `UGridWorldObjectDefinitionAsset.DefinitionId`, tests MIG06/MIG10. |
| Monstres : définition unique | `FGridMonsterSpawnInstance.MonsterDefinition`, tests MIG07 et MON13/14. |
| Persistance exclusivement typée | `GridLevelAsset.h`, `GridLevelPlacementTypes.h`, tests MIG07 TypedAuthoritySchema et MIG09 Editor. |
| Preview/runtime : mêmes définitions | `GridEditorPreviewComponent.cpp`, `GridPlacementTransformResolver.cpp`, runtime et tests MIG03/MIG05/MIG07. |
| SaveGame : deltas mutables | `GridDungeonRuntimeState.h` et tests de persistance MON13/MON19/MIG09 : états et références d'identité, pas de copie des définitions permanentes. |

Les onze symboles de compatibilité monolithique interdits par FINAL-D sont toujours absents de Source. `FGridRuntimeWorldObjectData` reste non réfléchi/non persistant et spécialisé. Les projections transitoires de placement internes à la définition et la migration audio antérieure, déjà présentes avant MIG10, ne sont ni des alias de noms MIG10 ni une persistance LevelAsset legacy ; C ne les modifie pas et n'annonce pas leur suppression.

## Documentation active et exceptions restantes

Les références centrales ont été renommées et réconciliées :

- [Définitions et placements](WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md) ;
- [Référence des paramètres](../Design/11_GRID_WORLD_OBJECT_DEFINITION_PARAMETERS_REFERENCE.md) ;
- [Roadmap](WORLDOBJ_MIGRATION_ROADMAP_AND_TARGET_DATA_MODEL.md), mind map, règle Definition/Instance et contrats audio ;
- contrats d'architecture, guides Editor et références de tests actifs utilisant les anciens noms.

Les liens internes vers les deux anciens noms de documents sont mis à jour. Les exceptions documentaires sont limitées à :

1. Rapports WORLDOBJ MIG00–MIG09, notes `*_CLEANUP_NOTES`, audits 07/08/09, GEUI10, TD05/TD07, comptes rendus MON13–MON20 et journaux de décisions/terrain : ancien état historique, non réécrit.
2. Citations explicites du renommage dans ce rapport et la roadmap ; annexe explicitement historique du guide de création des items, précédée du workflow actuel à définition unique.
3. Chemins et noms de packages historiques, noms de documents d'audit, ancien catalogue `02_OBJECT_ARCHETYPES.md`, conventions historiques et schémas `object_10_*` archivés (signalés par l'index). Ces noms ne sont pas des types C++ actifs.

## Definition of Done

- [x] modèle cible stabilisé ; items, world objects et monstres à définition unique
- [x] aucune compatibility C++ legacy de nom MIG10 ou de persistance monolithique LevelAsset
- [x] persistance LevelAsset exclusivement typée
- [x] `UGridWorldObjectDefinitionAsset` type final
- [x] 38 définitions sérialisées migrées
- [x] 92 DataAssets chargeables
- [x] 3 maps chargeables
- [x] build complet non-unity vert
- [x] Automation 0 failed / 0 not run
- [x] grep Source final vert sémantiquement, exceptions brutes détaillées
- [x] vocabulaire final des docs actives et liens réconciliés
- [x] Core Redirects documentés et conservés

Le dernier contrôle de livraison est `HEAD == origin/master`, un seul commit depuis B et `git status --short` vide après push. Aucun `Saved/`, binaire, asset resauvegardé ni nouveau chantier n'appartient à C.
