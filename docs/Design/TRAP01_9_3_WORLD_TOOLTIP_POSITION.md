# TRAP01.9.3 — Position de l'infobulle de survol du monde

**Statut : code publié sur master ; compilation UE 5.5.4 et recette PIE requises.**

## Constat PIE
TRAP01.9.2 fonctionne (20 exécutions automatisées et 5/5 contrôles PIE validés), mais l'infobulle se place sous et à droite du curseur, qui chevauche visuellement le début du panneau. Capture utilisateur du 1er novembre 2026.

## Règle de positionnement
- Position privilégiée : **au-dessus et à gauche du curseur**, qui apparaît donc en bas à droite du panneau.
- Décalage normal : **24 unités UMG** ; marge écran : **8 unités UMG**.
- Si le panneau dépasse seulement un peu du bord et reste clairement au-dessus/à gauche du curseur, le repositionner légèrement plutôt que le faire brusquement basculer à droite (cas de la capture).
- Sinon, quadrants essayés dans cet ordre : **haut-gauche, haut-droite, bas-gauche, bas-droite**.
- Si aucun quadrant n'est disponible, conserver le candidat qui exige le moins de correction puis borner sa position à l'écran. Un tooltip plus grand que le viewport utilisable ne peut mathématiquement pas tenir intégralement.
- Position recalculée dans l'espace **UMG logique**, avec la taille réelle du widget après `ForceLayoutPrepass()`. Pas de multiplication/division DPI supplémentaire pour la position de la souris.
- Tout le calcul est isolé dans `FGridWorldTooltipPlacement::Resolve` et ne touche ni le curseur, ni les collisions, ni l'éligibilité, ni les probabilités de désamorçage.
- Aucune création de widget par tick, aucun changement du présentateur `WBP_GridNavigationTooltip` ni de sa typographie.

## Fichiers
- `Source/GrimrockPrototype/Public/UI/GridWorldTooltipPlacement.h` : calcul pur quadrants / fallback / clamp.
- `Source/GrimrockPrototype/Private/Runtime/GrimrockPlayerController.cpp` : utilise le résultat du calcul.
- `Source/GrimrockPrototype/Private/Tests/GridTRAP0193TooltipPlacementTests.cpp` : cas de capture et de bords.
- `docs/Design/TRAP01_9_3_WORLD_TOOLTIP_POSITION.md` : présent document.

## Contrat UMG / localisation
- Présentateur inchangé : `/Game/GrimrockPrototype/Blueprints/UI/Interaction/WBP_GridNavigationTooltip` (parent `UGridNavigationTooltipWidget`, `Border_Tooltip > Text_Tooltip`).
- Les textes existants sont toujours fournis sous forme dynamique `FText` par `GetInteractionText`.
- `Localization/Source/UI.csv` : **inchangé**, aucune nouvelle clé.
- `Content/Localization/StringTables/ST_UI.uasset` : **inchangé**, aucune matérialisation requise.
- WBP : **inchangé**, aucune nouvelle affectation.
- Target `Game` fr/en : **inchangé**, pas de nouvelle chaîne à traduire ; conserver la couverture de localisation préexistante.

## Validation locale

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.9.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.UI.GlobalHud01"
```

## Recette PIE
1. Dans `L_Dungeon`, survolez le piège détecté : infobulle **à gauche et au-dessus du curseur** lorsque l'espace suffit.
2. Répétez avec un texte très large proche du centre (comme la capture) : léger clamp à gauche, sans bascule intempestive vers la droite.
3. Placez le curseur près du **bord gauche** : infobulle à droite si manque d'espace réel.
4. Placez le curseur près du **bord haut** : infobulle en bas.
5. Vérifiez les **quatre coins** et les résolutions / DPI différents : panneau visible dans la mesure où sa taille le permet.
6. Vérifiez que le survol désactivé, le clic gauche, les probabilités et les interactions restent ceux de TRAP01.9.2.

Ne pas clôturer sans tests UE et recette PIE.
