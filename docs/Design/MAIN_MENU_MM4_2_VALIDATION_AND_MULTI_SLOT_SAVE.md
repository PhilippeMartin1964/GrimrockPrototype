# MM4.2 - Validation finale Charger partie et sauvegarde multi-slot

## 1. Objet

MM4.2 ferme le flux `Charger partie` et ajoute le support Blueprint pour sauvegarder la partie dans plusieurs slots selon la progression du jeu.

Flux de chargement final :

```text
WBP_MainMenu
-> WBP_LoadGameMenu
-> WBP_LoadGameSlotRow
-> RequestLoadPartySaveSlot(SlotName, UserIndex)
-> Open Level L_GrimrockEditor
-> GrimrockStartupModeComponent applique le slot au pawn
-> AGrimrockPartyPawn charge la sauvegarde selectionnee
```

Flux de sauvegarde multi-slot :

```text
Progression / Trigger / Checkpoint / UI
-> SavePartyGameToSlot ou SavePartyGameCopyToSlot
-> PartySave Saved Slot=<slot choisi>
-> WBP_LoadGameMenu pourra ensuite lister ce slot
```

---

## 2. Ajout C++ MM4.2

Nouvelle librairie Blueprint :

```text
Source/GrimrockPrototype/Public/Save/GrimrockSaveSlotLibrary.h
Source/GrimrockPrototype/Private/Save/GrimrockSaveSlotLibrary.cpp
```

Classe :

```text
GrimrockSaveSlotLibrary
```

Fonctions Blueprint :

```text
SetPartySaveSlot(PartyPawn, SlotName, UserIndex, OutError)
SavePartyGameToSlot(PartyPawn, SlotName, UserIndex, OutError)
SavePartyGameCopyToSlot(PartyPawn, SlotName, UserIndex, OutError)
```

---

## 3. Difference entre les fonctions

### 3.1 SetPartySaveSlot

Change seulement le slot actif du pawn.

```text
SetPartySaveSlot
-> PartySaveSlotName = SlotName
-> PartySaveUserIndex = UserIndex
```

Cela ne sauvegarde rien tout seul.

Usage : preparer le pawn pour que les prochains appels a `SaveCurrentGame` utilisent un autre slot.

### 3.2 SavePartyGameToSlot

Sauvegarde dans le slot donne et laisse ce slot actif.

```text
SavePartyGameToSlot(GrimrockParty_2, 0)
-> slot actif = GrimrockParty_2
-> SaveCurrentGame
-> les autosaves futures continueront dans GrimrockParty_2
```

Usage conseille : changement durable de chapitre, zone, acte ou progression.

Exemples :

```text
Entree au niveau 2 -> SavePartyGameToSlot("GrimrockParty_2", 0)
Entree au niveau 3 -> SavePartyGameToSlot("GrimrockParty_3", 0)
```

### 3.3 SavePartyGameCopyToSlot

Sauvegarde une copie dans le slot donne, puis restaure le slot actif precedent.

```text
slot actif courant = GrimrockParty
SavePartyGameCopyToSlot(GrimrockParty_2, 0)
-> ecrit GrimrockParty_2
-> restaure slot actif = GrimrockParty
```

Usage conseille : checkpoint secondaire, sauvegarde de securite, snapshot avant boss, autosave de transition sans changer le slot principal.

---

## 4. Slots standards MM4

Slots reconnus par `UGrimrockGameInstance` :

```text
GrimrockParty
GrimrockParty_2
GrimrockParty_3
```

`WBP_LoadGameMenu` n'affiche que les slots existants. Donc un slot apparait seulement apres une sauvegarde reussie dans ce slot.

---

## 5. Blueprint - Sauvegarder selon la progression

### 5.1 Exemple : sauvegarde principale

```text
Get Player Pawn
-> Cast To BP_GrimrockPartyPawn
-> SavePartyGameToSlot
   PartyPawn = Cast Result
   SlotName  = GrimrockParty
   UserIndex = 0
   OutError  = variable Text
```

### 5.2 Exemple : passage au deuxieme niveau

```text
Trigger_Level2Reached
-> Get Player Pawn
-> Cast To BP_GrimrockPartyPawn
-> SavePartyGameToSlot
   SlotName  = GrimrockParty_2
   UserIndex = 0
```

Resultat : la partie est sauvegardee dans `GrimrockParty_2` et ce slot devient le slot actif pour les prochaines sauvegardes automatiques.

### 5.3 Exemple : copie de securite avant un combat

```text
Trigger_BeforeBoss
-> Get Player Pawn
-> Cast To BP_GrimrockPartyPawn
-> SavePartyGameCopyToSlot
   SlotName  = GrimrockParty_3
   UserIndex = 0
```

Resultat : `GrimrockParty_3` est cree ou remplace, mais le slot actif courant reste inchange.

---

## 6. Validation Charger partie

### 6.1 Test slot principal

1. Lancer PIE.
2. Cliquer `Charger partie`.
3. Cliquer la ligne `Sauvegarde principale / GrimrockParty / Disponible`.
4. Verifier que `L_GrimrockEditor` s'ouvre.
5. Verifier les logs.

Logs attendus :

```text
LoadGameMenu OpenRuntimeLevel Slot=GrimrockParty UserIndex=0 Level=L_GrimrockEditor
GrimrockGameInstance PendingLoadSlot Set Slot=GrimrockParty UserIndex=0
GrimrockGameInstance PendingStartupMode Set Mode=1
GrimrockGameInstance LoadSlot Requested Slot=GrimrockParty UserIndex=0
GrimrockStartupMode AppliedSaveSlot Pawn=... Slot=GrimrockParty UserIndex=0
PartySave Continued Slot=GrimrockParty CharacterCount=...
```

### 6.2 Test multi-slot

1. Dans le runtime, declencher une sauvegarde vers `GrimrockParty_2` avec `SavePartyGameToSlot` ou `SavePartyGameCopyToSlot`.
2. Revenir au menu principal.
3. Cliquer `Charger partie`.
4. Verifier qu'une deuxieme ligne apparait.
5. Cliquer `Sauvegarde 2 / GrimrockParty_2 / Disponible`.
6. Verifier les logs.

Logs attendus :

```text
PartySave Saved Slot=GrimrockParty_2
LoadGameMenu Refreshed Widget=... ExistingSlots=2
LoadGameMenu OpenRuntimeLevel Slot=GrimrockParty_2 UserIndex=0 Level=L_GrimrockEditor
GrimrockStartupMode AppliedSaveSlot Pawn=... Slot=GrimrockParty_2 UserIndex=0
PartySave Continued Slot=GrimrockParty_2 CharacterCount=...
```

---

## 7. Filtres Output Log

Filtrer avec :

```text
LoadGameMenu
GrimrockGameInstance
GrimrockStartupMode
PartySave
```

Pour valider la sauvegarde multi-slot, ajouter si besoin :

```text
SavedToSlot
CopyToSlot
ActiveSlot
```

---

## 8. Critere final MM4.2

MM4.2 est valide lorsque :

- cliquer une ligne de `WBP_LoadGameMenu` ouvre `L_GrimrockEditor` ;
- le pawn charge le slot selectionne ;
- le log `PartySave Continued Slot=<slot selectionne>` apparait ;
- `SavePartyGameToSlot` peut creer `GrimrockParty_2` ou `GrimrockParty_3` ;
- `SavePartyGameCopyToSlot` peut creer une copie sans changer le slot actif ;
- apres creation d'un nouveau slot, `WBP_LoadGameMenu` l'affiche ;
- le menu principal ne charge jamais directement une sauvegarde.

Statut :

```text
MM4.2 valide apres recompilation, test de chargement et test de creation d'au moins un deuxieme slot.
```
