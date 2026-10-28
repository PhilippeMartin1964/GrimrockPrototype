# ARMEMENT-LOT02.13.2.3 — Palette des consommables dans WeaponPipeline

Le script de production spécial PublishArrowPalette.ps1 et le commandlet GridArrowPaletteAuthoring ont été retirés. Cette intervention était une duplication évitable du pipeline industriel.

## Opération de production générique

L'action `-Action PaletteItem` du script industriel `Scripts/WeaponPipeline.ps1` charge un DataAsset existant depuis `Core/DataAssets/Items` et propose, sans autre asset auxiliaire, son inscription comme collectible dans `DA_ObjectPalette_Default`. Elle appelle le commandlet existant `GridWeaponPipeline` et réutilise `FGridWeaponPipelinePalette::ProposeDirectItem`.

Garde-fous : validation du DataAsset et de la palette, refus des doublons/alias ou des conflits, modification d'une seule entrée, `-Apply` explicite, conservation des autres entrées et aucune suppression ni publication Git. Les fichiers déjà modifiés sont indiqués *par chemin* et jamais réinitialisés.

## Commandes à exécuter après fermeture UE5

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
$arrow = '/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.DA_Item_Arrows_01'
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action PaletteItem -ItemPath $arrow
.\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action PaletteItem -ItemPath $arrow -Apply
```

La seconde commande exécute la suite `Grimrock.RPG.ARMEMENT.LOT02`. Le test `ArrowPaletteDirectCollectible` existant vérifie la présence de l'entrée et le placement sur une grille en mémoire. Vérifier le statut local avant de versionner seulement la palette.

En cas de refus pour source/palette déjà modifiée :

```powershell
git status --short -- Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Arrows_01.uasset
```

Ne pas exécuter `git reset` ni écraser les changements, analyser d'abord leur provenance.

## Portée

L'arc ARM048 requiert correctement `Item_Arrows_01` et refuse le tir sans ces flèches. Ce correctif expose la définition existante dans la catégorie Items ; il ne fournit pas de mesh ou d'icône à l'item. Leur création et l'essai réel du ramassage au sol restent nécessaires avant de valider le point PIE 5. Les onze armes restent Planned. Aucun kit, niveau, sauvegarde, classeur Excel ou document Meshy n'est modifié ici.
