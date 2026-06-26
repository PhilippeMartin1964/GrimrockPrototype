# MM2.1 - Consolidation C++ du GameInstance

## 1. Objet

MM2.1 consolide en C++ le transport du mode de demarrage choisi dans le menu principal.

Le menu principal ne doit pas connaitre le pawn runtime.

Le flux cible reste :

```text
WBP_MainMenu
-> UGrimrockGameInstance::SetPendingStartupMode(NewGame)
-> Open Level L_GrimrockEditor
-> GrimrockPartyPawn consomme le mode de demarrage
-> NewGame ou Continue
```

---

## 2. Fichiers C++ ajoutes

```text
Source/GrimrockPrototype/Public/Runtime/GrimrockGameInstance.h
Source/GrimrockPrototype/Private/Runtime/GrimrockGameInstance.cpp
```

Classe ajoutee :

```cpp
UGrimrockGameInstance
```

Role : stocker un `PendingStartupMode` persistant entre `L_MainMenu` et `L_GrimrockEditor`.

---

## 3. API exposee Blueprint

```text
SetPendingStartupMode(EGrimrockPartyStartupMode NewMode)
GetPendingStartupMode()
ConsumePendingStartupMode()
ClearPendingStartupMode()
```

`ConsumePendingStartupMode()` retourne le mode courant, puis remet le pending mode a :

```text
Continue
```

Cela evite qu'un ancien ordre `NewGame` soit rejoue accidentellement lors d'une future ouverture de carte.

---

## 4. Reparent BP_GrimrockGameInstance

Dans UE5, ouvrir :

```text
BP_GrimrockGameInstance
```

Reparenter le Blueprint sur :

```text
GrimrockGameInstance
```

Ensuite, dans `WBP_MainMenu`, remplacer le set direct de variable Blueprint par l'appel :

```text
SetPendingStartupMode(NewGame)
```

Le graph devient :

```text
OnNewGameRequested
-> Get Game Instance
-> Cast To BP_GrimrockGameInstance
-> SetPendingStartupMode(NewGame)
-> Open Level L_GrimrockEditor
```

---

## 5. Point runtime a connecter

Le pawn runtime doit consommer le mode au debut de son `BeginPlay`, avant la logique de chargement ou de reset.

Logique cible :

```cpp
if (UGrimrockGameInstance* GI = GetGameInstance<UGrimrockGameInstance>())
{
    PartyStartupMode = GI->ConsumePendingStartupMode();
}
```

Cette lecture doit arriver avant le bloc existant qui teste :

```cpp
PartyStartupMode == EGrimrockPartyStartupMode::NewGame
```

---

## 6. Reglages projet

Dans :

```text
Edit -> Project Settings -> Maps & Modes
```

Verifier :

```text
Game Instance Class = BP_GrimrockGameInstance
Editor Startup Map  = L_MainMenu
Game Default Map    = L_MainMenu
```

---

## 7. Validation attendue

Au lancement PIE :

```text
L_MainMenu
-> WBP_MainMenu
```

Au clic Nouvelle partie :

```text
SetPendingStartupMode(NewGame)
-> Open Level L_GrimrockEditor
-> GrimrockPartyPawn consomme NewGame
-> creation initiale du personnage
```

Output Log attendu :

```text
GrimrockGameInstance PendingStartupMode Set Mode=0
GrimrockGameInstance PendingStartupMode Consumed Mode=0 NextMode=1
```

---

## 8. Critere de validation MM2.1

MM2.1 est valide lorsque :

- `BP_GrimrockGameInstance` herite de `GrimrockGameInstance` ;
- `WBP_MainMenu` appelle `SetPendingStartupMode(NewGame)` ;
- `L_GrimrockEditor` est ouvert apres le clic sur `Nouvelle partie` ;
- le pawn runtime consomme le mode au debut du jeu ;
- le menu principal ne cherche aucun `GrimrockPartyPawn` ;
- le menu principal n'appelle jamais `StartNewGame` directement.
