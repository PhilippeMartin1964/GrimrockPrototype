# WORLDOBJ-MIG09 FINAL-D — clôture de la migration

Date : 2026-09-09. **WORLDOBJ-MIG09 ✅ CLOSED — FINAL-D publié dans `0bfe69583042f7b04fd0329df43d9a13e1333d76`. MIG10 non commencé.**

La clôture porte sur la migration du modèle de données. Elle ne certifie pas la jouabilité visuelle de tous les niveaux : le départ de `DA_GridLevel_01` est invalide, défaut de contenu préexistant décrit ci-dessous.

## Références Git et périmètre

| Étape | Commit publié |
|---|---|
| FINAL-A, clôture de l'autorité Editor | `1809a674ccc85bb5eef2584849df889a1d9ad4cd` |
| FINAL-B | `1a49329e9aa0ce3475ab8becefc8d2b7b209e018` |
| FINAL-C | `046d58bc429cfbff4b51576cd29ba46b10941eb5` |
| FINAL-D | `0bfe69583042f7b04fd0329df43d9a13e1333d76` |

Au démarrage, `git fetch origin` a réussi, HEAD et `origin/master` étaient tous deux à FINAL-C, sur `master`, avec un working tree propre. FINAL-A possède aussi des commits antérieurs de sous-lots ; le SHA donné est celui de sa clôture.

FINAL-D modifie uniquement les cinq documents de clôture demandés. Aucun changement C++, test compilé, asset, configuration non-unity ou nom `UGridObjectArchetypeAsset` n'a été nécessaire. Les scripts et rapports ponctuels d'audit restent dans `Saved/`, non versionnés.

## Audit structurel

Les onze recherches `git grep -n "<symbole>" -- Source` ne trouvent aucune occurrence (code 1, résultat attendu) :

```text
FGridLevelObjectData
GetObjectCompatibilityView
BuildCompatibilityObjectProjectionFromTyped
TryGetCompatibilityObjectSnapshot
CommitCompatibilityObjectEdit
RefreshLegacyObjectMirrorFromTyped
RebuildTypedPlacementProjectionFromLegacy
EnableTypedPlacementStorageFromLegacy
GridLevelPlacementCompatibility
GridLevelPlacementConversion
GridLogicRuntimeLegacy
```

`UGridLevelAsset` ne contient comme placements persistants que `WorldObjectInstances`, `LooseItemInstances`, `MonsterSpawns`, `ItemSpawns` et `LogicObjects`. Les cellules, liens, variables, scripts et références de quêtes restent des données distinctes des placements. `Objects`, les conversions et le monolithe ont été supprimés ; aucun consommateur de production ou fixture de test ne dépend encore de ces interfaces.

`FGridRuntimeWorldObjectData` reste une structure C++ non réfléchie, sans sérialisation, absente du stockage de `UGridLevelAsset`. Son seul constructeur de placement accepte `FGridWorldObjectInstance`. Elle transporte l'identité et l'état initial vers les initialisateurs d'acteurs world-object, le système de portes et la résolution de comportement des plaques. Elle ne représente pas les autres familles de placements et ne sert pas de DTO général d'authoring. Les règles partagées sont résolues depuis la définition, avec les cinq groupes d'overrides propres à l'instance. Les tests MIG07/MIG09 protègent le schéma et cette frontière.

## Rebuild complet

La commande demandée sans option supplémentaire a rencontré l'interdiction de rotation du journal UBT dans AppData. Avec le journal redirigé sous `Saved/`, elle a réussi mais indiquait `Target is up to date`. Pour valider une recompilation effective depuis le HEAD publié, la commande suivante a ensuite été exécutée :

```powershell
& 'D:\UE_5.5\Engine\Build\BatchFiles\Build.bat' GrimrockPrototypeEditor Win64 Development `
  '-Project=D:\Development\GrimrockPrototype\GrimrockPrototype.uproject' -WaitMutex -NoHotReloadFromIDE `
  -Rebuild -NoUBA -NoUBALocal '-Log=D:\Development\GrimrockPrototype\Saved\Logs\MIG09-FINAL-D-Rebuild.log'
```

Résultat : **561 actions, code 0, 320,16 secondes**. Les trois modules conservent `bUseUnity = false`. Avertissements : MSVC 14.44.35227 n'est pas la version préférée d'UE 5.5 ; 54 diagnostics de compilation concernent le code tiers Lua (conversions, décalages, variables potentiellement non initialisées et code inaccessible). Aucune erreur de compilation. Un fichier généré `Microsoft.Services.Store.winmd`, apparu pendant le rebuild, a été déplacé sous `Saved/Automation/MIG09/` ; il n'est pas versionné.

## Automation

Les filtres ont été vérifiés dans les déclarations du projet. Ils ont été exécutés en une session avec `Automation RunTests`, réunis par `+`, puis `Quit`. Le code de sortie 0 de cette session s'applique à chaque sous-ensemble du tableau. Les comptes proviennent des chemins de tests du rapport JSON, pas du nombre de déclarations trouvé par grep.

| Filtre | Succeeded | Succeeded with warnings | Failed | Not run | Exit code |
|---|---:|---:|---:|---:|---:|
| Grimrock.WorldObjects | 36 | 0 | 0 | 0 | 0 |
| Grimrock.MON19.2 | 18 | 2 | 0 | 0 | 0 |
| Grimrock.MON19.3 | 6 | 0 | 0 | 0 | 0 |
| Grimrock.MON19.4 | 5 | 1 | 0 | 0 | 0 |
| Grimrock.MON19.5 | 3 | 1 | 0 | 0 | 0 |
| Grimrock.MON19.6 | 4 | 0 | 0 | 0 | 0 |
| Grimrock.MON19.7 | 9 | 1 | 0 | 0 | 0 |
| Grimrock.MON19.8 | 4 | 0 | 0 | 0 | 0 |
| Grimrock.Monsters.MON13 | 14 | 4 | 0 | 0 | 0 |
| Grimrock.Monsters.MON14 | 20 | 0 | 0 | 0 | 0 |
| Grimrock.Pit | 5 | 3 | 0 | 0 | 0 |
| Grimrock.Runtime | 8 | 0 | 0 | 0 | 0 |
| Grimrock.TechnicalDebt | 159 | 0 | 0 | 0 | 0 |
| Grimrock.Editor | 3 | 0 | 0 | 0 | 0 |
| Grimrock.Items | 2 | 0 | 0 | 0 | 0 |
| Grimrock.MON20.4 | 18 | 0 | 0 | 0 | 0 |
| Grimrock.MON20.5 | 23 | 0 | 0 | 0 | 0 |
| **Total** | **337** | **12** | **0** | **0** | **0** |

**349 tests distincts exécutés.** `Grimrock.Runtime` inclut les 5 tests réels `Grimrock.Runtime.Doors` et le contrat audio des objets. `Grimrock.Pit` inclut les 5 tests `PIT03`/`PIT03_2`. WorldObjects couvre les placements et les plaques de pression, MON19.2 les primitives Logic et politiques de liens, MON19.3 à MON19.8 Lua et les puzzles, TechnicalDebt l'activation, les réceptacles, les transferts d'items et les audits de schéma.

Le rapport compte **133 avertissements de tests répartis sur 12 tests**, dont 114 dans `MON13.5.RealPIEIntegration`. Ils concernent les refus attendus de cycles/variables/budgets Lua, les fixtures de monstres et pits, les diagnostics des archétypes réels et l'environnement. Les journaux contiennent aussi des avertissements EOS, certificats, Trace Server et USD ; aucun `Error:` ni `LoadErrors` n'a été trouvé dans le journal Automation final.

Exécutable : `D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe`, options `-Unattended -NoSplash -NoP4 -NoSound -NullRHI -DDC=NoZenLocalFallback`. Variable de processus `UE-LocalDataCachePath` pointant sur le `DerivedDataCache/` du dépôt. Rapport : `Saved/Automation/MIG09/FINAL-D-20260909/index.json`, avec `Automation.log`, `Automation.console.log`, `exit-code.txt` et la liste `Saved/Automation/MIG09/FINAL-D-filters.txt`.

## Niveaux et assets réels

L'audit existant `Grimrock.TechnicalDebt.TD07_3_1.CurrentSchemaAssetAudit` charge **92 DataAssets, zéro échec de chargement et zéro finding de schéma** : notamment 3 LevelAssets, 1 DungeonAsset, 38 archétypes, 12 ItemDefinitions et 2 MonsterDefinitions. `TD07_3_6.Normalization.CurrentMonsterSpawnAssets` valide les MonsterSpawns réels. Les tests MIG09 inspectent également les références Blueprint items/mécanismes.

Un script ponctuel Python Editor, sans sauvegarde, utilise les mécanismes existants `LevelEditorSubsystem.load_level` et `AGridLevelEditorActor::ValidateCurrentLevel`. Le plugin Python est activé pour ce processus uniquement, sans modifier le projet. Les maps suivantes ont été chargées avec succès, sans erreur de chargement dans le journal :

- `/Game/GrimrockPrototype/Maps/L_MainMenu`, map de démarrage configurée ;
- `/Game/GrimrockPrototype/Maps/L_GrimrockRuntime` ;
- `/Game/GrimrockPrototype/Maps/L_GrimrockEditor`.

L'acteur Editor réel fournit `DA_Dungeon_01` et `DA_ObjectPalette_Default`. Les trois niveaux de ce donjon sont activés. Chaque LevelAsset est validé dans ce contexte ; les cinq collections et les liens exportés avant chargement sont comparés après validation et sont **strictement inchangés**. Aucun miroir legacy n'existe à reconstruire.

Les LevelAssets se trouvent sous `/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/` :

| Asset / LevelId | World | Items libres | Monstres | ItemSpawns | Logic | Liens | Erreurs Editor | Warnings Editor |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| DA_GridLevel_00 / Into_The_Dark | 56 | 7 | 7 | 0 | 0 | 16 | 0 | 59 |
| DA_GridLevel_01 / Old_Tunnels | 1 | 0 | 0 | 0 | 0 | 0 | 1 | 28 |
| DA_GridLevel_Pillars_of_Light / Pillars_of_Light | 0 | 0 | 0 | 0 | 0 | 0 | 0 | 27 |

Le niveau principal contient 15 portes, 22 réceptacles, 3 boutons, 3 leviers, 2 plaques de pression, 1 pit et 10 décorations. Les validations existantes couvrent références, types de placement, liens, items, réceptacles, monstres, pits et transitions ; aucune erreur n'est retournée pour ce niveau. Cela ne constitue pas un parcours manuel de chaque mécanisme ou transition.

Les warnings Editor signalent notamment des définitions de décorations sans partie visuelle, des ancrages partageables pour portes/remplacements de murs, un ancien identifiant de palette `DoorStone`, des côtés cardinaux ignorés sur certains placements centrés, des réceptacles sans liste d'items acceptés, un item sur cellule non marchable et des liens de réceptacles sans événement réciproque. Ils sont conservés et documentés, sans resauvegarde destinée à les masquer. Les avertissements communs de palette sont répétés pour chaque niveau : les totaux ne comptent donc pas des défauts uniques.

### Défaut préexistant de contenu

`DA_GridLevel_01` retourne : `Start cell X=28 Y=23 Facing=North is invalid. It must be inside the grid, non-empty and not block occupancy.` Le niveau `Old_Tunnels` est activé mais ne doit pas être présenté comme jouable depuis ce départ. `Pillars_of_Light` possède un départ valide mais aucun placement : il n'est pas certifié comme puzzle jouable.

Ce défaut n'est pas introduit par FINAL-C : `git diff 1fdae24e HEAD -- Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_01.uasset` est vide, et le corps de `IsStartCellValid()` est identique à celui de MIG08 (`1fdae24e`). Il reste à traiter dans une tâche de contenu distincte. Aucun asset n'est modifié dans FINAL-D.

Preuves : `Saved/Automation/MIG09/FINAL-D-assets.py`, `FINAL-D-assets.json`, `FINAL-D-assets.log`, `FINAL-D-assets-detail.log` ; les deux exécutions de cet audit ont retourné 0. Ce code de processus atteste l'exécution de l'audit, pas l'absence d'erreurs dans les messages Editor. Rapport de schéma : `Saved/Diagnostics/TD07/TD07_3_1_CurrentSchemaAssetAudit.txt`.

### Limite interactive

`Grimrock.Monsters.MON13.5.RealPIEIntegration` a réellement chargé la map Editor et créé des mondes PIE pour valider le runtime, le déclenchement de vagues et la reprise de sauvegarde. Il utilise une fixture transitoire d'encounter dans ce contexte réel ; il ne parcourt pas le niveau de production original à la place d'un joueur.

L'environnement de cette session ne fournit pas de contrôle natif visuel interactif d'Unreal. Le smoke test manuel déplacement, rotation, interaction, porte/mécanisme, item, monstre et puzzle/link **n'a pas été effectué**. Les tests automatisés de mouvement, portes, items et puzzles ne sont pas décrits comme une validation visuelle.

## État final et Definition of Done

| Tranche | État |
|---|---|
| MIG09-A | ✅ |
| MIG09-B | ✅ |
| MIG09-C | ✅ |
| MIG09-D | ✅ |
| MIG09-E1 | ✅ |
| MIG09-E2A | ✅ |
| MIG09-E2B | ✅ |
| MIG09-E2C | ✅ |
| FINAL-A | ✅ |
| FINAL-B | ✅ |
| FINAL-C | ✅ |
| FINAL-D | ✅ |

Les critères techniques de clôture MIG09 sont satisfaits : rebuild complet vert, Automation sans échec, grep legacy vide, LevelAsset exclusivement typé, aucun consommateur legacy, documentation réconciliée et `git diff --check` vert. Les réserves de contenu et de contrôle interactif ci-dessus restent explicites.

## Publication FINAL-D

FINAL-D a été publié sur `master` dans le commit `0bfe69583042f7b04fd0329df43d9a13e1333d76` (`WORLDOBJ-MIG09 FINAL-D close migration`). Le push a avancé `origin/master` depuis `046d58bc429cfbff4b51576cd29ba46b10941eb5` vers ce commit, puis `git status --short` a confirmé un working tree propre.

Le présent correctif documentaire met uniquement à jour les métadonnées de publication qui avaient été rédigées avant que le commit FINAL-D puisse être créé. Il ne modifie ni le périmètre, ni les validations, ni le SHA de FINAL-D.

**WORLDOBJ-MIG09 ✅ CLOSED. FINAL-D publié. Aucun renommage d'archétype. MIG10 non commencé.**
