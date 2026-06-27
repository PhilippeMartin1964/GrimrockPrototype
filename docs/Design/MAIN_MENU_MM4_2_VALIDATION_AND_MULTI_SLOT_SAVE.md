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
-> AGrimrockPartyPawn charge la sauvegarde sélectionnée
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

## 3. Différence entre les fonctions

### 3.1 SetPartySaveSlot

Change seulement le slot actif du pawn.

```text
SetPartySaveSlot
-> PartySaveSlotName = SlotName
-> PartySaveUserIndex = UserIndex
```

Cela ne sauvegarde rien tout seul.

Usage : préparer le pawn pour que les prochains appels à `SaveCurrentGame` utilisent un autre slot.

### 3.2 SavePartyGameToSlot

Sauvegarde dans le slot donné et laisse ce slot actif.

```text
SavePartyGameToSlot(GrimrockParty_2, 0)
-> slot actif = GrimrockParty_2
-> SaveCurrentGame
-> les autosaves futures continueront dans GrimrockParty_2
```

Usage conseillé : changement durable de chapitre, zone, acte ou progression.

Exemples :

```text
Entrée au niveau 2 -> SavePartyGameToSlot("GrimrockParty_2", 0)
Entrée au niveau 3 -> SavePartyGameToSlot("GrimrockParty_3", 0)
```

### 3.3 SavePartyGameCopyToSlot

Sauvegarde une copie dans le slot donné, puis restaure le slot actif précédent.

```text
slot actif courant = GrimrockParty
SavePartyGameCopyToSlot(GrimrockParty_2, 0)
-> écrit GrimrockParty_2
-> restaure slot actif = GrimrockParty
```

Usage conseillé : checkpoint secondaire, sauvegarde de sécurité, snapshot avant boss, autosave de transition sans changer le slot principal.

---

## 4. Slots standards MM4

Slots reconnus par `UGrimrockGameInstance` :

```text
GrimrockParty
GrimrockParty_2
GrimrockParty_3
```

`WBP_LoadGameMenu` n'affiche que les slots existants. Donc un slot apparaît seulement après une sauvegarde réussie dans ce slot.

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

### 5.2 Exemple : passage au deuxième niveau

```text
Trigger_Level2Reached
-> Get Player Pawn
-> Cast To BP_GrimrockPartyPawn
-> SavePartyGameToSlot
   SlotName  = GrimrockParty_2
   UserIndex = 0
```

Résultat : la partie est sauvegardée dans `GrimrockParty_2` et ce slot devient le slot actif pour les prochaines sauvegardes automatiques.

### 5.3 Exemple : copie de sécurité avant un combat

```text
Trigger_BeforeBoss
-> Get Player Pawn
-> Cast To BP_GrimrockPartyPawn
-> SavePartyGameCopyToSlot
   SlotName  = GrimrockParty_3
   UserIndex = 0
```

Résultat : `GrimrockParty_3` est créé ou remplacé, mais le slot actif courant reste inchangé.

---

## 6. Validation Charger partie

### 6.1 Test slot principal

1. Lancer PIE.
2. Cliquer `Charger partie`.
3. Cliquer la ligne `Sauvegarde principale / GrimrockParty / Disponible`.
4. Vérifier que `L_GrimrockEditor` s'ouvre.
5. Vérifier les logs.

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

1. Dans le runtime, déclencher une sauvegarde vers `GrimrockParty_2` avec `SavePartyGameToSlot` ou `SavePartyGameCopyToSlot`.
2. Revenir au menu principal.
3. Cliquer `Charger partie`.
4. Vérifier qu'une deuxième ligne apparaît.
5. Cliquer `Sauvegarde 2 / GrimrockParty_2 / Disponible`.
6. Vérifier les logs.

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

## 8. Critère final MM4.2

MM4.2 est validé lorsque :

- cliquer une ligne de `WBP_LoadGameMenu` ouvre `L_GrimrockEditor` ;
- le pawn charge le slot sélectionné ;
- le log `PartySave Continued Slot=<slot sélectionné>` apparaît ;
- `SavePartyGameToSlot` peut créer `GrimrockParty_2` ou `GrimrockParty_3` ;
- `SavePartyGameCopyToSlot` peut créer une copie sans changer le slot actif ;
- après création d'un nouveau slot, `WBP_LoadGameMenu` l'affiche ;
- le menu principal ne charge jamais directement une sauvegarde.

Statut :

```text
MM4.2 validé après recompilation, test de chargement et test de création d'au moins un deuxième slot.
```
