# CC7.4.2 - Refonte de l'écran Identité

## 1. Objectif

L'écran **Identité** correspond à l'étape `4 / 5` du wizard de création de personnage.

Son rôle n'est pas de choisir la race ou la classe. Ces choix sont déjà faits dans les étapes précédentes.

Son rôle est :

```text
- saisir le nom du personnage ;
- confirmer le sexe déjà choisi à l'étape Race ;
- choisir la variante de portrait correspondant à la race et au sexe ;
- afficher clairement le portrait final ;
- afficher une description lisible du portrait ;
- préparer le résumé final.
```

L'écran actuel est fonctionnel, mais il reste trop proche d'un prototype :

```text
- plusieurs Text Block non remplacés ;
- manque de titres explicites ;
- ComboBox non expliquées ;
- portrait placé sans cadre visuel fort ;
- colonnes mal équilibrées ;
- fond et bordure trop bruts ;
- absence de rappel Race / Classe / Sexe ;
- hiérarchie visuelle insuffisante.
```

L'objectif de CC7.4.2 est de rendre l'écran **clair, élégant et cohérent** avec les étapes Race, Classe et Caractéristiques.

---

## 2. État fonctionnel à conserver

Les widgets C++ actuellement utilisés pour l'identité sont hérités de `URPGCharacterCreationWidget`.

Ils doivent rester présents avec ces noms exacts :

```text
EditableText_Name
ComboBox_PortraitVariant
Image_Portrait
Text_PortraitDescription
```

Ces widgets sont essentiels :

```text
EditableText_Name          -> nom final du personnage ;
ComboBox_PortraitVariant   -> variante de portrait issue du PortraitSet ;
Image_Portrait             -> portrait final réellement utilisé ;
Text_PortraitDescription   -> description de la variante sélectionnée.
```

`Image_Portrait` doit rester unique dans tout le wizard. Il ne faut pas en créer un second dans Race, Classe ou Résumé.

---

## 3. Point important : le sexe ne doit plus être une ComboBox ici

Depuis CC7.2, le choix du sexe est fait avec deux boutons dans l'étape Race :

```text
Button_GenderMale
Button_GenderFemale
```

L'écran Identité ne doit donc plus présenter une `ComboBox_Gender` comme choix principal.

Recommandation :

```text
- ne pas afficher ComboBox_Gender dans l'écran Identité ;
- afficher seulement un rappel non modifiable : Sexe : Masculin ou Sexe : Féminin ;
- laisser le changement du sexe dans l'étape Race.
```

Raison : changer le sexe dans l'écran Identité modifierait aussi les portraits disponibles et pourrait créer une confusion avec l'étape Race.

Le C++ conserve encore `ComboBox_Gender` comme fallback legacy, mais le nouveau wizard ne doit pas l'utiliser visuellement.

---

## 4. Cible visuelle

La cible est un écran en deux colonnes :

```text
Gauche : formulaire d'identité et choix de portrait
Droite : grand aperçu du personnage / portrait
```

Structure visuelle attendue :

```text
Identité                                      4 / 5

┌──────────────────────────────────────────────────────────────┐
│ Identité du personnage                         Portrait final │
│ Donnez un nom à votre personnage et choisissez son portrait.  │
│                                                              │
│ Nom du personnage                                            │
│ [________________________]                                   │
│                                                              │
│ Rappel                                                       │
│ Race : Humain                                                │
│ Classe : Rôdeur                                              │
│ Sexe : Masculin                                              │
│                                                              │
│ Variante de portrait                                         │
│ [Humain masculin 01               v]                         │
│                                                              │
│ Description                                                  │
│ Portrait humain masculin de base.                            │
│                                                              │
│                                      [grand portrait à droite]│
└──────────────────────────────────────────────────────────────┘
```

---

## 5. Hiérarchie UMG recommandée

Dans `WBP_CharacterCreationWizard`, aller dans :

```text
WidgetSwitcher_Steps
└── Panel_StepIdentity
```

Le contenu recommandé est :

```text
Panel_StepIdentity                                      Border ou Overlay, Is Variable = true
└── Border_IdentityFrame                                Border, Is Variable = false
    └── HorizontalBox_IdentityLayout                    Horizontal Box, Is Variable = false
        ├── VerticalBox_IdentityForm                    Vertical Box, Is Variable = false
        │   ├── Text_IdentityTitle                      Text Block, Is Variable = false
        │   ├── Text_IdentityHelp                       Text Block, Is Variable = false
        │   ├── Spacer_IdentityTopGap                   Spacer, Is Variable = false
        │   ├── Text_NameLabel                          Text Block, Is Variable = false
        │   ├── SizeBox_NameInput                       Size Box, Is Variable = false
        │   │   └── EditableText_Name                   Editable Text, Is Variable = true
        │   ├── Border_IdentitySummaryFrame             Border, Is Variable = false
        │   │   └── VerticalBox_IdentitySummary         Vertical Box, Is Variable = false
        │   │       ├── Text_IdentitySummaryTitle       Text Block, Is Variable = false
        │   │       ├── Text_IdentityRace               Text Block, Is Variable = false ou futur variable
        │   │       ├── Text_IdentityClass              Text Block, Is Variable = false ou futur variable
        │   │       └── Text_IdentityGender             Text Block, Is Variable = false ou futur variable
        │   ├── Text_PortraitVariantLabel               Text Block, Is Variable = false
        │   ├── SizeBox_PortraitVariantCombo            Size Box, Is Variable = false
        │   │   └── ComboBox_PortraitVariant            ComboBox String, Is Variable = true
        │   ├── Text_PortraitDescriptionTitle           Text Block, Is Variable = false
        │   └── Border_PortraitDescriptionFrame         Border, Is Variable = false
        │       └── Text_PortraitDescription            Text Block, Is Variable = true
        └── Border_IdentityPortraitFrame                Border, Is Variable = false
            └── Overlay_IdentityPortrait                Overlay, Is Variable = false
                ├── Image_Portrait                      Image, Is Variable = true
                └── Border_PortraitCaptionOverlay       Border, Is Variable = false
                    └── Text_PortraitCaption            Text Block, Is Variable = false
```

---

## 6. Réglages du panneau principal

`Panel_StepIdentity` :

```text
Type        = Border ou Overlay
Is Variable = true
Padding     = 0 si le frame interne gère le padding
```

`Border_IdentityFrame` :

```text
Type        = Border
Is Variable = false
Padding     = 24
Brush Color = R 0.00 / G 0.00 / B 0.00 / A 0.78
```

Slot dans `Panel_StepIdentity` :

```text
Horizontal Alignment = Fill
Vertical Alignment   = Fill
```

`HorizontalBox_IdentityLayout` :

```text
Type        = Horizontal Box
Is Variable = false
```

Slot dans `Border_IdentityFrame` :

```text
Horizontal Alignment = Fill
Vertical Alignment   = Fill
```

---

## 7. Colonne gauche : formulaire

`VerticalBox_IdentityForm` :

```text
Type        = Vertical Box
Is Variable = false
```

Slot dans `HorizontalBox_IdentityLayout` :

```text
Size    = Fill
Padding = 0 ; 0 ; 24 ; 0
```

Largeur visuelle recommandée : environ 60 % de l'espace.

### 7.1. Titre

`Text_IdentityTitle` :

```text
Text        = Identité du personnage
Is Variable = false
Font Size   = 28
Color       = blanc cassé
Justification = Left
```

Slot :

```text
Size    = Auto
Padding = 0 ; 0 ; 0 ; 8
```

### 7.2. Texte d'aide

`Text_IdentityHelp` :

```text
Text        = Donnez un nom à votre personnage et choisissez le portrait qui sera utilisé dans l'interface de jeu.
Is Variable = false
Font Size   = 17
Color       = R 0.82 / G 0.82 / B 0.82 / A 1.00
Auto Wrap Text = true
```

Slot :

```text
Size    = Auto
Padding = 0 ; 0 ; 0 ; 20
```

### 7.3. Nom du personnage

`Text_NameLabel` :

```text
Text        = Nom du personnage
Is Variable = false
Font Size   = 18
Color       = blanc cassé
```

`SizeBox_NameInput` :

```text
Height Override = 44
```

`EditableText_Name` :

```text
Is Variable = true
Hint Text   = Entrez un nom
Text        = vide
Font Size   = 20
Select All Text When Focused = true
Revert Text On Escape        = true
```

Style recommandé :

```text
Background = gris très foncé / noir désaturé
Text       = blanc cassé
Padding    = 10 ; 6 ; 10 ; 6
```

Le C++ accepte actuellement un nom de 1 à 24 caractères. Le champ doit donc être assez large pour 24 caractères.

---

## 8. Bloc de rappel Race / Classe / Sexe

Ce bloc remplace les `Text Block` anonymes visibles dans l'écran actuel.

Il doit afficher clairement les choix déjà faits.

`Border_IdentitySummaryFrame` :

```text
Type        = Border
Is Variable = false
Padding     = 12
Brush Color = R 0.06 / G 0.05 / B 0.04 / A 0.85
```

Slot dans `VerticalBox_IdentityForm` :

```text
Size    = Auto
Padding = 0 ; 20 ; 0 ; 20
```

Hiérarchie :

```text
Border_IdentitySummaryFrame
└── VerticalBox_IdentitySummary
    ├── Text_IdentitySummaryTitle
    ├── Text_IdentityRace
    ├── Text_IdentityClass
    └── Text_IdentityGender
```

Textes recommandés pour le prototype :

```text
Text_IdentitySummaryTitle = Choix actuels
Text_IdentityRace         = Race : Humain
Text_IdentityClass        = Classe : Rôdeur
Text_IdentityGender       = Sexe : Masculin
```

Réglages communs :

```text
Font Size = 17 à 18
Color     = gris clair pour les libellés, blanc cassé pour les valeurs
```

Important : dans la version actuelle, ces trois textes peuvent rester statiques ou être mis à jour manuellement. Une étape ultérieure pourra les binder automatiquement au wizard.

---

## 9. Variante de portrait

`Text_PortraitVariantLabel` :

```text
Text        = Variante de portrait
Is Variable = false
Font Size   = 18
Color       = blanc cassé
```

`SizeBox_PortraitVariantCombo` :

```text
Height Override = 42
```

`ComboBox_PortraitVariant` :

```text
Is Variable = true
```

Ce ComboBox est alimenté par le C++ à partir de :

```text
AvailablePortraitSets
+ RaceId sélectionné
+ Sexe sélectionné
```

Il doit afficher des noms du type :

```text
Humain masculin 01
Humain masculin 02
Elfe féminin 01
```

Ne pas recréer `ComboBox_Portrait` : il a été supprimé.

---

## 10. Description du portrait

`Text_PortraitDescriptionTitle` :

```text
Text        = Description du portrait
Is Variable = false
Font Size   = 18
Color       = blanc cassé
```

`Border_PortraitDescriptionFrame` :

```text
Type        = Border
Is Variable = false
Padding     = 12
Brush Color = R 0.03 / G 0.03 / B 0.03 / A 0.85
```

`Text_PortraitDescription` :

```text
Is Variable = true
Font Size   = 20 si description courte, 16 à 18 si description longue
Color       = blanc cassé
Auto Wrap Text = true
```

Texte exemple :

```text
Portrait humain masculin de base.
```

À terme, cette description pourra indiquer le style du portrait : sobre, noble, brutal, mystique, jeune, âgé, etc.

---

## 11. Colonne droite : portrait final

La colonne droite doit donner de la présence au personnage.

`Border_IdentityPortraitFrame` :

```text
Type        = Border
Is Variable = false
Padding     = 12
Brush Color = R 0.02 / G 0.02 / B 0.02 / A 0.90
```

Slot dans `HorizontalBox_IdentityLayout` :

```text
Size    = Auto
Padding = 24 ; 0 ; 0 ; 0
```

Largeur recommandée :

```text
Width  = 360 à 420
Height = Fill
```

Utiliser un `SizeBox_IdentityPortraitColumn` autour du `Border_IdentityPortraitFrame` si nécessaire :

```text
Width Override = 380
```

`Overlay_IdentityPortrait` :

```text
Type        = Overlay
Is Variable = false
```

`Image_Portrait` :

```text
Is Variable = true
Brush Draw As = Image
Stretch       = Scale To Fit
Color and Opacity = blanc
```

Si vos portraits sont des personnages de pied ou buste 1024x1536, la zone doit plutôt être verticale :

```text
Width  = 360
Height = 520
```

Si vous affichez seulement un portrait carré ou visage :

```text
Width  = 360
Height = 360
```

Pour le système actuel, il est préférable de garder une zone verticale afin de permettre plus tard l'affichage du personnage complet avec équipement.

---

## 12. Légende du portrait

Ajouter une petite légende en bas de la colonne portrait.

`Border_PortraitCaptionOverlay` :

```text
Type        = Border
Is Variable = false
Padding     = 8
Brush Color = R 0.00 / G 0.00 / B 0.00 / A 0.55
```

Slot dans `Overlay_IdentityPortrait` :

```text
Horizontal Alignment = Fill
Vertical Alignment   = Bottom
```

`Text_PortraitCaption` :

```text
Text        = Portrait final
Is Variable = false
Font Size   = 16
Color       = gris clair
Justification = Center
```

---

## 13. Ce qu'il faut retirer de l'écran actuel

Supprimer ou remplacer les éléments suivants :

```text
- les Text Block génériques non renommés ;
- les labels non explicites ;
- la ComboBox vide sous la ComboBox_PortraitVariant si elle correspond à l'ancien ComboBox_Gender ;
- les grands espaces inutiles à gauche ;
- le portrait sans cadre ;
- les fonds blancs par défaut.
```

Ne pas supprimer :

```text
EditableText_Name
ComboBox_PortraitVariant
Image_Portrait
Text_PortraitDescription
```

---

## 14. Noms exacts à respecter

Widgets obligatoires lus par le C++ :

```text
EditableText_Name
ComboBox_PortraitVariant
Image_Portrait
Text_PortraitDescription
```

Widgets optionnels recommandés pour la lisibilité :

```text
Text_IdentityTitle
Text_IdentityHelp
Text_NameLabel
Text_IdentitySummaryTitle
Text_IdentityRace
Text_IdentityClass
Text_IdentityGender
Text_PortraitVariantLabel
Text_PortraitDescriptionTitle
Text_PortraitCaption
```

Ces widgets optionnels ne sont pas lus par le C++ actuellement. Ils servent à rendre l'écran compréhensible.

---

## 15. Procédure UE5 recommandée

```text
1. Ouvrir WBP_CharacterCreationWizard.
2. Aller dans WidgetSwitcher_Steps > Panel_StepIdentity.
3. Supprimer les Text Block génériques.
4. Supprimer l'ancienne ComboBox de sexe si elle est encore visible.
5. Créer Border_IdentityFrame.
6. Créer HorizontalBox_IdentityLayout.
7. Créer VerticalBox_IdentityForm à gauche.
8. Ajouter Text_IdentityTitle.
9. Ajouter Text_IdentityHelp.
10. Ajouter Text_NameLabel.
11. Placer EditableText_Name dans SizeBox_NameInput.
12. Ajouter le bloc de rappel Race / Classe / Sexe.
13. Ajouter Text_PortraitVariantLabel.
14. Placer ComboBox_PortraitVariant dans SizeBox_PortraitVariantCombo.
15. Ajouter Text_PortraitDescriptionTitle.
16. Placer Text_PortraitDescription dans Border_PortraitDescriptionFrame.
17. Créer la colonne droite avec Border_IdentityPortraitFrame.
18. Placer Image_Portrait dans Overlay_IdentityPortrait.
19. Ajouter la légende Portrait final.
20. Compiler WBP_CharacterCreationWizard.
21. Sauvegarder.
22. Lancer PIE.
23. Vérifier que le nom peut être saisi.
24. Vérifier que la ComboBox de portrait est alimentée.
25. Vérifier que Image_Portrait change quand la variante change.
```

---

## 16. Critères de validation

L'écran Identité est validé lorsque :

```text
- aucun Text Block générique n'est visible ;
- le champ de nom est clair et utilisable ;
- la ComboBox de portrait est lisible ;
- la description du portrait est affichée dans un cadre propre ;
- le portrait est bien cadré ;
- le sexe n'est pas redemandé ici par ComboBox ;
- les choix Race / Classe / Sexe sont rappelés clairement ;
- la navigation Précédent / Suivant fonctionne ;
- le résumé final reçoit bien les choix effectués.
```

---

## 17. Note pour une future extraction en sous-widget

Comme pour l'étape Caractéristiques, l'étape Identité pourra être extraite plus tard dans un sous-widget :

```text
WBP_CCStep_Identity
Parent Class = RPGCharacterCreationIdentityStepWidget
```

Mais cette extraction n'est pas nécessaire immédiatement.

Priorité actuelle : nettoyer l'UMG dans `Panel_StepIdentity` et conserver les bindings C++ existants.
