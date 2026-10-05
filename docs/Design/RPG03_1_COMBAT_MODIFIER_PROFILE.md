# RPG03.1 — Combat Modifier Profile (C2)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

Introduire une primitive C2 générique pour modifier combat et catalogue sans coder les Talents individuellement.

## Contrat

`FGridCombatModifierProfile` porte des filtres optionnels :

- `ActionIds` ;
- `SourceDefinitionIds` ;
- `SourcePolicies` ;
- `ActionTypes` ;
- `DamageTypes` ;
- `PhysicalSubtypes`.

Un tableau vide signifie « tout accepter » pour ce filtre.

Modificateurs :

- Accuracy ;
- Evasion ;
- dégâts sortants en pourcentage ;
- dégâts entrants en pourcentage ;
- chance de critique en points de pourcentage ;
- multiplicateur critique en points de pourcentage ;
- résistances par `EGridDamageType` ;
- coût PA ;
- coût mana ;
- portée.

Les profils sont stockés dans les `FRPGClassProgressionChoiceDefinition`. Les `SelectedClassProgressionChoiceIds` restent la seule autorité durable : les profils actifs sont reconstruits depuis la classe et les ChoiceIds.

## Critiques

Le système historique « 20 naturel = critique ×2 » devient paramétrable sans seconde RNG :

- chance par défaut : 5 % ;
- dégâts critiques par défaut : 200 % ;
- la chance est projetée sur le même d20 d'attaque, par bandes de 5 % ;
- 15 % => critique sur 18–20 ;
- un 1 naturel reste toujours un échec ;
- un 20 naturel reste toujours une réussite et, avec la valeur par défaut, un critique.

Cela préserve la déterminisme des tests explicites existants.

## Projection

Le resolver ne modifie jamais les DataAssets authorés.

`FGridCombatActionCatalog::Build()` copie la contribution puis applique sur cette copie :

- PA ;
- mana ;
- portée.

Les entrées d'attaque reçoivent ensuite :

- précision ;
- multiplicateur de dégâts sortants ;
- critique.

Les cibles personnages reçoivent :

- Evasion ;
- multiplicateur de dégâts entrants ;
- résistances.

## Portée actuelle

RPG03.1 raccorde les profils de **choix de classe**. Le pont StatusEffect→CombatModifier sera traité avec RPG03.2/C1 afin que les buffs/debuffs data-driven puissent réutiliser exactement le même profil sans logique par EffectId.

Les Spell Effects MON18 n'exposent pas encore leur DamageType/School dans `FGridCombatActionDefinition`. Les modificateurs de coûts/portée des sorts fonctionnent au niveau catalogue ; les multiplicateurs de dégâts de sorts Effect seront raccordés lorsque le bridge Magic consommera ce même profil. Aucun système parallèle n'est créé pour contourner cette limite.

## Automation ajoutée

Namespace :

`Grimrock.RPG.RPG03.1`

Tests :

- ProfileValidation
- ChoiceProjection
- ActionProjection
- FilterIsolation
- AttackModifiers
- CriticalResolution
- IncomingModifiers

Ces tests sont ajoutés au code mais ne doivent être déclarés Success qu'après exécution du harness UE5.5.4 et fourniture du log utilisateur.
