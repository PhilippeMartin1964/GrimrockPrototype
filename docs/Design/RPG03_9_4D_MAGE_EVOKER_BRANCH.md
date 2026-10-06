# RPG03.9.4D — branche Mage Évocateur complète

Statut : **IMPLÉMENTÉ — MATERIALISATION/VALIDATION UE UTILISATEUR REQUISE**

## Objectif

Ce jalon regroupe volontairement la fin de la branche Évocateur en une livraison fonctionnelle :

- Affinité élémentaire — conservée depuis C ;
- Surcharge élémentaire — conservée depuis C ;
- Explosion contrôlée ;
- Chaîne élémentaire ;
- Cataclysme.

Il n'introduit aucune branche `if Mage` dans le runtime. Les nouvelles capacités sont des primitives de combat génériques et data-driven.

## Explosion contrôlée

`Talent_Mage_Evoker_ControlledExplosion` projette un modifier uniquement sur les actions :

- `SourcePolicy=Spell` ;
- `TargetingPolicy=Area`.

Effets sur le dégât direct :

- alliés : -50 % ;
- lanceur : -100 % additionnels, donc zéro dégât direct.

Les surfaces et applications de statuts restent sur leurs pipelines normaux et ne sont pas réduites ou supprimées.

## Chaîne élémentaire

`Action_Mage_ElementalChain` :

- 3 PA ;
- 8 mana ;
- Cell R5 ;
- CD3 ;
- cible primaire puis au maximum deux autres hostiles ;
- chaque saut doit être à distance Manhattan <= 1 de la cible précédente ;
- une cible ne peut être sélectionnée qu'une fois ;
- choix déterministe des candidats : distance, puis Y, puis X ;
- dégâts : `7 + INT mod + Skill_Arcana Rank`.

Le sort utilise un seul ActionId. L'affinité sélectionnée projette le School tag et le DamageType au catalogue.

## Cataclysme

`Action_Mage_Cataclysm` :

- 4 PA ;
- 16 mana ;
- Cell R5, Area2 ;
- CD5 ;
- dégâts : `12 + INT mod + Skill_Arcana Rank`.

Après dégâts, si l'armure magique est épuisée :

- Fire -> `Status_Burning`, durée 2 selon l'unité canonique du statut (Turns) ;
- Frost -> `Status_Slow`, 2 rounds, Initiative -6 ;
- Air -> `Status_Stunned`, 1 Turn ;
- Earth -> `Status_Immobilized`, 1 round.

### Décision Earth

RPG02 ne définit aucun `EGridDamageType::Earth` et le Choice d'affinité Earth ne contient aucun sous-choix indiquant Physical versus Poison.

D utilise donc une interprétation déterministe et sans nouvelle autorité :

- `Spell.School.Earth` reste le School sémantique ;
- DamageType = `Physical` ;
- contrôle Cataclysme = `Status_Immobilized`.

Une future mécanique de sort Earth explicitement Poison pourra employer le même School avec un autre DamageType sans modifier l'identité de l'affinité.

## Primitives génériques ajoutées

D étend le modèle commun avec :

1. filtre de `FGridCombatModifierProfile` par `TargetingPolicies` ;
2. `SelfDirectDamagePercentModifier` pour distinguer le lanceur des autres alliés dans une AoE ;
3. `FGridCombatDirectDamageScalingProfile` pour le scaling générique par rang de compétence ;
4. `FGridCombatActionOwnerVariantProfile` pour projeter une variante d'action depuis les Requirements du propriétaire ;
5. `ChainJumpRangeCells` pour les topologies de chaîne déterministes ;
6. options d'attaque `bAlwaysHits` et `bCanCriticalHit` afin d'exprimer les formules de dégâts directs déterministes.

Aucune de ces primitives ne contient un identifiant Mage/Talent spécifique.

## Statuts

Le commandlet Mage crée ou met à jour :

- `DA_Status_ElementalOverload` ;
- `DA_Status_Burning` ;
- `DA_Status_Slow`.

Il exige et réutilise, sans les réécrire :

- `DA_Status_Stunned` ;
- `DA_Status_Immobilized`.

## Materialisation locale

Depuis un working tree propre sur `master` :

```powershell
.\Scripts\AuthorRPGMage.ps1 -EngineRoot D:\UE_5.5
```

Le script compile l'Editor, lance le commandlet `RPGMageAuthoring`, exécute la campagne
`Grimrock.RPG.RPG03.9.4D` puis affiche les changements binaires.

La validation UE n'est acquise qu'après réception de la sortie TD04.2 de l'utilisateur.
