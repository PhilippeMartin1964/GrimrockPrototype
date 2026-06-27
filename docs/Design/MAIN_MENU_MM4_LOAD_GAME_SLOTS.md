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

Important : `WBP_LoadGameMenu` est un modal plein ecran ajoute par-dessus le menu principal. Si le widget n'a pas un fond plein ecran et un panneau centre, il apparaitra comme des textes flottants par-dessus `WBP_MainMenu`.

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

## 3. WBP_MainMenu

### 3.1 Event Construct obligatoire

Dans `WBP_MainMenu`, il faut initialiser l'etat des boutons au lancement du menu.

Version MM4 recommandee :

```text
Event Construct
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> GetExistingPartySaveSlotInfos
-> Length
-> > 0
-> SetHasValidSaveGame
```

Raison : `Charger partie` doit etre actif si au moins une sauvegarde existe, meme si `GrimrockParty` n'existe pas mais `GrimrockParty_2` existe.

Controle important dans le Designer de `WBP_MainMenu` : les boutons doivent avoir exactement les noms attendus par le C++ :

```text
Button_Continue
Button_LoadGame
```

Sinon `SetHasValidSaveGame` ne pourra pas desactiver correctement les boutons.

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

`Continuer` garde son sens simple : charger le slot principal `GrimrockParty`.

### 3.3 OnLoadGameRequested

Graph attendu :

```text
Event OnLoadGameRequested
-> Create Widget WBP_LoadGameMenu
-> Add To Viewport
```

Reglage conseille sur `Add To Viewport` :

```text
ZOrder = 100
```

---

## 4. Creer WBP_LoadGameSlotRow

Creer :

```text
Content/GrimrockPrototype/UI/WBP_LoadGameSlotRow
```

Parent class :

```text
GrimrockLoadGameSlotWidget
```

### 4.1 Hierarchy exacte recommandee

```text
WBP_LoadGameSlotRow
-> SizeBox_Row
   -> Button_LoadSlot
      -> Border_RowBackground
         -> HorizontalBox_RowContent
            -> VerticalBox_Texts
               -> Text_DisplayName
               -> Text_SlotName
            -> Spacer_RowFill
            -> Text_Status
```

Noms obligatoires lus par le C++ :

```text
Button_LoadSlot
Text_DisplayName
Text_SlotName
Text_Status
```

Tous les quatre doivent avoir :

```text
Is Variable = true
```

`SizeBox_Row`, `Border_RowBackground`, `HorizontalBox_RowContent`, `VerticalBox_Texts` et `Spacer_RowFill` sont recommandes, mais leurs noms ne sont pas lus par le C++.

### 4.2 Reglages visuels obligatoires de la ligne

Le gros carre blanc sans nom signifie que `WBP_LoadGameSlotRow` est bien cree, mais que son contenu n'est pas visible.

Causes les plus probables :

- `Text_DisplayName`, `Text_SlotName` ou `Text_Status` ne portent pas exactement ces noms ;
- les widgets texte ne sont pas coches `Is Variable` ;
- les textes sont blancs sur un fond blanc ;
- les textes ne sont pas dans le contenu du `Button_LoadSlot` ;
- `Save Slot Entry Widget Class` pointe vers un autre widget que `WBP_LoadGameSlotRow`.

Reglages conseilles :

```text
SizeBox_Row
- Height Override = 72
- Width Override  = non renseigne

Button_LoadSlot
- Is Variable = true
- Horizontal Alignment = Fill
- Vertical Alignment   = Fill

Border_RowBackground
- Brush Color = gris tres fonce / charbon
- Alpha = 0.90 a 1.00
- Padding = 14 / 8 / 14 / 8

HorizontalBox_RowContent
- Horizontal Alignment = Fill
- Vertical Alignment   = Center

VerticalBox_Texts
- Size = Fill

Text_DisplayName
- Is Variable = true
- Text = Sauvegarde TEST
- Color = blanc
- Font Size = 24

Text_SlotName
- Is Variable = true
- Text = Slot TEST
- Color = gris clair
- Font Size = 15

Text_Status
- Is Variable = true
- Text = Disponible
- Color = blanc ou vert pale
- Font Size = 16
- Justification = Right
```

Apres compilation, les textes de test doivent etre visibles dans le Designer. En PIE, le C++ les remplacera par :

```text
Text_DisplayName = Sauvegarde principale / Sauvegarde 2 / Sauvegarde 3
Text_SlotName    = GrimrockParty / GrimrockParty_2 / GrimrockParty_3
Text_Status      = Disponible
```

### 4.3 Test rapide si la ligne reste blanche

Dans `WBP_LoadGameSlotRow`, mettez temporairement :

```text
Text_DisplayName = TEST DISPLAY
Text_SlotName    = TEST SLOT
Text_Status      = TEST STATUS
```

Puis choisissez une couleur visible :

```text
Text color = rouge ou noir
```

Si le texte de test n'apparait pas, le probleme est dans la hierarchy ou le style du widget.

Si le texte de test apparait dans le Designer mais pas en PIE, verifier les noms exacts et le parent class.

Checklist de correction :

```text
WBP_LoadGameSlotRow Parent Class = GrimrockLoadGameSlotWidget
Button_LoadSlot existe et Is Variable = true
Text_DisplayName existe et Is Variable = true
Text_SlotName existe et Is Variable = true
Text_Status existe et Is Variable = true
Les TextBlock sont enfants du contenu de Button_LoadSlot
WBP_LoadGameMenu / Class Defaults / Save Slot Entry Widget Class = WBP_LoadGameSlotRow
```

Aucun Graph n'est requis dans `WBP_LoadGameSlotRow` pour la version MM4.1.

---

## 5. Creer WBP_LoadGameMenu

Creer :

```text
Content/GrimrockPrototype/UI/WBP_LoadGameMenu
```

Parent class :

```text
GrimrockLoadGameMenuWidget
```

### 5.1 Hierarchy exacte recommandee

```text
WBP_LoadGameMenu
-> CanvasPanel_Root
   -> Border_ModalDim
      -> SizeBox_Dialog
         -> Border_DialogBackground
            -> VerticalBox_Dialog
               -> Text_Title
               -> Text_Subtitle
               -> Text_ErrorMessage
               -> Border_ListFrame
                  -> Overlay_ListArea
                     -> ScrollBox_SaveSlots
                        -> VerticalBox_SaveSlots
                     -> Text_EmptyState
               -> HorizontalBox_Footer
                  -> Spacer_FooterFill
                  -> Button_Back
                     -> Text_Back
```

Noms obligatoires lus par le C++ :

```text
VerticalBox_SaveSlots
Button_Back
Text_EmptyState
```

Nom conseille pour le message d'erreur Blueprint :

```text
Text_ErrorMessage
```

### 5.2 Reglages CanvasPanel_Root

Dans le slot Canvas de `Border_ModalDim` :

```text
Anchors       = Full Screen
Offsets Left  = 0
Offsets Top   = 0
Offsets Right = 0
Offsets Bottom= 0
Alignment     = 0,0
```

### 5.3 Reglages Border_ModalDim

```text
Brush Color = noir ou gris tres fonce
Alpha       = 0.70 a 0.85
Padding     = 0
```

### 5.4 Reglages SizeBox_Dialog

```text
Horizontal Alignment = Center
Vertical Alignment   = Center
Width Override       = 720
Height Override      = 520
```

### 5.5 Reglages Border_DialogBackground

```text
Brush Color = gris tres fonce ou noir charbon
Alpha       = 0.95
Padding     = 32
```

### 5.6 Reglages VerticalBox_Dialog

```text
Text_Title
- Text = Charger partie
- Font Size = 36 a 44
- Justification = Center

Text_Subtitle
- Text = Selectionnez une sauvegarde a charger.
- Font Size = 18 a 22
- Justification = Center

Text_ErrorMessage
- Text = Impossible de charger cette sauvegarde.
- Visibility = Collapsed
- Color = rouge clair ou orange
- Font Size = 18 a 20

Border_ListFrame
- Padding = 8
- Size rule dans VerticalBox = Fill

ScrollBox_SaveSlots
- Visibility = Visible

VerticalBox_SaveSlots
- Is Variable = true

Text_EmptyState
- Text = Aucune sauvegarde disponible.
- Visibility = Collapsed dans le Designer
- Justification = Center
- Font Size = 22 a 28

Button_Back
- Text_Back = Retour
```

Important : `VerticalBox_SaveSlots` doit etre un enfant du `ScrollBox_SaveSlots`. Le C++ ajoute les lignes dans `VerticalBox_SaveSlots`, pas directement dans le `ScrollBox`.

### 5.7 Defaults de WBP_LoadGameMenu

Dans les details du widget `WBP_LoadGameMenu`, regler :

```text
Save Slot Entry Widget Class = WBP_LoadGameSlotRow
Runtime Level Name           = L_GrimrockEditor
```

Sans `Save Slot Entry Widget Class`, la liste restera vide et le log indiquera :

```text
LoadGameMenu Refresh Failed Reason=NoSaveSlotEntryWidgetClass
```

---

## 6. Graph de WBP_LoadGameMenu

Le parent C++ remplit la liste et gere le bouton `Retour`. Le Graph Blueprint sert seulement a afficher une erreur lisible si `RequestLoadPartySaveSlot` echoue.

### 6.1 Event Construct

```text
Event Construct
-> Set Visibility
   Target = Text_ErrorMessage
   In Visibility = Collapsed
```

### 6.2 Event OnLoadSlotRequestFailed

```text
Event OnLoadSlotRequestFailed
-> Format Text
   Format = Impossible de charger la sauvegarde : {Slot}
   Slot   = Slot Name
-> Set Text
   Target  = Text_ErrorMessage
   In Text = Return Value de Format Text
-> Set Visibility
   Target        = Text_ErrorMessage
   In Visibility = Visible
```

Point important : pour creer `Set Text`, tirer le fil depuis la variable `Text_ErrorMessage`. Le `Target` du noeud doit etre `Text_ErrorMessage`, pas `self`.

---

## 7. Pourquoi le menu apparait par-dessus le menu principal ?

C'est normal : `WBP_LoadGameMenu` est ajoute par `Add To Viewport`. Il est donc affiche par-dessus `WBP_MainMenu`.

Ce qui n'est pas normal, c'est de voir les textes du menu principal au travers comme sur une superposition brouillonne. Cela signifie que `WBP_LoadGameMenu` n'a pas encore :

```text
Border_ModalDim plein ecran
SizeBox_Dialog centre
Border_DialogBackground opaque
```

Correction : appliquer la hierarchy et les reglages de la section 5.

---

## 8. Pourquoi une seule sauvegarde est affichee ?

C'est normal si un seul slot existe vraiment sur disque.

`GetExistingPartySaveSlotInfos()` ne retourne que les slots existants parmi :

```text
GrimrockParty
GrimrockParty_2
GrimrockParty_3
```

Donc si seule la sauvegarde principale a ete creee, `WBP_LoadGameMenu` affiche une seule ligne.

---

## 9. Pourquoi aucune sauvegarde n'est indiquee ?

`Aucune sauvegarde disponible` est normal si :

- aucune creation de personnage n'a encore ete finalisee ;
- `SaveCurrentGame` n'a jamais reussi ;
- la sauvegarde existe sous un autre nom de slot ;
- `Save Slot Entry Widget Class` n'est pas renseigne ;
- le projet tourne avec un autre `BP_GrimrockGameInstance` que celui reparente sur `GrimrockGameInstance`.

Pour creer une sauvegarde test :

```text
Nouvelle partie
-> finaliser la creation du personnage
-> attendre le log PartySave InitialCharacter ou PartySave Saved
-> revenir au menu principal
-> Charger partie
```

Logs utiles :

```text
PartySave Saved Slot=GrimrockParty
PartySave InitialCharacter Failed
LoadGameMenu Refreshed Widget=... ExistingSlots=0
LoadGameMenu Refresh Failed Widget=... Reason=NoSaveSlotEntryWidgetClass
```

---

## 10. Logs de validation

Filtrer l'Output Log avec :

```text
LoadGameMenu
GrimrockGameInstance
GrimrockStartupMode
PartySave
```

Pour `Charger partie` avec le slot principal :

```text
LoadGameMenu Refreshed Widget=... ExistingSlots=1
LoadGameMenu OpenRuntimeLevel Slot=GrimrockParty UserIndex=0 Level=L_GrimrockEditor
GrimrockGameInstance PendingLoadSlot Set Slot=GrimrockParty UserIndex=0
GrimrockGameInstance PendingStartupMode Set Mode=1
GrimrockGameInstance LoadSlot Requested Slot=GrimrockParty UserIndex=0
GrimrockStartupMode AppliedSaveSlot Pawn=... Slot=GrimrockParty UserIndex=0
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

## 11. Captures de validation MM4

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

## 12. Critere final MM4

MM4 est valide lorsque :

- `Charger partie` ouvre `WBP_LoadGameMenu` ;
- `WBP_LoadGameMenu` affiche un modal plein ecran propre, sans superposition brouillonne ;
- chaque ligne de sauvegarde affiche au moins `DisplayName`, `SlotName` et `Disponible` ;
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