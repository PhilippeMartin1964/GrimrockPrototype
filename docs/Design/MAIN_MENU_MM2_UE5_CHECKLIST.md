# MM2 - Nouvelle partie vers CC7

## 1. Objet

MM2 branche le bouton `Nouvelle partie` du menu principal vers le flux de creation initiale du personnage.

Aucun nouveau code C++ n'est requis pour cette tranche, car le projet expose deja la fonction suivante en Blueprint :

```cpp
AGrimrockPartyPawn::StartNewGame(FText& OutError)
```

Le flux MM2 cible est donc :

```text
WBP_MainMenu
-> OnNewGameRequested
-> Get Actor Of Class: GrimrockPartyPawn
-> StartNewGame
-> ShowInitialCharacterCreationWidget
```

A terme, `ShowInitialCharacterCreationWidget` devra afficher le wizard CC7 complet.

Pour le moment, il affiche le widget de creation de personnage configure dans :

```text
AGrimrockPartyPawn::CharacterCreationWidgetClass
```

Ce widget represente le pont fonctionnel actuel vers CC7 tant que `WBP_CharacterCreationWizard` n'est pas encore implemente.

---

## 2. Principe

Le menu principal ne doit pas creer lui-meme le personnage.

Il doit uniquement lancer le flux New Game.

Le pawn gere deja la logique suivante :

- suppression de la sauvegarde courante si elle existe ;
- reset du groupe pour nouvelle partie ;
- fermeture des widgets precedents ;
- remise a zero de l'etat modal ;
- remise a zero de l'objet tenu en main ;
- reset de l'etat runtime du donjon ;
- repositionnement sur la cellule de depart ;
- affichage de la creation initiale du personnage.

---

## 3. Travail UE5 dans WBP_MainMenu

Dans `WBP_MainMenu`, remplacer le `Print String` provisoire de `OnNewGameRequested`.

Ancien branchement MM1 :

```text
OnNewGameRequested
-> Print String "Nouvelle partie - TODO MM2"
```

Nouveau branchement MM2 :

```text
OnNewGameRequested
-> Get Actor Of Class
   Actor Class = GrimrockPartyPawn
-> Cast To GrimrockPartyPawn si necessaire
-> StartNewGame
-> Branch sur Return Value
```

Si `Return Value == true` :

```text
Remove From Parent sur WBP_MainMenu
Print String "Nouvelle partie lancee" facultatif
```

Si `Return Value == false` :

```text
Print String OutError
```

Ne pas brancher encore :

- Continuer ;
- Charger partie ;
- Options ;
- Credits ;
- Licence ;
- Quitter.

Ces points restent pour MM3, MM4 et MM5.

---

## 4. Variante propre si le pawn est deja connu

Si le widget est cree depuis un Blueprint qui connait deja le `GrimrockPartyPawn`, il est possible de stocker cette reference dans une variable Blueprint locale du menu.

Pour MM2, la solution simple avec `Get Actor Of Class` est acceptee.

A terme, on evitera `Get Actor Of Class` dans le menu principal definitif, au profit d'un gestionnaire de flux de type GameInstance, GameMode ou MainMenuController.

---

## 5. Configuration requise dans la carte de test

MM2 suppose que le monde courant contient un `AGrimrockPartyPawn` valide.

Deux configurations sont possibles.

### Option A - Menu affiche dans la carte runtime existante

Le menu principal est affiche au debut de la carte runtime.

Dans ce cas, `Get Actor Of Class: GrimrockPartyPawn` trouve directement le pawn.

C'est l'option recommandee pour MM2.

### Option B - Menu dans une carte L_MainMenu dediee

Cette option est plus propre a terme, mais demande une etape supplementaire.

Si `L_MainMenu` ne contient pas de `AGrimrockPartyPawn`, alors `Get Actor Of Class` ne trouvera rien.

Dans ce cas, il faudra plus tard ajouter un gestionnaire de flux pour ouvrir la carte runtime avec un contexte New Game.

Cette partie est hors perimetre MM2.

---

## 6. Lien avec CC7

MM2 ne cree pas encore tout le wizard CC7.

Le lien avec CC7 est le suivant :

```text
StartNewGame
-> ShowInitialCharacterCreationWidget
-> CharacterCreationWidgetClass
```

Quand `WBP_CharacterCreationWizard` sera disponible, il devra remplacer le widget actuellement configure dans :

```text
AGrimrockPartyPawn::CharacterCreationWidgetClass
```

Le menu principal ne doit pas manipuler directement :

- les races ;
- les classes ;
- les attributs ;
- les portraits ;
- l'inventaire ;
- `PartyInventoryState`.

Il lance uniquement le flux New Game.

---

## 7. Checklist UE5

### A. Ouvrir WBP_MainMenu

Dans le Graph, localiser :

```text
Event OnNewGameRequested
```

### B. Remplacer le Print String

Supprimer ou debrancher le `Print String` provisoire.

Ajouter :

```text
Get Actor Of Class
Actor Class = GrimrockPartyPawn
```

Puis :

```text
StartNewGame
```

Utiliser la sortie :

```text
Return Value
Out Error
```

### C. Gerer le resultat

Si `Return Value` est vrai :

```text
Remove From Parent
```

Si `Return Value` est faux :

```text
Print String OutError
```

### D. Tester

1. Lancer la carte de test qui contient le `AGrimrockPartyPawn`.
2. Afficher `WBP_MainMenu`.
3. Cliquer sur `Nouvelle partie`.
4. Verifier que le menu disparait.
5. Verifier que le widget de creation initiale apparait.
6. Verifier que le groupe est bloque tant que la creation n'est pas terminee.

---

## 8. Output Log attendu

En cas de succes, le pawn doit produire un log de type :

```text
PartySave NewGame Slot=...
CharacterCreation UI Shown Pawn=...
```

Ces logs confirment que le flux est passe par le pawn et non par une creation directe depuis le menu.

---

## 9. Captures attendues

Pour valider MM2, transmettre :

1. Graph `OnNewGameRequested` avec `Get Actor Of Class` et `StartNewGame` ;
2. menu principal juste avant le clic ;
3. widget de creation initiale affiche apres le clic ;
4. Output Log montrant `PartySave NewGame` et `CharacterCreation UI Shown` ;
5. en cas d'echec, capture du `Print String` ou de l'Output Log avec l'erreur.

---

## 10. Critere de validation MM2

MM2 est valide lorsque :

- le bouton `Nouvelle partie` n'est plus un simple `Print String` ;
- `OnNewGameRequested` appelle `StartNewGame` sur `AGrimrockPartyPawn` ;
- `StartNewGame` retourne true ;
- le menu principal disparait apres succes ;
- la creation initiale du personnage apparait ;
- le menu principal ne cree pas lui-meme le personnage ;
- aucune logique de sauvegarde complete n'est ajoutee ;
- aucun autre bouton du menu principal n'est branche hors perimetre MM2.
