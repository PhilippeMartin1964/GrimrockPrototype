# ARM-CLEAN06.4X.16.6.3 — Manifeste canonique exact avant migration binaire

## Origine vérifiée

Issue #176 : https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/176

- Test UE source : db4d5114b068d2487a46c72caf30ef9c8e08c7ff
- Publication filtrée : 6c3c72e24083bfbfaaf5df4e0f23a91e596a0745
- Contrôles : SilentTask SUCCESS, LocalAuto SUCCESS,
  ARM-CLEAN06.4X1662 SUCCESS.

Le rapport Windows confirme 44 définitions d'objet et
quatre niveaux : six listes de réceptacles sûres (deux
règles), deux listes de clés sûres (deux pointeurs),
zéro override de ces règles dans les niveaux et
zéro référence inconnue, manquante ou contradictoire.

## Quatre packages exacts — pas un de plus

| Package UE | Index | Ancien ID | ID canonique |
|---|---:|---|---|
| /Game/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder | 0 | Item_Torch | Item_Torch |
| /Game/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian | 0 | Gem_Blue | Gem_Blue |
| /Game/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall | 0 | Key_Copper | Key_Copper |
| /Game/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall | 0 | Key_Iron | Key_Iron |

Chemins Git LFS des futurs fichiers concernés :

- Content/GrimrockPrototype/Core/DataAssets/DA_Receptacle_TorchHolder.uasset
- Content/GrimrockPrototype/Core/DataAssets/GridObjectArchetypeAsset/DA_Guardian.uasset
- Content/GrimrockPrototype/Core/DataAssets/DA_Lock_CopperWall.uasset
- Content/GrimrockPrototype/Core/DataAssets/DA_Lock_IronWall.uasset

**Ces quatre fichiers n'ont PAS encore été modifiés.**

## Transaction A : deux réceptacles

Conserver les pointeurs historiques, écrire seulement
DefaultBehavior.Receptacle.AcceptedItems[0].CanonicalItemId :
- DA_Receptacle_TorchHolder = Item_Torch ;
- DA_Guardian = Gem_Blue.

Ne jamais modifier bAcceptAnyItem, MaxContainedItems,
InitialContent, règles de retrait, GUID, liens logiques
ou présentation. Le vieux pointeur doit correspondre
exactement à l'ItemId du registre DA_ItemSystem.
Rouvrir les deux packages dans un second processus UE
et contrôler les ID ainsi que les pointeurs conservés.

## Transaction B : deux serrures (commit séparé)

Conserver les AcceptedKeyItems historiques et ajouter
uniquement à la liste AcceptedKeyIds :
- DA_Lock_CopperWall = Key_Copper ;
- DA_Lock_IronWall = Key_Iron.

Préserver les ID déjà présents, leur ordre et les
doublons. L'acteur AGridWallLockActor calcule une
union AddUnique : le résultat avant/après doit rester
identique. Ne pas modifier la consommation des clés,
les textes ni les états initiaux.

## Conditions avant toute transaction binaire

1. Vérifier le master, son parent exact, le worktree
   dédié propre, les OID Git LFS et tailles. Refuser
   tout pointeur LFS non hydraté.
2. Faire des copies privées de chaque .uasset avec
   taille et empreinte SHA-256 ; revérifier la copie.
3. Auditer les valeurs dans UE et refuser toute
   dérive du manifeste AVANT tout SavePackage.
4. Modifier deux packages seulement par transaction,
   réceptacles d'abord, serrures ensuite.
5. Fermer/réouvrir dans un deuxième processus Unreal,
   tester la parité des règles et des champs hérités.
6. Exécuter la suite Grimrock ; ne commiter que les
   deux chemins autorisés, contrôler le SHA final.
7. Exiger SilentTask et LocalAuto SUCCESS sur le
   SHA final, puis PIE TorchHolder/Guardian/
   CopperWall/IronWall et Save/Continue sur copies.
8. Aucun fichier .sav personnel, .umap, WBP ou
   Blueprint ne peut être modifié implicitement.

Ce document prépare la migration mais n'exécute
aucune commande -Apply. Ne pas rejouer 06.4W/X.5.
