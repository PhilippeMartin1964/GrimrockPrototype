# ARM-CLEAN06.4X.42 — Visuel équipé canonique à niveau physique explicite

**Parent :** `02ffcb99ad48d3a308a5d31f76cfb59d88199294`. **Lot :** code C++ Pawn, un test physique
sur UE réel, documentation. Aucun `.uasset`, `.umap` ou SaveGame.

## Problème et contrat

`EquipHeldItem(FName)` ne connaît pas le `ItemLevel` d'une arme
réellement équipée. La routine `SyncHeldVisualFromSelectedCharacterEquipment`
possède au contraire le `FGridItemInstance` complet N1–N20.
Après X.41, les sources de lumière sont canoniques mais le chemin
ancien pouvait encore demander `DA_Weapon_*` pour construire
l'acteur tenu, contradiction pour une arme luminescente pointerless.

## Correction

Nouvelle méthode C++ `EquipHeldItemFromInstance(const FGridItemInstance&)` :
reconnaît l'ItemId enregistré dans `UGridItemSubsystem`, refuse
tout niveau physique invalide (ARM N0, non-ARM N1), crée un acteur
`AGridItemActor` sans source legacy, initialise sa présentation
complète à l'aide de `InitializeFromCanonicalItem` au **niveau réel**,
puis charge `EquippedMesh` du catalogue et désactive seulement son
PointLight local au profit de l'éclairage du groupe. Les effets
persistants restent attachés. Un GUID distinct, transitoire, est
attribué au visuel tenu, sans modifier le GUID de l'équipement.

`SyncHeldVisualFromSelectedCharacterEquipment` utilise cette méthode
lorsqu'un objet lumineux d'équipement doit être représenté ; un
changement de niveau N1/N20 régénère la présentation même si
l'ItemId est identique. `EquipHeldItem(FName)`, conservé pour
l'interface Blueprint historique, **refuse désormais de deviner
le niveau d'un ARM canonique** ; il continue à fonctionner sur
non-ARM N0 et les identités réellement hors registre.

## Test

`Grimrock.RPG.ARM.CLEAN06_4X42.CanonicalHeldPhysicalArm` :
vraie arme `Weapon_StraightSword_01` aux niveaux N1/N20, sans
`DA_Weapon_*` ; maillage équipé canonique, ID/niveau conservés,
présentation complète. Refus sans destruction de l'ancien acteur
pour ARM N0 et torche N1 ; torch N0 canonique et chemin historique
hors registre inchangés. Les autres tests de synchronisation de
la torche et de lumière X.17/X.23/X.41 restent dans la suite.

La validation Windows `SilentTask` / `LocalAuto` doit passer pour
ce SHA exact avant de commencer un autre lot. `NullRHI` ne suffit
pas pour qualifier le rendu PIE de l'arme tenue.

**ARM-CLEAN06.4 PARTIELLE ; ARM-CLEAN06.5 BLOQUÉE.**
