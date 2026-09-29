# MAP-UI01 — Standalone Map Window

Date : 29 septembre 2026  
Statut : **C++ IMPLÉMENTÉ — CONFIGURATION UMG + VALIDATION LOCALE/PIE REQUISES**

## Objectif

Découpler la carte de `WBP_GrimrockMenu` afin que la touche `M` ouvre directement `WBP_GridMap`, comme l'inventaire ouvre ses surfaces dédiées.

Ce ticket est strictement structurel. Il n'ajoute pas encore le bandeau supérieur, le bouton de fermeture, l'overlay de navigation d'étage, les nouveaux glyphes cartographiques ni les marqueurs joueur.

## Architecture

Avant :

```text
M
 -> AGrimrockPartyPawn::ToggleMapWidget()
 -> WBP_GrimrockMenu
 -> WidgetSwitcher_MainContent
 -> Page_Map / WBP_GridMap
```

Après :

```text
M
 -> AGrimrockPartyPawn::ToggleMapWidget()
 -> MapWidgetClass
 -> WBP_GridMap directement dans le viewport
```

`WBP_GrimrockMenu` reste le shell temporaire de Skills, Journal, Recipes, Codex et Spellbook.

La carte conserve exactement les autorités validées par MON21.6 :

```text
Dungeon/Level runtime state
 -> MapExploration
 -> filtered read model
 -> FGridMapFloorView
 -> UGridMapWidget::NativePaint()
```

Aucune autorité Map supplémentaire n'est créée.

## C++

`AGrimrockPartyPawn` expose maintenant :

```text
MapWidgetClass
MapWidgetInstance
ToggleMapWidget()
ShowMapWidget()
IsMapWidgetVisible()
```

Les transitions entre surfaces restent exclusives :

- ouvrir Inventory replie Map et le shell partagé ;
- ouvrir Map replie Inventory et le shell partagé ;
- ouvrir Skills/Journal/Recipes/Codex replie Inventory et Map ;
- ESC/combat replient aussi Map via le chemin Major UI existant.

Le HUD persistant considère désormais Map comme sélectionnée depuis `MapWidgetInstance`, et non depuis `UGrimrockMenuWidget::CurrentTopTab`.

## Modification UMG manuelle requise

Aucun `.uasset` n'est modifié à l'aveugle dans ce ticket.

Après compilation :

1. ouvrir `BP_GrimrockPartyPawn` ;
2. affecter **Map Widget Class** = `WBP_GridMap` ;
3. ouvrir `WBP_GrimrockMenu` ;
4. supprimer `Page_Map` / l'instance embarquée de `WBP_GridMap` du `WidgetSwitcher_MainContent` ;
5. conserver `WBP_GridMap` comme UserWidget autonome.

Ne pas ajouter le bandeau ou déplacer les contrôles d'étage dans MAP-UI01 ; ils appartiennent aux tickets suivants.

## Automation

Filtre ciblé :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.MapUI01"
```

Attendu : **2 tests**.

Régressions UI utiles ensuite :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Navigation01"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.GlobalHud01"
```

## Smoke PIE

- `M` ouvre directement la carte sans `WBP_GrimrockMenu` ;
- `M` referme la carte ;
- le bouton Map du HUD persistant fait de même ;
- `I` depuis Map ferme Map et ouvre l'inventaire ;
- `K/G/J/H` depuis Map ferment Map et ouvrent leur page ;
- ESC ferme Map avant le menu principal ;
- zoom, pan, recentrage, changement d'étage et rendu MON21.6 restent inchangés.

## Stop condition

MAP-UI01 est validé uniquement après build UE 5.5.4 vert, filtre ciblé vert, régressions Navigation01/GlobalHud01 vertes, configuration UMG manuelle effectuée et smoke PIE confirmé par l'utilisateur.
