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

Dans `Panel_StepAttributes`, utiliser de préférence un `GridPanel` :

```text
Panel_StepAttributes
-> Border_AttributesFrame
   -> VerticalBox_AttributesContent
      -> Text_AttributesTitle
      -> Text_AttributeHelp
      -> Text_AttributePointsRemaining
      -> GridPanel_Attributes
         -> ligne d'en-tête
         -> ligne Force
         -> ligne Dextérité
         -> ligne Constitution
         -> ligne Intelligence
         -> ligne Sagesse
         -> ligne Charisme
      -> HorizontalBox_DerivedStats
         -> Text_HealthValue
         -> Text_ManaValue
         -> Text_CarryWeightValue
      -> Button_ResetRecommendedAttributes
```

Colonnes conseillées :

```text
0 Icône
1 Nom
2 -
3 Classe
4 +
5 Race
6 Total
7 Mod
8 Effets
```

Largeurs de départ :

```text
Icône   : 48
Nom     : 150
-       : 40
Classe  : 50
+       : 40
Race    : 70
Total   : 70
Mod     : 70
Effets  : Fill
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
