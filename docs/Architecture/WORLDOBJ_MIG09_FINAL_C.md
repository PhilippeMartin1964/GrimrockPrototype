# WORLDOBJ-MIG09 FINAL-C — Rapport de clôture locale

Date : 2026-09-09. Branche : `master`. Base : `1a49329e9aa0ce3475ab8becefc8d2b7b209e018` (FINAL-B).

**Purge et validation locale terminées. MIG10 non commencé.**

## Reprise du working tree

L'état local repris contenait 32 fichiers modifiés, 26 insertions et 1049 suppressions, avec `git diff --check` réussi. Aucun reset, restore, rétablissement des suppressions ni restauration de session n'a été effectué.

La lecture du diff a identifié les groupes déjà terminés :

- Core : suppression de `FGridLevelObjectData`, `UGridLevelAsset::Objects`, `AddObject`, du writer de compatibilité, de `PostLoad` reconstruisant le miroir, des projections, conversions et lookups legacy.
- Runtime : suppression des surcharges legacy Activation, DoorSystem et acteurs, du transform adapter et de `GridLogicRuntimeLegacy.cpp`, avec ses déclarations.
- Editor : suppression des surcharges legacy de `GridEditorLinkPolicy` et des adaptateurs de sélection, position et placement ; classification native dans l'édition et la validation.
- Tests : MIG07 utilise la classification native et vérifie par réflexion l'absence de `Objects` ainsi que la présence des cinq collections.
- Frontière runtime : constructeur legacy retiré de `FGridRuntimeWorldObjectData` et contrat spécialisé déjà documenté dans l'en-tête.

Le build immédiat a confirmé un état cohérent : aucune correction C++ ni suppression supplémentaire n'a été nécessaire. La reprise a complété l'audit des callers, l'Automation et la documentation. Elle ne permet pas de reconstituer le moment exact de l'interruption ; aucun changement partiellement appliqué n'a été détecté.

## Autorité finale et décision runtime

Les seuls placements persistants de `UGridLevelAsset` sont `WorldObjectInstances`, `LooseItemInstances`, `MonsterSpawns`, `ItemSpawns` et `LogicObjects`.

`GridLevelPlacement::GetBucket` conserve uniquement la classification scalaire des familles ; il ne convertit aucune donnée. Aucun nouveau DTO générique ne remplace le monolithe.

**Décision A : conserver `FGridRuntimeWorldObjectData` comme frontière runtime native utile.**

`AGridLevelRuntimeActor::AddRuntimeObjectActor(const FGridWorldObjectInstance&)` le construit depuis un world-object typé puis le transmet aux initialisateurs virtuels des acteurs, aux visuels de mécanismes, aux objets génériques et à `UGridDoorSystemComponent::RegisterDoorObject`. Activation l'utilise aussi pour résoudre le comportement d'une plaque. Les acteurs Door, Button, Lever, PressurePlate, PitTrapdoor, Receptacle, WallLock et Trigger ont des consommateurs actifs de cette frontière.

La structure est C++ non réfléchie, sans sérialisation ni stockage dans le niveau. Son unique constructeur de placement accepte `FGridWorldObjectInstance` ; elle ne représente pas les loose items, spawns de monstres, générateurs d'items ou objets logiques. Elle transmet identité, cellule/côté et état initial ; le transform monde est fourni séparément. Le comportement partagé est résolu depuis la définition et seuls les cinq groupes d'overrides d'instance sont superposés. `Grimrock.WorldObjects.MIG09.RuntimeWorldObjectPayload` vérifie cette séparation et l'équivalence avec la résolution native.

Cela respecte la séparation Definition / instance / runtime de la [mind map cible](Maps/Grimrock_MindMap_Architecture_Cible_v2_XMind.md). La suite de la migration spatiale ne fait pas partie de FINAL-C.

## Vérifications

La commande demandée a d'abord échoué avant compilation : accès refusé lors de la rotation du journal UBT dans AppData. La même commande avec le journal dans le dépôt a réussi, code 0, `Target is up to date` :

```powershell
& 'D:\UE_5.5\Engine\Build\BatchFiles\Build.bat' GrimrockPrototypeEditor Win64 Development `
  '-Project=D:\Development\GrimrockPrototype\GrimrockPrototype.uproject' -WaitMutex -NoHotReloadFromIDE `
  '-Log=D:\Development\GrimrockPrototype\Saved\Logs\MIG09-FINAL-C-Build.log'
```

Les onze commandes `git grep -n "<symbole>" -- Source` ont toutes retourné 1 sans résultat :

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

Les filtres suivants ont été vérifiés dans les déclarations de tests du projet, puis exécutés ensemble avec `Automation RunTests`, séparés par `+` :

```text
Grimrock.WorldObjects
Grimrock.MON19.2
Grimrock.Editor.MON14.3.1
Grimrock.Editor.MonsterSpawn
Grimrock.TechnicalDebt.TD03_2
Grimrock.TechnicalDebt.TD03_3
Grimrock.TechnicalDebt.TD01_3.EventCommandContract
Grimrock.MON19.6.Editor
Grimrock.MON19.7.1
Grimrock.MON19.4.LuaBridge
Grimrock.Monsters.MON13
Grimrock.Monsters.Perception.AcousticHearing
Grimrock.Pit.PIT01
Grimrock.Pit.PIT03
Grimrock.MON20.4.RecruitmentUI
```

Exécution avec `UnrealEditor-Cmd.exe`, `-Unattended -NoSplash -NoP4 -NoSound -NullRHI -DDC=NoZenLocalFallback`, cache `UE-LocalDataCachePath` dans `DerivedDataCache/`, `-ExecCmds="Automation RunTests <filtres>;Quit"` et rapport dans `Saved/Automation/MIG09/FINAL-C-20260909/`.

| Résultat Automation | Nombre |
|---|---:|
| Réussis sans avertissement | 112 |
| Réussis avec avertissements | 11 |
| Échecs | 0 |
| Non exécutés | 0 |
| Total exécuté | 123 |
| Code de sortie | 0 |

`Grimrock.WorldObjects` compte 36 réussites sans avertissement. Le filtre pit réel comprend `Grimrock.Pit.PIT03_2` ; aucun filtre `PIT03_1` fictif n'est utilisé.

Preuves locales : `Saved/Automation/MIG09/FINAL-C-20260909/index.json`, `Automation.log`, `Automation.console.log` et `Saved/Logs/MIG09-FINAL-C-Build.log`. Ces sorties générées ne sont pas versionnées.

Les avertissements concernent notamment les refus de logique/variables et budgets Lua, les fixtures de monstres et pits, les diagnostics d'archetypes et l'intégration PIE sur les assets réels. Le journal contient aussi les connexions EOS refusées par l'environnement. Les tests concernés restent tous `Success`. Cette validation ne constitue pas une inspection visuelle interactive de l'Editor.

`git diff --check` est réussi. Aucun `.uasset` ou `.umap` n'a été modifié. Arrêt après FINAL-C.

## Publication bloquée par l'environnement

`git ls-remote origin refs/heads/master` confirme `1a49329e9aa0ce3475ab8becefc8d2b7b209e018`, sans divergence à la vérification finale. Le staging des seuls fichiers de l'étape échoue : `Unable to create '.git/index.lock': Permission denied`. Aucun commit ni push n'a donc été réalisé ; tout le travail reste dans le working tree sur `master`.

Depuis un terminal autorisé en écriture sur `.git`, après vérification que `origin/master` est toujours à cette base :

```powershell
git status --short
git ls-remote origin refs/heads/master
git add -- Source/GrimrockPrototype Source/GrimrockPrototypeEditor docs/Architecture/WORLDOBJ_MIG09_LEGACY_PURGE.md docs/Architecture/WORLDOBJ_MIG09_E2C_EDITOR_AUTHORITY.md docs/Architecture/WORLDOBJ_MIG09_FINAL_C.md
git diff --cached --check
git commit -m "WORLDOBJ-MIG09 FINAL-C remove legacy placement compatibility"
git rev-list --count 1a49329e9aa0ce3475ab8becefc8d2b7b209e018..HEAD
git push origin master
```

Le compte doit être exactement `1` avant le push ; arrêter en cas de divergence distante ou de modifications imprévues.
