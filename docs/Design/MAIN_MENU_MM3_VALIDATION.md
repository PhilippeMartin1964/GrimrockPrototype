# MM3 - Validation finale Continuer

## 1. Objet

MM3 valide le bouton `Continuer` du menu principal.

Le menu principal ne charge pas lui-meme la sauvegarde. Il verifie seulement si une sauvegarde existe, active ou desactive les boutons, puis transmet le mode de demarrage au niveau runtime via `UGrimrockGameInstance`.

Flux attendu :

```text
PIE
-> L_MainMenu
-> WBP_MainMenu
-> HasDefaultPartySaveGame
-> Continuer actif si GrimrockParty existe
-> clic Continuer
-> SetPendingStartupMode(Continue)
-> Open Level L_GrimrockEditor
-> BP_GrimrockPartyPawn
-> GrimrockStartupModeComponent consomme Continue
-> AGrimrockPartyPawn charge la sauvegarde
```

---

## 2. Etat C++ valide

Fichiers concernes :

```text
Source/GrimrockPrototype/Public/Runtime/GrimrockGameInstance.h
Source/GrimrockPrototype/Private/Runtime/GrimrockGameInstance.cpp
Source/GrimrockPrototype/Public/Runtime/GrimrockStartupModeComponent.h
Source/GrimrockPrototype/Private/Runtime/GrimrockStartupModeComponent.cpp
Source/GrimrockPrototype/Public/UI/GrimrockMainMenuWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockMainMenuWidget.cpp
```

Points valides :

- `UGrimrockGameInstance::HasDefaultPartySaveGame()` teste le slot par defaut ;
- le slot par defaut est `GrimrockParty` ;
- l'index utilisateur par defaut est `0` ;
- la detection utilise `UGameplayStatics::DoesSaveGameExist` ;
- `UGrimrockMainMenuWidget::SetHasValidSaveGame()` rafraichit les boutons ;
- `Button_Continue` est desactive si aucune sauvegarde valide n'existe ;
- `Button_LoadGame` suit le meme etat, en attendant MM4 ;
- `HandleContinueClicked()` ignore le clic si aucune sauvegarde valide n'existe ;
- `UGrimrockStartupModeComponent` consomme le mode du `GameInstance` au lancement du pawn runtime ;
- `AGrimrockPartyPawn::BeginPlay()` charge la sauvegarde uniquement quand `PartyStartupMode == Continue` et qu'un slot existe.

MM2.2 est donc considere comme raccorde par le composant :

```text
BP_GrimrockPartyPawn
-> GrimrockStartupModeComponent
-> UGrimrockGameInstance::ConsumePendingStartupMode()
-> PartyStartupMode
```

---

## 3. Reglages Blueprint requis

### 3.1 BP_GrimrockGameInstance

```text
Parent Class = GrimrockGameInstance
```

Dans les Project Settings :

```text
Maps & Modes
-> Game Instance Class = BP_GrimrockGameInstance
```

### 3.2 BP_GrimrockPartyPawn

Ajouter le composant :

```text
GrimrockStartupModeComponent
```

Nom conseille :

```text
StartupModeComponent
```

### 3.3 WBP_MainMenu - Construct

Graph attendu :

```text
Event Construct
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> HasDefaultPartySaveGame
-> SetHasValidSaveGame
```

### 3.4 WBP_MainMenu - Continuer

Graph attendu :

```text
Event OnContinueRequested
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> SetPendingStartupMode(Continue)
-> Open Level by Name: L_GrimrockEditor
```

Ne pas utiliser :

```text
Get Actor Of Class GrimrockPartyPawn
LoadCurrentGame
StartNewGame
```

Le menu reste un menu pur.

---

## 4. Captures de validation a conserver

Conserver quatre captures pour cloturer MM3 :

```text
01_MM3_PIE_MainMenu_NoSave.png
```

Menu principal lance depuis PIE, sans sauvegarde valide. Le bouton `Continuer` doit etre grise.

```text
02_MM3_PIE_MainMenu_WithSave.png
```

Menu principal lance depuis PIE, avec une sauvegarde `GrimrockParty`. Le bouton `Continuer` doit etre actif.

```text
03_MM3_WBP_MainMenu_Construct.png
```

Graph `Event Construct` montrant `HasDefaultPartySaveGame -> SetHasValidSaveGame`.

```text
04_MM3_WBP_MainMenu_Continue.png
```

Graph `OnContinueRequested` montrant `SetPendingStartupMode(Continue) -> Open Level L_GrimrockEditor`.

Capture optionnelle utile :

```text
05_MM3_BP_GrimrockPartyPawn_StartupModeComponent.png
```

Elle montre que `GrimrockStartupModeComponent` est bien ajoute au pawn.

---

## 5. Filtres Output Log

Dans Unreal, filtrer l'Output Log avec :

```text
GrimrockGameInstance
GrimrockStartupMode
PartySave
```

Pour le flux `Continuer`, les logs attendus sont :

```text
GrimrockGameInstance PendingStartupMode Set Mode=1
GrimrockGameInstance PendingStartupMode Consumed Mode=1 NextMode=1
GrimrockStartupMode Applied Pawn=... Mode=1
PartySave Continued Slot=GrimrockParty CharacterCount=...
```

Pour le flux `Nouvelle partie`, utile en regression MM2 :

```text
GrimrockGameInstance PendingStartupMode Set Mode=0
GrimrockGameInstance PendingStartupMode Consumed Mode=0 NextMode=1
GrimrockStartupMode Applied Pawn=... Mode=0
```

---

## 6. Nettoyage valide

Aucun nettoyage C++ supplementaire n'est necessaire pour MM3 si les logs ci-dessus apparaissent.

Les anciennes notes indiquant que le pawn doit encore etre raccorde sont considerees comme obsoletes depuis MM2.2. Le raccord effectif est maintenant fait par `GrimrockStartupModeComponent` ajoute a `BP_GrimrockPartyPawn`.

---

## 7. Critere final MM3

MM3 est valide lorsque :

- PIE demarre sur `L_MainMenu` ;
- sans sauvegarde, `Continuer` est grise ;
- avec sauvegarde `GrimrockParty`, `Continuer` est actif ;
- `OnContinueRequested` definit `PendingStartupMode = Continue` ;
- `OnContinueRequested` ouvre `L_GrimrockEditor` ;
- `GrimrockStartupModeComponent` applique `Mode=1` au pawn ;
- `AGrimrockPartyPawn` charge la sauvegarde ;
- l'Output Log contient `PartySave Continued Slot=GrimrockParty` ;
- `WBP_CharacterCreation` ne reapparait pas lors d'un vrai Continue avec sauvegarde complete.

Statut :

```text
MM3 valide si captures et logs conformes.
```