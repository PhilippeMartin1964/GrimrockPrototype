# MM5 - Nettoyage et finition du menu principal

## 1. Objet

MM5 finalise les boutons secondaires du menu principal :

```text
Options
Credits
Licence
Quitter
```

Objectif : garder le menu principal propre, sans gameplay, et ouvrir des modals simples pour les ecrans secondaires.

---

## 2. Ajouts C++

### 2.1 UGrimrockMainMenuModalWidget

Fichiers ajoutes :

```text
Source/GrimrockPrototype/Public/UI/GrimrockMainMenuModalWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockMainMenuModalWidget.cpp
```

Role : parent C++ commun pour les modals secondaires.

Fonctionnement :

```text
Button_Back
-> RemoveFromParent
```

Nom obligatoire lu par le C++ :

```text
Button_Back
```

Fonction Blueprint disponible :

```text
CloseModal()
```

### 2.2 UGrimrockMainMenuWidget

Le widget principal expose maintenant :

```text
OpenOptionsMenu()
OpenCreditsMenu()
OpenLicenseMenu()
QuitMainMenu()
```

Et les Class Defaults de `WBP_MainMenu` contiennent :

```text
Options Menu Widget Class
Credits Menu Widget Class
License Menu Widget Class
Modal ZOrder
Quit Directly From Main Menu
```

Quand ces classes sont renseignees, les boutons ouvrent automatiquement les modals correspondants.

---

## 3. Creer WBP_OptionsMenu

Creer :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_OptionsMenu
```

Parent class :

```text
GrimrockMainMenuModalWidget
```

Hierarchy recommandee :

```text
WBP_OptionsMenu
-> CanvasPanel_Root
   -> Border_ModalDim
      -> SizeBox_Dialog
         -> Border_DialogBackground
            -> VerticalBox_Dialog
               -> Text_Title
               -> Text_Subtitle
               -> Border_ContentFrame
                  -> VerticalBox_Content
                     -> Text_OptionPlaceholder
               -> HorizontalBox_Footer
                  -> Spacer_FooterFill
                  -> Button_Back
                     -> Text_Back
```

Noms obligatoires :

```text
Button_Back
```

Textes conseilles :

```text
Text_Title             = Options
Text_Subtitle          = Reglages du prototype
Text_OptionPlaceholder = Options audio/video a venir.
Text_Back              = Retour
```

Aucun Graph n'est requis.

---

## 4. Creer WBP_CreditsMenu

Creer :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_CreditsMenu
```

Parent class :

```text
GrimrockMainMenuModalWidget
```

Hierarchy identique a `WBP_OptionsMenu`.

Textes conseilles :

```text
Text_Title    = Credits
Text_Subtitle = Grimrock Prototype
Text_Content  = Prototype developpe sous Unreal Engine 5.5.4.
Text_Back     = Retour
```

Aucun Graph n'est requis.

---

## 5. Creer WBP_LicenseMenu

Creer :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LicenseMenu
```

Parent class :

```text
GrimrockMainMenuModalWidget
```

Hierarchy identique a `WBP_OptionsMenu`.

Textes conseilles :

```text
Text_Title    = Licence
Text_Subtitle = Informations legales
Text_Content  = Prototype prive de developpement. Contenu, assets et licences a documenter avant distribution.
Text_Back     = Retour
```

Aucun Graph n'est requis.

---

## 6. Configurer WBP_MainMenu

Ouvrir :

```text
WBP_MainMenu
```

Puis :

```text
Class Defaults
-> Main Menu|Modal
```

Regler :

```text
Options Menu Widget Class = WBP_OptionsMenu
Credits Menu Widget Class = WBP_CreditsMenu
License Menu Widget Class = WBP_LicenseMenu
Modal ZOrder              = 200
```

Puis verifier :

```text
Quit Directly From Main Menu = true
```

Ainsi :

```text
Options -> ouvre WBP_OptionsMenu
Credits -> ouvre WBP_CreditsMenu
Licence -> ouvre WBP_LicenseMenu
Quitter -> QuitGame
```

---

## 7. Graph WBP_MainMenu

Pour MM5, aucun Graph n'est requis pour :

```text
Options
Credits
Licence
Quitter
```

Le C++ gere ces quatre boutons si les Class Defaults sont renseignes.

Conserver les Graphs deja faits pour :

```text
OnNewGameRequested
OnContinueRequested
OnLoadGameRequested
```

Si une classe de modal n'est pas renseignee, le C++ appelle encore l'evenement Blueprint correspondant :

```text
OnOptionsRequested
OnCreditsRequested
OnLicenseRequested
OnQuitRequested
```

Cela permet de garder une voie de secours Blueprint.

---

## 8. Layout modal conseille

Pour chaque modal secondaire, utiliser un vrai fond plein ecran :

```text
Border_ModalDim
- Anchors = Full Screen
- Offsets = 0 / 0 / 0 / 0
- Brush Color = noir
- Alpha = 0.75 a 0.85
```

Panneau central :

```text
SizeBox_Dialog
- Horizontal Alignment = Center
- Vertical Alignment   = Center
- Width Override       = 680
- Height Override      = 460
```

Fond du panneau :

```text
Border_DialogBackground
- Brush Color = noir charbon / gris tres fonce
- Alpha = 0.95 a 1.0
- Padding = 32
```

---

## 9. Logs attendus

Ouvrir Options :

```text
MainMenu Modal Opened Widget=... Class=WBP_OptionsMenu_C ZOrder=200
```

Ouvrir Credits :

```text
MainMenu Modal Opened Widget=... Class=WBP_CreditsMenu_C ZOrder=200
```

Ouvrir Licence :

```text
MainMenu Modal Opened Widget=... Class=WBP_LicenseMenu_C ZOrder=200
```

Quitter :

```text
MainMenu Quit Requested Widget=...
```

En PIE, `QuitGame` arrete normalement la session PIE.

---

## 10. Critere final MM5

MM5 est valide lorsque :

- `Options` ouvre un modal propre avec un bouton `Retour` ;
- `Credits` ouvre un modal propre avec un bouton `Retour` ;
- `Licence` ouvre un modal propre avec un bouton `Retour` ;
- chaque bouton `Retour` ferme uniquement le modal ;
- `Quitter` quitte proprement le PIE ou l'application ;
- les flux `Nouvelle partie`, `Continuer` et `Charger partie` restent inchanges.

Statut :

```text
MM5 valide apres recompilation, creation des trois widgets UMG et test des quatre boutons.
```
