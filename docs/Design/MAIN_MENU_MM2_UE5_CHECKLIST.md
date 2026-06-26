# MM2 - Nouvelle partie vers CC7

## 1. Objet

MM2 branche le bouton `Nouvelle partie` du menu principal vers le niveau runtime jouable, en passant par un `GameInstance` qui transporte le choix du joueur entre la carte de menu et la carte de jeu.

La solution definitive retenue est :

```text
PIE
-> L_MainMenu
-> BP_MainMenuGameMode
-> BP_MainMenuPlayerController
-> WBP_MainMenu
-> clic Nouvelle partie
-> BP_GrimrockGameInstance.PendingStartupMode = NewGame
-> Open Level L_GrimrockEditor
-> GrimrockPartyPawn applique le mode NewGame
-> ShowInitialCharacterCreationWidget
```

Le menu principal ne doit jamais appeler directement `StartNewGame` sur un `GrimrockPartyPawn`.

---

## 2. Decision de conception definitive

Le projet utilise deux cartes distinctes :

```text
L_MainMenu       = carte de menu pur
L_GrimrockEditor = carte runtime jouable
```

`L_MainMenu` ne contient pas de gameplay.

`L_GrimrockEditor` contient le donjon, le runtime actor, le pawn, l'inventaire, les interactions et la creation initiale du personnage.

Le choix du joueur dans le menu est transmis par :

```text
BP_GrimrockGameInstance.PendingStartupMode
```

Raison : le `GameInstance` survit au `Open Level`, alors que le widget de menu, la carte de menu et leurs acteurs sont detruits.

---

## 3. Roles des elements

### 3.1 L_MainMenu

Role : afficher uniquement le menu principal.

Doit contenir :

```text
BP_MainMenuGameMode
BP_MainMenuPlayerController
WBP_MainMenu cree au BeginPlay du PlayerController
```

Ne doit pas contenir :

```text
GrimrockPartyPawn
GridLevelRuntimeActor
donjon runtime
creation de personnage automatique
```

### 3.2 BP_MainMenuGameMode

Role : GameMode propre au menu principal.

Configuration attendue :

```text
Default Pawn Class      = None ou SpectatorPawn
Player Controller Class = BP_MainMenuPlayerController
```

Point essentiel :

```text
BP_MainMenuGameMode ne doit jamais utiliser BP_GrimrockPartyPawn.
```

### 3.3 BP_MainMenuPlayerController

Role : creer le widget de menu et configurer l'input UI.

BeginPlay attendu :

```text
Event BeginPlay
-> Create Widget WBP_MainMenu
-> Add To Viewport
-> Set Input Mode UI Only
-> Set Show Mouse Cursor true
```

Recommandation : connecter aussi le `Return Value` de `Create Widget` sur `In Widget to Focus` du noeud `Set Input Mode UI Only`.

### 3.4 WBP_MainMenu

Role : afficher les boutons du menu principal.

Pour MM2, seul le bouton `Nouvelle partie` est branche.

`WBP_MainMenu` ne doit pas :

- chercher un `GrimrockPartyPawn` ;
- appeler `StartNewGame` ;
- creer le personnage ;
- modifier `PartyInventoryState` ;
- ouvrir directement le widget de creation de personnage.

### 3.5 BP_GrimrockGameInstance

Role : transporter le mode de demarrage entre `L_MainMenu` et `L_GrimrockEditor`.

Variable requise :

```text
PendingStartupMode : EGrimrockPartyStartupMode
```

Valeur utilisee par MM2 :

```text
NewGame
```

### 3.6 L_GrimrockEditor

Role : carte runtime jouable.

Doit utiliser le GameMode runtime normal, avec :

```text
Default Pawn Class = BP_GrimrockPartyPawn
```

Apres chargement de la carte, le pawn applique le mode de demarrage puis affiche la creation initiale du personnage si necessaire.

---

## 4. Graph definitif de WBP_MainMenu

Dans `WBP_MainMenu`, l'evenement doit etre :

```text
Event OnNewGameRequested
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> Set PendingStartupMode = NewGame
-> Open Level by Name: L_GrimrockEditor
```

Il ne faut plus utiliser :

```text
Get Actor Of Class: GrimrockPartyPawn
StartNewGame
Remove From Parent avant Open Level
```

`Remove From Parent` est inutile ici, car `Open Level` detruit la carte de menu et ses widgets.

---

## 5. Reglages projet requis

Dans :

```text
Edit
-> Project Settings
-> Maps & Modes
```

Reglages attendus :

```text
Game Instance Class = BP_GrimrockGameInstance
Editor Startup Map  = L_MainMenu
Game Default Map    = L_MainMenu
```

Ces reglages assurent que le jeu demarre toujours par le menu principal.

---

## 6. Reglages de L_MainMenu

Dans `L_MainMenu` :

```text
World Settings
-> GameMode Override = BP_MainMenuGameMode
```

Le Level Blueprint de `L_MainMenu` doit rester vide autant que possible.

La creation de `WBP_MainMenu` doit etre dans :

```text
BP_MainMenuPlayerController::BeginPlay
```

et non dans le Level Blueprint.

---

## 7. Reglages de L_GrimrockEditor

Dans `L_GrimrockEditor` :

```text
World Settings
-> GameMode Override = GameMode runtime du jeu
```

Ce GameMode runtime doit utiliser :

```text
Default Pawn Class = BP_GrimrockPartyPawn
```

`L_GrimrockEditor` est la seule carte qui doit contenir ou spawner le pawn de gameplay.

---

## 8. Lien avec CC7

MM2 ne cree pas encore le vrai wizard CC7.

Le flux actuel est :

```text
Open Level L_GrimrockEditor
-> GrimrockPartyPawn BeginPlay
-> mode NewGame
-> ShowInitialCharacterCreationWidget
-> CharacterCreationWidgetClass
```

Quand `WBP_CharacterCreationWizard` sera disponible, il remplacera le widget actuellement configure dans :

```text
AGrimrockPartyPawn::CharacterCreationWidgetClass
```

Le menu principal restera inchange : il continuera seulement a definir `PendingStartupMode` puis a ouvrir la carte runtime.

---

## 9. Point C++ a consolider

Pour que la solution soit completement robuste, `AGrimrockPartyPawn` doit lire le `PendingStartupMode` du `GameInstance` au debut de son `BeginPlay`, avant sa logique de chargement ou de nouvelle partie.

Logique cible :

```cpp
if (UGrimrockGameInstance* GI = GetGameInstance<UGrimrockGameInstance>())
{
    PartyStartupMode = GI->ConsumePendingStartupMode();
}
```

Puis le `BeginPlay` existant continue normalement.

But : eviter que le menu ait besoin de connaitre un pawn qui n'existe pas encore.

---

## 10. Checklist UE5

### A. GameInstance

1. Creer `BP_GrimrockGameInstance`.
2. Parent class : `GameInstance`.
3. Ajouter la variable :

```text
PendingStartupMode : EGrimrockPartyStartupMode
```

4. Valeur par defaut recommandee :

```text
Continue
```

5. Dans Project Settings / Maps & Modes :

```text
Game Instance Class = BP_GrimrockGameInstance
```

### B. Carte menu

1. Creer ou ouvrir :

```text
Content/GrimrockPrototype/Maps/L_MainMenu
```

2. Assigner :

```text
World Settings -> GameMode Override = BP_MainMenuGameMode
```

3. Verifier qu'aucun `BP_GrimrockPartyPawn` n'est place dans la carte.

### C. GameMode menu

Creer ou verifier :

```text
BP_MainMenuGameMode
```

Reglages :

```text
Default Pawn Class      = None ou SpectatorPawn
Player Controller Class = BP_MainMenuPlayerController
```

### D. PlayerController menu

Dans `BP_MainMenuPlayerController::BeginPlay` :

```text
Create Widget WBP_MainMenu
-> Add To Viewport
-> Set Input Mode UI Only
-> Set Show Mouse Cursor true
```

Connecter le widget cree sur `In Widget to Focus` si possible.

### E. WBP_MainMenu

Dans `OnNewGameRequested` :

```text
Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> Set PendingStartupMode = NewGame
-> Open Level by Name: L_GrimrockEditor
```

Supprimer toute logique :

```text
Get Actor Of Class: GrimrockPartyPawn
StartNewGame
Remove From Parent
```

### F. Carte runtime

Dans `L_GrimrockEditor` :

```text
World Settings -> GameMode Override = GameMode runtime
Default Pawn Class = BP_GrimrockPartyPawn
```

---

## 11. Test attendu

### Au lancement PIE

```text
PIE
-> L_MainMenu
-> WBP_MainMenu
```

Resultat attendu :

```text
Le menu principal apparait.
WBP_CharacterCreation n'apparait pas.
Aucun donjon runtime n'est charge.
Aucun GrimrockPartyPawn de gameplay n'est actif dans la carte menu.
```

### Au clic Nouvelle partie

```text
WBP_MainMenu
-> PendingStartupMode = NewGame
-> Open Level L_GrimrockEditor
```

Puis :

```text
L_GrimrockEditor
-> BP_GrimrockPartyPawn
-> NewGame
-> WBP_CharacterCreation ou futur WBP_CharacterCreationWizard
```

Resultat attendu :

```text
Le menu disparait.
La carte runtime se charge.
La creation de personnage apparait.
```

---

## 12. Captures attendues

Pour valider MM2, transmettre :

1. PIE montrant `WBP_MainMenu` au demarrage ;
2. `OnNewGameRequested` avec `GameInstance -> PendingStartupMode -> Open Level` ;
3. `BP_MainMenuPlayerController::BeginPlay` avec creation du menu ;
4. ecran de creation de personnage apres clic sur `Nouvelle partie` ;
5. reglages Maps & Modes montrant `BP_GrimrockGameInstance` et `L_MainMenu`.

---

## 13. Critere de validation MM2

MM2 est valide lorsque :

- `PIE` demarre sur `L_MainMenu` ;
- `L_MainMenu` affiche uniquement le menu principal ;
- `WBP_CharacterCreation` n'apparait pas au lancement du menu ;
- `OnNewGameRequested` ne cherche aucun `GrimrockPartyPawn` ;
- `OnNewGameRequested` definit `PendingStartupMode = NewGame` ;
- `OnNewGameRequested` ouvre `L_GrimrockEditor` ;
- `L_GrimrockEditor` affiche la creation initiale du personnage ;
- le menu principal ne manipule pas directement `PartyInventoryState` ;
- la distinction `L_MainMenu` / `L_GrimrockEditor` est respectee.
