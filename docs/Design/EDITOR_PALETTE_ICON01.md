# EDITOR-PALETTE-ICON01 — Source unique des icônes en palette

## Motif
Le 29.10.2026, la recette ARMEMENT-LOT02 a échoué car l'entrée de palette
`Item_Arrows_01` possédait une icône redondante alors que sa définition
`DA_Item_Arrows_01.Icon` était déjà renseignée. Après suppression du doublon
dans le DataAsset, la suite LOT02 est passée à **12/12**, sans avertissement.

## Règles conservées
- Une entrée avec `DefaultItemDefinition` non nul tire son icône de
  `DefaultItemDefinition.Icon`, comme le fait déjà la palette Slate.
- Une entrée `DefaultWorldObjectDefinition` sans `DefaultItemDefinition`
  peut définir `FGridObjectPaletteEntry.Icon`.
- `FGridObjectPaletteEntry::IsValidEntry` et
  `UGridObjectPaletteAsset::ValidatePalette` continuent à refuser les
  doublons d'icône : aucune exception ni désactivation de validation.

## Amélioration UE5.5.4
- `EditCondition = "DefaultItemDefinition == nullptr"` et `EditConditionHides`
  masquent `Icon` dans le panneau *Details* lorsqu'une définition d'objet
  ramassable est sélectionnée.
- `PostEditChangeChainProperty` efface une éventuelle ancienne icône
  de palette **sur l'entrée précisément éditée**, au moment où une définition
  de collectible est affectée ou une ligne est ajoutée/dupliquée.
- Les autres entrées, icônes du monde et icônes des DataAssets d'items
  ne sont pas modifiées. Aucun traitement global au chargement.

## Validation locale
Fermer l'éditeur avant de compiler, puis :

```powershell
cd D:\Development\GrimrockPrototype
git fetch origin master
git diff --name-only HEAD..origin/master
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.Editor.Palette.ICON01'
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
```

**Vérification manuelle dans DA_ObjectPalette_Default :**
1. Sur une entrée à `DefaultItemDefinition`, le champ `Icon` doit disparaître ;
   changer la définition après avoir préalablement renseigné une icône sur une
   entrée de test doit supprimer ce doublon sans toucher aux autres lignes.
2. Sur une porte (aucun `DefaultItemDefinition`), `Icon` doit rester
   éditable et visible.
3. Confirmer l'affichage de l'icône canonique dans la palette d'éditeur
   et l'inventaire.

Le commit ne contient aucun `.uasset` ni opération sur la carte `L_Dungeon`.
