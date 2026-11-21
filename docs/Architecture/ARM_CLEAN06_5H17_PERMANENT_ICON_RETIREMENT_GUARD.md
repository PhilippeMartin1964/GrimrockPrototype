# ARM-CLEAN06.5H.17 — Garde permanent sur la palette historique supprimée

**Parent :** `76f13b1b90e1efb22e0af68aa9655b8eccd5d65b` (publication du seul retrait binaire `DA_EquippedWeaponActionIcons.uasset`, validée `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success`).

## Constat validé

- Le reçu H.16 local confirme 54 → 53 packages ; vérifications des **sept sauvegardes v27** et des deux Readables OK, source palette uniquement retirée, copie privée de 4 340 octets préservée.
- Le commit parent retire strictement le pointeur Git LFS à la palette, et aucun autre objet dans le dépôt.
- La suite Windows Grimrock complète a réussi sur le SHA **du commit de suppression**, pas seulement sur son parent.

## Pourquoi un nouveau garde ?

Le test existant `Grimrock.RPG.ARM.CLEAN06_5G.DetachedIconPaletteSchema` confirmait l'absence de références à la palette et les 12 catégories canoniques, mais n'échouait **pas** si quelqu'un réintroduisait ultérieurement un package isolé sans référent : une régression possible malgré l'ancien retrait natif.

H.17 complète le même test Editor :

1. `Registry.SearchAllAssets(true)` puis `GetAssetsByPackageName(..., true)` doit retourner **zéro** enregistrement pour `/Game/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons` ;
2. `IFileManager::Get().FileExists(...)` exige que le véritable `Content/GrimrockPrototype/UI/Icons/Combat/DA_EquippedWeaponActionIcons.uasset` n'existe plus, y compris si une découverte de registre échoue à le repérer ;
3. la vérification existante zéro référent hard/soft AssetRegistry et l'autorité canonique des **12 icônes** restent intactes.

Aucun asset, niveau, SaveGame ou catalogue canonique n'est modifié par H.17. Les scripts d'audit H.13 peuvent toujours classifier 54/53 packages **pour l'analyse et le retour arrière**, mais la suite Automation de production signale désormais toute **réintroduction de la palette**.

**Tests d'acceptation :** commit atomique code+docs, puis `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA publié. **Hors validation de ce jalon :** ouvrir PIE avec rendu, reprendre une sauvegarde et vérifier un build Cook/Shipping complet.
