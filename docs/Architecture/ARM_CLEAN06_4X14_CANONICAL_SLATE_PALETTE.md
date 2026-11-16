# ARM-CLEAN06.4X.14 — Tuiles et recherche Slate canoniques

**Parent validé : ARM-CLEAN06.4X.13** sur
`d330d613d175ad6ee2e18829130aec4d8cbb8ca7`,
avec `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`
SUCCESS. Les trois sondes X.13 ont prouvé **33/33
parités de titres, descriptions et chemins d'icônes**.

## Modification ciblée

`SGridEditorToolPalettePanel` construit une seule projection
de lecture transitoire, à partir du **même `DA_ItemSystem`**
que le runtime. Elle ne publie ni n'enregistre de second catalogue.
Pour les 33 entrées canoniques :
- les labels, la recherche et les icônes des tuiles proviennent de
  `FGridResolvedItem` et non de `DefaultItemDefinition` ;
- un `CanonicalItemId` manquant ou contradictoire avec l'ancien
  pointeur n'autorise aucun retour sur un titre/icone historique ;
- les sources synthétiques sans `CanonicalItemId` restent
  temporairement compatibles, sans changer les objets non-Item.

Le modèle `FGridEditorCanonicalPaletteReadModel` est sans mutation,
testé sur les 33 entrées réelles, les cas contradictoires et la
compatibilité des fixtures de test. La validation générique
`UGridObjectPaletteAsset::ValidatePalette`, l'affichage des
autres inspecteurs, les règles d'acceptation, les ARM physiques
et les SaveGames restent des jalons distincts.

Aucun asset natif (`.uasset`, `.umap`, `.sav`), WBP, Blueprint
ou catalogue n'est réécrit. Compiler UE5.5.4 et lancer la suite
complète `Grimrock` sur le SHA final. Les tests `-NullRHI`
ne remplacent pas une recette visuelle réelle de la palette Slate.
