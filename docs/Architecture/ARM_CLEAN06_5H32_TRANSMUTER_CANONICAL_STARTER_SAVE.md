# ARM-CLEAN06.5H.32 — Transmuter N1, kit et Save/Continue exclusivement canoniques

## Remplacement du test historique de kit

Le fichier `GridARMEMENT01171TransmuterOilTests.cpp` ne charge plus `DA_Item_Flask_Oil`. Le scénario `TransmuterNewGameSaveContinue` n'utilise plus les pointeurs `FRPGStartingEquipmentEntry::ItemDefinition` pour trouver la flasque : la recette Transmuter est résolue depuis `CanonicalItemId` avec le `UGridItemSubsystem` configuré.

Une fixture `UWorld` + `UGameInstance` utilise désormais la même voie de création que le frontend : `TryPopulateApprovedTransientClass(..., Registry)` et `CreateInitialCharacter` depuis un vrai `UGridPartyInventoryComponent` de Pawn. Le test contrôle trois orientations, Dagger/Orb/Oil, quantité 1, N0, sac, aucune définition héritée, absence de mutation de la classe source, identité du kit et niveau initial.

Save/Continue reste un aller-retour binaire en mémoire `SaveGameToMemory`/`LoadGameFromMemory` et `RestorePartyInventoryState` sur un nouvel inventaire dans le même monde de test. GUID de la flasque, personnage, propriété, quantité et kit sont préservés; aucun `.sav` réel n'est touché.

## Restrictions

Aucune modification de la recette de production, de code gameplay, `.uasset`, `.umap`, `.sav`, HUD, Niagara ou huile Shipping. Les 53 packages H.23 restent intacts. H.33 n'est pas autorisé sans les statuts exacts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` tous deux SUCCESS, avec compilation Editor/Shipping et Automation `Grimrock.RPG.ARMEMENT01.17.1`. Les sept SaveGames privés requièrent toujours une nouvelle validation native avant retrait.
