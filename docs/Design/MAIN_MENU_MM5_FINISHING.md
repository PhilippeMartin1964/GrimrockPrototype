# MM5 - Nettoyage et finition du menu principal

## 1. Objet

MM5 finalise les boutons secondaires du menu principal :

```text
Options
Crédits
Licence
Quitter
```

Objectif : garder le menu principal propre, sans gameplay, et ouvrir des modals simples pour les écrans secondaires.

---

## 2. Ajouts C++

### 2.1 UGrimrockMainMenuModalWidget

Fichiers ajoutés :

```text
Source/GrimrockPrototype/Public/UI/GrimrockMainMenuModalWidget.h
Source/GrimrockPrototype/Private/UI/GrimrockMainMenuModalWidget.cpp
```

Rôle : parent C++ commun pour les modals secondaires.

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

Quand ces classes sont renseignées, les boutons ouvrent automatiquement les modals correspondants.

---

## 3. Créer WBP_OptionsMenu

Créer :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_OptionsMenu
```

Parent Class :

```text
GrimrockMainMenuModalWidget
```

Hiérarchie recommandée :

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

Textes conseillés :

```text
Text_Title             = Options
Text_Subtitle          = Réglages du prototype
Text_OptionPlaceholder = Options audio/vidéo à venir.
Text_Back              = Retour
```

Aucun Graph n'est requis.

---

## 4. Créer WBP_CreditsMenu

Créer :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_CreditsMenu
```

Parent Class :

```text
GrimrockMainMenuModalWidget
```

Hiérarchie identique à `WBP_OptionsMenu`.

Textes conseillés :

```text
Text_Title    = Crédits
Text_Subtitle = Grimrock Prototype
Text_Content  = Prototype développé sous Unreal Engine 5.5.4.
Text_Back     = Retour
```

Aucun Graph n'est requis.

---

## 5. Créer WBP_LicenseMenu

Créer :

```text
Content/GrimrockPrototype/Blueprints/UI/MainMenu/WBP_LicenseMenu
```

Parent Class :

```text
GrimrockMainMenuModalWidget
```

Hiérarchie identique à `WBP_OptionsMenu`.

Textes conseillés :

```text
Text_Title    = Licence
Text_Subtitle = Informations légales
Text_Content  = Prototype privé de développement. Contenu, assets et licences à documenter avant distribution.
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

Régler :

```text
Options Menu Widget Class = WBP_OptionsMenu
Credits Menu Widget Class = WBP_CreditsMenu
License Menu Widget Class = WBP_LicenseMenu
Modal ZOrder              = 200
```

Puis vérifier :

```text
Quit Directly From Main Menu = true
```

Ainsi :

```text
Options -> ouvre WBP_OptionsMenu
Crédits -> ouvre WBP_CreditsMenu
Licence -> ouvre WBP_LicenseMenu
Quitter -> QuitGame
```

---

## 7. Graph WBP_MainMenu

Pour MM5, aucun Graph n'est requis pour :

```text
Options
Crédits
Licence
Quitter
```

Le C++ gère ces quatre boutons si les Class Defaults sont renseignés.

Conserver les Graphs déjà faits pour :

```text
OnNewGameRequested
OnContinueRequested
OnLoadGameRequested
```

Si une classe de modal n'est pas renseignée, le C++ appelle encore l'événement Blueprint correspondant :

```text
OnOptionsRequested
OnCreditsRequested
OnLicenseRequested
OnQuitRequested
```

Cela permet de garder une voie de secours Blueprint.

---

## 8. Layout modal conseillé

Pour chaque modal secondaire, utiliser un vrai fond plein écran :

```text
Border_ModalDim
- Anchors = Full Screen
- Offsets = 0 / 0 / 0 / 0
- Brush Color = noir
- Alpha = 0.75 à 0.85
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
- Brush Color = noir charbon / gris très foncé
- Alpha = 0.95 à 1.0
- Padding = 32
```

---

## 9. Logs attendus

Ouvrir Options :

```text
MainMenu Modal Opened Widget=... Class=WBP_OptionsMenu_C ZOrder=200
```

Ouvrir Crédits :

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

En PIE, `QuitGame` arrête normalement la session PIE.

---

## 10. Critère final MM5

MM5 est validé lorsque :

- `Options` ouvre un modal propre avec un bouton `Retour` ;
- `Crédits` ouvre un modal propre avec un bouton `Retour` ;
- `Licence` ouvre un modal propre avec un bouton `Retour` ;
- chaque bouton `Retour` ferme uniquement le modal ;
- `Quitter` quitte proprement le PIE ou l'application ;
- les flux `Nouvelle partie`, `Continuer` et `Charger partie` restent inchangés.

Statut :

```text
MM5 validé après recompilation, création des trois widgets UMG et test des quatre boutons.
```
