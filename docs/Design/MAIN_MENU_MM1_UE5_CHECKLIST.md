# MM1 - Maquette visuelle UE5 du menu principal

## 1. Objet

MM1 cree la premiere maquette visuelle UE5 du menu principal.

Cette tranche installe :

- le socle C++ `UGrimrockMainMenuWidget` ;
- le futur Widget Blueprint `WBP_MainMenu` ;
- les sept boutons du menu principal ;
- les hooks Blueprint minimaux ;
- l'etat grise de `Continuer` et `Charger partie` lorsqu'aucune sauvegarde valide n'existe.

MM1 ne branche pas encore :

- le systeme complet de sauvegarde ;
- le wizard CC7 ;
- le chargement de partie ;
- les vrais ecrans Options, Credits et Licence.

---

## 2. Fichiers C++ ajoutes

```text
Source/GrimrockPrototype/Public/UI/GrimrockMainMenuWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockMainMenuWidget.cpp
```

Role : fournir une classe parent propre pour `WBP_MainMenu`.

Le widget expose :

```text
SetHasValidSaveGame(bool)
HasValidSaveGame()
RefreshButtonStates()
```

Il expose aussi les evenements Blueprint :

```text
OnContinueRequested
OnNewGameRequested
OnLoadGameRequested
OnOptionsRequested
OnCreditsRequested
OnLicenseRequested
OnQuitRequested
```

Ces evenements permettent de construire la maquette sans placer de logique de flux complexe dans le C++ pour MM1.

---

## 3. Widget Blueprint a creer

Dans UE5, creer :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_MainMenu
```

Parent class :

```text
GrimrockMainMenuWidget
```

Si le dossier n'existe pas, le creer depuis le Content Browser.

Ne pas creer ce widget dans l'explorateur Windows.

---

## 4. Hierarchie recommandee

Dans le Designer de `WBP_MainMenu`, construire une structure simple :

```text
CanvasPanel_Root
└─ Overlay_Main
   ├─ Image_Background
   ├─ Border_DarkVeil
   └─ VerticalBox_MenuPanel
      ├─ Text_Title
      ├─ Button_Continue
      ├─ Button_NewGame
      ├─ Button_LoadGame
      ├─ Button_Options
      ├─ Button_Credits
      ├─ Button_License
      ├─ Button_Quit
      └─ Text_BuildVersion
```

Les noms exacts obligatoires pour le C++ sont :

```text
Button_Continue
Button_NewGame
Button_LoadGame
Button_Options
Button_Credits
Button_License
Button_Quit
```

Ces boutons utilisent `BindWidgetOptional`, donc le Blueprint compile meme si un bouton manque. Mais pour valider MM1, les sept boutons doivent exister.

---

## 5. Textes visibles recommandes

| Widget | Texte |
|---|---|
| `Text_Title` | `GRIMROCK PROTOTYPE` |
| `Button_Continue` | `Continuer` |
| `Button_NewGame` | `Nouvelle partie` |
| `Button_LoadGame` | `Charger partie` |
| `Button_Options` | `Options` |
| `Button_Credits` | `Credits` |
| `Button_License` | `Licence` |
| `Button_Quit` | `Quitter` |
| `Text_BuildVersion` | `Prototype - UE 5.5.4` |

Si les accents posent probleme dans une police provisoire, garder temporairement `Credits` puis corriger plus tard en `Credits` avec accent ou dans une table de localisation.

---

## 6. Mise en page recommandee

Disposition cible :

```text
+-------------------------------------------------------------+
|                      GRIMROCK PROTOTYPE                     |
|                                                             |
|                    Fond de donjon sombre                    |
|                                                             |
|                         Continuer                           |
|                         Nouvelle partie                     |
|                         Charger partie                      |
|                         Options                             |
|                         Credits                             |
|                         Licence                             |
|                         Quitter                             |
|                                                             |
|                    Prototype - UE 5.5.4                     |
+-------------------------------------------------------------+
```

Reglages simples :

- `CanvasPanel_Root` plein ecran ;
- `Image_Background` ancree plein ecran ;
- `Border_DarkVeil` plein ecran, couleur noire avec alpha leger ;
- `VerticalBox_MenuPanel` centre horizontalement ;
- largeur recommandee du panneau : 420 a 560 px ;
- espacement entre boutons : 8 a 16 px ;
- titre au-dessus des boutons ;
- version en bas ou sous le menu.

---

## 7. Direction visuelle

Style MM1 : sobre, sombre, medieval fantastique.

A privilegier :

```text
Fond de donjon sombre
Pierre sombre
Cuir use
Metal vieilli
Texte ivoire clair
Boutons rectangulaires sobres
Survol discret
```

A eviter :

```text
Fond uni trop clair
Style cartoon
Couleurs trop saturees
Gros contours mobiles
Effets magiques excessifs
Lisibilite faible
```

MM1 peut utiliser des couleurs plates provisoires. Le polissage viendra plus tard.

---

## 8. Etat des sauvegardes pour MM1

Par defaut, aucune sauvegarde valide n'est connue.

Dans `WBP_MainMenu`, la valeur initiale peut rester :

```text
bHasValidSaveGame = false
```

Effet attendu :

```text
Continuer      -> grise / disabled
Charger partie -> grise / disabled
Nouvelle partie, Options, Credits, Licence, Quitter -> actifs
```

Pour tester l'etat actif, appeler temporairement dans le Blueprint :

```text
SetHasValidSaveGame(true)
```

Puis verifier que `Continuer` et `Charger partie` deviennent actifs.

---

## 9. Evenements Blueprint provisoires

Dans le Graph de `WBP_MainMenu`, il est acceptable pour MM1 de brancher uniquement des traces ou messages temporaires.

Exemples :

```text
OnNewGameRequested  -> Print String "Nouvelle partie - TODO MM2"
OnOptionsRequested  -> Print String "Options - TODO MM5"
OnCreditsRequested  -> Print String "Credits - TODO MM5"
OnLicenseRequested  -> Print String "Licence - TODO MM5"
OnQuitRequested     -> Print String "Quitter - TODO MM5"
```

Ne pas encore :

- ouvrir le wizard CC7 ;
- charger une carte de jeu ;
- creer un personnage ;
- modifier `PartyInventoryState` ;
- implementer un vrai systeme de sauvegarde.

---

## 10. Carte de test du menu principal

Creer plus tard, si necessaire :

```text
Content/GrimrockPrototype/Maps/L_MainMenu
```

Pour MM1, deux options sont acceptables :

1. tester `WBP_MainMenu` directement en Designer / Preview ;
2. creer une carte temporaire simple avec un fond sombre et afficher le widget au BeginPlay.

Si une carte est creee, le faire dans le Content Browser, puis Save All.

---

## 11. Checklist UE5

### A. Recompiler

1. Fermer Unreal Editor.
2. Compiler `GrimrockPrototypeEditor` en Development Editor / Win64.
3. Rouvrir le projet.

Resultat attendu : la classe `GrimrockMainMenuWidget` est disponible comme parent UMG.

### B. Creer le Widget Blueprint

1. Content Browser.
2. Creer le dossier :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/
```

3. Creer un Widget Blueprint.
4. Parent class : `GrimrockMainMenuWidget`.
5. Nom : `WBP_MainMenu`.

### C. Ajouter les boutons

Ajouter les sept boutons avec les noms exacts :

```text
Button_Continue
Button_NewGame
Button_LoadGame
Button_Options
Button_Credits
Button_License
Button_Quit
```

Pour chacun :

- cocher `Is Variable` ;
- ajouter un Text Block enfant avec le texte visible ;
- ne pas creer d'evenement OnClicked manuel ;
- laisser le C++ binder les clics.

### D. Tester les evenements

Dans le Graph, implementer les evenements Blueprint `On...Requested` avec des `Print String` temporaires.

### E. Tester les etats

1. Lancer le widget.
2. Verifier que `Continuer` et `Charger partie` sont disabled par defaut.
3. Appeler temporairement `SetHasValidSaveGame(true)`.
4. Verifier que les deux boutons deviennent actifs.

---

## 12. Captures a transmettre

Pour validation, transmettre :

1. `WBP_MainMenu` en Designer avec la hierarchie visible ;
2. `WBP_MainMenu` en plein ecran ou Preview ;
3. etat par defaut avec `Continuer` et `Charger partie` grises ;
4. etat avec `SetHasValidSaveGame(true)` ;
5. etat de survol d'un bouton ;
6. Graph montrant uniquement les `Print String` temporaires si besoin.

---

## 13. Critere de validation MM1

MM1 est valide lorsque :

- `WBP_MainMenu` existe ;
- il herite de `GrimrockMainMenuWidget` ;
- les sept boutons sont visibles ;
- `Continuer` et `Charger partie` sont grises sans sauvegarde ;
- les boutons actifs declenchent leurs evenements Blueprint ;
- aucune vraie logique de sauvegarde, de chargement ou de creation de personnage n'est encore branchee ;
- le style general est lisible, sombre et coherent avec le projet.
