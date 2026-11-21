# ARM-CLEAN06.5H.35 — Bouclier ARM075 et récupération d'armure sans ancien DataAsset

Deux fixtures de `GridRPGShield022Tests.cpp` ne chargent plus `DA_Weapon_RoundShield_01` et n'appellent plus `RegisterItemDefinition` pour un item de production.

Un petit `FWorldFixture` donne aux tests un vrai `UGameInstance`, son `UGridItemSubsystem` configuré et le `UGridPartyInventoryComponent` d'un Pawn réel. La définition `Weapon_RoundShield_01` N1 est résolue dans le registre canonique, en imposant le caractère ARM et les cinq points d'armure physique. Les instances d'équipement, sac et curseur portent uniquement l'ItemId, le niveau et leur épuisement de réserve.

Les contrôles d'origine restent en place : réapprovisionnement après victoire de l'armure physique/magique, absence de soin HP/mana, GUID conservés dans les trois supports, deuxième victoire, accord UI/combat et **aucune** récupération si le personnage est à terre. La classe C++ de l'ancien item n'est pas requise pour ce scénario.

Pas de changement gameplay, d'asset `.uasset`, de carte `.umap`, de SaveGame `.sav` ou de matériau. Les 53 packages historiques ne sont pas supprimés.

H.36 exige `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA exact, incluant Development Editor, Shipping et Automation `Grimrock.RPG.SHIELD02.2`. L'inventaire H.23 et les sept sauvegardes privées restent des barrières avant tout retrait natif.
