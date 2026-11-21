# ARM-CLEAN06.5H.66 — Présentations canoniques et audio indépendant du double maniement

## Conversion réelle du test de combat MON12

`Grimrock.RPG.ARMEMENT.LOT02.19.2.N1DualWieldTwoProfilesOneCost` n'ouvre plus les quatre anciens `DA_Weapon_*` Dagger (ARM001), Main-gauche (ARM013), RoundShield (ARM075) et GreatAxe (ARM031). Chaque équipement N1 est résolu depuis `UGridItemSubsystem`, ses instances ne portent que les identités canoniques, GUID et slots. Aucun `RegisterItemDefinition` historique n'est appelé.

Le test audio exige les vrais profils de présentation des deux attaques. `FGridCanonicalLootTestScope::AttachWithWeights` duplique temporairement `DA_ItemSystem` dans le package transitoire. Le callback **duplique explicitement aussi** `UGridActionPresentationCatalogueAsset` dans cette racine avant toute mutation ; le catalogue original et les `.uasset` de production restent intacts. Les deux bindings d'attaque ARM001/ARM013 sont recherchés dans le catalogue canonique : chacun doit avoir un `PresentationId` unique. Une copie d'une présentation audio par main reçoit des `USoundWave` transitoires distincts pour le whoosh et l'impact. Aucune définition d'arme ni copie N1 n'est introduite.

## Non-régressions strictes

Le test maintient : deux armes physiques N1, action composite coût 3 PA, deux traces de combat, coefficient OffHand 50 %, deux sons d'attaque séparés, deux impacts audibles séparés, `AudioPlaybackRequestCount = 4`, séparation temporelle, blocage pendant la séquence puis libération des actions, refus d'un second coup faute de PA, suppression du combo avec un bouclier et absence de combo avec Great Axe. Les quatre demandes sonores originales sont conservées sans relâcher leurs assertions.

Cette fixture ne prétend pas écouter les haut-parleurs : elle vérifie les objets `USoundWave` effectivement résolus et les demandes de lecture dans le runtime, sans audio de sortie natif. Aucun fichier sonore, WBP, `.uasset`, `.umap`, `.sav` ou code gameplay modifié. Les 53 packages H.23 restent protégés.

## Validation

H.67 est interdit avant `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur ce SHA (Editor, Shipping et `Grimrock.RPG.ARMEMENT.LOT02.19.2`). Les sept sauvegardes natives doivent être réauditées sur le SHA final avant tout retrait d'asset historique.
