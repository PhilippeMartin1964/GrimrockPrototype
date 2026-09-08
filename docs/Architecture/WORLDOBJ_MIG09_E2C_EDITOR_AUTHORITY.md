# WORLDOBJ-MIG09-E2C — Autorité typée des helpers d’édition

Statut : **candidat à validation locale UE5.5.4**

Date : 2026-09-08

## Objectif

Cette tranche retire progressivement les accès directs au cache transitoire `UGridLevelAsset::Objects` et les dépendances aux pointeurs de DTO temporaires dans les helpers centraux du Grid Editor.

L’autorité persistante reste exclusivement :

```text
WorldObjectInstances
LooseItemInstances
MonsterSpawns
ItemSpawns
LogicObjects
```

`FGridLevelObjectData` reste temporairement un DTO de compatibilité en valeur jusqu’à la fin de MIG09-E2C. Il ne doit plus être utilisé comme stockage persistant ni comme cible d’écriture.

## 1. Blocs déjà validés

Les helpers de mutation centraux (`RemoveObjectsAtSelectionInternal()`, `RemoveObjectsConflictingWithPlacementInternal()`, `ApplyEditedSelectedObject()`), la sélection par cellule, le panneau Lua de bindings, EdMode, Overview, LinksPanel et les fixtures MIG06/MON19.2 concernés lisent désormais depuis l’autorité typée ou des snapshots DTO locaux construits en valeur.

Validations locales fournies le 8 septembre 2026 :

```text
Grimrock.WorldObjects
Succeeded              : 34
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

```text
Grimrock.MON19.2.Editor
Succeeded              : 8
Succeeded with warnings: 0
Failed                 : 0
Process exit code       : 0
```

Le build Development Editor UE5.5.4 est également passé.

## 2. Bloc courant — Inspector et fin de l’API pointeur

### Inspector

`SGridEditorObjectInspectorPanel` consomme désormais l’objet sélectionné uniquement via :

```cpp
bool TryGetSelectedObjectData(FGridLevelObjectData& OutObject) const;
```

Le panneau construit donc ses vues à partir d’un snapshot local en valeur. Le chemin PressurePlate relit également le comportement sélectionné par valeur avant mutation.

Le panneau ne conserve plus de pointeur vers un DTO temporaire de sélection.

### Suppression de GetSelectedObjectData()

L’ancienne API :

```cpp
const FGridLevelObjectData* GetSelectedObjectData() const;
```

est supprimée de `AGridLevelEditorActor`.

Le wrapper statique qui exposait un pointeur vers une vue temporaire est supprimé de `GridLevelEditorActor_EditingObjectsLinks_06.inl`.

La frontière de lecture sélectionnée devient donc explicitement une API **par valeur**.

## 3. Validation requise pour le bloc courant

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON19.2.Editor"
```

## 4. Reste de WORLDOBJ-MIG09-E2C

Après validation de ce bloc :

1. sortir `GridEditorLuaService` et `GridLevelEditorActor_Validation.inl` des derniers accès directs à `Objects` ;
2. migrer les fixtures/tests résiduels encore branchés sur `Objects` ou sur des helpers DTO legacy ;
3. supprimer physiquement `Objects`, `FGridLevelObjectData`, `GridLevelPlacementCompatibility` et les conversions/projections transitoires devenues inutiles.
