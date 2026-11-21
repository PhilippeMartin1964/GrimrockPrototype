# ARM-CLEAN06.5H.6 — Suppression du chargeur des anciens kits dans le jeu packagé

**Parent :** `5e6e3a751bbb48c33ffb9ea3e292f0b7398c3cf3` (ne publier le lot qu'après deux statuts Windows SUCCESS sur ce parent).

## Défaut restant

`RPGStartingEquipmentPresetCatalog.cpp` était le dernier fichier de **production** retrouvé par recherche C++ qui pouvait appeler `LoadObject<UGridItemDefinitionAsset>` pour retrouver les anciennes définitions `DA_Weapon_*` et `DA_Item_*`. H.3 a verrouillé les 18 kits approuvés hors Automation si le registre canonique manque. Toutefois l'ancien générateur `TryPopulateTransientClass` de 12 kits restait compilé et appelable dans un build Game/Shipping, bien qu'utilisé uniquement pour des tests et audits Editor.

## Résolution

- La déclaration/collection des 12 kits historiques `FKitSpec/Kits` n'est compilée que sous `WITH_EDITOR`.
- `RPGStartingPresets::ResolveItem` (chargeur direct des anciens paths) et la branche correspondante de `AddItem` n'existent plus dans le code compilé hors Editor. Un appel invalide renvoie une erreur sans charger de package.
- `TryPopulateTransientClass` conserve son API pour les anciens clients, mais **refuse sans mutation** dans un exécutable non Editor.
- `IsSupportedClass` s'appuie sur la liste des **18 recettes approuvées**, qui comporte les mêmes six classes, et non sur l'ancien catalogue de 12 kits.
- Les vrais kits approuvés avec `DA_ItemSystem` continuent de suivre le chemin résolu existant, niveau réel N0/N1.

Une nouvelle Automation `Grimrock.RPG.ARM.CLEAN06_5H.EditorOnlyHistoricalTwoKitParity` contrôle les six classes, les 12 kits historiques en Editor, les 18 kits approuvés en Automation et la non-mutation des DataAssets de classes. Les suites existantes `ARMEMENT01.9` et `ARM.CLEAN06_5C.AllApprovedKitsCanonical` restent complémentaires.

**Niveau de preuve :** une validation Windows Editor `-NullRHI` prouve compilation et Automations Editor mais ne remplace pas un Cook/Shipping et une validation de démarrage du jeu. Le code est volontairement protégé par compilation `WITH_EDITOR`, mais l'étape de packaging final demeure nécessaire.

**Invariants :** aucun `.sav`, `.uasset`, `.umap` ni tableau de recettes approuvées édité ; pas de seconde autorité canonique. Les audits Editor historiques peuvent encore charger leurs assets sources tant qu'une migration native de ces assets n'est pas autorisée.
