# CC7.6 - Résumé visuel avec icônes

## 1. Objet

CC7.6 transforme l'étape `Résumé` du wizard de création de personnage en une fiche de validation visuelle.

L'objectif n'est plus d'afficher un simple bloc de texte de secours, mais de préparer une présentation claire :

```text
Portrait du personnage
Nom
Race · Classe · Genre
Portrait sélectionné
Caractéristiques finales
Statistiques dérivées
État de validation
```

Cette étape reste une étape de validation. Les choix eux-mêmes continuent d'être faits dans les étapes précédentes : `Race`, `Classe`, `Caractéristiques` et `Identité`.

---

## 2. Suppression du texte statique obsolète

Dans `WBP_CharacterCreationWizard`, supprimer le `TextBlock` statique qui contient encore :

```text
Le résumé détaillé sera enrichi en CC7.2...
```

Ce texte ne vient pas du C++. Il est stocké dans le Blueprint UMG.

Chemin recommandé dans l'éditeur :

```text
WBP_CharacterCreationWizard
-> Widget Tree
-> Panel_StepSummary
-> supprimer le TextBlock obsolète
```

Le C++ alimente désormais les champs `Text_Summary*` et les images `Image_Summary*`.

---

## 3. Structure UMG recommandée

Remplacer l'ancien contenu de `Panel_StepSummary` par une structure de ce type :

```text
Panel_StepSummary
-> VerticalBox_SummaryRoot

   -> HorizontalBox_SummaryHeader
      -> Image_SummaryPortrait
      -> VerticalBox_SummaryIdentity
         -> Text_SummaryName
         -> Text_SummaryRace
         -> Text_SummaryClass
         -> Text_SummaryGender
         -> Text_SummaryPortrait

   -> VerticalBox_SummaryAttributes
      -> HorizontalBox_SummaryStrength
         -> Image_SummaryStrengthIcon
         -> Text_SummaryStrength
      -> HorizontalBox_SummaryDexterity
         -> Image_SummaryDexterityIcon
         -> Text_SummaryDexterity
      -> HorizontalBox_SummaryConstitution
         -> Image_SummaryConstitutionIcon
         -> Text_SummaryConstitution
      -> HorizontalBox_SummaryIntelligence
         -> Image_SummaryIntelligenceIcon
         -> Text_SummaryIntelligence
      -> HorizontalBox_SummaryWisdom
         -> Image_SummaryWisdomIcon
         -> Text_SummaryWisdom
      -> HorizontalBox_SummaryCharisma
         -> Image_SummaryCharismaIcon
         -> Text_SummaryCharisma

   -> HorizontalBox_SummaryDerivedStats
      -> Image_SummaryHealthIcon
      -> Text_SummaryHealth
      -> Image_SummaryManaIcon
      -> Text_SummaryMana
      -> Image_SummaryCarryWeightIcon
      -> Text_SummaryCarryWeight

   -> HorizontalBox_SummaryValidation
      -> Image_SummaryValidationIcon
      -> Text_SummaryValidationState
```

`Text_SummaryHelp` peut rester temporairement comme fallback de debug, mais il ne devrait plus être l'élément principal de présentation une fois la fiche visuelle en place.

---

## 4. Widgets C++ optionnels disponibles

Les widgets suivants sont bindés en `BindWidgetOptional`. Le Blueprint continue donc de compiler même s'ils ne sont pas encore tous présents.

### Identité finale

```text
Text_SummaryName
Text_SummaryRace
Text_SummaryClass
Text_SummaryGender
Text_SummaryPortrait
```

### Caractéristiques finales

```text
Text_SummaryStrength
Text_SummaryDexterity
Text_SummaryConstitution
Text_SummaryIntelligence
Text_SummaryWisdom
Text_SummaryCharisma
```

### Statistiques dérivées

```text
Text_SummaryHealth
Text_SummaryMana
Text_SummaryCarryWeight
```

### Validation

```text
Text_SummaryValidationState
```

### Images et icônes

```text
Image_SummaryPortrait
Image_SummaryClassIcon
Image_SummaryValidationIcon

Image_SummaryStrengthIcon
Image_SummaryDexterityIcon
Image_SummaryConstitutionIcon
Image_SummaryIntelligenceIcon
Image_SummaryWisdomIcon
Image_SummaryCharismaIcon

Image_SummaryHealthIcon
Image_SummaryManaIcon
Image_SummaryCarryWeightIcon
```

---

## 5. Propriétés d'icônes à renseigner dans le Blueprint

Dans les détails de `WBP_CharacterCreationWizard`, catégorie :

```text
RPG | Character Creation | Summary Icons
```

Renseigner les textures suivantes :

```text
SummaryReadyIcon
SummaryBlockedIcon
SummaryStrengthIcon
SummaryDexterityIcon
SummaryConstitutionIcon
SummaryIntelligenceIcon
SummaryWisdomIcon
SummaryCharismaIcon
SummaryHealthIcon
SummaryManaIcon
SummaryCarryWeightIcon
```

Les icônes de portrait et de classe ne sont pas à renseigner ici :

- `Image_SummaryPortrait` est alimentée depuis le portrait sélectionné dans les `PortraitSets` ;
- `Image_SummaryClassIcon` est alimentée depuis les `AvailableClassVisuals`.

---

## 6. Format des textes affichés

Le résumé compact affiche les informations dans cet esprit :

```text
Elias
Humain · Guerrier · Masculin
Portrait : Aëlric de Valombre
```

Puis les caractéristiques finales :

```text
Force : 15 + +1 = 16  mod +3
Dextérité : 11 + +1 = 12  mod +1
Constitution : 13 + +1 = 14  mod +2
Intelligence : 9 + +1 = 10  mod +0
Sagesse : 9 + +1 = 10  mod +0
Charisme : 9 + +1 = 10  mod +0
```

Puis les statistiques dérivées :

```text
PV : 20 | Mana : 0 | Charge max. : 80
```

Puis l'état final :

```text
Prêt à créer le personnage.
```

ou, si une condition n'est pas remplie :

```text
Création impossible : saisissez un nom.
Création impossible : le nom dépasse 24 caractères.
Création impossible : 2 point(s) à répartir.
```

---

## 7. Responsabilités C++ / UMG

Le C++ est responsable de :

```text
- calculer les valeurs finales ;
- appliquer les bonus raciaux ;
- calculer les statistiques dérivées ;
- résoudre le portrait sélectionné ;
- résoudre l'icône de classe ;
- choisir l'icône de validation ;
- alimenter les TextBlock et Image optionnels.
```

L'UMG est responsable de :

```text
- la mise en page ;
- les tailles des icônes ;
- les marges ;
- les couleurs ;
- les bordures ;
- le style médiéval fantastique.
```

Il ne faut pas déplacer la logique de calcul dans le Blueprint.

---

## 8. Réglages visuels recommandés

Pour les icônes :

```text
Taille : 32x32 ou 40x40
Alignement vertical : Center
Brush Draw As : Image
Color and Opacity : blanc par défaut, sauf style volontaire
```

Pour le portrait :

```text
Taille recommandée : 160x220 ou 180x240
Preserve Aspect Ratio : activé si disponible
Encadrement : Border sombre / doré / pierre selon le style final
```

Pour les lignes de caractéristiques :

```text
Icône à gauche
Texte à droite
Espacement horizontal : 8 à 12 px
Hauteur de ligne : 32 à 40 px
```

Pour l'état de validation :

```text
Image_SummaryValidationIcon
Text_SummaryValidationState
```

L'icône doit afficher `SummaryReadyIcon` si le personnage peut être créé, sinon `SummaryBlockedIcon`.

---

## 9. Test de validation

Après modification du Blueprint :

```text
1. Compiler le C++.
2. Ouvrir WBP_CharacterCreationWizard.
3. Supprimer le TextBlock obsolète du panneau Résumé.
4. Ajouter les Text_Summary* et Image_Summary* souhaités.
5. Renseigner les Summary*Icon dans les détails du widget.
6. Compiler le Blueprint.
7. Lancer Nouvelle partie.
8. Aller jusqu'à l'étape Résumé.
9. Vérifier le portrait, la classe, les icônes et l'état de validation.
10. Créer le personnage.
```

Aucun message de type `MissingRequiredWidget` ne doit apparaître, sauf si `Widget_StepAttributes` est absent.

---

## 10. Suite envisagée

Après CC7.6, les améliorations naturelles sont :

```text
CC7.7 - Style médiéval fantastique du résumé
CC7.8 - Remplacement progressif de Text_SummaryHelp par une fiche entièrement structurée
CC7.9 - Harmonisation des icônes de caractéristiques avec l'inventaire et la feuille de personnage
```
