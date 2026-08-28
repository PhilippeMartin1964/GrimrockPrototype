# TD07.3.5.5 — Regressions / Shipping / Closure

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Parent : TD07.3.5 — Combat Data Schema Reset
Statut : REGRESSION FIXTURES NORMALISÉES — À VALIDER

## 1. Objectif

Fermer TD07.3.5 avec le schéma combat courant uniquement :

- items : `CombatActions` autoritaire ;
- monster presentation : définitions Audio/VFX courantes uniquement ;
- monster range : `MinRangeCells + MaxRangeCells` ;
- aucun adapter ou champ legacy conservé.

## 2. Régressions historiques découvertes

Un filtre trop large `Grimrock.Monsters.MON1` a exécuté aussi MON11/MON12 et révélé six tests historiques rouges.

Ils n'indiquaient pas un défaut du runtime courant. Les fixtures exprimaient encore des contrats antérieurs :

- validation offensive déduite du type de slot au niveau de l'item ;
- indisponibilité d'une action équipement simulée via `PlayerAttackActionPointCost` ;
- portrait injecté directement dans le cache transient ;
- Save memory round-trip avec `ClassId=Warrior` synthétique non enregistré dans le resolver.

## 3. Normalisation des fixtures

Les tests sont réalignés sur les autorités TD07.3 actuelles :

- `CanProvideAttackFromSlot()` vérifie l'autorité main/off-hand ;
- les tests de coût modifient le `ActionPointCost` du `FGridCombatActionDefinition` enregistré ;
- les portraits synthétiques utilisent `RaceId + PortraitGender + PortraitVariantId` et `FRPGAuthoringIdentityResolver` ;
- le round-trip Save v22 enregistre une classe synthétique résoluble avant désérialisation ;
- les mentions "legacy shuriken adapter" sont remplacées par le contrat `CombatActions` courant.

Aucun changement runtime/gameplay n'est introduit par cette sous-étape.

## 4. Validation suivante

Valider d'abord les six tests précédemment rouges, puis les régressions CombatActions/monster ciblées. Si elles sont vertes, exécuter le packaging Win64 Shipping avant clôture de TD07.3.5.
