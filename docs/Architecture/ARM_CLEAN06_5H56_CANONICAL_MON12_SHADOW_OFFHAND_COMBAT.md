# ARM-CLEAN06.5H.56 — MON12 Ombre, double maniement par ItemSystem

## Conversion

Le scénario `Grimrock.RPG.ARMEMENT.LOT02.19.1.ShadowN1OffHandCombat` attache le registre de production `FGridCanonicalLootTestScope` au monde MON12 déjà créé. Il résout les cinq équipements N1 par leur `ItemId` : Dagger (ARM001), Main-gauche (ARM013), Round Shield (ARM075), Assassin Dagger (ARM002) et Great Axe (ARM031).

Les lectures de `DA_Weapon_*` et les `RegisterItemDefinition` historiques sont supprimés **de ce scénario uniquement**. Les instances équipées gardent leurs GUID déterministes, propriétaires et slots. Les politiques de présence d'une action offensive en OffHand et le caractère passif du bouclier sont vérifiés par `FGridResolvedItem.Weapon`.

## Contrats préservés

Le test d'origine conserve le combat réel, le coût PA d'attaque Main-gauche, la combinaison des deux armes en un raccourci principal, l'exécution depuis le HUD, les raccourcis explicitement épinglés et leur GUID sauvegardé, la persistance des choix utilisateur, puis les refus d'attaques fictives avec bouclier, Assassin Dagger en OffHand ou hache deux mains.

Le fichier MON12 contient d'autres scénarios à définitions historiques et des sons transitoires; **ils ne sont pas modifiés** dans H.56. Aucun changement runtime, `.uasset`, `.umap`, `.sav`, Niagara ou audio de production. Les 53 packages H.23 restent conservés.

H.57 n'est autorisé qu'après `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA H.56 (compilations Editor/Shipping, tests `Grimrock.RPG.ARMEMENT.LOT02.19.1`). L'audit des sept sauvegardes demeure obligatoire.
