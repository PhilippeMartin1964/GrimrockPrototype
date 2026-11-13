# ARMEMENT01.17.1.3 — Flasque d'huile en combat et lisibilité des infobulles

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/test historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Non implémentée : zéro legacy dans le runtime final.**


**Date : 03.11.2026 — code/CSV/tests publiés, compilation UE5.5.4 et matérialisation ST_UI locales requises. Pas encore validé en PIE.**

## Constat utilisateur

- La flasque `DA_Item_Flask_Oil` est réellement attribuée au Transmutateur,
  visible dans l'inventaire, la hotbar et le donjon. Visuels versionnés dans
  le commit utilisateur `327150ff`.
- En combat, la hotbar affiche `Cette action n’aurait actuellement aucun effet utile.`
- L'infobulle inventaire ne décrit pas la zone, la portée, la durée ou le coût.

## Cause technique

`FGridCombatActionCatalog::EvaluateMON126Availability` reconnaît explicitement
les QuickItems `Effect + Cell/Area + SurfaceEffects/Conversions/Interaction`.
Mais le contrôle suivant `NoApplicableEffect` n'acceptait que le soin, le
mana, les statuts, l'armure et les mouvements du groupe. Une flasque d'huile
qui **ne modifie pas les PV du lanceur**, mais **crée une surface dans le
monde**, était donc désactivée avant la sélection de cellule.

## Correction

1. Le contrôle d'utilité du **QuickItem à effet** reconnaît maintenant
   `Cell/Area` et `SurfaceEffects/SurfaceConversions/SurfaceInteraction`.
   Le ciblage reste contrôlé par `BuildTargetingPreviewForAction` :
   distance, ligne de vue, cellule, occupation et réaction de surface
   éventuelle. Aucune consommation/PA au stade du catalogue.
2. Le runtime de ciblage existant `RequestCharacterTargetedAttack` applique
   les surfaces, débite une flasque et 2 PA. **Aucune modification de ce
   runtime** ; l'obstacle était le gate du catalogue.
3. `FGridQuickItemSurfaceTooltipText::Build` expose une **projection
   mécanique** des QuickItems `Effect/Area` créant de l'huile. Sa source
   est la vraie `FGridCombatActionDefinition`, sans ItemId spécifique ni
   attribution du talent N2. Format unique `Combat.QuickItem.Surface.OilUsage`.
4. Inventaire : `UGridInventorySlotWidget::GetTooltipView` conserve le
   `Description` du DataAsset et ajoute le texte mécanique s'il existe.
5. Hotbar combat : `BuildAssignedHotbarToolTip` affiche le même texte
   mécanique, après le `Description` de l'action.
6. Les WBP sont **inchangés** : `WBP_InventorySlot` passe par
   `FGridItemTooltipView.Description` et la hotbar via son texte existant.
   Pas de nouvelle mise en page ou de Blueprint Graph.
7. Le coût du QuickItem, le rayon, la portée, la durée et le surcoût de
   déplacement ne sont jamais inventés : ils suivent les valeurs
   numériques de la définition réellement équipée.

## Validation C++ locale

Après `git pull --ff-only` avec un working tree propre et Unreal Editor
**fermé** :

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation

& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" `
  -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi

if ($LASTEXITCODE -ne 0) { throw "ST_UI materialization failed" }
```

Avant de tester l'interface, ouvrir l'asset dans Unreal et vérifier que
`/Game/Localization/StringTables/ST_UI` contient **réellement** la clé
`Combat.QuickItem.Surface.OilUsage`. Vérifier le log `Changed=true/false`.

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.17.1.3"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.ARMEMENT01.17.1"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.RPG.RPG03.7"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Monsters.MON12.8.6"
```

## PIE — recette fonctionnelle

1. New Game → Alchimiste/Transmutateur : la flasque est visible et
   l'infobulle inventaire mentionne **huile**, portée 4, rayon 1,
   durée 4 rounds, pénalité traversée +1, coût 2 PA/une flasque.
2. Lancer un combat avec au moins 2 PA, une flasque dans le sac et le
   raccourci dans la hotbar. Son infobulle ne doit plus afficher
   `aucun effet utile` ; cliquer sur la flasque, choisir une cellule
   valide à quatre cases au maximum, confirmer.
3. Vérifier que le moteur consomme **une flasque et 2 PA**, crée une
   surface d'huile sur les cellules de la zone, à l'état persistant
   attendu. Sur une cellule invalide, refuser sans consommation.
4. Sans flasque ou avec seulement 1 PA, la hotbar refuse avec sa
   **vraie raison** ; aucune dépense.
5. Save → Quitter → Continue **avant** et **après** l'action pour vérifier
   l'état des objets et des surfaces selon les contrats SaveGame.

## Localisation : passage obligatoire

`UI.csv` est modifié dans le commit C++; **le seul CSV ne suffit pas**.
`ST_UI.uasset` est une projection binaire Unreal locale : elle doit être
créée avec `UIStringTableAuthoring`, inspectée dans le Content Browser,
puis validée et versionnée sans écraser d'autres modifications locales.

- Source `UI.csv` : préparée
- `ST_UI` : **à matérialiser localement**
- WBP : existants réutilisés, **aucun changement**
- Target `Game/fr/en` : gather/traduction/compile/vérification anglais
  **à confirmer** ; ne pas annoncer EN complet

Après validation, vérifier `git status --short`. La modification
`ST_UI.uasset` doit être poussée depuis le poste Windows (Git LFS)
comme une synchronisation des packages après le ticket source.

**Clôture conditionnelle**, uniquement après les tests locaux, la
matérialisation de ST_UI, la recette PIE et la publication du binaire.
