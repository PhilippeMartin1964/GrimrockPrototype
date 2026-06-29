# CC7.3 - Répartition interactive des caractéristiques

## 1. Objectif

CC7.3 transforme l'étape **Caractéristiques** du wizard de création de personnage.

Avant CC7.3, l'écran affichait seulement le total calculé à partir de la classe et de la race :

```text
Caractéristiques = Attributs de classe + Bonus de race
```

Ce n'est pas suffisant pour un vrai écran de création de personnage. Le joueur doit pouvoir comprendre et modifier la répartition.

CC7.3 introduit donc :

```text
valeur de classe modifiable
+ bonus racial fixe
= total final
= modificateur
= effets lisibles
```

La classe reste la **répartition recommandée**. Le joueur peut ensuite redistribuer les points avec des boutons `+` et `-`.

---

## 2. Règle de répartition

La règle simple de CC7.3 est la suivante :

```text
Minimum avant bonus racial = 8
Maximum avant bonus racial = 16
Budget = somme des points de la classe au-dessus de 8
```

Exemple avec un guerrier :

```text
Force        15
Dextérité    11
Constitution 13
Intelligence  9
Sagesse       9
Charisme      9
```

Le budget est calculé ainsi :

```text
(15-8) + (11-8) + (13-8) + (9-8) + (9-8) + (9-8)
= 7 + 3 + 5 + 1 + 1 + 1
= 18 points
```

Les bonus de race ne sont jamais modifiés par les boutons. Ils sont appliqués après la répartition.

---

## 3. Affichage attendu

```text
Répartition des caractéristiques                  Points restants : 0 / 18

Les valeurs de classe peuvent être ajustées. Les bonus de race sont appliqués ensuite.

FOR  Force          [-] 15 [+]   Race +1   Total 16   Mod +3   Charge 80 · CàC +3
DEX  Dextérité      [-] 11 [+]   Race +1   Total 12   Mod +1   Précision +1 · Esquive +1
CON  Constitution   [-] 13 [+]   Race +1   Total 14   Mod +2   Santé 20 · Résistance +2
INT  Intelligence   [-]  9 [+]   Race +1   Total 10   Mod +0   Savoirs +0 · Alchimie +0
SAG  Sagesse        [-]  9 [+]   Race +1   Total 10   Mod +0   Perception +0 · Volonté +0
CHA  Charisme       [-]  9 [+]   Race +1   Total 10   Mod +0   Influence +0 · Recrutement +0

Santé : 20     Mana : 0     Charge max : 80
```

---

## 4. Modificateurs

Le modificateur utilise la formule RPG classique :

```text
Modificateur = floor((Valeur totale - 10) / 2)
```

Table de lecture :

```text
8-9   => -1
10-11 => +0
12-13 => +1
14-15 => +2
16-17 => +3
18-19 => +4
```

Le modificateur affiché utilise toujours la valeur finale, donc la valeur après bonus racial.

---

## 5. Effets courts affichés

| Caractéristique | Effets courts |
|---|---|
| Force | `Charge X · CàC +N` |
| Dextérité | `Précision +N · Esquive +N` |
| Constitution | `Santé X · Résistance +N` |
| Intelligence | `Savoirs +N · Alchimie +N` |
| Sagesse | `Perception +N · Volonté +N` |
| Charisme | `Influence +N · Recrutement +N` |

Ces effets sont une première lecture gameplay. Ils pourront être raffinés plus tard lorsque les systèmes de combat, de compétences, d'interaction et de recrutement seront plus avancés.

---

## 6. Widgets à prévoir dans `Panel_StepAttributes`

Widgets globaux :

```text
Text_AttributePointsRemaining
Text_AttributeHelp
Button_ResetRecommendedAttributes
```

Widgets par caractéristique, exemple pour Force :

```text
Button_StrengthMinus
Text_StrengthClassValue
Button_StrengthPlus
Text_StrengthRaceBonus
Text_StrengthTotalValue
Text_StrengthModifier
Text_StrengthEffects
```

Même logique pour :

```text
Dexterity
Constitution
Intelligence
Wisdom
Charisma
```

Le widget historique `Text_StrengthValue`, `Text_DexterityValue`, etc. peut rester utilisé pour le total final si nécessaire.

---

## 7. Hiérarchie UMG recommandée

### 7.1. Objectif de la hiérarchie

L'étape `Panel_StepAttributes` doit être construite comme un vrai écran de répartition, pas comme une simple liste de valeurs.

Elle doit permettre de lire immédiatement :

```text
- la réserve de points restante ;
- la valeur de classe modifiable ;
- le bonus racial fixe ;
- le total final ;
- le modificateur ;
- l'effet concret de la caractéristique ;
- les boutons + et - ;
- les valeurs dérivées Santé, Mana et Charge max.
```

Le C++ de CC7.3 pilote les widgets par leur nom. Il faut donc respecter exactement les noms des widgets variables listés plus bas.

### 7.2. Structure générale dans `WidgetSwitcher_Steps`

Dans `WBP_CharacterCreationWizard`, l'étape `Panel_StepAttributes` est un enfant de `WidgetSwitcher_Steps`.

Le panneau peut être un `Canvas Panel`. C'est le choix le plus simple, car il permet de placer un grand cadre centré dans l'écran.

Hiérarchie recommandée :

```text
Panel_StepAttributes                         Canvas Panel, Is Variable = true
└── SizeBox_AttributesOuter                  Size Box, Is Variable = false
    └── Border_AttributesFrame               Border, Is Variable = false
        └── VerticalBox_AttributesContent    Vertical Box, Is Variable = false
            ├── HorizontalBox_AttributesHeader
            │   ├── Text_AttributesTitle
            │   └── Text_AttributePointsRemaining
            ├── Text_AttributeHelp
            ├── GridPanel_Attributes
            ├── HorizontalBox_DerivedStats
            │   ├── Text_HealthValue
            │   ├── Text_ManaValue
            │   └── Text_CarryWeightValue
            └── HorizontalBox_AttributeActions
                └── Button_ResetRecommendedAttributes
```

### 7.3. Réglages du `Panel_StepAttributes`

`Panel_StepAttributes` :

```text
Type        = Canvas Panel
Is Variable = true
Visibility  = Visible
```

`SizeBox_AttributesOuter` :

```text
Type            = Size Box
Is Variable     = false
Width Override  = 1260
Height Override = 650
```

Slot du `SizeBox_AttributesOuter` dans `Panel_StepAttributes` :

```text
Anchors   = Center
Alignment = 0.5 ; 0.5
Position  = 0 ; 0
Size      = 1260 ; 650
```

`Border_AttributesFrame` :

```text
Type        = Border
Is Variable = false
Padding     = 24
Brush Color = R 0.00 / G 0.00 / B 0.00 / A 0.78
```

Cette couleur garde la cohérence avec les autres écrans du wizard : fond sombre, lisible, sobre.

`VerticalBox_AttributesContent` :

```text
Type        = Vertical Box
Is Variable = false
```

Slot du `VerticalBox_AttributesContent` dans `Border_AttributesFrame` :

```text
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Padding              = 0
```

### 7.4. En-tête de l'écran

Hiérarchie :

```text
HorizontalBox_AttributesHeader                 Horizontal Box, Is Variable = false
├── Text_AttributesTitle                       Text Block, Is Variable = false
└── Text_AttributePointsRemaining              Text Block, Is Variable = true
```

`HorizontalBox_AttributesHeader` :

```text
Padding dans le Vertical Box Slot = 0 ; 0 ; 0 ; 8
Horizontal Alignment              = Fill
Vertical Alignment                = Auto
```

`Text_AttributesTitle` :

```text
Text        = Répartition des caractéristiques
Is Variable = false
Font Size   = 28
Color       = blanc
Justification = Left
```

Slot de `Text_AttributesTitle` dans `HorizontalBox_AttributesHeader` :

```text
Size    = Fill
Padding = 0 ; 0 ; 16 ; 0
```

`Text_AttributePointsRemaining` :

```text
Text        = Points restants : 0 / 18
Is Variable = true
Font Size   = 24
Color       = blanc légèrement doré conseillé : R 1.00 / G 0.86 / B 0.45 / A 1.00
Justification = Right
```

Slot de `Text_AttributePointsRemaining` dans `HorizontalBox_AttributesHeader` :

```text
Size    = Auto
Padding = 16 ; 0 ; 0 ; 0
```

Le texte est mis à jour par le C++.

### 7.5. Texte d'aide

`Text_AttributeHelp` :

```text
Type        = Text Block
Is Variable = true
Text        = Les valeurs de classe peuvent être ajustées. Les bonus de race sont appliqués ensuite.
Font Size   = 18
Color       = R 0.82 / G 0.82 / B 0.82 / A 1.00
Auto Wrap Text = true
```

Slot dans `VerticalBox_AttributesContent` :

```text
Size    = Auto
Padding = 0 ; 0 ; 0 ; 16
```

Le texte peut être rempli par défaut dans le Designer, mais il est aussi mis à jour par le C++.

### 7.6. Grille principale `GridPanel_Attributes`

`GridPanel_Attributes` contient la ligne d'en-tête et les six lignes de caractéristiques.

```text
GridPanel_Attributes             Grid Panel, Is Variable = false
├── ligne 0 : en-têtes de colonnes
├── ligne 1 : Force
├── ligne 2 : Dextérité
├── ligne 3 : Constitution
├── ligne 4 : Intelligence
├── ligne 5 : Sagesse
└── ligne 6 : Charisme
```

Réglages du `GridPanel_Attributes` :

```text
Is Variable = false
```

Slot dans `VerticalBox_AttributesContent` :

```text
Size    = Fill
Padding = 0 ; 0 ; 0 ; 16
```

### 7.7. Colonnes du `GridPanel_Attributes`

Colonnes recommandées :

```text
Colonne 0 : Icône
Colonne 1 : Nom
Colonne 2 : Bouton -
Colonne 3 : Valeur de classe
Colonne 4 : Bouton +
Colonne 5 : Bonus de race
Colonne 6 : Total final
Colonne 7 : Modificateur
Colonne 8 : Effets
```

Largeurs visuelles conseillées :

```text
0 Icône           : 48
1 Nom             : 160
2 -               : 42
3 Classe          : 70
4 +               : 42
5 Race            : 85
6 Total           : 80
7 Mod             : 70
8 Effets          : Fill / le reste de la largeur
```

Dans UE5, un `Grid Panel` ne se règle pas exactement comme un tableau HTML. Il faut donc régler chaque enfant via son `Grid Slot` :

```text
Row
Column
Layer
Padding
Horizontal Alignment
Vertical Alignment
```

Pour tous les widgets du tableau, utiliser de préférence :

```text
Vertical Alignment = Center
Padding            = 4 ; 3 ; 4 ; 3
```

### 7.8. Ligne d'en-tête du tableau

Créer les `Text Block` suivants, non variables :

```text
Text_HeaderIcon       = ""
Text_HeaderName       = "Caractéristique"
Text_HeaderMinus      = ""
Text_HeaderClass      = "Classe"
Text_HeaderPlus       = ""
Text_HeaderRace       = "Race"
Text_HeaderTotal      = "Total"
Text_HeaderModifier   = "Mod"
Text_HeaderEffects    = "Effets"
```

Réglages communs :

```text
Is Variable = false
Font Size   = 16
Color       = R 0.75 / G 0.75 / B 0.75 / A 1.00
Justification = Center, sauf Text_HeaderName et Text_HeaderEffects en Left
```

Slots dans `GridPanel_Attributes` :

```text
Row = 0
Column = selon la colonne
Padding = 4 ; 0 ; 4 ; 6
```

### 7.9. Modèle d'une ligne de caractéristique

Chaque ligne suit exactement le même modèle.

Exemple complet pour Force :

```text
Row 1 / Col 0 : Image_StrengthIcon
Row 1 / Col 1 : Text_StrengthLabel
Row 1 / Col 2 : Button_StrengthMinus
Row 1 / Col 3 : Text_StrengthClassValue
Row 1 / Col 4 : Button_StrengthPlus
Row 1 / Col 5 : Text_StrengthRaceBonus
Row 1 / Col 6 : Text_StrengthTotalValue
Row 1 / Col 7 : Text_StrengthModifier
Row 1 / Col 8 : Text_StrengthEffects
```

Les noms suivants doivent être exacts, car le C++ les cherche directement :

```text
Button_StrengthMinus
Text_StrengthClassValue
Button_StrengthPlus
Text_StrengthRaceBonus
Text_StrengthTotalValue
Text_StrengthModifier
Text_StrengthEffects
```

`Image_StrengthIcon` et `Text_StrengthLabel` sont purement visuels. Ils ne sont pas lus par le C++.

### 7.10. Ligne Force

```text
Row = 1
```

Widgets :

```text
Image_StrengthIcon              Image, Is Variable = false
Text_StrengthLabel              Text Block, Is Variable = false, Text = Force
Button_StrengthMinus            Button, Is Variable = true
Text_StrengthClassValue         Text Block, Is Variable = true
Button_StrengthPlus             Button, Is Variable = true
Text_StrengthRaceBonus          Text Block, Is Variable = true
Text_StrengthTotalValue         Text Block, Is Variable = true
Text_StrengthModifier           Text Block, Is Variable = true
Text_StrengthEffects            Text Block, Is Variable = true
```

Textes internes des boutons :

```text
Button_StrengthMinus -> TextBlock_StrengthMinusLabel = -
Button_StrengthPlus  -> TextBlock_StrengthPlusLabel  = +
```

Les `TextBlock_*Label` placés dans les boutons ne doivent pas être variables.

### 7.11. Ligne Dextérité

```text
Row = 2
```

Widgets :

```text
Image_DexterityIcon
Text_DexterityLabel              Text = Dextérité
Button_DexterityMinus            Is Variable = true
Text_DexterityClassValue         Is Variable = true
Button_DexterityPlus             Is Variable = true
Text_DexterityRaceBonus          Is Variable = true
Text_DexterityTotalValue         Is Variable = true
Text_DexterityModifier           Is Variable = true
Text_DexterityEffects            Is Variable = true
```

### 7.12. Ligne Constitution

```text
Row = 3
```

Widgets :

```text
Image_ConstitutionIcon
Text_ConstitutionLabel              Text = Constitution
Button_ConstitutionMinus            Is Variable = true
Text_ConstitutionClassValue         Is Variable = true
Button_ConstitutionPlus             Is Variable = true
Text_ConstitutionRaceBonus          Is Variable = true
Text_ConstitutionTotalValue         Is Variable = true
Text_ConstitutionModifier           Is Variable = true
Text_ConstitutionEffects            Is Variable = true
```

### 7.13. Ligne Intelligence

```text
Row = 4
```

Widgets :

```text
Image_IntelligenceIcon
Text_IntelligenceLabel              Text = Intelligence
Button_IntelligenceMinus            Is Variable = true
Text_IntelligenceClassValue         Is Variable = true
Button_IntelligencePlus             Is Variable = true
Text_IntelligenceRaceBonus          Is Variable = true
Text_IntelligenceTotalValue         Is Variable = true
Text_IntelligenceModifier           Is Variable = true
Text_IntelligenceEffects            Is Variable = true
```

### 7.14. Ligne Sagesse

```text
Row = 5
```

Widgets :

```text
Image_WisdomIcon
Text_WisdomLabel              Text = Sagesse
Button_WisdomMinus            Is Variable = true
Text_WisdomClassValue         Is Variable = true
Button_WisdomPlus             Is Variable = true
Text_WisdomRaceBonus          Is Variable = true
Text_WisdomTotalValue         Is Variable = true
Text_WisdomModifier           Is Variable = true
Text_WisdomEffects            Is Variable = true
```

### 7.15. Ligne Charisme

```text
Row = 6
```

Widgets :

```text
Image_CharismaIcon
Text_CharismaLabel              Text = Charisme
Button_CharismaMinus            Is Variable = true
Text_CharismaClassValue         Is Variable = true
Button_CharismaPlus             Is Variable = true
Text_CharismaRaceBonus          Is Variable = true
Text_CharismaTotalValue         Is Variable = true
Text_CharismaModifier           Is Variable = true
Text_CharismaEffects            Is Variable = true
```

### 7.16. Réglages communs des icônes

Pour chaque icône :

```text
Image_StrengthIcon
Image_DexterityIcon
Image_ConstitutionIcon
Image_IntelligenceIcon
Image_WisdomIcon
Image_CharismaIcon
```

Réglages recommandés :

```text
Type        = Image
Is Variable = false
Brush Image = icône correspondante
Desired Size Override = 36 x 36
Color and Opacity = blanc
```

Slot dans le `GridPanel` :

```text
Column = 0
Horizontal Alignment = Center
Vertical Alignment   = Center
Padding              = 4
```

Si l'image paraît écrasée, placer l'`Image` dans un `SizeBox` de 40 x 40.

### 7.17. Réglages communs des noms de caractéristiques

Pour chaque nom :

```text
Text_StrengthLabel
Text_DexterityLabel
Text_ConstitutionLabel
Text_IntelligenceLabel
Text_WisdomLabel
Text_CharismaLabel
```

Réglages recommandés :

```text
Type        = Text Block
Is Variable = false
Font Size   = 20
Color       = blanc
Justification = Left
```

Slot dans le `GridPanel` :

```text
Column = 1
Horizontal Alignment = Fill
Vertical Alignment   = Center
Padding              = 8 ; 2 ; 8 ; 2
```

### 7.18. Réglages communs des boutons `-` et `+`

Tous ces boutons doivent être variables :

```text
Button_StrengthMinus
Button_StrengthPlus
Button_DexterityMinus
Button_DexterityPlus
Button_ConstitutionMinus
Button_ConstitutionPlus
Button_IntelligenceMinus
Button_IntelligencePlus
Button_WisdomMinus
Button_WisdomPlus
Button_CharismaMinus
Button_CharismaPlus
```

Réglages recommandés :

```text
Type        = Button
Is Variable = true
Min Desired Width  = 34
Min Desired Height = 30
```

Chaque bouton contient un `Text Block` non variable :

```text
Text = - ou +
Font Size = 20
Justification = Center
```

Slot dans le `GridPanel` :

```text
Column = 2 pour les boutons -
Column = 4 pour les boutons +
Horizontal Alignment = Center
Vertical Alignment   = Center
Padding              = 4
```

Le C++ active ou désactive automatiquement les boutons selon :

```text
- valeur minimale atteinte ;
- valeur maximale atteinte ;
- points restants disponibles.
```

### 7.19. Réglages communs des textes numériques

Sont concernés :

```text
Text_*ClassValue
Text_*RaceBonus
Text_*TotalValue
Text_*Modifier
```

Réglages recommandés :

```text
Type        = Text Block
Is Variable = true
Font Size   = 20
Justification = Center
Color       = blanc
```

Couleurs conseillées :

```text
Text_*ClassValue  : blanc
Text_*RaceBonus   : doré léger, R 1.00 / G 0.82 / B 0.35 / A 1.00
Text_*TotalValue  : blanc fort
Text_*Modifier    : bleu pâle ou vert pâle, R 0.70 / G 0.90 / B 1.00 / A 1.00
```

Le C++ remplit les valeurs sous la forme :

```text
Classe = 15
Race   = +1
Total  = 16
Mod    = +3
```

### 7.20. Réglages communs des effets

Sont concernés :

```text
Text_StrengthEffects
Text_DexterityEffects
Text_ConstitutionEffects
Text_IntelligenceEffects
Text_WisdomEffects
Text_CharismaEffects
```

Réglages recommandés :

```text
Type        = Text Block
Is Variable = true
Font Size   = 17
Color       = R 0.82 / G 0.82 / B 0.82 / A 1.00
Justification = Left
Auto Wrap Text = true
```

Slot dans le `GridPanel` :

```text
Column = 8
Horizontal Alignment = Fill
Vertical Alignment   = Center
Padding              = 12 ; 2 ; 4 ; 2
```

Textes remplis automatiquement par le C++ :

```text
Force        -> Charge X · CàC +N
Dextérité    -> Précision +N · Esquive +N
Constitution -> Santé X · Résistance +N
Intelligence -> Savoirs +N · Alchimie +N
Sagesse      -> Perception +N · Volonté +N
Charisme     -> Influence +N · Recrutement +N
```

### 7.21. Valeurs dérivées en bas de l'écran

Hiérarchie :

```text
HorizontalBox_DerivedStats            Horizontal Box, Is Variable = false
├── Text_HealthValue                  Text Block, Is Variable = true
├── Text_ManaValue                    Text Block, Is Variable = true
└── Text_CarryWeightValue             Text Block, Is Variable = true
```

Réglages du `HorizontalBox_DerivedStats` :

```text
Padding dans le Vertical Box Slot = 0 ; 8 ; 0 ; 12
Horizontal Alignment = Fill
```

Réglages communs des trois textes :

```text
Font Size = 22
Color     = blanc
Is Variable = true
```

Slot conseillé pour chaque texte :

```text
Size = Fill
Padding = 0 ; 0 ; 16 ; 0
```

Le C++ hérité du widget de base continue à mettre à jour :

```text
Text_HealthValue
Text_ManaValue
Text_CarryWeightValue
```

Grâce aux overrides CC7.3, ces valeurs utilisent la répartition choisie par le joueur.

### 7.22. Bouton de réinitialisation

Hiérarchie :

```text
HorizontalBox_AttributeActions
└── Button_ResetRecommendedAttributes
    └── Text_ResetRecommendedAttributesLabel
```

`HorizontalBox_AttributeActions` :

```text
Type        = Horizontal Box
Is Variable = false
Horizontal Alignment = Right
```

`Button_ResetRecommendedAttributes` :

```text
Type        = Button
Is Variable = true
Min Desired Width  = 260
Min Desired Height = 42
```

`Text_ResetRecommendedAttributesLabel` :

```text
Text        = Répartition recommandée
Is Variable = false
Font Size   = 18
Justification = Center
```

Le bouton est activé seulement si la répartition actuelle diffère de celle proposée par la classe.

### 7.23. Widgets historiques à conserver ou à ignorer

Les anciens widgets suivants peuvent rester dans l'UMG si vous les avez déjà :

```text
Text_StrengthValue
Text_DexterityValue
Text_ConstitutionValue
Text_IntelligenceValue
Text_WisdomValue
Text_CharismaValue
```

Mais pour l'écran CC7.3, il vaut mieux utiliser les nouveaux widgets :

```text
Text_*ClassValue
Text_*RaceBonus
Text_*TotalValue
Text_*Modifier
Text_*Effects
```

Si vous gardez les anciens `Text_*Value`, considérez-les comme des alias visuels du total final. Ne les utilisez pas en plus du tableau principal, sinon l'écran deviendra redondant.

### 7.24. Résumé des widgets qui doivent absolument être variables

Globaux :

```text
Text_AttributePointsRemaining
Text_AttributeHelp
Text_HealthValue
Text_ManaValue
Text_CarryWeightValue
Button_ResetRecommendedAttributes
```

Force :

```text
Button_StrengthMinus
Text_StrengthClassValue
Button_StrengthPlus
Text_StrengthRaceBonus
Text_StrengthTotalValue
Text_StrengthModifier
Text_StrengthEffects
```

Dextérité :

```text
Button_DexterityMinus
Text_DexterityClassValue
Button_DexterityPlus
Text_DexterityRaceBonus
Text_DexterityTotalValue
Text_DexterityModifier
Text_DexterityEffects
```

Constitution :

```text
Button_ConstitutionMinus
Text_ConstitutionClassValue
Button_ConstitutionPlus
Text_ConstitutionRaceBonus
Text_ConstitutionTotalValue
Text_ConstitutionModifier
Text_ConstitutionEffects
```

Intelligence :

```text
Button_IntelligenceMinus
Text_IntelligenceClassValue
Button_IntelligencePlus
Text_IntelligenceRaceBonus
Text_IntelligenceTotalValue
Text_IntelligenceModifier
Text_IntelligenceEffects
```

Sagesse :

```text
Button_WisdomMinus
Text_WisdomClassValue
Button_WisdomPlus
Text_WisdomRaceBonus
Text_WisdomTotalValue
Text_WisdomModifier
Text_WisdomEffects
```

Charisme :

```text
Button_CharismaMinus
Text_CharismaClassValue
Button_CharismaPlus
Text_CharismaRaceBonus
Text_CharismaTotalValue
Text_CharismaModifier
Text_CharismaEffects
```

### 7.25. Checklist de construction dans UE5

Procédure recommandée :

```text
1. Ouvrir WBP_CharacterCreationWizard.
2. Aller dans Panel_StepAttributes.
3. Supprimer l'ancienne liste simple si elle gêne.
4. Créer SizeBox_AttributesOuter.
5. Créer Border_AttributesFrame.
6. Créer VerticalBox_AttributesContent.
7. Créer HorizontalBox_AttributesHeader.
8. Ajouter Text_AttributesTitle et Text_AttributePointsRemaining.
9. Ajouter Text_AttributeHelp.
10. Ajouter GridPanel_Attributes.
11. Créer la ligne d'en-tête.
12. Créer les six lignes de caractéristiques.
13. Respecter exactement les noms C++ pour les widgets variables.
14. Ajouter HorizontalBox_DerivedStats.
15. Ajouter Text_HealthValue, Text_ManaValue et Text_CarryWeightValue.
16. Ajouter Button_ResetRecommendedAttributes.
17. Compiler le widget.
18. Sauvegarder.
19. Lancer PIE.
20. Tester les boutons + et -.
```

---

## 8. Critères de validation CC7.3

CC7.3 sera validée lorsque :

```text
- les boutons + / - modifient les valeurs de classe ;
- les points restants montent et descendent correctement ;
- les bonus de race restent visibles et fixes ;
- le total final change immédiatement ;
- le modificateur est mis à jour ;
- Santé, Mana et Charge max se recalculent ;
- le bouton Répartition recommandée réinitialise la classe ;
- la création du personnage conserve bien les valeurs finales choisies.
```

---

## 9. Note d'implémentation prévue

L'implémentation C++ doit rester dans le widget de création afin que le Graph Blueprint ne calcule rien.

Le flux recommandé est :

```text
ClassDefinition->BaseAttributes
-> copie locale modifiable
-> boutons + / -
-> ajout des bonus de race
-> aperçu total
-> création du personnage avec les valeurs finales
```
