# MM4 - Charger partie et slots de sauvegarde

## 1. Objet

MM4 ajoute une gestion propre des slots de sauvegarde pour le bouton `Charger partie`.

Le menu principal reste un menu pur : il ne charge pas la sauvegarde lui-meme. Il affiche les slots disponibles, demande au `GameInstance` quel slot charger, puis ouvre la carte runtime.

Flux cible :

```text
WBP_MainMenu
-> Charger partie
-> WBP_LoadGameMenu
-> GetExistingPartySaveSlotInfos
-> selection d'un slot
-> RequestLoadPartySaveSlot(SlotName, UserIndex)
-> Open Level L_GrimrockEditor
-> GrimrockStartupModeComponent applique le slot au pawn
-> AGrimrockPartyPawn charge la sauvegarde selectionnee
```

---

## 2. Ajouts C++

### 2.1 FGrimrockSaveSlotInfo

Structure Blueprint ajoutee dans :

```text
Source/GrimrockPrototype/Public/Runtime/GrimrockGameInstance.h
```

Champs :

```text
SlotName
UserIndex
bExists
bIsDefaultSlot
DisplayName
```

Elle permet a un widget de liste de sauvegardes d'afficher les slots configurables sans connaitre les details du systeme de sauvegarde.

### 2.2 UGrimrockGameInstance

Fonctions Blueprint :

```text
GetPartySaveSlotInfos()
GetExistingPartySaveSlotInfos()
RequestContinueDefaultPartySaveSlot()
RequestLoadPartySaveSlot(SlotName, UserIndex)
SetPendingLoadSlot(SlotName, UserIndex)
HasPendingLoadSlot()
ConsumePendingLoadSlot(OutSlotName, OutUserIndex)
ClearPendingLoadSlot()
```

Slots configures par defaut :

```text
GrimrockParty
GrimrockParty_2
GrimrockParty_3
```

`RequestLoadPartySaveSlot` verifie que le slot existe, definit le slot pending, puis definit :

```text
PendingStartupMode = Continue
```

### 2.3 GrimrockStartupModeComponent

Le composant consomme maintenant :

```text
PendingStartupMode
PendingLoadSlot
```

Puis il applique au pawn :

```text
PartyStartupMode
PartySaveSlotName
PartySaveUserIndex
```

Cela garantit que `AGrimrockPartyPawn::BeginPlay()` charge le bon slot.

### 2.4 Widgets C++ MM4.1

Deux classes C++ servent de parents aux Blueprints UMG :

```text
Source/GrimrockPrototype/Public/UI/GrimrockLoadGameMenuWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockLoadGameMenuWidget.cpp
Source/GrimrockPrototype/Public/UI/GrimrockLoadGameSlotWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockLoadGameSlotWidget.cpp
```

`UGrimrockLoadGameMenuWidget` :

- recupere `GetExistingPartySaveSlotInfos()` ;
- cree une ligne par slot existant ;
- appelle `RequestLoadPartySaveSlot(SlotName, UserIndex)` au clic ;
- ouvre `L_GrimrockEditor` si la requete reussit ;
- gere `Button_Back` par `RemoveFromParent`.

`UGrimrockLoadGameSlotWidget` :

- affiche `DisplayName`, `SlotName` et l'etat du slot ;
- desactive le bouton si le slot n'existe pas ;
- emet `OnSaveSlotSelected(SlotName, UserIndex)` au clic.

---

## 3. Blueprint WBP_MainMenu

### 3.1 Event Construct

Pour garder MM3 compatible :

```text
Event Construct
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> HasDefaultPartySaveGame
-> SetHasValidSaveGame
```

Option MM4 plus complete :

```text
Event Construct
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> GetExistingPartySaveSlotInfos
-> Length > 0
-> SetHasValidSaveGame
```

Ainsi, `Charger partie` peut etre actif si au moins un slot existe, meme si le slot principal n'existe pas.

### 3.2 OnContinueRequested

Version recommandee MM4 :

```text
Event OnContinueRequested
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> RequestContinueDefaultPartySaveSlot
-> Branch
    True  -> Open Level by Name: L_GrimrockEditor
    False -> rester sur le menu
```

`Continuer` garde donc son sens simple : charger le slot principal `GrimrockParty`.

### 3.3 OnLoadGameRequested

```text
Event OnLoadGameRequested
-> Create Widget WBP_LoadGameMenu
-> Add To Viewport
```

Ne pas ouvrir `L_GrimrockEditor` directement depuis le bouton `Charger partie`. Le choix du slot doit se faire dans `WBP_LoadGameMenu`.

---

## 4. Creer WBP_LoadGameMenu

### 4.1 Creer la ligne de slot

Creer :

```text
Content/GrimrockPrototype/UI/WBP_LoadGameSlotRow
```

Parent class :

```text
GrimrockLoadGameSlotWidget
```

Hierarchy minimale :

```text
Button_LoadSlot
-> HorizontalBox
   -> Text_DisplayName
   -> Text_SlotName
   -> Text_Status
```

Les noms doivent correspondre exactement aux variables `BindWidgetOptional`.

Aucun Graph n'est requis dans `WBP_LoadGameSlotRow` pour la version MM4.1.

### 4.2 Creer le menu de chargement

Creer :

```text
Content/GrimrockPrototype/UI/WBP_LoadGameMenu
```

Parent class :

```text
GrimrockLoadGameMenuWidget
```

Hierarchy minimale :

```text
Root Canvas ou Border
-> VerticalBox
   -> Text_Title
   -> Text_EmptyState
   -> ScrollBox
      -> VerticalBox_SaveSlots
   -> Button_Back
```

Noms obligatoires :

```text
VerticalBox_SaveSlots
Button_Back
Text_EmptyState
```

Dans les defaults de `WBP_LoadGameMenu` :

```text
Save Slot Entry Widget Class = WBP_LoadGameSlotRow
Runtime Level Name           = L_GrimrockEditor
```

Aucun Graph n'est requis pour remplir la liste ou charger le slot : le parent C++ le fait.

### 4.3 Texte conseille

```text
Text_Title      = Charger partie
Text_EmptyState = Aucune sauvegarde disponible.
Button_Back     = Retour
```

`Text_EmptyState` est automatiquement visible uniquement si aucun slot existant n'est trouve.

---

## 5. Logs de validation

Filtrer l'Output Log avec :

```text
LoadGameMenu
GrimrockGameInstance
GrimrockStartupMode
PartySave
```

Pour `Continuer` :

```text
GrimrockGameInstance PendingLoadSlot Set Slot=GrimrockParty UserIndex=0
GrimrockGameInstance PendingStartupMode Set Mode=1
GrimrockGameInstance LoadSlot Requested Slot=GrimrockParty UserIndex=0
GrimrockGameInstance PendingStartupMode Consumed Mode=1 NextMode=1
GrimrockGameInstance PendingLoadSlot Consumed Slot=GrimrockParty UserIndex=0
GrimrockStartupMode AppliedSaveSlot Pawn=... Slot=GrimrockParty UserIndex=0
GrimrockStartupMode Applied Pawn=... Mode=1 Slot=GrimrockParty UserIndex=0
PartySave Continued Slot=GrimrockParty CharacterCount=...
```

Pour `Charger partie` avec le slot 2 :

```text
LoadGameMenu Refreshed Widget=... ExistingSlots=...
LoadGameMenu OpenRuntimeLevel Slot=GrimrockParty_2 UserIndex=0 Level=L_GrimrockEditor
GrimrockGameInstance PendingLoadSlot Set Slot=GrimrockParty_2 UserIndex=0
GrimrockGameInstance PendingStartupMode Set Mode=1
GrimrockGameInstance LoadSlot Requested Slot=GrimrockParty_2 UserIndex=0
GrimrockStartupMode AppliedSaveSlot Pawn=... Slot=GrimrockParty_2 UserIndex=0
PartySave Continued Slot=GrimrockParty_2 CharacterCount=...
```

---

## 6. Captures de validation MM4

Conserver :

```text
01_MM4_MainMenu_LoadGameButton.png
02_MM4_LoadGameMenu_List.png
03_MM4_WBP_LoadGameMenu_Hierarchy.png
04_MM4_WBP_LoadGameSlotRow_Hierarchy.png
05_MM4_OutputLog_SelectedSlot.png
```

Capture optionnelle :

```text
06_MM4_BP_GrimrockGameInstance_ConfiguredSlots.png
```

---

## 7. Critere final MM4

MM4 est valide lorsque :

- `Charger partie` ouvre `WBP_LoadGameMenu` ;
- le widget liste les slots existants via `GetExistingPartySaveSlotInfos()` ;
- cliquer un slot appelle `RequestLoadPartySaveSlot(SlotName, UserIndex)` ;
- le niveau `L_GrimrockEditor` s'ouvre seulement si la requete retourne `true` ;
- `GrimrockStartupModeComponent` applique le slot selectionne au pawn ;
- le log `PartySave Continued Slot=<slot selectionne>` apparait ;
- le menu principal ne cherche toujours aucun `GrimrockPartyPawn` ;
- le menu principal ne charge jamais directement une sauvegarde.

Statut :

```text
MM4 valide apres creation des deux Blueprints UMG et logs conformes.
```