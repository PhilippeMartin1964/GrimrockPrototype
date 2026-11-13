# ARM-CLEAN06.2AJ — Liaisons de présentation explicitement authorées dans le manifeste

## But

Réduire les attaques sans `PresentationProfileId` sans recréer les
deux couches de proposition 06.2Z/06.2AA, ni inventer un profil audio/VFX.

Le **seul** manifeste de production
`Config/Armement/WeaponProductionManifest.csv` comporte désormais une
colonne `MigrationPresentationId` (17e champ).
Le choix est **éditorial et explicite** : `PRES_ARMxxx` pour
chacune des **19 attaques existantes** ; `None` pour les
trois boucliers/focaliseurs passifs. Les identifiants sont de
nouvelles clés de migration, **pas des IDs historiquement présents**
dans les vieux DataAssets et ne constituent pas des données runtime.

Le lecteur vérifie unicité, présence et séparation des IDs. La
projection continue de vérifier d'abord que l'action source est
intégralement représentable (`TryProjectGeneratedAttack`).
Elle retient le `PresentationProfileId` source **s'il existe** ;
seulement lorsqu'il manque, elle utilise la clé éditée du manifeste.
Elle copie exclusivement le véritable
`PlayerAttackPresentationProfile` de l'arme via
`GridArmClean06PresentationProjection::TryProject`. Un profil
absent/invalide, une collision ou une action non standard restent
bloquants. Aucun fallback ou synthèse de FX n'est introduit.

Les tests 06.2Q vérifient les 19 `AttackId`, la priorité de
l'ID source, l'absence de présentation pour un profil non
représentable et le maintien de trois passifs sans action.
Les validations 06.2U et 06.2Y conservent les garde-fous
de compatibilité ; la migration des assets et lecteurs 06.3–06.5
n'est PAS autorisée sans leurs preuves séparées.

**Portée :** manifest officiel seulement, aucun asset hors manifeste,
aucun `.uasset`, `.umap`, SaveGame ou mécanisme de combat modifié.
Le champ éditorial pourra être retiré avec le pipeline de migration
lors de 06.5 ; la seule autorité finale sera le catalogue canonique.
