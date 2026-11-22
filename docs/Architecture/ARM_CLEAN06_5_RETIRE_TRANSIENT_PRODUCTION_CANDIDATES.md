# ARM-CLEAN06.5 — Retrait de six générateurs de candidats historiques

Ce lot remplace huit tests Automation de 06.2 P/Q/S/U/W/X par des contrôles
en lecture seule du DA_ItemSystem installé. Les tests gardent leurs noms exacts
et vérifient les identités, l'unicité, les profils N1-N20, les 19 attaques
physiquement liées, les permissions de main, les 14 actions détaillées,
les trois projectiles spécialisés et le refus de niveaux/IDs invalides.

Les six constructeurs de candidats de production qui dépendaient des anciens
DA_WeaponCatalogue et DA_WeaponVisualCatalogue ne sont plus utilisés :

- GridArmClean06ProductionCandidate
- GridArmClean06ProductionNumericsCandidate
- GridArmClean06ProductionActionsCandidate
- GridArmClean06FullProductionCandidate
- GridArmClean06ProductionAmmoCandidate
- GridArmClean06CompleteProductionPreview

Leurs paires .cpp/.h (12 fichiers) sont retirées d'UnrealBuildTool.
Les utilitaires de projection restants ne sont pas supprimés ici : certains
ont encore des tests de conversion distincts et seront triés ultérieurement.

Les rapports de parité historiques sont conservés dans docs/ et servent de
preuve figée ; l'ancienne source C++ ne peut plus reconstituer un concurrent
de l'autorité unique. Aucun package Content, SaveGame ou donnée utilisateur
n'est touché. Le retrait des 53 candidats natifs reste strictement interdit
avant une preuve AssetRegistry/SaveGames liée au SHA final et la recette UE.
