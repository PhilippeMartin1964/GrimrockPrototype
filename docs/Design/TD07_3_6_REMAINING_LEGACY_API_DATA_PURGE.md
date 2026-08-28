# TD07.3.6 — Remaining Legacy API/Data Purge

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3 — Prototype Data Model Reset
Statut : CHARACTERIZATION VALIDÉE — STATIC PURGE IMPLÉMENTÉ / MONSTERSPAWN ASSET REPAIR PREPARED

## 1. Objectif

Supprimer les API, champs et chemins legacy encore présents après la normalisation Save, Character State, Authoring Identity et Combat Data.

Baseline TD07.3.1 :

```text
bPlaceOnEdge
bPlaceAtCellCenter
GetFacingForLegacyYaw
HasCharacterCommittedAttackThisPhase
bEnableLegacyKeyboardUseAction
EGridRuntimeRebuildMode::ObjectsOnly
```

L'audit actuel étend aussi le chemin clavier lié au flag legacy :

```text
UseAction
HandleUse
TryUseFrontInteraction
BufferUseCommand
EBufferedCommandType::Use
```

Ces symboles doivent être traités comme un ensemble cohérent si leur suppression est validée.

## 2. Caractérisation — placement

`UGridObjectArchetypeAsset::PlacementKind` est explicitement documenté comme source de vérité courante Editor/Runtime.

```text
bPlaceOnEdge
bPlaceAtCellCenter
```

ne servent plus qu'à :
- sérialiser des miroirs historiques ;
- produire des warnings de validation en cas de divergence ;
- être initialisés par quelques fixtures/outils historiques.

Aucune logique de placement courante ne doit dépendre de ces booléens.

## 3. Caractérisation — MonsterSpawn yaw/facing

Le cas suivant reste un vrai fallback de compatibilité de données :

```cpp
if (!IsValidMonsterSpawnFacing(ObjectData.InitialFacing))
{
    ObjectData.InitialFacing = GetFacingForLegacyYaw(ObjectData.LocalYaw);
}
ObjectData.LocalYaw = GetYawForFacing(ObjectData.InitialFacing);
```

`InitialFacing` est l'autorité gameplay actuelle. `LocalYaw` reste un champ générique de transform/preview pour les objets de niveau.

Avant suppression de `GetFacingForLegacyYaw`, il faudra déterminer si les LevelAssets courants doivent être resauvegardés afin que tous les MonsterSpawn portent déjà un `InitialFacing` cardinal persistant.

Ce point peut nécessiter un one-shot asset repair, comme TD07.3.5.4, mais aucune migration backward ne sera conservée.

## 4. Caractérisation — combat query

`HasCharacterCommittedAttackThisPhase()` est marqué `DeprecatedFunction`.

L'autorité actuelle est :

```text
GetPlayerCharacterTurnState()
CanCharacterAct()
CanCharacterSpendActionPoints()
```

Les usages source trouvés hors déclaration/implémentation sont limités aux tests historiques MON11.

## 5. Caractérisation — ancien Use clavier

Le système souris est l'autorité d'interaction joueur.

Le chemin clavier historique reste désactivé par défaut via :

```cpp
bEnableLegacyKeyboardUseAction = false;
```

Lorsque le flag est activé, il réactive :

```text
UseAction
 -> HandleUse
 -> TryUseFrontInteraction
 -> éventuellement BufferUseCommand
 -> EBufferedCommandType::Use
```

La cible TD07.3.6 est de supprimer cette voie legacy sans réintroduire la touche F. Les interactions souris et leurs services runtime restent inchangés.

## 6. Caractérisation — runtime rebuild

`EGridRuntimeRebuildMode::ObjectsOnly` est déclaré comme mode réservé legacy.

Les call-sites courants utilisent uniquement :

```text
Full
GeometryOnly
```

Aucun call-site `ObjectsOnly` n'a été trouvé dans le runtime, le startup ou le Grid Editor.

## 7. Gate

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_6.Characterization
```

Tests :

```text
LegacyPlacementMirrors
MonsterSpawnYawFallback
DeprecatedCombatAndKeyboardUse
LegacyRebuildMode
```

Attendu : 4/4, zéro warning, zéro échec.

## 8. Séquence après caractérisation verte

1. supprimer les APIs/paths purement morts sans dépendance asset ;
2. adapter les tests historiques à l'autorité courante ;
3. qualifier le besoin de resauvegarde des LevelAssets pour MonsterSpawn ;
4. si nécessaire, faire un one-shot current-asset repair ;
5. supprimer le fallback yaw ;
6. adapter l'audit TD07.3.1 ;
7. ajouter le gate `TD07_3_6.Normalization` ;
8. régressions + Shipping avant clôture.

Politique : **aucune compatibilité arrière**.


## 9. Characterization validée

Validation locale du 28 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_6.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260828-095553
```

## 10. Static purge implémenté

Supprimés :

```text
bPlaceOnEdge
bPlaceAtCellCenter
HasCharacterCommittedAttackThisPhase
bEnableLegacyKeyboardUseAction
UseAction
HandleUse
TryUseFrontInteraction
BufferUseCommand
EBufferedCommandType::Use
EGridRuntimeRebuildMode::ObjectsOnly
ARCHETYPE.LEGACY_PLACEMENT_MIRROR
```

Les tests MON11 lisent désormais directement `GetPlayerCharacterTurnState()`.

Le système souris n'est pas modifié et l'ancien binding clavier F/Use n'existe plus côté pawn.

Commits :

```text
a3abb959a45b12e4d304e1356d9ac88b16bb8452
Purge TD07.3.6 placement and rebuild legacy

210f093dfa72f48339a1d236bc98cd185251abc5
Purge TD07.3.6 combat and keyboard use legacy
```

## 11. MonsterSpawn facing repair préparé

`GetFacingForLegacyYaw` reste temporairement présent.

L'Automation one-shot charge tous les `UGridLevelAsset`, valide les MonsterSpawn après le `PostLoad` courant, puis resauvegarde les seuls LevelAssets qui contiennent au moins un MonsterSpawn. Cela matérialise le `InitialFacing` cardinal dans le package avant suppression du fallback.

```text
Grimrock.TechnicalDebt.TD07_3_6.AssetRepair.MonsterSpawnFacing
Scripts/RepairTD0736MonsterSpawnFacing.ps1
```

Commit de préparation :

```text
5f8fa484341338f1cdf100d8435b92d89525870c
Prepare TD07.3.6 MonsterSpawn facing repair
```

Après réparation LFS, la normalisation finale supprimera `GetFacingForLegacyYaw` et le fallback `InitialFacing <- LocalYaw`.
