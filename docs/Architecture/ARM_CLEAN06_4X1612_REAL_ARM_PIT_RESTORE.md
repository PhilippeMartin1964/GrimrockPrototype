# ARM-CLEAN06.4X.16.12 — Correction de la chute réelle des armes sur une trappe

## Déclencheur et portée

Le propriétaire a testé le scénario réellement jouable : placer des armes sur une fosse piégée, échouer au désamorçage, laisser la trappe s'ouvrir, puis retrouver les armes dans le niveau inférieur. Elles ne réapparaissent pas où attendu. À ce stade, il **n'existe pas** de recette PIE praticable où une torche est placée manuellement sur la fosse.

Les anciennes régressions PIT02/03 prouvaient surtout un `Item_PIT02_Stone` historique et une torche N0 non-ARM. Elles ne prouvaient pas le comportement des ARM N1–N20, ni une réouverture puis un deuxième chargement du niveau inférieur.

## Corrections

- `TryDropItemInstanceAtCell` : tous les objets **déjà physiques** et canoniques, ARM N1–N20 inclus, sont initialisés par `InitializeFromCanonicalItem`. Ce chemin applique la projection complète (mesh, physique, lumière éventuelle, Niagara, scintillement) depuis le registre au lieu du chemin ARM qui réattachait une ancienne `UGridItemDefinitionAsset` puis ne changeait que le mesh. Aucun N0 ARM valide n'est inventé.
- `TryRouteWorldItemThroughOpenPit` : `FGridPendingWorldItemState.ItemDefinitionAsset = nullptr` pour tout ItemId canonique valide, y compris les ARM.
- `ApplyPendingInboundItemsForCurrentLevel` : ignore le pointeur obsolète pour tous les ARM canoniques, matérialise la définition à son niveau N1–N20 et consomme la file seulement si la création a réussi.
- `DropWorldItemsThroughOpenPitAtCell` : rafraîchit les cellules effectives des meshes en physique avant de sélectionner les objets situés sur la trappe. Si un routage est refusé, émet un Warning avec l'ItemId, son niveau et son GUID ; ne détruit pas silencieusement l'objet.
- `ApplyCurrentLevelRuntimeState` : lorsque le joueur revient dans le niveau inférieur, un ARM canonique rechargé depuis `Items` récupère de nouveau sa présentation complète.

Les objets historiques sans ItemId canonique restent sur le pipeline préexistant. Les ARM dans les réceptacles, l'équipement tenu et les catalogues d'actions n'ont pas été basculés ici. Aucune suppression de propriété SaveGame v27, aucune écriture de sauvegarde personnelle et aucune modification binaire Unreal.

## Régression automatisée

`Grimrock.RPG.ARM.CLEAN06_4X1612.TrapOpensUnderTwoWeaponsRealLevels`

1. Dungeon temporaire de deux niveaux, trappe fermée pilotable, registre de production actif.
2. Dépose physique d'une `Weapon_StraightSword_01` N1 et d'une `Weapon_ThrowingKnife_01` N20 sur la cellule de trappe ; comparaison de chaque mesh avec son profil canonique réel, absence de pointeur de définition ancien.
3. Ouverture via `SetPitOpen` (le mécanisme qui est également sollicité par le piège à désamorcer) ; disparition des deux acteurs du niveau supérieur, file `PendingInboundItems` de deux GUID avec les mêmes niveaux et sans vieux pointeurs.
4. `SaveGameToMemory`/`LoadGameFromMemory` sans fichier `.sav`, voyage vers le niveau inférieur : deux acteurs, meshes corrects, pas de duplication.
5. Aller-retour entre les deux niveaux : présence stable des mêmes armes aux mêmes niveaux et avec leur présentation canonique.

La validation `UE5.5.4/LocalAuto` compile et lance l'automatisation Grimrock sous Unreal Engine 5.5.4 en environnement automatisé ; elle ne prouve pas le rendu physique GPU/PIE de la trappe réelle ni le chemin exact « échec de désamorçage » côté UI.

## Validation visuelle ciblée

Sur une copie de partie, déposer deux armes déjà disponibles sur une trappe **fermée**. Déclencher l'ouverture par l'échec du désamorçage. Vérifier leur absence au-dessus puis leur présence et récupération au niveau inférieur, même après un Save/Continue et un changement de niveau. En cas de non-apparition, fournir uniquement les messages `GridPit WorldItem remained`, `GridPit ItemTransfer`, `GridPit PendingItem`, `GridInventory WorldDrop` — sans journal intégral ni données privées.

Suivi : attendre `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA exact, puis demander uniquement la recette PIE réellement disponible.
