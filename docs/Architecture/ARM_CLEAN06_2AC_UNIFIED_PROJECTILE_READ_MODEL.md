# ARM-CLEAN06.2AC — Lecture unique de la physique et des visuels de projectile

Le modèle canonique ARM-CLEAN06 doit offrir aux futurs lecteurs
du monde, des équipements et des projectiles un seul
`UGridItemSubsystem`, sans référencer directement les
`DA_Weapon_*` historiques.

## Vue résolue

`FGridResolvedItem` possède désormais deux champs
**transients** supplémentaires :

- `ProjectileMesh` : ressource visuelle de vol issue
  uniquement de `DA_WeaponVisualCatalogue` ;
- `ProjectileAppearance` : `Stable/Tumble/Spin`,
  rotation, échelle, axe avant du mesh, axes/vitesses de
  rotation, même autorité visuelle.

La physique existe déjà sous
`FGridResolvedItem::Weapon.Projectile` et provient
uniquement du `DA_WeaponCatalogue` (06.2W).
Aucune propriété de vol ou de présentation n'est copiée
dans la définition d'objet ARM à six champs.

## Cohérence obligatoire

Lorsqu'une arme ARM concrète est enregistrée,
`UGridItemSystemAsset::Validate` exige :

1. la même activation de projectile dans la physique et
   dans la présentation ;
2. un `ProjectileMesh` réellement référencé si les
   deux politiques sont activées.

`UGridItemSubsystem::ResolveItem` répète le contrôle au
moment de la lecture, afin de refuser tout registre modifié
après enregistrement. Le nouveau code d'erreur
`IncompleteProjectileVisual` est distinct d'une
identité de visuel manquante. La sortie est entièrement
remise à zéro en cas de refus.

Les 60 archétypes numériques sans objet concret n'exigent
pas d'apparence artificielle. Les armes non jetables,
boucliers/focus et consommables n'acquièrent pas de mesh
de projectile par le seul fait de consulter le résolveur.

## Automation

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.SingleProjectilePhysicsVisualResolver`
construit un registre canonique autonome de 82 ARM, configure
un projectile synthétique autorisé et prouve que sa vitesse,
son décalage d'impact, le mesh et le mode Spin sont
résolus ensemble. Le test supprime ensuite successivement
l'activation visuelle, le mesh et l'activation physique,
et vérifie à chaque fois le refus du registre et du
résolveur, sans conserver les valeurs de la réponse
réussie précédente.

Le rétablissement complet et la lecture d'un bouclier
sans projectile sont également testés. Le jeu historique
ne change pas : aucun acteur volant, WBP, action en combat,
fichier `.uasset`, niveau ni SaveGame n'est modifié.
