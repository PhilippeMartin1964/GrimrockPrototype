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

Nouvelles fonctions Blueprint :

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

Le composant consomme maintenant deux choses au lancement du pawn :

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

Pour la premiere version MM4 :

```text
Event OnLoadGameRequested
-> Create Widget WBP_LoadGameMenu
-> Add To Viewport
```

Ne pas ouvrir `L_GrimrockEditor` directement depuis le bouton `Charger partie`. Le choix du slot doit se faire dans `WBP_LoadGameMenu`.

---

## 4. Blueprint WBP_LoadGameMenu

Creer :

```text
Content/GrimrockPrototype/UI/WBP_LoadGameMenu
```

Parent conseille :

```text
UserWidget
```

Contenu minimal :

```text
VerticalBox_SaveSlots
Button_Back
```

Au `Event Construct` :

```text
Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> GetExistingPartySaveSlotInfos
-> ForEachLoop
-> creer une ligne de slot
```

Pour chaque ligne, afficher au minimum :

```text
DisplayName
SlotName
```

Au clic sur une ligne :

```text
RequestLoadPartySaveSlot(SlotName, UserIndex)
-> Branch
    True  -> Open Level by Name: L_GrimrockEditor
    False -> rafraichir la liste ou afficher une erreur
```

`Button_Back` :

```text
Remove From Parent
```

---

## 5. Logs de validation

Filtrer l'Output Log avec :

```text
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
03_MM4_LoadGameMenu_SelectedSlotGraph.png
04_MM4_OutputLog_SelectedSlot.png
```

Capture optionnelle :

```text
05_MM4_BP_GrimrockGameInstance_ConfiguredSlots.png
```

---

## 7. Critere final MM4

MM4 est valide lorsque :

- `Charger partie` ouvre un widget de selection de sauvegarde ;
- le widget liste les slots existants via `GetExistingPartySaveSlotInfos()` ;
- cliquer un slot appelle `RequestLoadPartySaveSlot(SlotName, UserIndex)` ;
- le niveau `L_GrimrockEditor` s'ouvre seulement si la requete retourne `true` ;
- `GrimrockStartupModeComponent` applique le slot selectionne au pawn ;
- le log `PartySave Continued Slot=<slot selectionne>` apparait ;
- le menu principal ne cherche toujours aucun `GrimrockPartyPawn` ;
- le menu principal ne charge jamais directement une sauvegarde.

Statut :

```text
MM4 valide apres branchement Blueprint et logs conformes.
```