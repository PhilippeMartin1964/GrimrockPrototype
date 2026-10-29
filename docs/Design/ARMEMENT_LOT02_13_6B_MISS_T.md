# ARMEMENT-LOT02.13.6B — Déviation latérale du tir manqué (« T »)

Statut : implémenté, **validation UE5 locale / PIE à effectuer**.

## Convention du T (choix de conception)
Le code antérieur ne spécifiait pas de règle mathématique du T. Le présent
jalon formalise un T *local au tir* :
- Le pied du T est la direction horizontale du tireur vers la cible.
- La traverse est l'axe droite/gauche perpendiculaire au tir :
  `Right = Cross(Forward, WorldUp)`.
- Sur un résultat combat `Miss`, une seule branche gauche ou droite est
  choisie de manière reproductible d'après le GUID de la munition.
- Le point de passage visé est au plan de la cible, hors de son volume :
  demi-largeur projetée du `UBoxComponent` + rayon de collision du
  projectile + marge fixe de 12 cm.
- Seule la vitesse est réorientée, **aucune téléportation**, aucun reroll.
  Les collisions de murs, portes et sol restent autoritaires.
- Une fois le plan de la cible dépassé de 25 cm, le projectile reprend une
  gravité normale. L'expiration et les collisions produisent un pickup selon
  le mécanisme existant. Le prochain jalon 13.6C traitera l'orientation
  réelle du pickup après impact et la libération physique des flèches fichées.

## Frontières
`FGridProjectileGeometry::CalculateMissDeflection` est sans état, réutilisable
dans les directions N/E/S/O. Seules les munitions avec
`bRangedAmmunitionProjectile` reçoivent cette déviation. Les armes de jet
(shurikens), les projectiles de monstres et de sorts ne sont pas concernés.
Sur `Hit`, le fichage introduit en 13.6A reste inchangé.
Ni les dégâts, ni l'initiative, ni l'inventaire, ni les PA ne sont calculés
dans le projectile.

## Contrôles
- Test `Grimrock.RPG.ARMEMENT.LOT02.MissTDeflectionAndGravity` :
  géométrie 4 directions x 2 côtés, dégagement du monstre, normalisation,
  vitesse et position inchangées à la résolution, gravité après passage,
  non-application aux projectiles ordinaires.
- `Grimrock.RPG.ARMEMENT.LOT02` : attendu 14 tests (13 précédents + 1).
- `Grimrock.Monsters.MON11.Presentation` : attendu 8 tests.
- PIE : forcer plusieurs tirs manqués avec monstre à 1 et 4 cases ; vérifier
  départ gauche/droite, pas de flèche fichée, choc contre géométrie ou chute
  après passage, un pickup par flèche. Conserver une cible de face quand
  le groupe tourne au nord, est, sud, ouest.

Diagnostics : `[GridProjectile] MissDeflect` et
`[GridProjectile] MissGravity`.

**Limite volontaire :** sans collision/mesh Chaos appropriés, un pickup
peut encore présenter une orientation de repos incorrecte. La rectification
des transformations physiques et la persistance sont prévues pour 13.6C.
