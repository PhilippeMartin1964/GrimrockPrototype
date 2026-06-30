# CC7.3 - Répartition interactive des caractéristiques

> Document unique à suivre pour construire l'écran **Caractéristiques** après l'extraction CC7.4.1.
>
> L'ancien document séparé `CHARACTER_CREATION_CC7_4_1_ATTRIBUTES_STEP_WIDGET.md` a été fusionné ici afin d'éviter deux sources concurrentes.

## 1. Objectif

CC7.3 transforme l'étape **Caractéristiques** du wizard de création de personnage en véritable écran de répartition de points.

Avant CC7.3, l'écran affichait seulement le total calculé :

```text
Caractéristiques = Attributs de classe + Bonus de race
```

Ce n'est pas suffisant. Le joueur doit pouvoir :

```text
- voir la réserve de points ;
- modifier les valeurs de classe avec + / - ;
- voir les bonus de race séparément ;
- voir le total final ;
- voir le modificateur ;
- comprendre les effets concrets de chaque caractéristique.
```

Le principe final est :

```text
Valeur de classe modifiable
+ Bonus racial fixe
= Total final
= Modificateur
= Effets lisibles
```

La classe reste la **répartition recommandée**. Le joueur peut la reprendre avec le bouton `Répartition recommandée`.

---

## 2. Architecture actuelle après CC7.4.1

Après CC7.4.1, l'écran Caractéristiques ne doit plus être construit directement dans `WBP_CharacterCreationWizard`.

La cible est maintenant :

```text
WBP_CharacterCreationWizard
└── WidgetSwitcher_Steps
    └── Panel_StepAttributes
        └── Widget_StepAttributes : WBP_CCStep_Attributes
```

`WBP_CCStep_Attributes` doit avoir pour classe parente C++ :

```text
RPGCharacterCreationAttributesStepWidget
```

Le widget principal garde :

```text
- l'état global de création ;
- la race sélectionnée ;
- la classe sélectionnée ;
- les attributs modifiés ;
- la validation finale ;
- la création du personnage.
```

Le sous-widget `WBP_CCStep_Attributes` possède seulement :

```text
- les boutons + / - ;
- les textes de valeurs ;
- les textes d'effets ;
- le bouton de réinitialisation ;
- l'affichage des points restants.
```

Il ne crée pas le personnage et ne modifie pas les DataAssets.

---

## 3. Fichiers C++ concernés

Fichiers du wizard :

```text
Source/GrimrockPrototype/Public/UI/RPGCharacterCreationWizardWidget.h
Source/GrimrockPrototype/Private/UI/RPGCharacterCreationWizardWidget.cpp
```

Fichiers du sous-widget Caractéristiques :

```text
Source/GrimrockPrototype/Public/UI/RPGCharacterCreationAttributesStepWidget.h
Source/GrimrockPrototype/Private/UI/RPGCharacterCreationAttributesStepWidget.cpp
```

Classe C++ du sous-widget :

```text
URPGCharacterCreationAttributesStepWidget
```

Nom du Blueprint à créer dans UE5 :

```text
WBP_CCStep_Attributes
```

Nom de l'instance dans `WBP_CharacterCreationWizard` :

```text
Widget_StepAttributes
```

Ce nom est important : le C++ du wizard possède un `BindWidgetOptional` nommé exactement `Widget_StepAttributes`.

---

## 4. Règle de répartition

La règle simple est :

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

Budget :

```text
(15-8) + (11-8) + (13-8) + (9-8) + (9-8) + (9-8)
= 7 + 3 + 5 + 1 + 1 + 1
= 18 points
```

Les bonus de race ne sont jamais modifiés par les boutons. Ils sont appliqués après la répartition.

---

## 5. Modificateurs

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

## 6. Affichage attendu

Exemple visuel :

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

## 7. Effets courts affichés

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

## 8. Création de `WBP_CCStep_Attributes`

Dans l'éditeur Unreal, créer un nouveau `User Widget` :

```text
Content/GrimrockPrototype/Blueprints/UI/CharacterCreation/WBP_CCStep_Attributes
```

Réglage principal :

```text
Parent Class = RPGCharacterCreationAttributesStepWidget
```

Ce widget contient toute l'interface de l'écran Caractéristiques.

Il est ensuite inséré dans :

```text
WBP_CharacterCreationWizard
└── WidgetSwitcher_Steps
    └── Panel_StepAttributes
        └── Widget_StepAttributes
```

Réglages de l'instance `Widget_StepAttributes` dans `WBP_CharacterCreationWizard` :

```text
Name        = Widget_StepAttributes
Is Variable = true
Horizontal Alignment = Fill
Vertical Alignment   = Fill
```

Dans un `Canvas Panel`, utiliser :

```text
Anchors   = Full
Offsets   = 0 ; 0 ; 0 ; 0
Alignment = 0 ; 0
```

---

## 9. Hiérarchie UMG recommandée dans `WBP_CCStep_Attributes`

### 9.1. Structure générale

Dans `WBP_CCStep_Attributes`, construire :

```text
WBP_CCStep_Attributes
└── SizeBox_AttributesOuter
    └── Border_AttributesFrame
        └── VerticalBox_AttributesContent
            ├── HorizontalBox_AttributesHeader
            │   ├── Text_AttributesTitle
            │   └── Text_AttributePointsRemaining
            ├── Text_AttributeHelp
            ├── GridPanel_Attributes
            │   ├── ligne 0 : en-têtes
            │   ├── ligne 1 : Force
            │   ├── ligne 2 : Dextérité
            │   ├── ligne 3 : Constitution
            │   ├── ligne 4 : Intelligence
            │   ├── ligne 5 : Sagesse
            │   └── ligne 6 : Charisme
            ├── HorizontalBox_DerivedStats
            │   ├── Text_HealthValue
            │   ├── Text_ManaValue
            │   └── Text_CarryWeightValue
            └── HorizontalBox_AttributeActions
                └── Button_ResetRecommendedAttributes
```

### 9.2. Réglages du cadre extérieur

`SizeBox_AttributesOuter` :

```text
Type            = Size Box
Is Variable     = false
Width Override  = 1260
Height Override = 650
```

`Border_AttributesFrame` :

```text
Type        = Border
Is Variable = false
Padding     = 24
Brush Color = R 0.00 / G 0.00 / B 0.00 / A 0.78
```

`VerticalBox_AttributesContent` :

```text
Type        = Vertical Box
Is Variable = false
Horizontal Alignment = Fill
Vertical Alignment   = Fill
```

### 9.3. En-tête

```text
HorizontalBox_AttributesHeader
├── Text_AttributesTitle
└── Text_AttributePointsRemaining
```

`Text_AttributesTitle` :

```text
Text        = Répartition des caractéristiques
Is Variable = false
Font Size   = 28
Color       = blanc
Justification = Left
Slot Size   = Fill
```

`Text_AttributePointsRemaining` :

```text
Text        = Points restants : 0 / 18
Is Variable = true
Font Size   = 24
Color       = R 1.00 / G 0.86 / B 0.45 / A 1.00
Justification = Right
Slot Size   = Auto
```

Le C++ met à jour `Text_AttributePointsRemaining`.

### 9.4. Texte d'aide

`Text_AttributeHelp` :

```text
Type        = Text Block
Is Variable = true
Text        = Les valeurs de classe peuvent être ajustées. Les bonus de race sont appliqués ensuite.
Font Size   = 18
Color       = R 0.82 / G 0.82 / B 0.82 / A 1.00
Auto Wrap Text = true
Slot Padding = 0 ; 0 ; 0 ; 16
```

Le C++ peut aussi le remplir automatiquement.

### 9.5. Colonnes du `GridPanel_Attributes`

Colonnes recommandées :

```text
0 Icône
1 Nom
2 Bouton -
3 Classe
4 Bouton +
5 Race
6 Total
7 Mod
8 Effets
```

Largeurs visuelles conseillées :

```text
0 Icône   : 48
1 Nom     : 160
2 -       : 42
3 Classe  : 70
4 +       : 42
5 Race    : 85
6 Total   : 80
7 Mod     : 70
8 Effets  : Fill
```

Pour chaque enfant du `GridPanel`, régler :

```text
Row
Column
Padding = 4 ; 3 ; 4 ; 3
Horizontal Alignment
Vertical Alignment = Center
```

### 9.6. Ligne d'en-tête

Créer les `Text Block` non variables suivants :

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
Justification = Center, sauf Nom et Effets en Left
Row = 0
```

### 9.7. Modèle d'une ligne

Chaque caractéristique suit le même modèle :

```text
Col 0 : Image_<Attribute>Icon
Col 1 : Text_<Attribute>Label
Col 2 : Button_<Attribute>Minus
Col 3 : Text_<Attribute>ClassValue
Col 4 : Button_<Attribute>Plus
Col 5 : Text_<Attribute>RaceBonus
Col 6 : Text_<Attribute>TotalValue
Col 7 : Text_<Attribute>Modifier
Col 8 : Text_<Attribute>Effects
```

`Image_*Icon` et `Text_*Label` sont purement visuels.

Les autres widgets doivent être variables et nommés exactement comme ci-dessous.

---

## 10. Widgets variables obligatoires

### 10.1. Globaux

```text
Text_AttributePointsRemaining
Text_AttributeHelp
Button_ResetRecommendedAttributes
```

Les valeurs dérivées peuvent rester dans le widget principal ou être dupliquées dans le sous-widget selon l'état de votre UMG. À terme, elles doivent être dans `WBP_CCStep_Attributes` :

```text
Text_HealthValue
Text_ManaValue
Text_CarryWeightValue
```

### 10.2. Force

```text
Button_StrengthMinus
Text_StrengthClassValue
Button_StrengthPlus
Text_StrengthRaceBonus
Text_StrengthTotalValue
Text_StrengthModifier
Text_StrengthEffects
```

### 10.3. Dextérité

```text
Button_DexterityMinus
Text_DexterityClassValue
Button_DexterityPlus
Text_DexterityRaceBonus
Text_DexterityTotalValue
Text_DexterityModifier
Text_DexterityEffects
```

### 10.4. Constitution

```text
Button_ConstitutionMinus
Text_ConstitutionClassValue
Button_ConstitutionPlus
Text_ConstitutionRaceBonus
Text_ConstitutionTotalValue
Text_ConstitutionModifier
Text_ConstitutionEffects
```

### 10.5. Intelligence

```text
Button_IntelligenceMinus
Text_IntelligenceClassValue
Button_IntelligencePlus
Text_IntelligenceRaceBonus
Text_IntelligenceTotalValue
Text_IntelligenceModifier
Text_IntelligenceEffects
```

### 10.6. Sagesse

```text
Button_WisdomMinus
Text_WisdomClassValue
Button_WisdomPlus
Text_WisdomRaceBonus
Text_WisdomTotalValue
Text_WisdomModifier
Text_WisdomEffects
```

### 10.7. Charisme

```text
Button_CharismaMinus
Text_CharismaClassValue
Button_CharismaPlus
Text_CharismaRaceBonus
Text_CharismaTotalValue
Text_CharismaModifier
Text_CharismaEffects
```

---

## 11. Détail des six lignes

### 11.1. Force

```text
Row = 1
Image_StrengthIcon              Is Variable = false
Text_StrengthLabel              Is Variable = false, Text = Force
Button_StrengthMinus            Is Variable = true
Text_StrengthClassValue         Is Variable = true
Button_StrengthPlus             Is Variable = true
Text_StrengthRaceBonus          Is Variable = true
Text_StrengthTotalValue         Is Variable = true
Text_StrengthModifier           Is Variable = true
Text_StrengthEffects            Is Variable = true
```

Boutons internes :

```text
Button_StrengthMinus -> TextBlock_StrengthMinusLabel = -
Button_StrengthPlus  -> TextBlock_StrengthPlusLabel  = +
```

### 11.2. Dextérité

```text
Row = 2
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

### 11.3. Constitution

```text
Row = 3
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

### 11.4. Intelligence

```text
Row = 4
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

### 11.5. Sagesse

```text
Row = 5
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

### 11.6. Charisme

```text
Row = 6
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

---

## 12. Réglages visuels communs

### 12.1. Icônes

```text
Desired Size Override = 36 x 36
Color and Opacity     = blanc
Horizontal Alignment  = Center
Vertical Alignment    = Center
Padding               = 4
```

Si une icône paraît écrasée, la placer dans un `SizeBox` de `40 x 40`.

### 12.2. Noms de caractéristiques

```text
Font Size      = 20
Color          = blanc
Justification  = Left
Padding        = 8 ; 2 ; 8 ; 2
```

### 12.3. Boutons `-` et `+`

```text
Type               = Button
Is Variable        = true
Min Desired Width  = 34
Min Desired Height = 30
```

Chaque bouton contient un `Text Block` non variable :

```text
Text = - ou +
Font Size = 20
Justification = Center
```

Le C++ active ou désactive automatiquement les boutons selon :

```text
- valeur minimale atteinte ;
- valeur maximale atteinte ;
- points restants disponibles.
```

### 12.4. Textes numériques

Pour `Text_*ClassValue`, `Text_*RaceBonus`, `Text_*TotalValue`, `Text_*Modifier` :

```text
Font Size     = 20
Justification = Center
```

Couleurs conseillées :

```text
Classe : blanc
Race   : doré léger, R 1.00 / G 0.82 / B 0.35 / A 1.00
Total  : blanc fort
Mod    : bleu pâle, R 0.70 / G 0.90 / B 1.00 / A 1.00
```

### 12.5. Textes d'effets

Pour `Text_*Effects` :

```text
Font Size      = 17
Color          = R 0.82 / G 0.82 / B 0.82 / A 1.00
Justification  = Left
Auto Wrap Text = true
Padding        = 12 ; 2 ; 4 ; 2
```

---

## 13. Valeurs dérivées en bas de l'écran

Hiérarchie :

```text
HorizontalBox_DerivedStats
├── Text_HealthValue
├── Text_ManaValue
└── Text_CarryWeightValue
```

Réglages communs :

```text
Is Variable = true
Font Size   = 22
Color       = blanc
Slot Size   = Fill
Padding     = 0 ; 0 ; 16 ; 0
```

Ces valeurs utilisent la répartition choisie par le joueur grâce aux overrides C++ du wizard.

---

## 14. Bouton de réinitialisation

Hiérarchie :

```text
HorizontalBox_AttributeActions
└── Button_ResetRecommendedAttributes
    └── Text_ResetRecommendedAttributesLabel
```

`Button_ResetRecommendedAttributes` :

```text
Type               = Button
Is Variable        = true
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

---

## 15. Fallback temporaire

CC7.4.1 garde un fallback :

```text
- si Widget_StepAttributes existe, le sous-widget est utilisé ;
- sinon, les anciens widgets directement placés dans WBP_CharacterCreationWizard continuent de fonctionner.
```

Ce fallback sera supprimé plus tard, lorsque `WBP_CCStep_Attributes` sera validé dans UE5.

---

## 16. Procédure UE5 recommandée

```text
1. Recompiler le projet C++.
2. Créer WBP_CCStep_Attributes.
3. Définir Parent Class = RPGCharacterCreationAttributesStepWidget.
4. Construire dans ce widget toute la hiérarchie décrite dans ce document.
5. Respecter exactement les noms C++ pour les widgets variables.
6. Ouvrir WBP_CharacterCreationWizard.
7. Aller dans WidgetSwitcher_Steps > Panel_StepAttributes.
8. Supprimer l'ancienne hiérarchie directe si elle gêne.
9. Ajouter une instance de WBP_CCStep_Attributes.
10. Renommer l'instance exactement Widget_StepAttributes.
11. Mettre Is Variable = true.
12. Régler le slot en Fill / Anchors Full.
13. Compiler WBP_CCStep_Attributes.
14. Compiler WBP_CharacterCreationWizard.
15. Sauvegarder.
16. Lancer PIE.
17. Tester les boutons + et -.
18. Vérifier les points restants.
19. Vérifier les bonus de race.
20. Créer un personnage et vérifier les attributs finaux.
```

---

## 17. Critères de validation

Cette étape est validée lorsque :

```text
- le projet C++ compile ;
- WBP_CCStep_Attributes existe avec la bonne classe parente ;
- WBP_CharacterCreationWizard contient une instance nommée Widget_StepAttributes ;
- les boutons + / - fonctionnent depuis le sous-widget ;
- les points restants se mettent à jour ;
- les bonus de race restent visibles et fixes ;
- les totaux et modificateurs se mettent à jour ;
- Santé, Mana et Charge max se recalculent ;
- le bouton Répartition recommandée réinitialise la classe ;
- la création finale du personnage conserve les attributs choisis.
```

---

## 18. Note d'implémentation

Le flux C++ reste :

```text
ClassDefinition->BaseAttributes
-> copie locale modifiable dans le wizard
-> boutons + / - dans WBP_CCStep_Attributes
-> appel du wizard pour modifier la copie
-> ajout des bonus de race
-> aperçu total
-> création du personnage avec les valeurs finales
```

Le sous-widget ne porte pas de logique métier définitive. Il affiche l'état et appelle le wizard.
