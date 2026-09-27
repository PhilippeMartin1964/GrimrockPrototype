# UI-CODE-AUDIT01 — Combat / Inventory C++ audit and dead-code cleanup

Date : **27 septembre 2026**  
Statut : **C++ prêt pour validation UE5.5.4**

## Périmètre

Audit ciblé des contrats natifs qui portent les WBP suivants :

```text
Content/GrimrockPrototype/Blueprints/UI/Combat
    WBP_GridCombatActionPanel
    WBP_GridCombatHud
    WBP_GridCombatHudAction
    WBP_GridCombatHudInitiativeSlot

Content/GrimrockPrototype/Blueprints/UI/Inventory
    WBP_CharacterSheet
    WBP_InventoryBag
    WBP_InventorySlot
    WBP_ItemActionButton
    WBP_ItemActionMenu
    WBP_ItemReadPanel
    WBP_ItemTooltip
    WBP_ItemTooltipComparisonRow
    WBP_ItemTooltipStatLine
    WBP_PartyMember
```

L'audit couvre aussi les adaptateurs nécessaires au fonctionnement de ces WBP :
`UGridPersistentHudWidget`, les opérations drag/drop, les item context actions et
`AGrimrockPartyPawn` côté orchestration UI.

Les `.uasset` binaires ne sont jamais modifiés à l'aveugle. Les dépendances
Blueprint connues sont conservées tant qu'une compilation UE/Reference Viewer
ne démontre pas leur absence.

## État général

L'architecture actuelle est saine sur ses autorités principales :

```text
UGridPartyInventoryComponent
    -> autorité groupe / inventaire / équipement / hotbar

UGridTurnManagerComponent
    -> autorité combat

UGridCombatHudWidget
    -> projection combat + backend d'exécution/ciblage

UGridPersistentHudWidget
    -> navigation globale + barre d'actions persistante

UGridInventoryWidget
    -> projection/interactions partagées CharacterSheet + InventoryBag
```

UI-COMBAT-CLEAN01 a déjà supprimé la double projection des panneaux de
personnages du HUD combat. `UGridCombatActionPanelWidget` reste un renderer
léger recevant `FGridCombatHudPartyMemberView`.

## Nettoyages réalisés par UI-CODE-AUDIT01

### 1. Inventory — suppression du polling par frame

`UGridInventoryWidget::NativeTick()` ne faisait qu'appeler
`RefreshSelectedCharacterClassIcon()` à chaque frame.

La projection inventaire est déjà événementielle via
`OnPartyInventoryChanged`, la sélection de personnage et
`RefreshInventory()`.

Décision :

```text
SUPPRIMÉ
    UGridInventoryWidget::NativeTick()

REMPLACÉ PAR
    RefreshSelectedCharacterDetails()
        -> RefreshSelectedCharacterClassIcon()
```

L'icône et l'accent de classe sont donc resynchronisés avec le même refresh
autoritaire que le reste de la fiche personnage, sans polling permanent.

### 2. Inventory — resolver équipement dupliqué

`HandleSlotDrop()` possédait une lambda `ResolveEquipmentSlot` qui recopiait
exactement la règle déjà portée par `ResolveUiEquipmentSlot()`.

La lambda est supprimée ; le helper unique est réutilisé.

### 3. Combat — suppression de la présentation targeting fantôme

Le WBP courant validé ne contient pas :

```text
Panel_Targeting
Text_TargetingInstructions
Text_TargetingCell
```

Le C++ conservait pourtant ces trois `BindWidgetOptional` et
`RefreshTargetingWidgets()`. Les tests recréaient même artificiellement ces
widgets, ce qui caractérisait un fallback absent de l'asset réel.

Sont supprimés :

```text
Panel_Targeting
Text_TargetingInstructions
Text_TargetingCell
RefreshTargetingWidgets()
```

Le backend de ciblage reste intégralement conservé :

```text
TargetingPreview
bCombatActionTargetingActive
BeginCombatActionTargeting()
UpdateCombatActionTargetingPreview()
ConfirmCombatActionTarget()
ClearCombatActionTargetingPreview()
CancelCombatActionTargeting()
ValidateCombatActionTargetingState()
```

Le test MON12.10 vérifie désormais ce backend directement depuis la hotbar,
sans fabriquer une présentation UMG inexistante.

### 4. Combat — suppression du fallback de clearance pré-Persistent HUD

La hiérarchie WBP validée possède maintenant le conteneur canonique :

```text
Panel_CombatBottomRight
    -> Text_MobilityActionPoints
    -> Button_EndTurn
    -> Text_EndTurnDisabledReason
```

La translation de 56 px est appliquée à ce conteneur unique.

Supprimés :

```text
fallback de translation individuelle sur les trois enfants
TMap<TWeakObjectPtr<UWidget>, FVector2D> CombatBottomBaseTranslations
ApplyBottomClearanceToWidget()
```

Le HUD conserve une seule baseline de translation pour
`Panel_CombatBottomRight`.

### 5. Combat — suppression du remount legacy dans le Pawn

`CollapseMajorGameplayUi()` conservait encore un chemin ancien qui retirait
puis réajoutait `CombatHudWidgetInstance` au viewport lorsque le Persistent HUD
n'était pas présent.

Cette branche contredisait l'architecture UI-GLOBALHUD01, où le Persistent HUD
est la surface permanente. Elle est supprimée.

### 6. Combat — nom de helper corrigé

`ApplyHotbarPresentationFallbacks()` n'était plus un fallback de widget : il
complète les métadonnées de présentation d'une action (nom, description,
icône, raison d'indisponibilité).

Le helper interne devient :

```text
EnrichHotbarActionPresentation()
```

Aucun comportement n'est modifié.

## Code conservé volontairement

### WBP_InventorySlot

Conserver :

```text
RefreshSlotVisual()
GetDisplayNameText()
GetQuantityText()
GetTooltipView()
GetTooltipText()
```

`RefreshSlotVisual` est un BlueprintNativeEvent réellement utilisé par
`WBP_InventorySlot`. Les deux helpers compacts ont déjà été supprimés par
erreur puis restaurés après détection de dépendances Blueprint sérialisées.

### WBP_InventoryBag / menus d'item

Conserver :

```text
OnContextActionsRequested
ExecuteInventoryContextActionByIndex()
OnItemActionMenuCloseRequested
OnItemReadPanelCloseRequested
PresentItemExamination()
PresentItemReading()
```

Le presenter de `WBP_ItemActionMenu` et les panneaux Lire/Examiner restent
Blueprint-owned.

### Persistent HUD

Le `NativeTick()` de `UGridPersistentHudWidget` reste nécessaire : il
surveille les changements de largeur réelle du viewport et ajuste le nombre de
slots visibles. Ce tick possède une garde par cache et ne reconstruit pas la
barre lorsque la géométrie et le nombre de slots utilisés n'ont pas changé.

### Combat action / initiative widgets

Les fallbacks natifs de badge quantité et de progress bar d'initiative restent
fonctionnels et testés. Ils ne sont pas supprimés sans vérification WBP dédiée.

## Points détectés mais non supprimés sans passe Blueprint

### API Blueprint potentiellement historiques

Plusieurs méthodes de `UGridInventoryWidget` n'ont pas de consommateur C++
extérieur identifié, par exemple certains helpers d'enregistrement/curseur ou
wrappers BlueprintCallable.

Ils ne sont pas supprimés dans ce ticket : les `.uasset` sont sous Git LFS et
une recherche textuelle GitHub ne peut pas prouver l'absence de nœuds Blueprint
sérialisés. Leur suppression doit être faite par petite tranche avec
compilation des WBP concernés.

### Slot types MainHand / OffHand / Cursor

Le paper doll courant utilise la forme canonique :

```text
EGridInventoryUiSlotType::Equipment
+ EGridEquipmentSlot
```

Les alias UI `MainHand`, `OffHand` et le chemin `Cursor` restent présents
dans plusieurs routes historiques. Ils sont de bons candidats de normalisation,
mais l'enum est `BlueprintType`; aucune valeur n'est retirée sans audit
sérialisé UE.

### CurrentItemActionMenu par réflexion

`UGridInventoryWidget` recherche encore la variable Blueprint
`CurrentItemActionMenu` par réflexion afin de déterminer si le menu contextuel
est réellement ouvert/détaché.

Ce contrat fonctionne mais est fragile. Le remplacer demande une modification
coordonnée du Graph de `WBP_InventoryBag` (setter/registration explicite) et
constitue un ticket séparé.

## Anomalie fonctionnelle détectée

`UGridItemContextActionLibrary::BuildItemContextActions()` génère actuellement
`EGridItemActionType::Consume` pour Potion/Food, mais
`UGridInventoryWidget::ExecuteResolvedInventoryContextAction()` ne possède pas
de branche `Consume`.

Conséquence : l'action visible peut tomber sur le chemin `NotImplemented`.

Ce n'est pas traité comme code mort : le design d'item actions prévoit
explicitement « Consommer ». Il faut soit implémenter la consommation réelle,
soit masquer temporairement l'action. Cette correction doit être un ticket
fonctionnel dédié pour ne pas mélanger cleanup et gameplay.

Les enum values réservées `Use`, `UseOnTarget`, `Combine` et
`ToggleLight` ne sont pas retirées dans ce ticket pour la même raison :
contrat `BlueprintType` + fonctionnalités prévues.

## Structuration recommandée après validation

`GridInventoryWidget.cpp` reste très volumineux. Le prochain refactor de
structure utile peut répartir l'implémentation de la même classe sans créer
d'autorité supplémentaire :

```text
GridInventoryWidget.cpp
    lifecycle / queries / orchestration

GridInventoryWidgetCharacter.cpp
    party selector / character details / paper doll / class visual

GridInventoryWidgetProjection.cpp
    bag grid / filter / sort / slot projection

GridInventoryWidgetContextActions.cpp
    context menu / read / examine / action execution

GridInventoryWidgetDragDrop.cpp
    drag/drop / swap / cursor routes
```

Même principe possible pour `GridCombatHudWidget.cpp` :

```text
GridCombatHudWidget.cpp
GridCombatHudViewModel.cpp
GridCombatHudActionWidget.cpp
GridCombatHudInitiativeSlotWidget.cpp
```

Ce découpage est organisationnel seulement : aucune nouvelle classe
d'autorité, aucun ViewModel supplémentaire, aucune donnée dupliquée.

## Validation attendue

Filtres prioritaires après ce cleanup :

```text
Grimrock.UI.GlobalHud01
Grimrock.Monsters.MON12.10
Grimrock.Monsters.MON12.CombatHUD
Grimrock.Monsters.MON12.8
Grimrock.UI.Character01
Grimrock.UI.Character02
Grimrock.UI.Inventory01
Grimrock.UI.Inventory02
Grimrock.UI.InventoryProjection02
Grimrock.UI.Filter01
Grimrock.UI.Item01
Grimrock.UI.Split02
```

PIE :

- CharacterSheet : sélection, classe, stats, paper doll ;
- InventoryBag : grille, tri, filtres, poids, clic droit, Lire/Examiner ;
- drag/drop inventaire <-> équipement et inventaire -> portrait ;
- Persistent HUD : barre dynamique et navigation ;
- combat : portraits, PA/PAM, initiative, End Turn ;
- targeting Cell/Area depuis la hotbar ;
- aucune duplication de barre ni présentation targeting fantôme.
