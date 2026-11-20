# ARM-CLEAN06.5G.124 — Fixtures Editor sans copies d'instance

Parent exact `be0819966d02cf57b2770f7ee732fceac3809cf6`. Retirer cinq affectations non sérialisées dans les essais Editor d'armes industrielles N1/N20, SaveGame v27, trajectoires des flèches, géométrie de tir et impact fiché sur monstre. Leurs GUID, niveaux, quantités, world ownership, état sauvegardé, maillages, durée de vol, position et récupération restent vérifiés. Les poids statiques sont conservés sur les définitions (legacy explicitement fournies aux fixtures ou ItemSystem canonique), jamais recopiés dans `FGridItemInstance`.

Ce nettoyage de fixtures ne remplace pas la validation Windows ni les tests PIE finaux. Aucun changement runtime, asset ni SaveGame.
