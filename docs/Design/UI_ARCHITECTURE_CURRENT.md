# UI Architecture Current State

Statut : **CURRENT — UI-COMBAT-UNIFY01 CONTRACT NORMALIZED ; VALIDATION UE EN ATTENTE**  
Date : **7 octobre 2026**

## Références canoniques

Le détail technique du menu joueur reste :

```text
docs/Design/UI_GRIMROCK_MENU_CURRENT.md
```

Pour la dette technique transversale :

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Pour la roadmap active :

```text
docs/Design/PROJECT_COMPLETION_ROADMAP.md
```

## Menu joueur actuel

`WBP_GrimrockMenu` est désormais uniquement le shell temporaire des pages non-Inventaire encore non migrées. Son parent natif est `UGrimrockMenuWidget`. L'Inventaire n'y possède plus aucun fallback C++.

### UI-FOUNDATION01 — fondation historique du workspace

UI-FOUNDATION01 a validé les deux responsabilités CharacterSheet / InventoryBag dans un même WBP. Cette disposition monolithique est désormais **superseded côté présentation par UI-SPLIT01**. Les invariants de données restent valides : un seul `SelectedCharacterIndex`, un seul `UGridPartyInventoryComponent`, aucune duplication de gameplay.

Référence historique : `docs/Design/UI_FOUNDATION01_UNIFIED_INVENTORY_CHARACTER_WORKSPACE.md`.

### UI-SPLIT01 — deux fenêtres viewport indépendantes

L'Inventaire quitte `WBP_GrimrockMenu` lorsque les classes split sont configurées.

```text
Viewport
├── WBP_CharacterSheet       gauche, pleine hauteur utile
├── vue 3D                   centre
├── WBP_InventoryBag         droite, pleine hauteur utile
├── WBP_GridCombatHud        overlay combat uniquement
└── WBP_GridPersistentHud    barre basse persistante
```

Les deux fenêtres dérivent de la même implémentation native `UGridInventoryWidget` via deux classes sémantiques fines. Elles partagent le même composant inventaire et se resynchronisent via `OnPartyInventoryChanged`.

`WBP_GrimrockMenu` reste temporairement utilisé uniquement pour Spellbook/Journal/Recipes/Codex. Skills et Map sont désormais des surfaces autonomes. Depuis UI-CLEAN01, il ne possède plus du tout `Page_Inventory`, même comme fallback.

Référence : `docs/Design/UI_SPLIT01_INDEPENDENT_INVENTORY_WINDOWS.md`.

### UI-SPLIT02 — inventaire split non modal

Le workspace split laisse désormais la vue 3D centrale interactive. Les interactions monde restent disponibles pendant que CharacterSheet et InventoryBag sont ouverts. Le menu contextuel d'item, lui, reste modal lorsqu'il est visible.

Le presenter Blueprint de WBP_ItemActionMenu a été migré vers WBP_InventoryBag et validé en PIE.

Référence : `docs/Design/UI_SPLIT02_WORLD_INTERACTION_AND_CONTEXT_MENU.md`.

### UI-SPLIT03 — paper doll dépendant du rôle

UGridInventoryWidget n'enregistre plus automatiquement les 15 slots paper doll dans toutes ses sous-vues.

```text
WBP_CharacterSheet -> paper doll présent -> enregistrement + validation
WBP_InventoryBag   -> aucun paper doll   -> aucun faux warning
```

Référence : `docs/Design/UI_SPLIT03_ROLE_AWARE_PAPERDOLL.md`.

### UI-CLEAN01 — suppression du monolithe Inventory

L'architecture split devient exclusive.

```text
SUPPRIMÉ C++ :
WBP_GrimrockMenu -> Page_Inventory
UGridInventoryWidget -> Panel_CharacterSheet / Panel_InventoryBag
UGridInventoryWidget -> ResetInventoryWorkspace / états internes de panneaux
```

Les boutons de fermeture appartiennent désormais aux classes spécifiques `UGridCharacterSheetWidget` et `UGridInventoryBagWidget`. L'ancien fichier `GridInventoryWidgetWorkspace.cpp` et son test de caractérisation historique sont supprimés.

Référence : `docs/Design/UI_CLEAN01_REMOVE_MONOLITHIC_INVENTORY.md`.

### UI-CLEAN02 — suppression du générateur paper doll

Le paper doll n'a plus de chemin de construction runtime alternatif. `WBP_CharacterSheet` authorise directement les 18 `SlotWidget_*`; le C++ ne fait plus que les enregistrer et les rafraîchir.

Supprimés :

```text
BuildPaperDollEquipmentPanel
Border_EquipmentPanel
GeneratedPaperDollSlotWidgets
état de build paper doll runtime
```

Référence : `docs/Design/UI_CLEAN02_REMOVE_GENERATED_PAPERDOLL.md`.

### UI-CLEAN03 — suppression de l'alias d'armure

La migration de la feuille personnage n'entretient plus deux bindings pour la même donnée.

```text
SUPPRIMÉ :
Text_CharacterArmor

CANONIQUE :
Text_CharacterPhysicalArmor
```

La valeur continue de provenir du même résumé autoritaire ; seul le doublon de présentation est supprimé.

Référence : `docs/Design/UI_CLEAN03_REMOVE_LEGACY_ARMOR_ALIAS.md`.

### UI-CLEAN04 — suppression des helpers texte primitifs

Les anciennes chaînes de diagnostic Blueprint ne font plus partie du contrat de `UGridInventoryWidget`.

```text
SUPPRIMÉS :
GetItemDisplayString
GetCursorItemDisplayText
GetMainHandDisplayText
GetOffHandDisplayText
GetInventorySlotDisplayText
GetCharacterDisplayText
GetSelectedCharacterDisplayText
```

Les slots, portraits et statistiques utilisent désormais uniquement leurs projections structurées.

Référence : `docs/Design/UI_CLEAN04_REMOVE_PRIMITIVE_DISPLAY_HELPERS.md`.

### UI-ITEM01 — tooltip structuré et comparaison équipement

`UGridInventorySlotWidget` expose désormais un read model unique `FGridItemTooltipView`.

```text
FGridItemInstance + UGridItemDefinitionAsset
    -> FGridItemTooltipView
       -> identité / description / poids
       -> capacités
       -> bonus / résistances
       -> comparaison avec l'équipement du personnage sélectionné
```

Le tooltip réutilise `WBP_ItemTooltip` existant. Aucun deuxième widget tooltip n'est créé.

Référence : `docs/Design/UI_ITEM01_ITEM_TOOLTIP_COMPARISON.md`.

### UI-NAV01 — barre inférieure persistante

La navigation `ESC / I / K / G / M / J / H` n'est pas enfant du menu ni du HUD de combat. Depuis UI-GLOBALHUD01, elle appartient à la surface runtime permanente `WBP_GridPersistentHud`, à côté de la barre générale d'actions.

```text
WBP_GridPersistentHud
└── HorizontalBox_BottomBar (ancrée en bas)
    ├── Panel_GlobalNavigation    toujours visible
    └── Panel_ActionBar           N slots d'action jointifs, calculés selon le viewport
```

Le menu peut être ouvert, fermé ou changer de page sans modifier la visibilité de `Panel_GlobalNavigation`. Les boutons et les touches passent par les mêmes commandes C++.

Référence : `docs/Design/UI_NAV01_PERSISTENT_BOTTOM_NAVIGATION.md`.

### UI-GLOBALHUD01 — séparation Persistent HUD / Combat HUD

La barre basse permanente quitte `WBP_GridCombatHud`. `UGridPersistentHudWidget` devient l'unique propriétaire de la navigation globale et de la barre générale d'actions. Le Combat HUD est désormais masqué hors combat et ne doit plus contenir conceptuellement ces surfaces. La barre d'actions comporte au minimum 12 raccourcis suisses `1..0, ', ^`, puis autant de slots souris que la largeur le permet. Les slots utilisent `Auto` et `Padding=0`; le reliquat reste vide à droite.

Référence : `docs/Design/UI_GLOBALHUD01_PERSISTENT_NAV_ACTION_BAR.md`.

### UI-COMBAT01 — combat modal pour les grands panneaux

Quand `UGridTurnManagerComponent::bCombatActive` devient vrai, il appelle une unique transition C++ `AGrimrockPartyPawn::HandleCombatStarted()`. Le Pawn ferme les surfaces gameplay incompatibles puis réutilise le helper privé `CollapseMajorGameplayUi()`, également utilisé par la fermeture utilisateur de l'inventaire. L'autosave reste uniquement dans `HideInventoryWidget()`. Les grands panneaux ne peuvent pas être rouverts tant que le combat reste actif ; la barre inférieure persistante et la hotbar restent visibles.

Référence : `docs/Design/UI_COMBAT01_CLOSE_NON_COMBAT_UI_ON_COMBAT_START.md`.

### UI-CHAR01 — personnage sélectionné unique

La refonte ne crée aucun état parallèle. `UGridPartyInventoryComponent::SelectedCharacterIndex` reste l'unique autorité pour la feuille, le paper doll et le sac.

`WBP_CharacterSheet` expose six instances canoniques `PartyMember_1..6`, enregistrées nativement sur les indices `0..5`. `WBP_PartyMember` reçoit en option `Image_Portrait` et `Border_Selected` pour projeter le portrait et l'état de sélection sans logique Blueprint.

Référence : `docs/Design/UI_CHAR01_SINGLE_SELECTED_CHARACTER.md`.

### UI-CHAR01.1 — sélecteur de groupe compact

`WBP_PartyMember` est uniquement un sélecteur visuel de membre du groupe. Les informations textuelles détaillées sont déjà affichées par `WBP_CharacterSheet`.

Bindings supprimés du contrat natif :

```text
Text_Name
Text_ClassLevel
Text_Weight
```

Helpers supprimés :

```text
GetDisplayNameText()
GetClassLevelText()
GetWeightText()
RefreshBoundMemberFields()
```

Le portrait conserve uniquement les éléments utiles à la sélection et au feedback compact : portrait, cadre de sélection, alerte de surcharge et effets de statut.

Les éléments de classe redondants ont également été supprimés du sélecteur :

```text
Image_ClassIcon
Border_ClassAccent
AvailableClassVisuals
SetAvailableClassVisuals()
FindClassVisualForCachedClass()
```

L'icône et les informations de classe restent présentées dans `WBP_CharacterSheet`, qui est leur surface détaillée canonique.

### UI-CHAR02 — projection de la feuille personnage

`UGridInventoryWidget` continue de lire directement `FGridInventoryCharacterSummary`. Aucun ViewModel ou calcul gameplay UI supplémentaire n'est ajouté.

La feuille expose désormais, en plus des champs existants :

```text
ProgressBar_CharacterHealth
ProgressBar_CharacterMana
Text_CharacterInventorySlots
Text_CharacterPhysicalArmor
Text_CharacterMagicalArmor
Text_CharacterInitiative
Text_CharacterAccuracy
Text_CharacterEvasion
Text_ResistancePhysical
```

Les ratios sont uniquement des projections visuelles des valeurs canoniques et sont clampés dans `[0..1]`.

Référence : `docs/Design/UI_CHAR02_CHARACTER_SHEET_PROJECTION.md`.

### UI-INV01 — un seul sac pour le personnage sélectionné

Le panneau droit ne représente jamais simultanément les inventaires des six membres. `WBP_InventoryBag` conserve une seule `InventorySlotsGridPanel`, alimentée par `SelectedCharacterIndex`.

Le résumé du sac est projeté vers :

```text
Text_InventoryBagOwner
Text_InventoryBagWeight
```

La capacité et le nombre de colonnes sont globaux au groupe et configurés sur `PartyInventoryComponent`. Tous les personnages utilisent le même nombre de slots.

Référence : `docs/Design/UI_INV01_SELECTED_CHARACTER_SINGLE_BAG.md`.

### UI-WEIGHT01 — poids/encombrement dans le sac uniquement

Le sac affiche uniquement `Text_InventoryBagWeight` sous la forme `Poids : CurrentWeight / MaxWeight`.

La progress bar de poids et le hook Blueprint `PresentInventoryWeightState` ont été supprimés. `WeightState` reste calculé en C++ pour les usages gameplay/UI utiles, notamment l'alerte de surcharge du portrait. Le poids n'influence jamais la capacité en slots.

### PARTY-WEIGHT01 — surcharge bloquante pour le déplacement

`UGridPartyInventoryComponent` expose la requête gameplay `IsAnyActiveCharacterOverloaded()`, fondée sur le même calcul canonique que `WeightState`. `AGrimrockPartyPawn::TryStartMove()` refuse toute translation avant les règles spatiales ou le TurnManager lorsqu'au moins un personnage actif dépasse strictement sa capacité de portage.

La surcharge bloque donc Forward / Backward / Strafe en exploration comme en combat et réutilise le feedback d'obstacle existant : petit mouvement d'impact, retour à la cellule et son `BlockedMoveSounds`. Le refus survient avant la translation combat, donc aucune translation/AP n'est consommée. La rotation sur place, les pits et les relocations forcées restent autorisés.

Référence : `docs/Design/PARTY_WEIGHT01_OVERLOAD_MOVEMENT_LOCK.md`.

### UI-INV02 — transfert par drag vers un portrait

Le drag d'un slot d'inventaire capture désormais le personnage source en plus du slot et de l'identité runtime. Les portraits `UGridPartyMemberWidget` sont des drop targets natifs.

Le routage reste :

```text
slot inventaire
-> UGridInventoryDragDropOperation
-> portrait
-> UGridInventoryWidget
-> UGridItemTransferService
-> UGridPartyInventoryComponent
```

La sélection UI ne change pas à la suite d'un transfert. Le drag UI transfère toujours la pile complète ; la scission est exclusivement une action contextuelle de l'inventaire.

Référence : `docs/Design/UI_INV02_PARTY_DRAG_TRANSFER.md`.

```text
Inventaire      fonctionnel
Compétences     autonome ; UI-RPG06 clos
Talents         autonome ; UI-RPG04/05 clos
Sorts           fonctionnel MON18
Journal         shell présent ; read model prévu MON21.5
Carte           autonome via WBP_GridMap
Recettes        shell présent ; fonctionnalité future
Codex           shell présent ; discovery prévu MON21.7
```

La navigation des onglets est portée par le C++. Le Graph de `WBP_GrimrockMenu` ne doit pas recréer une navigation parallèle.

## Quest runtime

MON21.2 et MON21.3 ont livré la couche métier avant l’UI Journal :

```text
UGridQuestDefinitionAsset
    -> QuestId / Objectives

UGridQuestSubsystem
    -> état runtime campagne
    -> OnQuestStateChanged

FGridObjectLink / Event -> Command
    -> QuestStart
    -> QuestCompleteObjective
    -> QuestComplete
    -> QuestFail
```

Le Journal futur doit relire cette autorité. Il ne doit pas stocker sa propre copie des quêtes.

L’état Quest n’est pas encore persistant en SaveGame v9 ; **MON21.4 — Quest Persistence / Migration** est la prochaine tranche.

## Spellbook

`WBP_GridSpellbook` utilise `UGridSpellbookWidget` comme parent natif. Le modèle de connaissance autoritaire reste :

```text
UGridPartySpellbookComponent
    -> CharacterId
    -> KnownSpellIds[]
```

La vue est construite depuis les autorités runtime et ne possède aucune copie gameplay autoritaire.

## Skills / Talents

### UI-RPG-DESC01.5-.11 — audits validés / DESC01.12 normalisation croisée

Les captures PIE de DESC01.4 ont invalidé le contrat de présentation malgré une
validation Automation verte. Le contrat sémantique DESC01.5 est désormais validé :

```text
NOM
TYPE
STATUT
PRINCIPE
EFFETS
[UTILISATION]
[VARIANTES]
ACQUISITION
```

Le focus de consultation d'un nœud est indépendant de son statut d'acquisition.
Toutes les variantes doivent être lisibles simultanément ; le dropdown n'est plus
retenu comme interaction finale.

Références actives :
`docs/Design/UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`.
`docs/Design/UI_RPG_DESC01_6_WARRIOR_TALENT_AUDIT.md` — validé.
`docs/Design/UI_RPG_DESC01_7_ROGUE_TALENT_AUDIT.md` — validé.
`docs/Design/UI_RPG_DESC01_8_RANGER_TALENT_AUDIT.md` — validé.
`docs/Design/UI_RPG_DESC01_9_MAGE_TALENT_AUDIT.md` — validé.
`docs/Design/UI_RPG_DESC01_10_PRIEST_TALENT_AUDIT.md` — validé.
`docs/Design/UI_RPG_DESC01_11_ALCHEMIST_TALENT_AUDIT.md` — validé.
`docs/Design/UI_RPG_DESC01_12_TALENT_NORMALIZATION_90.md` — validé.
`docs/Design/UI_RPG_DESC01_13_TALENT_READ_MODEL_CONTRACT.md` — validé.
`docs/Design/UI_RPG_DESC01_14_1_STRUCTURED_READ_MODEL_SOURCE.md` — **VALIDÉ / CLOS** (6/6 ProductionAssets, 14/14 DESC01, 193/193 RPG03).
`docs/Design/RPG_TALENT_FIX01_D05_TURN_UNDEAD_FIXED_DAMAGE.md` — **VALIDÉ / CLOS** (21/21 RPG03.9.5, assets Prêtre commités).
`docs/Design/RPG_TALENT_FIX02_D01_SECOND_WIND_FULL_HEALTH.md` — **VALIDÉ / CLOS** (12/12 RPG03.9.1).
D02 Désamorçage/Crochetage — **DIFFÉRÉ au vrai runtime Lock/Trap** ; primitives SafeFailure déjà opérationnelles.
D03 Sabotage monde — **DIFFÉRÉ au flux générique d'aptitudes hors combat** ; primitives monde déjà présentes.
`docs/Design/RPG_TALENT_FIX04_D06_CHAIN_REACTION_SURFACE_EFFECT_BRIDGE.md` — **D06 VALIDÉ / CLOS** (9/9 RPG03.9.6A + 7/7 RPG03.9.6B1).
`docs/Design/RPG_TALENT_FIX05_D07_MAJOR_TRANSMUTATION_FIXED_DURATION.md` — **D07 VALIDÉ / CLOS** (10/10 RPG03.9.6A + 4/4 RPG03.9.4F1 + 7/7 RPG03.9.6B2).
`docs/Design/RPG_TALENT_FIX06_D08_MAJOR_TRANSMUTATION_REACTION_BRIDGE.md` — **D08 VALIDÉ / CLOS** (11/11 RPG03.9.6A + 7/7 RPG03.9.6B2 + 4/4 RPG03.9.4F1).
`docs/Design/RPG_TALENT_FIX07_D04_FAVORED_ENEMY_CATEGORY_PRESENTATION.md` — D04 **VALIDÉ / CLOS** : six assets matérialisés (`f305dbc`) ; validations RPG03.9.3 12/12 et RPG01.ProductionAssets 7/7.
`docs/Design/UI_RPG_DESC01_13_CPP_TALENT_AUDIT_90.md` — audit statique C++ 90/90.

Les audits de classe restent documentaires ; aucun nouveau C++ / UMG / DataAsset
DESC01 avant le contrat de read-model DESC01.13.



MON20 reste le socle de la page :

```text
SelectedCharacterIndex
    -> FGridSkillsPageService
    -> UGridSkillsWidget
    -> WBP_GridSkills
```

UI-RPG01 a étendu ce même chemin sans créer de second service ni de seconde autorité :

```text
URPGClassAsset::ProgressionChoices
    -> TalentBranchId
    -> TalentNodeId
    -> ChoiceId / variants
    -> FRPGClassProgressionService
    -> FGridSkillsPageService
    -> FGridTalentTreeView
       -> FGridTalentBranchView[3]
          -> FGridTalentNodeView[5]
             -> FGridTalentVariantView[1..N]
```

Les six `DA_Class_*` de production sont matérialisés. Les invariants validés sont **6 classes / 18 branches / 90 nœuds conceptuels**, avec regroupement des variantes de Spécialisation martiale, Ennemi juré, Affinité élémentaire et Imprégnation.

RPG-SKILL01 complète désormais la moitié Skills de la progression :

```text
Character.Level
    -> points accordés = Level + 3

FGridCharacterInventoryState::SkillRanks
    -> points dépensés = somme des Rank

FRPGSkillPointService
    -> Remaining = Granted - Spent
    -> RankCap = 2 / 3 / 4 / 5
    -> TryPurchaseNextRank()
    -> TryRefundPurchasedRank(SessionFloorRank)

FGridSkillsPageService
    -> RemainingSkillPoints / SkillRankCap
    -> bCanIncreaseRank

UGridSkillsWidget
    -> session d'annulation transitoire
    -> bCanDecreaseRank
    -> WBP_RPGSkillEntry [ − / + ]
```

Aucun compteur de Skill Points n'est persisté. Le bouton `−` n'est pas un
respec : il ne peut annuler que les rangs achetés depuis l'ouverture courante
de COMPÉTENCES. Fermer puis rouvrir la fenêtre fixe une nouvelle frontière.

Validation finale RPG-SKILL01 : **8/8 Automation, 0 warning, 0 échec, PIE validé**.

### UI-RPG-DESC01 — CLOS le 9 octobre 2026

Le contrat historique DESC01.4 est superseded par DESC01.5, la normalisation
croisée DESC01.12 et l'implémentation finale DESC01.13→17.

La fiche Talent canonique est unique pour les six classes :

```text
NOM
TYPE
STATUT
PRINCIPE
EFFETS
[UTILISATION]
[VARIANTES]
ACQUISITION
```

État de production :

- 6 classes / 18 branches / 90 Talents conceptuels ;
- 86 Talents simples ;
- exactement 4 familles exclusives à variantes : Spécialisation martiale,
  Ennemi juré, Affinité élémentaire et Imprégnation ;
- toutes les variantes sont visibles simultanément dans la fiche ;
- aucun ComboBox de choix de variante ;
- consultation, TYPE et STATUT sont trois notions indépendantes ;
- aucun Talent n'est acquis par simple consultation ;
- le panneau de détail est vide avant le premier Talent consulté ;
- un seul scroll vertical porte la fiche ;
- ACQUISITION reste la dernière section ;
- les réactions automatiques sans déclenchement volontaire n'affichent pas
  UTILISATION ;
- les identifiants techniques ne sont pas du texte joueur ;
- les 15 recettes de l'Alchimiste sont projetées avec des noms canoniques
  français dans ACQUISITION ;
- disponibilité de combat, quantité d'objet et crafting restent des autorités
  distinctes du statut Talent.

Architecture :

```text
URPGClassAsset / ProgressionChoices
    -> identité et mécanique Talent

FRPGClassProgressionService
FRPGClassProgressionTransactionService
    -> disponibilité / acquisition / choix exclusif

FGridSkillsPageService
    -> read-model canonique read-only
    -> TYPE / STATUT / PRINCIPE / EFFETS / UTILISATION / ACQUISITION
    -> noms joueur des recettes

UGridSkillsWidget
    -> sélection/navigation

UGridTalentDetailWidget
UGridTalentVariantBlockWidget
    -> présentation native

WBP_RPGTalentDetail
WBP_RPGTalentVariantBlock
    -> composition UMG
```

La projection Talent plate et les chemins de détail legacy ont été supprimés par
DESC01.15.5. Ils ne constituent plus une compatibilité à maintenir.

Validation finale rapportée par l'utilisateur :

```text
Grimrock.UI.RPG.DESC01.QA16   4/4
Grimrock.UI.RPG.DESC01       19/19
warnings                         0
échecs                           0
PIE six classes               validé
PIE recettes                  validé
```

Références canoniques :

- `docs/Design/UI_RPG_DESC01_5_TALENT_UX_CONTRACT.md`
- `docs/Design/UI_RPG_DESC01_12_TALENT_NORMALIZATION_90.md`
- `docs/Design/UI_RPG_DESC01_13_TALENT_READ_MODEL_CONTRACT.md`
- `docs/Design/UI_RPG_DESC01_16_2_SIX_CLASS_PIE_QA.md`
- `docs/Design/UI_RPG_DESC01_17_FINAL_CLOSURE.md`

RPG-ATTR01.1 ajoute l'économie des points de caractéristiques sans nouveau
snapshot persistant :

```text
Character.Level
    -> Granted = floor(Level / 4)

Class.BaseAttributes + Race.AttributeBonuses
Character.Attributes
    -> Spent / Remaining dérivés

FRPGAttributePointService
    -> TryPurchasePoint()
    -> TryRefundPurchasedPoint(SessionFloorValue)

UGridCharacterSheetWidget
    -> session d'annulation transitoire
    -> Text_AttributePoints
    -> boutons − / + optionnels
```

La source RPG-ATTR01.1 est implémentée ; la matérialisation
`WBP_CharacterSheet` et la validation locale restent requises.

`ChoiceId` reste l'identité gameplay/persistante. `TalentNodeId` et `TalentBranchId` sont des métadonnées structurelles destinées à la projection UI. La disponibilité reste calculée par `FRPGClassProgressionService`.

UI-RPG02 à UI-RPG05 ont ensuite livré la présentation, les widgets réutilisables, le détail, les acquisitions simple/variantes et les notifications de progression.

UI-RPG06 clôt l'unification Compétences + Talents :

```text
25 URPGSkillAsset de production
    -> FGridSkillsPageService
    -> FGridSkillEntryView[]
    -> UGridSkillsWidget
    -> WBP_RPGSkillEntry[]

Talents
    -> FGridTalentTreeView
    -> WBP_RPGTalentBranch
    -> WBP_RPGTalentNode
    -> WBP_RPGTalentDetail
```

Validation finale :

```text
Grimrock.UI.RPG06.Skills        7/7
Grimrock.MON20.8.SkillsPage     8/8
Grimrock.UI.RPG04              12/12
Grimrock.UI.RPG05               5/5
PIE Skills + Talents           validé
```

La projection Talent plate historique a depuis été supprimée par UI-RPG-DESC01.15.5.

Références :

- `docs/Design/RPG03_SKILL_TREE_SYNTHESIS.md`
- `docs/Design/UI_RPG_TALENT_TREE_ARCHITECTURE.md`
- `docs/Design/UI_RPG06_3B_FINAL_CLOSURE.md`

## Hotbar

Le Spellbook réutilise la hotbar MON12 à dix slots. Aucun second stockage de raccourcis n’existe.

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
```

## Responsabilités

```text
UGrimrockMenuWidget
    shell temporaire Spellbook / Journal / Recipes / Codex

UGridInventoryWidget
    mécanique/projection commune réellement partagée
    sélection / slots / refresh / interactions item

UGridCharacterSheetWidget
    fenêtre gauche / party / stats / paper doll

UGridInventoryBagWidget
    fenêtre droite / grille inventaire / menu contextuel

UGridPersistentHudWidget
    HUD gameplay persistant
    navigation globale ESC/I/K/G/M/J/H
    barre générale d'actions à nombre dynamique de slots

UGridCombatHudWidget
    présentation combat uniquement
    PAM / initiative / round / fin de tour / ciblage

UGridSkillsWidget
    projection Skills / Talents

UGridSpellbookWidget
    présentation Sorts

UGridPartySpellbookComponent
    connaissance runtime des sorts

UGridPartyInventoryComponent
    autorité groupe/inventaire + sélection + hotbar

UGridQuestSubsystem
    autorité runtime des quêtes

UGridTurnManagerComponent
    autorité combat
```

## Sélection / held visual

`TD-PARTY-001` est **RÉSOLU**. `SetSelectedCharacterIndex()` déclenche la notification autoritaire ; `AGrimrockPartyPawn` resynchronise son held visual.

TD06.9 a clôturé le découpage PartyInventory ; aucune nouvelle extraction n’est recommandée sans signal concret.

## Dette technique UI active

Ne pas mélanger dette technique et fonctionnalités futures.

Dette réelle suivie dans `TECHNICAL_DEBT_REGISTER.md` :

- `TD-UI-001` : nommage historique `EInventoryTopTab` / `ToggleInventoryWidget()` ; faible priorité ;
- `TD-LOG-001` : certaines zones utilisent encore `LogTemp` ;
- divergence visuelle potentielle entre surfaces UMG : à réduire seulement si une douleur concrète apparaît.

## Travail futur qui n’est pas de la dette technique

- MON21.4 : persistance Quest / migration ;
- MON21.5 : Journal Read Model / intégration WBP ;
- MON21.6 : Map Geometry / Exploration ;
- MON21.7 : Codex Discovery / projections ;
- sélection explicite d’un autre allié pour les sorts `Ally` : amélioration fonctionnelle/UX ;
- icônes finales : contenu de production ;
- Recipes / D09 : fonctionnalité future de Crafting ;
- LOC01 — Localization Foundation : future migration des textes joueur vers les
  mécanismes de localisation Unreal (FText / clés stables / String Tables /
  Localization Dashboard), sans modifier les IDs gameplay.

## Roadmap UI canonique

La cible visuelle de référence est : feuille de personnage à gauche, vue 3D centrale interactive, inventaire à droite, barre de navigation/hotbar persistante en bas.

| Ticket | État courant |
|---|---|
| UI-FOUNDATION01 | réalisé, présentation historique superseded par UI-SPLIT01 |
| UI-NAV01 | validé |
| UI-CHAR01 | validé |
| UI-CHAR02 | validé ; poids retiré de la feuille par UI-WEIGHT01-CLEAN01 |
| UI-INV01 | validé |
| UI-INV02 | validé côté C++ et intégré dans l'architecture split |
| UI-ITEM01 | fonctionnel |
| UI-WEIGHT01 | validé et clos le 22 septembre 2026 |
| UI-FILTER01 | UI-FILTER01.1/.2/.3 validés côté C++ ; UI-FILTER01.3.1 validé en PIE |
| UI-INVENTORY02 | UI-INVENTORY02.9 C++ prêt : projection triée/filtrée mise à jour en place, sans recréer les slots ; Automation/PIE à valider |
| UI-HOTBAR01 | réalisé |
| UI-FEEDBACK01 | UI-FEEDBACK01.1 surcharge close ; UI-FEEDBACK01.2 effets de statut portrait actif |
| UI-SKILLS01 | fonctionnel ; UI-RPG01→06 clos ; RPG-SKILL01 validé ; UI-RPG-DESC01 clos le 9 octobre 2026 |
| UI-CRAFT01 | shell |
| UI-MAP01 | shell ; fonctionnalité prévue MON21.6 |
| UI-JOURNAL01 | shell ; fonctionnalité prévue MON21.5 |
| UI-CODEX01 | shell ; fonctionnalité prévue MON21.7 |
| UI-POLISH01 | futur |
| UI-QA01 | futur |

UI-FILTER01 regroupe les `EGridItemType` existants sans créer de seconde taxonomie gameplay. UI-FILTER01.1 fixe le mapping canonique :

```text
Tous
Équipement     -> Torch, Weapon, Shield, Armor, Jewelry
Consommables   -> Potion, Food
Magie          -> Scroll, Gem
Ingrédients    -> Component
Livres et clés -> Key, Book
Divers         -> Quest, Misc, None
```

`None -> Divers` est un fallback de présentation afin qu'un item mal typé ne disparaisse pas d'un filtre nommé. `Tous` accepte tous les types.

Référence : `docs/Design/UI_FILTER01_CATEGORY_MAPPING.md`.

## Validation

TD04 a établi :

```text
Scripts/ValidateUE.ps1       -> Editor build + Automation
Scripts/ValidatePackage.ps1  -> Win64 Shipping cook/package
```

Les bindings/Blueprints/UMG/assets réellement touchés restent à vérifier en PIE.

## Phase actuelle

```text
MON20      CLOS
MON21.1    CLOS
MON21.2    VALIDÉ
MON21.3    VALIDÉ
TD05.9     STOP CONDITION ATTEINTE
TD06.9     STOP CONDITION ATTEINTE
MON21.4    PROCHAIN — Quest Persistence / Migration
```


## Jalons de refonte UI validés côté Automation au 20 septembre 2026

```text
UI-NAV01   2/2
UI-CHAR01  2/2
UI-CHAR02  2/2
UI-INV01   2/2
UI-INV02   2/2
```

`WBP_GridCombatHud`, `WBP_CharacterSheet` et `WBP_InventoryBag` ont désormais fait l'objet de la passe UMG/PIE. UI-CLEAN01 retire le chemin monolithique restant avant de poursuivre les fonctionnalités UI.


## Validation UMG UI-NAV01

`WBP_GridCombatHud` a été validé manuellement en PIE le 20 septembre 2026 : barre inférieure réellement collée au viewport, navigation globale fonctionnelle et hotbar MON12 conservée. Le monolithe `WBP_GridInventory` est supprimé ; les validations UMG concernent désormais `WBP_CharacterSheet` et `WBP_InventoryBag`.


## UI-INVENTORY02.5 — configuration de grille centralisée

La capacité du sac et le nombre de colonnes se règlent désormais au même endroit :

```text
BP_GrimrockPartyPawn
└── PartyInventoryComponent
    ├── Inventory Slots Per Character
    └── Inventory Columns
```

`WBP_InventoryBag` ne possède plus de propriété `InventorySlotColumnCount`.

Règles :

- `Inventory Slots Per Character` définit la capacité réelle et identique de tous les personnages ;
- `Inventory Columns` définit uniquement la disposition visuelle ;
- le nombre de lignes est dérivé automatiquement ;
- la capacité en slots reste indépendante de la capacité de portage/du poids ;
- aucun second réglage de capacité ou de colonnes n'existe dans le widget.

Exemple 8 × 8 :

```text
Inventory Slots Per Character = 64
Inventory Columns             = 8
=> 8 lignes
```


## UI-INV2F — slots d'équipement supprimés

Les slots `Face`, `Earring1` et `Earring2` sont abandonnés et supprimés du modèle runtime, du stockage d'équipement, du paper doll, des tooltips, des actions contextuelles et des diagnostics.

Le paper doll canonique comporte désormais 15 slots :

```text
MainHand, OffHand,
Head, Chest, Legs, Feet,
Amulet, Ring1, Ring2,
Shoulders, Gloves, Belt, Cloak,
Shirt, Bracers
```



## UI-INVENTORY02.9 — projection rapide en place

Les changements de tri/filtre ne font plus partie de la clé de reconstruction de la grille. Les `GeneratedInventorySlotWidgets` sont conservés et réaffectés en place tant que capacité, colonnes, classe de slot et panneau restent inchangés.

Le tri pré-calcule ses clés par item, le compteur d'items visibles ne retrie plus l'inventaire et l'enregistrement d'un slot ne provoque plus de rafraîchissement global de tous les slots.

### UI-GLOBALHUD01.1 — largeur dynamique sans étirement

Le Persistent HUD calcule `floor((ViewportWidth - NavigationWidth) / ActionSlotWidth)`. Les boutons gardent leur largeur UMG réelle et restent jointifs. Le stockage d'un personnage grandit si nécessaire mais n'est jamais réduit automatiquement. Aucun `Fill` n'est utilisé pour répartir ou espacer les slots.


### UI-GLOBALHUD01.2 — séparation verticale des contrôles combat

Le HUD combat conserve PAM et Fin du tour, mais réserve 56 px au-dessus de la barre persistante. `Panel_CombatBottomRight` est désormais le conteneur canonique de ces contrôles ; `UGridCombatHudWidget` applique la clearance à ce conteneur unique et conserve sa translation UMG d'origine comme baseline. Le fallback historique qui déplaçait séparément PAM, Fin du tour et le texte de refus a été supprimé par UI-CODE-AUDIT01.


### UI-GLOBALHUD01.3 — suppression du chrome legacy du Combat HUD

`UGridCombatHudWidget` ne possède plus de navigation globale ni de barre d'actions visuelle. Ont été supprimés du contrat natif : `Panel_GlobalNavigation`, `Button_Nav*`, `Panel_Actions`, `ActionWidgetClass`, `HotbarActionWidgets`, `HotbarRow`, `HotbarSlotSpacing` et les handlers navigation associés.

`UGridPersistentHudWidget` est l'unique propriétaire de `ESC/I/K/G/M/J/H` et des widgets de slots. `ActionWidgetClass` doit y être configuré explicitement.

Le Combat HUD conserve uniquement : initiative, panneaux des combattants, PAM, fin de tour et le backend d'exécution/ciblage des actions. Le ciblage `Cell/Area` reste autoritaire en C++ via `TargetingPreview` et les méthodes Begin/Update/Confirm/Cancel ; l'ancien panneau texte optionnel `Panel_Targeting` n'existe plus dans le WBP courant et son fallback C++ a été supprimé par UI-CODE-AUDIT01.

La politique SaveGame du prototype est stricte : aucune migration des anciennes hotbars à 10 slots ou absentes. `FGridCombatHotbarBinding::SlotCount` est supprimé ; `MinimumSlotCount = 12` décrit uniquement le minimum du schéma courant.


### UI-COMBAT-UNIFY01 — contrats UMG du Combat HUD

`WBP_GridCombatActionPanel` reste un renderer réutilisable d'un seul membre du groupe. Il ne porte plus de racine viewport ni de position absolue : sa racine cible est `SizeBox_ActionPanel`. Les deux sorties de statut `Text_StatusEffects` et `Text_StatusFeedback` sont authored explicitement dans le WBP ; le fallback C++ qui tentait de fabriquer des `TextBlock` à l'exécution est supprimé.

`WBP_GridCombatHud` reste le propriétaire plein écran du combat et génère les quatre panneaux dans `Panel_PartyMembers`. L'espacement horizontal entre ces panneaux est réglable par `PartyMemberPanelSpacing` ; aucune seconde autorité de layout n'est créée.

Les paramètres éditables des deux classes utilisent désormais la même convention `EditDefaultsOnly` et les catégories `Combat|UI|Appearance`, `Combat|UI|Classes`, `Combat|UI|Initiative` et `Combat|UI|Layout`. Ils se règlent dans un Widget Blueprint via `Graph > My Blueprint > Show Inherited Variables > Default Value`.

Référence canonique : `docs/Design/UI_COMBAT_WIDGETS_CURRENT.md`.


### UI-COMBAT-UNIFY02 — autorité de layout unique

Le positionnement runtime ajouté par UI-COMBAT-LAYOUT01 est supprimé :
`PartyMembersPositionOffset`, `CombatControlsPositionOffset`,
`PersistentHudBottomClearance` et `ApplyBottomCombatLayout()` n'existent plus.

`WBP_GridCombatHud` doit utiliser une racine unique `Panel_CombatHud` de type
`Overlay`, sans `Canvas_Root` externe. Les deux surfaces basses sont placées
dans un même `HorizontalBox_CombatBottomBar`, aligné Bottom, avec un seul
Padding Bottom authored dans UMG. `Panel_PartyMembers` et
`Panel_CombatBottomRight` utilisent tous deux Vertical Alignment = Bottom.

`WBP_GridCombatActionPanel` conserve `SizeBox_ActionPanel` comme racine car il
s'agit d'un composant répétable à taille intrinsèque, et non d'un HUD plein
écran. Il ne porte aucune position viewport.


## Level Up non modal — RPG-LEVELUX01

```text
FRPGLevelUpService
    -> event source-aware
URPGLevelUpNotificationSubsystem
    -> queue transitoire uniquement
WBP_GridPersistentHud
    -> Notification_Progression
```

Il n'existe plus de `URPGLevelUpWidget`, de pause Level Up ni de watermark
`LastAcknowledgedLevel` durable.


## RPG-LEVELUX01 — clôture

RPG-LEVELUX01 est **CLOS**.

```text
FRPGLevelUpService
    -> event source-aware
URPGLevelUpNotificationSubsystem
    -> file transitoire de toasts uniquement
WBP_GridPersistentHud
    -> Notification_Progression
```

Supprimés définitivement :

```text
URPGLevelUpWidget
LastAcknowledgedLevel
RefreshFromPartyState()
ObservedPartyInventory
AcknowledgeNotification()
IsLevelUpModalOpen()
GetPendingLevelUpNotificationCount()
```

Le SaveGame courant est **v24 exact-match**. L'audit Automation du Content n'a
détecté aucune référence sérialisée aux anciens symboles Level-Up.


## RPG-ATTR01 — allocation des points de caractéristiques

RPG-ATTR01 est **CLOS**.

```text
Level
    -> FRPGAttributePointService::GetTotalPointsGranted()

Class.BaseAttributes + Race.AttributeBonuses
Character.Attributes
    -> balance dérivée

WBP_CharacterSheet
    -> Text_AttributePoints
    -> boutons − / +

UGridCharacterSheetWidget
    -> Safe Undo limité à la session courante
    -> refresh via l'unique RefreshInventory() canonique
```

Aucune monnaie Attribute Point n'est persistée. `Character.Attributes` reste
l'autorité durable. Le SaveGame reste **v24 exact-match**.

Validation finale : **10/10 Automation, 0 warning, 0 échec, PIE validé**.
