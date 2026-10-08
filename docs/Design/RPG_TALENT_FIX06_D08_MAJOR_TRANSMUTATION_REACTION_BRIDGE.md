# RPG-TALENT-FIX06 — D08 Transmutation majeure / réaction avant conversion

Date : **8 octobre 2026**  
État : **VALIDÉ / CLOS — 8 octobre 2026**

## Décision

Conserver le contrat RPG02 :

```text
Les dégâts de réaction déclenchés pendant cette conversion : +50 %
```

et choisir l'option générique **conversion -> réaction**, plutôt que supprimer le
bonus authoré.

## Nouvelle primitive

`FGridCombatSurfaceConversionProfile` possède :

```text
bTriggerCanonicalReactionBeforeConversion
```

Valeur par défaut : `false`.

Les conversions existantes, notamment celles du Mage, ne changent donc pas.

## Résolution

Lorsque l'option est active :

```text
1. lire la surface existante ;
2. mapper la sortie de conversion vers une interaction canonique si possible :
   Fire -> Fire
   Ice  -> Ice
3. résoudre/émettre SurfaceReaction avec le même ActionInstanceId ;
4. appliquer les modificateurs de réaction et de l'action ;
5. effectuer ensuite la conversion normale ;
6. la sortie de conversion reste l'état final.
```

Aucune interaction Poison/Huile n'est inventée.

## Transmutation majeure

Les recettes :

```text
Fire -> opt-in réaction
Ice  -> opt-in réaction
Poison -> conversion pure
Oil    -> conversion pure
```

Le Choice possède déjà :

```text
ActionId = Action_Alchemist_MajorTransmutation
SurfaceReactionDamagePercentModifier = +50 %
```

Ce modificateur atteint désormais le
`FGridCombatSurfaceReactionResult::ExplosionDamagePercentModifier`.

Exemple :

```text
Poison + recette Fire
-> réaction Poison + Fire, explosive
-> ExplosionDamagePercentModifier = +50
-> conversion finale Fire
-> durée finale 4 rounds
```

## Limite globale C6

Le moteur ne définit toujours pas de magnitude de base universelle pour une
explosion de surface. Ce ticket n'invente donc aucun dégât fixe.

Le raccord du +50 % est réel au niveau du résultat de réaction ; l'exécution
d'une future magnitude de base pourra le consommer sans changer ce contrat.

Cette limite est globale aux réactions explosives et n'est plus une divergence
spécifique de Transmutation majeure.

## Validation attendue

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6A"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6B2"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.RPG.RPG03.9.4F1"
```

Aucun DataAsset n'est modifié.


## Validation finale

Validation utilisateur reçue le 8 octobre 2026 :

```text
Grimrock.RPG.RPG03.9.6A
Succeeded              : 11
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0

Grimrock.RPG.RPG03.9.6B2
Succeeded              : 7
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0

Grimrock.RPG.RPG03.9.4F1
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

D08 est clos.

La magnitude de base universelle d'une explosion de surface reste une limite
générale de C6 ; elle n'est plus une divergence spécifique de Transmutation
majeure.
