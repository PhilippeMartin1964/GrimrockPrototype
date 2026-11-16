# ARM-CLEAN06.4X.11 — Préflight lecture seule de la palette réelle

Parent fonctionnel : `ARM-CLEAN06.4X.10` sur
`62dbfa08dea9fdee034754c7093934177d5ee3d3`,
validé par `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`.

La palette réelle `DA_ObjectPalette_Default` contient **33 entrées
directes** dont les `CanonicalItemId` et
`CanonicalDefaultItemLevel` ont été enregistrés une seule fois
dans les assets Unreal par 06.4W et 06.4X.5. **Ne pas relancer
ces convertisseurs.**

Le test Editor `Grimrock.RPG.ARM.CLEAN06_4X11.RealCanonicalPaletteReadOnly`
charge le vrai `DA_ItemSystem` (34 familles) et la palette,
enregistre un seul catalogue transitoire de lecture puis vérifie :

- 33 entrées directes réelles et absence de doublon `EntryId` ;
- identité historique/physique cohérente et niveau réel résoluble ;
- ARM avec niveau N1–N20, non-ARM au niveau N0 ;
- lecture sans mutation des packages des deux DataAssets ;
- comptage des différences de libellé, description et icône entre
  l'ancien `DefaultItemDefinition` et la projection canonique.

**Des compteurs de divergence ne sont pas une preuve de parité.**
Les valeurs exactes et chemins d'assets ne sont ni migrés ni modifiés.
Un écart doit être qualifié avant de basculer les tuiles et la recherche
dans le panneau Slate.

**Zéro modification de gameplay ou de `.uasset`, `.umap`, `.sav`,
WBP/Blueprint.** Compilation et suite `Grimrock` UE5.5.4 requises
sur le SHA final avec les deux statuts Windows SUCCESS. Aucun
résultat `-NullRHI` ne valide à lui seul l'interface Slate/PIE.

**Suite :** sélectionner/placer/actualiser depuis les 33 entrées
en enregistrant strictement le couple (`CanonicalItemId`,
`CanonicalDefaultItemLevel`) ; ensuite seulement basculer
texte, icône, recherche et validation de la palette.
