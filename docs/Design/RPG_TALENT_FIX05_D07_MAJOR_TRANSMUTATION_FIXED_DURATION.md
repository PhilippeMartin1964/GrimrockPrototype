# RPG-TALENT-FIX05 — D07 Transmutation majeure / durée finale fixe

Date : **8 octobre 2026**  
État : **SOURCE PRÊTE — validation locale requise**

## Contrat

RPG02 impose :

```text
toute cellule convertie par Transmutation majeure
-> durée finale exactement 4 rounds
```

Avant D07, `EmptyCellDurationRounds = 4` ne concernait que les cellules sans
surface. Une surface préexistante conservait sa propre durée, puis recevait le
modificateur de durée de la source.

## Correctif générique

`FGridCombatSurfaceConversionProfile` gagne :

```text
bUseFixedFinalDuration
FixedFinalDurationRounds
```

Comportement par défaut inchangé :

```text
surface existante -> Existing.RemainingRounds + SurfaceDurationRoundsModifier
cellule vide       -> EmptyCellDurationRounds + SurfaceDurationRoundsModifier
```

Mode fixe :

```text
bUseFixedFinalDuration = true
-> RemainingRounds = FixedFinalDurationRounds
-> aucun héritage de durée
-> aucun SurfaceDurationRoundsModifier
```

Cette sémantique est volontaire : le champ exprime une **durée finale**, pas une
durée de base.

## Transmutation majeure

`BuildMajorTransmutationRecipeAction()` authorise maintenant :

```text
EmptyCellDurationRounds = 4
bUseFixedFinalDuration  = true
FixedFinalDurationRounds = 4
```

Les quatre sorties Fire / Ice / Poison / Oil utilisent le même contrat.

Aucun switch sur TalentId n'est introduit dans le resolver.

## Non-régression

Les conversions du Mage n'activent pas le nouveau booléen. Leur sémantique
historique reste donc inchangée, notamment Surface persistante :

```text
durée existante + SurfaceDurationRoundsModifier
```

## D08 non traité

Ce ticket ne donne aucune nouvelle sémantique au bonus :

```text
Transmutation majeure -> dégâts de réaction +50 %
```

D08 reste séparé.

## Validation attendue

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6A"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.RPG.RPG03.9.4F1"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6B2"
```

Aucun DataAsset n'est modifié.
