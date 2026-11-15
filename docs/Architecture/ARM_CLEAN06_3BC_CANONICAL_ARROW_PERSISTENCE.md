# ARM-CLEAN06.3BC — Restauration Save/Continue des flèches canoniques

**Suite de BB** (SHA `69f122f`) ; jalon C++ et tests, sans édition binaire UE.

## Diagnostic

Le projectile `AGridThrownItemActor` pouvait déjà voler avec la vraie définition
`Item_Arrows_01` du registre, sans vieux `UGridItemDefinitionAsset`.
Mais `ApplyCurrentLevelRuntimeState` exigeait `IsValid(Definition)`
pour **réattacher** une flèche fichée sur un monstre vivant.
La capture persistait aussi un `ProjectileDefinitionAsset` ancien
même lorsque le projectile appartenait au registre.

## Implémentation

- **Capture** : les projectiles `bRangedAmmunitionProjectile` issus du
  registre conservent `ItemId`, `ItemLevel=0`, `RuntimeObjectId`,
  cellule et pose du mesh, mais pas l'ancien `ProjectileDefinitionAsset`.
  La propriété SaveGame demeure dans le schéma tant que 06.4–06.5
  n'ont pas converti et vérifié les sauvegardes existantes.
- **Chargement des flèches en vol** : un seul pickup réel est créé à la
  pose capturée, à partir de l'identité canonique, en ignorant un ancien
  pointeur éventuellement stocké dans une sauvegarde précédente.
  Toute identité enregistrée mais à niveau invalide échoue fermement.
- **Chargement des flèches fichées** : l'unique `AmmoKindId` N0 et
  la pile valide sont contrôlés par le registre. Si le monstre persistant
  existe et est vivant, l'acteur projectile peut rétablir son attachement
  sans ancienne définition. Sinon le runtime tente le dépôt physique
  déjà existant à la pose sauvegardée, sous le même GUID.
- **Dépôt monde** : une flèche canonique ignore un ancien objet d'item
  explicitement passé par un appelant. Les autres objets restent sur leurs
  chemins inchangés durant la migration.

Pas de nouveau projectile, d'ID, de vitesse de combat ou de table de
métadonnées. Les lancers ordinaires dépourvus de `ProjectilePolicy`
restent hors périmètre : aucune vitesse arbitraire n'est ajoutée.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalAmmoPersistence` vérifie avec
`DA_ItemSystem` réel : (1) capture d'une flèche volante avec un GUID
unique, `ItemLevel=0`, sans pointeur historique ; (2) rechargement comme
pickup physique et conservation de la pose ; (3) dépôt avec ancien
asset contradictoire ignoré ; (4) sauvegarde d'une flèche fichée,
restauration et attachement au monstre vivant ou dépôt de sécurité.

La validation Windows compile et lance l'ensemble de `Grimrock`,
mais `-NullRHI` ne prouve pas la restitution GPU/Chaos avec
mouvements du monstre et vraie sauvegarde inter-processus.

**Localisation :** `UI.csv → ST_UI.uasset → WBP → Game/fr/en`
inchangée ; aucun texte visible/UMG ne change.

**Toujours ouvert :** conversion Editor 06.4, suppression legacy 06.5,
PIE/Save/Continue/Cook 06.6.
