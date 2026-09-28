# GrimrockMenu — Current Technical Reference

Date : **22 septembre 2026**  
Statut : **CURRENT — UI-CLEAN01 + UI-ASSET-ORG01**

## Rôle actuel

WBP_GrimrockMenu / UGrimrockMenuWidget est uniquement un shell temporaire pour les pages qui n'ont pas encore été migrées vers des fenêtres autonomes.

Il ne porte plus l'Inventaire.

~~~text
WBP_GrimrockMenu
└── WidgetSwitcher_MainContent
    ├── Page_Skills
    ├── Page_Spellbook
    ├── Page_Journal
    ├── Page_Map
    ├── Page_Recipes
    └── Page_Codex
~~~

L'Inventaire est exclusivement :

~~~text
WBP_CharacterSheet
+
WBP_InventoryBag
~~~

Référence : docs/Design/UI_CLEAN01_REMOVE_MONOLITHIC_INVENTORY.md.

## Navigation

La navigation visible appartient à WBP_GridPersistentHud :

~~~text
ESC / I / K / G / M / J / H
~~~

Le shell ne possède plus de barre d'onglets C++.

Les touches routent vers AGrimrockPartyPawn :

~~~text
I -> ToggleInventoryWidget() -> CharacterSheet + InventoryBag
K -> ToggleSkillsWidget()    -> Page_Skills
G -> ToggleCraftingWidget()  -> Page_Recipes
M -> ToggleMapWidget()       -> Page_Map
J -> ToggleJournalWidget()   -> Page_Journal
H -> ToggleHelpWidget()      -> Page_Codex
~~~

Spellbook reste une page du shell et conserve son accès fonctionnel existant.

## Contrat C++

UGrimrockMenuWidget expose uniquement :

~~~text
InitializeMenuWidget(...)
SetActiveTopTab(...)
RefreshSkills()
RefreshSpellbook()
GetSkillsWidget()
GetSpellbookWidget()
~~~

Bindings UMG :

~~~text
WidgetSwitcher_MainContent
Page_Skills
Page_Spellbook
Page_Journal
Page_Map        -> WBP_GridMap (MON21.6.8 : reparent cible UGridMapWidget)
Page_Recipes
Page_Codex
~~~

Il n'existe plus côté C++ :

~~~text
Page_Inventory
Button_TabInventory
OpenInventoryWorkspace
RefreshInventory
GetInventoryWidget
~~~

## Inventaire

Le shell ne doit jamais être utilisé comme fallback Inventory.

Chemin unique :

~~~text
AGrimrockPartyPawn
├── CharacterSheetWidgetClass -> WBP_CharacterSheet
└── InventoryBagWidgetClass   -> WBP_InventoryBag
~~~

Si l'une de ces classes n'est pas configurée, l'ouverture de l'Inventaire échoue explicitement ; elle ne revient pas à un ancien écran.

## Pages restantes

Le shell subsiste uniquement parce que les pages Skills, Spellbook, Journal, Map, Recipes et Codex sont encore réellement utilisées.

À mesure que ces pages deviennent autonomes, leurs bindings doivent être retirés du shell. Lorsque la dernière page aura migré, WBP_GrimrockMenu / UGrimrockMenuWidget devra être supprimé.

## Règle d'architecture

Aucun nouveau code Inventory ne doit être ajouté à UGrimrockMenuWidget.

Aucun nouvel onglet visuel supérieur ne doit être ajouté au shell.

La barre basse persistante reste l'unique navigation globale visible.


## Audit assets UI — 22 septembre 2026

L'inventaire complet des assets sous `Content/GrimrockPrototype/Blueprints/UI`, leur statut et le plan de rangement/nettoyage sont documentés dans :

```text
docs/Design/UI_ASSET_AUDIT_2026_09_22.md
```

Le shell `WBP_GrimrockMenu` reste temporairement nécessaire. Il ne doit pas être supprimé tant que Skills, Spellbook, Journal, Map, Recipes et Codex n'ont pas tous quitté ce shell.

## MON21.6.8 — Map native rendering

`WBP_GridMap` reste la surface Map existante du shell. La migration MON21.6.8 ne crée pas de second écran.

Cible native :

```text
WBP_GridMap
    Parent Class -> UGridMapWidget
```

Le binding du shell reste volontairement :

```cpp
TObjectPtr<UWidget> Page_Map;
```

Le shell effectue un cast via `GetMapWidget()`, initialise la page avec le Party Pawn et, depuis MON21.6.9, appelle `SelectPartyFloor()` à chaque activation de l’onglet Map afin de respecter le contrat d’ouverture sur l’étage courant.

Le reparent de l’asset doit être effectué manuellement dans Unreal Editor. Aucun `.uasset` n’est modifié à l’aveugle.

## MON21.6.9 — Floor Navigation

`UGridMapWidget` porte une sélection d’étage transient et expose :

```text
NavigateFloorUp()
NavigateFloorDown()
CanNavigateFloorUp()
CanNavigateFloorDown()
SelectPartyFloor()
```

Bindings UMG optionnels à ajouter manuellement dans `WBP_GridMap` après validation C++ :

```text
Button_LevelUp   : Button
Button_LevelDown : Button
Text_FloorLabel  : TextBlock
```

Le C++ gère les clics et l’état Enabled. Aucun Graph Blueprint n’est requis.

## MON21.6.10 — Zoom / Pan / Recenter

`UGridMapWidget` porte une caméra de présentation transitoire :

```text
ZoomScale
PanOffsetPixels
bCenterViewOnParty
```

Interactions natives :

```text
molette              -> AdjustZoom()
clic gauche + drag   -> PanMapByPixels()
Button_Recenter      -> RecenterMap()
```

`Button_Recenter` est un `BindWidgetOptional` à ajouter manuellement dans `WBP_GridMap` après validation C++.

Zoom et pan n’entraînent aucune reconstruction du `FGridMapFloorView` ; ils invalident uniquement le Paint.

## MON21.6.11 — Hand-Drawn Parchment Artistic Pass

Aucun nouveau widget UMG n’est requis.

`UGridMapWidget::NativePaint()` applique directement :

```text
fond parchemin procédural
grain déterministe
lavis + hachures des cellules explorées
feather léger aux frontières d’exploration
double trait manuscrit déterministe
```

Réglages exposés dans `Map|Art` :

```text
bEnableParchmentStyle
ParchmentColor
ParchmentGrainColor
CellHatchColor
FogFeatherColor
HandDrawnJitterPixels
SecondaryStrokeAlpha
ParchmentGrainLineCount
CellHatchLineCount
```

Si `WBP_GridMap` avait explicitement surchargé les anciennes couleurs `Map|Rendering`, utiliser `Reset to Default` pour récupérer la nouvelle palette C++.

## MON21.6.12 — Map Symbols

`UGridWorldObjectDefinitionAsset` expose un opt-in de présentation :

```text
MapSymbolStyle = None | StairsUp | StairsDown | Relocation | Pit | PointOfInterest
```

Cette valeur ne change jamais le `SupportedType` gameplay.

Le read model expose uniquement `Cell + Kind` lorsque la cellule est explorée ; aucune identité d’objet n’atteint le WBP.

`UGridMapWidget::NativePaint()` dessine les glyphes directement. Aucun nouveau widget UMG n’est requis.

Réglages dans `Map|Symbols` :

```text
NavigationSymbolColor
HazardSymbolColor
SymbolStrokeThickness
SymbolScale
SymbolMinCellPixels
```

Les DataAssets concernés doivent être configurés manuellement dans UE après validation C++.

## MON21.6.13 — Final Fit / Polish / Closure

`UGridMapWidget` adapte maintenant automatiquement la carte à l’`AllottedGeometry` réel du WBP.

```text
MapDrawPadding      = 48 / 72 / 48 / 96
AutoFitMarginCells = 0.75
MaxCellPixels      = 0  // aucun plafond fixe par défaut
```

Le zoom est appliqué après le fit et le dessin est clippé à la surface parchemin : le pan/zoom ne peut plus recouvrir les contrôles du WBP. La molette et le démarrage du clic-glisser sont eux aussi limités à cette zone.

Polish final :

```text
ParchmentEdgeColor / Thickness
WallUnderlayColor / ThicknessScale
DoorJambLengthScale
PartyMarkerScale
hachures et jitter allégés
```

Aucun nouveau widget UMG n’est requis. Si le Blueprint conserve une ancienne surcharge héritée (notamment `MaxCellPixels=64`), utiliser `Reset to Default` sur la propriété concernée.
