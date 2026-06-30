# CC7.4.1 - Extraction de l'écran Caractéristiques

## 1. Objectif

CC7.4.1 extrait l'écran **Caractéristiques** du widget principal `WBP_CharacterCreationWizard` vers un sous-widget dédié.

L'objectif est d'éviter que `WBP_CharacterCreationWizard` devienne un très gros widget contenant toutes les lignes, tous les boutons et tous les textes de chaque étape.

La nouvelle cible est :

```text
WBP_CharacterCreationWizard
└── Panel_StepAttributes
    └── Widget_StepAttributes : WBP_CCStep_Attributes
```

`WBP_CCStep_Attributes` doit utiliser comme classe parente C++ :

```text
URPGCharacterCreationAttributesStepWidget
```

---

## 2. Principe d'architecture

Le wizard reste propriétaire de l'état global :

```text
- race sélectionnée ;
- classe sélectionnée ;
- sexe sélectionné ;
- portrait sélectionné ;
- nom du personnage ;
- répartition des caractéristiques ;
- création finale du personnage.
```

Le sous-widget `WBP_CCStep_Attributes` possède seulement l'interface de l'étape Caractéristiques :

```text
- textes de points restants ;
- boutons + / - ;
- textes Classe / Race / Total / Mod ;
- textes d'effets ;
- bouton Répartition recommandée.
```

Il ne crée pas le personnage et ne modifie pas directement les DataAssets.

---

## 3. Nouveaux fichiers C++

Ajoutés par CC7.4.1 :

```text
Source/GrimrockPrototype/Public/UI/RPGCharacterCreationAttributesStepWidget.h
Source/GrimrockPrototype/Private/UI/RPGCharacterCreationAttributesStepWidget.cpp
```

Classe ajoutée :

```text
URPGCharacterCreationAttributesStepWidget
```

Rôle :

```text
- se connecter au wizard ;
- lire les valeurs du wizard ;
- afficher les valeurs ;
- appeler le wizard quand un bouton + / - est cliqué ;
- rafraîchir son propre écran.
```

---

## 4. Création du Blueprint `WBP_CCStep_Attributes`

Dans l'éditeur Unreal :

```text
Content/GrimrockPrototype/Blueprints/UI/CharacterCreation/
```

Créer un nouveau `User Widget` :

```text
WBP_CCStep_Attributes
```

Parent Class :

```text
RPGCharacterCreationAttributesStepWidget
```

Le widget doit contenir la hiérarchie décrite dans CC7.3, mais seulement pour l'écran Caractéristiques.

---

## 5. Hiérarchie recommandée de `WBP_CCStep_Attributes`

Dans `WBP_CCStep_Attributes`, créer :

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
            ├── HorizontalBox_DerivedStats
            │   ├── Text_HealthValue
            │   ├── Text_ManaValue
            │   └── Text_CarryWeightValue
            └── HorizontalBox_AttributeActions
                └── Button_ResetRecommendedAttributes
```

Les widgets variables lus par le C++ doivent garder exactement les noms suivants :

```text
Text_AttributePointsRemaining
Text_AttributeHelp
Button_ResetRecommendedAttributes
```

Puis, pour chaque caractéristique :

```text
Button_StrengthMinus
Text_StrengthClassValue
Button_StrengthPlus
Text_StrengthRaceBonus
Text_StrengthTotalValue
Text_StrengthModifier
Text_StrengthEffects

Button_DexterityMinus
Text_DexterityClassValue
Button_DexterityPlus
Text_DexterityRaceBonus
Text_DexterityTotalValue
Text_DexterityModifier
Text_DexterityEffects

Button_ConstitutionMinus
Text_ConstitutionClassValue
Button_ConstitutionPlus
Text_ConstitutionRaceBonus
Text_ConstitutionTotalValue
Text_ConstitutionModifier
Text_ConstitutionEffects

Button_IntelligenceMinus
Text_IntelligenceClassValue
Button_IntelligencePlus
Text_IntelligenceRaceBonus
Text_IntelligenceTotalValue
Text_IntelligenceModifier
Text_IntelligenceEffects

Button_WisdomMinus
Text_WisdomClassValue
Button_WisdomPlus
Text_WisdomRaceBonus
Text_WisdomTotalValue
Text_WisdomModifier
Text_WisdomEffects

Button_CharismaMinus
Text_CharismaClassValue
Button_CharismaPlus
Text_CharismaRaceBonus
Text_CharismaTotalValue
Text_CharismaModifier
Text_CharismaEffects
```

---

## 6. Intégration dans `WBP_CharacterCreationWizard`

Dans `WBP_CharacterCreationWizard`, aller dans :

```text
WidgetSwitcher_Steps
└── Panel_StepAttributes
```

Puis remplacer l'ancienne hiérarchie de l'écran Caractéristiques par une instance de :

```text
WBP_CCStep_Attributes
```

Nom de l'instance dans le Designer :

```text
Widget_StepAttributes
```

Ce nom est important : le C++ du wizard possède maintenant un `BindWidgetOptional` nommé `Widget_StepAttributes`.

Réglages de l'instance :

```text
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

## 7. Fallback temporaire

Pour éviter de casser le travail déjà fait, CC7.4.1 garde un fallback :

```text
- si Widget_StepAttributes existe, le sous-widget est utilisé ;
- sinon, les anciens widgets directement placés dans WBP_CharacterCreationWizard continuent de fonctionner.
```

Ce fallback sera supprimé dans une étape ultérieure, une fois que `WBP_CCStep_Attributes` sera validé dans l'éditeur.

---

## 8. Critères de validation

CC7.4.1 est validée lorsque :

```text
- le projet C++ compile ;
- WBP_CCStep_Attributes peut être créé avec la bonne classe parente ;
- WBP_CharacterCreationWizard contient une instance nommée Widget_StepAttributes ;
- les boutons + / - fonctionnent depuis le sous-widget ;
- les points restants se mettent à jour ;
- les bonus de race restent visibles ;
- les totaux et modificateurs se mettent à jour ;
- la création finale du personnage conserve les attributs choisis.
```
