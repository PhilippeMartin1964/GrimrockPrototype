# WORLDOBJ-MIG09-C-C1 — Détachement runtime/tests des paramètres d’animation legacy

Statut : **candidat — validation locale UE5.5.4 requise**.

## Objectif

Cette sous-tranche prépare la suppression physique du schéma d’animation pré-MIG04 sans produire un gros diff mêlant runtime, Slate et structures de données.

Le contrat de présentation reste :

```text
WorldObject Definition
└── MovingParts[]
    └── Motion
        ├── Type
        ├── Axis
        ├── Pivot
        ├── Amount
        └── Duration
```

## Changements C-C1

- suppression du cache runtime `AGridDoorActor::OpenHeight` ;
- `AGridDoorActor` continue d’obtenir sa durée depuis `GetTargetMotionDuration()` ;
- les fixtures Door n’écrivent plus `Behavior.DoorAnimation.OpenHeight` ni `Behavior.DoorAnimation.MoveDuration` lorsqu’elles utilisent déjà `GridDoorTestUtils::InitializeDoorFromMotion()` ;
- les fixtures Pit construisent directement les pivots, angles et durées dans `MovingParts[].Motion` au lieu d’écrire `Behavior.PitAnimation` ;
- le test `Grimrock.WorldObjects.MIG04.RuntimeGenericMotionContract` exige désormais l’absence de `AGridDoorActor::OpenHeight` ;
- les états runtime nécessaires aux animations (`MoveDuration`, alpha courant, état d’animation, audio) restent inchangés.

## Ce qui n’est volontairement pas supprimé dans C-C1

Les structures C++ et contrôles Slate pré-MIG04 restent encore présents afin que cette tranche demeure petite et facilement validable :

```text
FGridPitAnimationParams
FGridButtonAnimationParams.ButtonPressDistance
FGridButtonAnimationParams.ButtonPressDuration
FGridButtonAnimationParams.ButtonReleaseDuration
FGridLeverAnimationParams
FGridPressurePlateAnimationParams
FGridDoorAnimationParams.OpenHeight
FGridDoorAnimationParams.MoveDuration
SGridEditorObjectInspectorPanel : anciens contrôles d’animation
```

Leur suppression physique appartient à **MIG09-C-C2**.

## Validation

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.WorldObjects"
```

Critère : build réussi, `0` Automation failure. Les warnings doivent être distingués des échecs.

## Suite

Après validation de C-C1 :

1. retirer les structures/champs d’animation legacy de `GridObjectBehavior.h` ;
2. supprimer les contrôles correspondants du Grid Editor ;
3. simplifier `GridObjectArchetypeAsset.cpp` pour ne plus migrer/valider ces anciens champs ;
4. mettre à jour les documents architecturaux consolidés ;
5. valider à nouveau `Grimrock.WorldObjects` avant MIG09-D.
