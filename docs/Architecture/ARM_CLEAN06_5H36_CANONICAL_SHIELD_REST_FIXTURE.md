# ARM-CLEAN06.5H.36 — Repos explicite avec bouclier canonique N1

La fixture `GridRPGShield023RestTests.cpp` n'a plus besoin de l'ancien `DA_Weapon_RoundShield_01` pour exercer les règles de repos. Son monde de test dispose maintenant d'un `UGameInstance` et d'un `UGridItemSubsystem` réels, détruits proprement après le test.

Le bouclier ARM075 N1 est projeté avec `ResolveItem("Weapon_RoundShield_01",1)` et l'exigence de cinq points d'armure physique. L'inventaire du Pawn ne reçoit aucune ancienne définition `UGridItemDefinitionAsset`. L'objet équipé conserve son identité, son GUID, son emplacement OffHand et la réserve consommée.

Les deux tests existants conservent leurs contrôles : repos explicite hors combat, recharge des pools physique/magique sans restaurer HP/mana, idempotence, repos après victoire, refus en combat, en défaite, hors centre de cellule et lorsque le groupe ne compte plus de vivant.

Aucun code gameplay, `.uasset`, `.umap`, `.sav`, Niagara ou sauvegarde joueur n'est modifié. Les 53 packages historiques restent protégés.

H.37 ne sera publié qu'après `UE5.5.4/SilentTask = SUCCESS` ET `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA exact, incluant Editor, Shipping et Automation `Grimrock.RPG.SHIELD02.3`. Les sept sauvegardes privées exigent encore un contrôle natif renouvelé avant toute suppression.
