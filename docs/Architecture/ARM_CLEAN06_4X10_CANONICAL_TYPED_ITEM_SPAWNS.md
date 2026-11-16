# ARM-CLEAN06.4X.10 — Générateurs typés avec identité physique canonique

Le jalon 06.4X.9.2 (SHA `3a6832d918fb20cb6bd061b6cd8848db612880b5`)
est validé par `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`.

## Inventaire et règles

Les quatre niveaux réels ne contiennent **aucune instance `ItemSpawns`**.
On ne rejoue donc ni le convertisseur 06.4W, ni les écritures binaires 06.4X.5.

Le modèle `FGridCanonicalItemSpawnReadModel::TryMaterialize` transforme
un générateur typé en `FGridLooseItemInstance` initial, sans écrire
l'asset. Si `CanonicalItemId` est persisté, le seul `UGridItemSubsystem`
valide l'identité, le niveau physique, la quantité et la compatibilité de
la source historique éventuelle. Il refuse une famille inconnue, un
ARM N0, un objet N0 en N1, une quantité nulle et un pointeur contradictoire.
Ni niveau N1 inventé, ni fallback `DA_Item_*`.

Le `SpawnId` reste le GUID stable de la première instance. Les règles
de `bSpawnAtStart`, de case valide, de non-duplication et de suivi
`ObjectPresence` après collecte/restauration sont préservées. Le chemin
`AddPlacedItemActor` déjà migré (06.4X.8) monte les non-ARM canoniques,
tandis que les ARM gardent leur présentation historique jusqu'au cutover ARM.
Les fixtures synthétiques sans `CanonicalItemId` restent compatibles,
sans devenir une autorité alternative pour les identités persistées.

## Tests et interdits

`Grimrock.RPG.ARM.CLEAN06_4X10.CanonicalItemSpawnMaterialization`
vérifie le compte réel nul, le GUID, la quantité, les niveaux N0/N20,
la création d'un acteur canonique non-ARM et le refus des états invalides.
Compilation UE5.5.4 et suite `Grimrock` requises sur le SHA final :
`SilentTask` et `LocalAuto` doivent être SUCCESS. Ces tests ne
certifient pas PIE, Niagara visible, Chaos ou la persistance Save/Continue
réelle. Pas d'écriture de `.uasset`, `.umap`, `.sav`, Blueprint ou WBP.

**Suite :** 33 entrées directes de palette, règles d'acceptation
des réceptacles, présentation des armes ARM, outils éditeur, puis
SaveGames sur copies privées avant tout retrait de pointeur ancien.

## Correction de la porte de niveau physique

Le diagnostic des sondes X.10 a isolé le cas `Item_Arrows_01 N1` :
`UGridItemSubsystem::ResolveItem` projette une famille à un niveau donné,
mais n'est pas à lui seul la validation d'un **niveau physique persisté**
pour les non-ARM. Les autres lecteurs utilisent déjà
`FGridWorldItemLevelReadModel::ResolvePersistedInstance`, qui refuse
N1 pour une famille non-ARM et N0 pour une arme canonique connue.
Le générateur adopte exactement ce lecteur strict, puis seulement la
projection visuelle canonique. Les sondes C1–C6 restent indépendantes
pour exposer immédiatement une future régression, sans journal privé.
