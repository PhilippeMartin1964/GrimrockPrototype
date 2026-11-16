# ARM-CLEAN06.4X.9.2 — Acteurs canoniques des contenus initiaux de réceptacles

## Périmètre et parent

Parent : `ARM-CLEAN06.4X.9.1` (`d345268e8f2bc23c7ee1aaf9992a2071c317eed8`),
dont les contrôles `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` ont réussi.

Les quatre niveaux comportent **18 contenus initiaux** : leurs `CanonicalItemId`
et niveaux physiques ont été matérialisés en 06.4W / X.5, puis lus strictement en X.9.1.
Ce jalon ne concerne que l'**acteur de présentation des objets non-ARM
initialement contenus dans un réceptacle**.

## Règles

- `FGridInitialReceptacleItem::bHasPersistedCanonicalIdentity` est un témoin
  **transient**, produit exclusivement par `FGridReceptacleInitialCanonicalReadModel`
  lorsque la source porte réellement un `CanonicalItemId`. Aucune nouvelle
  donnée n'est écrite dans un `.uasset`.
- Pour ce seul parcours, `AddContainedItem` vérifie le niveau physique exact
  dans `UGridItemSubsystem`, puis initialise l'acteur avec
  `AGridItemActor::InitializeFromCanonicalItem`.
- L'identité, la quantité, le GUID et le niveau sont transmis sans substitution.
  Mesh, transform visuel, Niagara, sparkle, masse et lumière proviennent de
  la projection canonique ; l'état initial de la lumière correspond au
  `bDefaultEnabled` enregistré.
- En cas d'absence de registre, de niveau erroné ou d'échec d'initialisation,
  la création est refusée : **aucun repli physique sur `DA_Item_*`**.
- Les items ARM des réceptacles gardent provisoirement leur ancien montage
  jusqu'au jalon dédié. Les fixtures synthétiques sans `CanonicalItemId`
  persistent dans leur parcours historique.
- Les pointeurs legacy transitoires des entrées logiques restent présents
  pour les consommateurs de sauvegarde non encore migrés ; ils ne
  gouvernent plus l'acteur de présentation non-ARM.
- Aucune règle d'acceptation, capacité, socket, placement `PhysicalAtHit`,
  position, simulation Chaos, signal logique ou sauvegarde n'est modifiée.

## Preuves et limites

Tests `Grimrock.RPG.ARM.CLEAN06_4X92.RealInitialCanonicalReceptaclePresentation`
et `Grimrock.RPG.ARM.CLEAN06_4X91.RealInitialReceptacleIdentities` :
sources des quatre niveaux, 18 identités et niveaux, acteur physique non-ARM,
lumière/Niagara/GUID/quantité, refus de niveau invalide, protection des fixtures
historiques et maintien du parcours ARM.

La publication nécessite compilation et suite `Grimrock` UE5.5.4 réussies
**sur le SHA final** (contrôles `SilentTask` et `LocalAuto`). Un test
`-NullRHI` ne démontre ni rendu PIE, ni Chaos dynamique, ni Niagara visible,
ni Save/Continue, ni Cook/Shipping. Aucun `.sav`, `.uasset`, `.umap`,
WBP ni Blueprint ne doit être modifié.

Suite : spawns (zéro occurrence réelle), palette (33 entrées), acceptation des
réceptacles, objets ARM, éditeur, puis SaveGame sur copies vérifiées avant
retrait du legacy.
