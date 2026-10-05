# RPG03.9.0 — Weapon-based Talent Attacks

Date : **5 octobre 2026**  
Parent : **RPG03.8 validé TD04.2 — 8/8**  
Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

Fermer le contrat générique nécessaire à l'authoring RPG03.9 pour les Talents exprimés en pourcentage de `WD`.

RPG02 définit `WD` comme le jet Min/Max de l'arme actuellement utilisée, augmenté du bonus plat et du modificateur d'attribut, puis multiplié par le coefficient du Talent avec arrondi inférieur. Le runtime existant ne pouvait auparavant qu'utiliser un `OffensiveProfile` entièrement authoré sur l'action de classe.

## Contrat

`FGridCombatWeaponAttackProfile` permet à une action `Ability|Spell + Attack` de :

- résoudre l'arme offensive équipée au moment de l'action ;
- filtrer cette arme par `ItemTags` sans branche sur un `TalentId` ;
- reprendre Min/Max, bonus plat et attribut de scaling de l'arme ;
- appliquer un `WeaponDamagePercent` indépendant du multiplicateur C2 ;
- remplacer optionnellement le DamageType/PhysicalSubtype de l'attaque ;
- utiliser l'identité et la portée de l'action de Talent ;
- autoriser explicitement, si nécessaire, le profil unarmed.

`OffensiveProfile` et `WeaponAttackProfile.bUseEquippedWeapon` sont mutuellement exclusifs pour une attaque : il n'existe donc pas deux autorités de dégâts de base.

## Résolution WD

Le coefficient est appliqué dans `FGridCombatResolver` après :

1. jet Min/Max ;
2. bonus plat ;
3. modificateur d'attribut ;

et avant :

4. multiplicateur de critique ;
5. modificateurs C2/résistances ;
6. armure/HP.

Le calcul entier est un floor. Si le WD avant coefficient est positif et le coefficient non nul, le minimum est 1.

## Ciblage

La même projection d'arme est utilisée par :

- `FirstAxialTarget` ;
- `Cell` ;
- `Area`.

Une action de zone résout l'arme une seule fois puis réutilise cette projection pour toutes ses cibles. Les coûts et la consommation restent ceux de l'action de classe ; l'arme n'est jamais consommée par ce mécanisme.

## Modificateurs

`MakeResolvedActionAttackContext(...)` combine :

- identité/tags de l'action ;
- tags de l'item offensif sélectionné ;
- DamageType/PhysicalSubtype effectivement résolus.

Les passifs C2 peuvent donc filtrer une attaque WD par tags d'arme ou par subtype sans connaître le `TalentId`.

## Catalogue

Le catalogue reçoit uniquement les ensembles de tags des mains pouvant réellement fournir une attaque. Une action WD dont les `RequiredItemTags` ne correspondent à aucune arme équipée reste visible mais désactivée avec :

`RequiredOffensiveEquipmentUnavailable`.

## Assets

RPG03.9.0 ne modifie aucun `.uasset`, `.umap` ou WBP.

L'authoring des classes commence après validation de ce contrat, avec **RPG03.9.1 — Guerrier**.

## Automation

Filtre :

`Grimrock.RPG.RPG03.9.0`

Tests :

1. `ProfileValidation`
2. `ItemTagMatching`
3. `ActionValidation`
4. `DescriptorProjection`
5. `RawDamageFloor`
6. `RawDamageMinimum`
7. `CriticalOrdering`
8. `ResolvedModifierContext`
9. `CatalogAvailability`

Ces tests ne sont pas déclarés passants tant que la sortie TD04.2 utilisateur n'a pas été fournie.
