# ARM-CLEAN06.4X.23 — Torche équipée sans définition historique : modèle de lumière et visuel tenu

**Date :** 17 novembre 2026. **Parent :** `7f5f5ae4045f93e273fc904e351502dd580084eb`. **C++ et tests uniquement.** Aucun WBP, asset natif, map ou SaveGame modifié.

## Problème

Le registre `DA_ItemSystem` contient déjà la torche non-ARM `Item_Torch` à N0 et sa présentation `LightEmitter`. Pourtant, `GrimrockPartyPawnHeldItem.cpp` exigeait une ancienne `UGridItemDefinitionAsset` pour créer l'objet visible en main et détecter sa lumière ; `UGridPartyIlluminationComponent::RefreshFromEquipment` sélectionnait encore la lumière la plus forte exclusivement à partir de ces anciennes définitions. Supprimer les pointeurs aurait éteint les torches canoniques.

## Correction à une seule autorité

- `EquipHeldItem` accepte un ItemId canonique **non-ARM à N0** même si aucune définition historique ne peut être trouvée ; il crée un `AGridItemActor`, appelle `InitializeFromCanonicalItem`, puis utilise `EquippedMesh` du registre. Le contrat public `EquipHeldItem(FName)` ne transporte **pas** de niveau N1–N20 : la fonction ne fabrique donc jamais un niveau N1 arbitraire pour un ARM. Les anciennes armes enregistrées avec une définition historique conservent leur chemin actuel.
- `DoesEquippedItemEmitLight` et `UGridPartyIlluminationComponent::RefreshFromEquipment` utilisent la configuration `LightEmitter` issue de `ResolveItem(ItemId, ItemLevel)` pour tous les non-ARM enregistrés, y compris avec un ancien item référencé dans le sac. Les identités canoniques mal formées échouent sans repli.
- Pour ARM actuellement liés à un ancien item, on conserve leur éclairage historique jusqu'à la migration native des paramètres X.20. Si l'ancien asset d'un ARM disparaît plus tard, la configuration canonique du profil visuel peut servir de source, sans faire dériver l'ID de l'ancienne définition.
- Le choix du meilleur éclairage reste déterminé par l'intensité et `bLightsEnabled` de chaque instance ; aucun recalcul de durée ni modification du gameplay d'équipement.

## Régression automatisée

`Grimrock.RPG.ARM.CLEAN06_4X23.CanonicalHeldTorchWithoutLegacySource` réutilise un monde et un registre réels mais transitoires. Teste une `Item_Torch` N0 directement équipée en main principale puis secondaire, sans enregistrement de l'ancienne définition, vérifie l'acteur tenu sans pointeur historique, la projection de présentation, l'éclairage du groupe et l'extinction (plus de lampe ni objet tenu). Les tests existants X.17 de l'équipement de la torche restent obligatoires.

Les statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` seront relevés sur le SHA exact après publication.

## Limites et suite

La migration en binaires UE des profils ARM (Niagara persistant, lumière, sparkle, physique), les 14 IDs hors registre, les presets d'équipement initial et le SaveGame v27 restent hors périmètre. Une vérification PIE de la lumière/torche sera nécessaire avant la recette 06.6. **ARM-CLEAN06.4 toujours partielle ; 06.5 bloquée.**
