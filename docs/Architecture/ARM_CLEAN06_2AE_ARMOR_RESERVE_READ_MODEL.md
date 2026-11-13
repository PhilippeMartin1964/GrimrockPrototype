# ARM-CLEAN06.2AE — Réserves d'armure restantes sans recharge implicite

## Autorité des données

Le `DA_WeaponCatalogue` canonique fournit, pour chaque ARM
et niveau N1–N20, `PhysicalArmor` et `MagicalArmor`
**nominaux**. L'instance persistée détient exclusivement
les compteurs mutables `AbsorbedEquipmentPhysicalArmor`
et `AbsorbedEquipmentMagicalArmor`. Aucun rechargement
ne doit se produire lors d'un changement de main, d'un
transfert sac/curseur ou d'un Save/Continue.

Le nouveau calculateur pur
`FGridCanonicalArmorReserveResolver::TryCompute`
renvoie une `FGridCanonicalArmorReserveView` transitoire,
sans modifier les arguments, les DataAssets ni le SaveGame.

Pour chaque type de protection :

- `Nominal` = protection du catalogue pour le niveau réel ;
- `Spent` = points absorbés par **cette instance** ;
- `Remaining` = `max(0, Nominal - Spent)`.

Un `Spent` supérieur au nominal est **conservé intégralement** :
l'état historique n'est pas réécrit ni rechargé à zéro.
Seul `Remaining` est ramené à zéro pour la lecture.
Une protection nominale ou une dette négative provoque
un refus explicite et un résultat réinitialisé.

## Preuves natives

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionInstanceArmorReserveParity`
charge les 22 ARM du manifeste, lit leurs profils N1 et N20
dans le vrai catalogue source et compare 44 calculs de
protection nominale/restante ; les trois types passifs
sont inclus et les packages source restent propres.

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.NoFreeArmorRefillDuringCanonicalResolution`
vérifie un bouclier de 5 points qui a consommé 3,
son épuisement à 5, la conservation de toute dette
supérieure au nominal, ainsi que le refus des
valeurs négatives.

Les règles déjà jouables de dépense d'armure
(`ApplyPhysicalArmorDamage`), recharge explicite par
repos/victoire et persistance des GUID ne changent pas.
Le présent jalon implémente **uniquement une vue de
lecture canonique**, pas une deuxième autorité de dégâts
ni une conversion de sauvegarde.

Aucun `.uasset`, carte, WBP, objet en jeu ou SaveGame
n'est modifié. Une migration UE réelle exige encore
le raccordement des instances et des lecteurs du runtime,
avec validation PIE spécifique aux boucliers.
