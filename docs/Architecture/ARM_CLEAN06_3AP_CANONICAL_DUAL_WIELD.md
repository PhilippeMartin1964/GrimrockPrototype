# ARM-CLEAN06.3AP — Dual-wield canonique N1–N20

Parent : ARM-CLEAN06.3AO (`ce8fb71`), deux statuts Windows verts.

## Contrat et défaut corrigé

La génération des attaques individuelles utilise `DA_ItemSystem`, tandis
que le dual-wield continuait à lire `DA_Weapon_*.HandUsage`, les
`CombatActions` N1, l'ancienne définition de main secondaire et ses tags.
Ce décalage pouvait refuser deux armes valides ou utiliser des données
historiques pour les frappes.

Un lecteur partagé `FGridCanonicalEquippedAttackReadModel` matérialise
dorénavant le transport transitoire d'une action équipée depuis
`ResolveItem` et `ResolveAttackFromEquippedInstance`. L'action
individuelle du TurnManager et la politique du dual-wield l'utilisent.

Pour chaque `ItemId` enregistré : la validation vérifie l'identité
physique, les mains réellement autorisées, le niveau N1–N20 fixe et
l'action liée (sans secours par catalogue historique). La frappe double
exige deux armes de mêlée à une main avec deux GUID distincts, une portée
de 1, sans coût de mana/quantité ni cooldown ; pas de bouclier ou de
ranged attack ajouté artificiellement. Chaque main a ses propres dés,
son AttackId et ses tags canoniques. L'icône du composite relève de
`WeaponVisualCatalogue.ActionIcons`, pas de l'ItemIcon.

Le coût unique reste **3 PA**, avec deux résolutions et la main gauche
à **50 %** des dégâts bruts ; une revalidation des GUID **et niveaux**
empêche de frapper avec une autre source entre les deux événements.
Pour les ItemId absents du registre, le chemin historique reste
temporairement disponible jusqu'à la conversion 06.4 et la suppression 06.5.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalDualWieldIndependentLevels`
charge le registre réel et exerce ARM001 N3 / ARM013 N19 sans aucun ancien
`UGridItemDefinitionAsset` : dés/AttackId/tags indépendants, rejet
N0 persisté, passif ARM075, arme à deux mains ARM031, même GUID ou
propriété de slot incorrecte. Le lecteur partagé prouve la parité de
l'attaque principale avec le catalogue standard.

Régressions : `Grimrock.RPG.ARMEMENT.LOT02.19.2.*` et suite `Grimrock`.
Aucune modification d'assets Unreal binaires, UMG, sauvegarde ou libellé
visible : `UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
La recette GPU/PIE reste distincte de `UE5.5.4/LocalAuto`.
