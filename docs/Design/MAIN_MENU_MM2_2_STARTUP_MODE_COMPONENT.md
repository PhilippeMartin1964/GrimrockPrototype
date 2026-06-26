# MM2.2 - Raccord runtime du startup mode

## 1. Objet

MM2.2 raccorde le mode de demarrage stocke dans `UGrimrockGameInstance` au pawn runtime.

But : lorsque le joueur clique sur `Nouvelle partie` depuis `WBP_MainMenu`, le mode `NewGame` est stocke dans le `GameInstance`, puis consomme par le pawn au lancement de `L_GrimrockEditor`.

---

## 2. Fichiers C++ ajoutes

```text
Source/GrimrockPrototype/Public/Runtime/GrimrockStartupModeComponent.h
Source/GrimrockPrototype/Private/Runtime/GrimrockStartupModeComponent.cpp
```

Classe ajoutee :

```cpp
UGrimrockStartupModeComponent
```

---

## 3. Principe technique

`UGrimrockStartupModeComponent` doit etre ajoute a `BP_GrimrockPartyPawn`.

Lors de son `BeginPlay`, le composant :

1. verifie que son owner est un `AGrimrockPartyPawn` ;
2. recupere `UGrimrockGameInstance` ;
3. appelle `ConsumePendingStartupMode()` ;
4. applique le resultat sur `PartyPawn->PartyStartupMode`.

Les composants d'acteur commencent leur `BeginPlay` pendant le `Super::BeginPlay()` du pawn. Cela permet d'appliquer le startup mode avant que `AGrimrockPartyPawn` n'execute sa logique runtime de chargement ou de nouvelle partie.

---

## 4. Travail UE5 requis

Ouvrir :

```text
BP_GrimrockPartyPawn
```

Ajouter un composant :

```text
GrimrockStartupModeComponent
```

Nom recommande :

```text
StartupModeComponent
```

Compiler et sauvegarder le Blueprint.

---

## 5. Flux cible apres raccord

```text
WBP_MainMenu
-> SetPendingStartupMode(NewGame)
-> Open Level L_GrimrockEditor
-> BP_GrimrockPartyPawn commence BeginPlay
-> StartupModeComponent consomme PendingStartupMode
-> PartyStartupMode = NewGame
-> AGrimrockPartyPawn continue son BeginPlay
-> ResetPartyForNewGame
-> ShowInitialCharacterCreationWidget
```

---

## 6. Output Log attendu

Au clic `Nouvelle partie` :

```text
GrimrockGameInstance PendingStartupMode Set Mode=0
```

Au lancement de `L_GrimrockEditor` :

```text
GrimrockGameInstance PendingStartupMode Consumed Mode=0 NextMode=1
GrimrockStartupMode Applied Pawn=... Mode=0
PartySave NewGame Slot=...
CharacterCreation UI Shown Pawn=...
```

---

## 7. Critere de validation MM2.2

MM2.2 est valide lorsque :

- `BP_GrimrockPartyPawn` contient `GrimrockStartupModeComponent` ;
- `Nouvelle partie` definit `PendingStartupMode = NewGame` ;
- le composant consomme `NewGame` au debut de `L_GrimrockEditor` ;
- `PartyStartupMode` vaut `NewGame` avant la logique de sauvegarde du pawn ;
- la creation initiale du personnage apparait ;
- le menu principal ne cherche toujours aucun `GrimrockPartyPawn`.
