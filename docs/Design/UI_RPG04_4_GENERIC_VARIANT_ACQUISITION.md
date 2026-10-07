# UI-RPG04.4 — Acquisition générique des talents à variantes

Date : **7 octobre 2026**  
État : **SOURCE PRÊTE — validation locale UE5.5.4 + matérialisation UMG requises**

## Objectif

Gérer tous les nœuds conceptuels contenant plusieurs `ChoiceId` avec un seul mécanisme UI générique.

Exemples actuellement couverts :

- Guerrier — **Spécialisation martiale** : 3 variantes ;
- Rôdeur — **Ennemi juré** : N variantes ;
- Mage — **Affinité élémentaire** : 4 variantes ;
- Mage — **Imprégnation** : 4 variantes.

Aucune logique spécifique à une classe n’est ajoutée.

## Flux fonctionnel

```text
nœud conceptuel à variantes sélectionné
→ CHOISIR UNE VARIANTE
→ ouverture du ComboBoxString générique
→ choix d’une variante concrète
→ CONFIRMER / ANNULER
→ OnAcquireConfirmed(ChoiceId concret)
→ UGridSkillsWidget::CommitConfirmedTalentChoice()
→ FRPGClassProgressionTransactionService::TryCommitChoices()
→ refresh complet
```

Le `ChoiceId` concret reste l’unité réellement persistée.

## Autorité métier

L’UMG ne décide pas :

- du coût ;
- du niveau minimum ;
- des prérequis ;
- de l’exclusivité entre variantes ;
- du budget de points ;
- de la validité finale de la transaction.

Toutes ces règles restent sous l’autorité de :

```cpp
FRPGClassProgressionTransactionService::TryCommitChoices(...)
```

## Libellés courts des variantes

Les données de production peuvent contenir :

```text
Spécialisation martiale — Tranchant
Spécialisation martiale — Perforant
Spécialisation martiale — Contondant
```

Le panneau de détail retire automatiquement le préfixe conceptuel et affiche :

```text
Variantes : Tranchant / Perforant / Contondant
```

Après acquisition :

```text
Variante choisie : Tranchant
```

Le même principe s’applique aux autres talents à variantes.

# Matérialisation Designer dans WBP_RPGTalentDetail

## Principe important

**On ne recrée pas `VB_Detail`.**

`VB_Detail` existe déjà dans `WBP_RPGTalentDetail`.

Il reste le conteneur vertical principal du panneau de détail.

UI-RPG04.4 consiste uniquement à **ajouter deux nouveaux enfants dans ce `VB_Detail` existant** :

```text
Button_ChooseVariant
Combo_VariantChoice
```

Aucun autre conteneur n’est nécessaire.

## Hiérarchie complète attendue après UI-RPG04.4

La hiérarchie finale de `WBP_RPGTalentDetail` doit être :

```text
Border_DetailRoot                         [Border]
└── VB_Detail                             [VerticalBox] EXISTANT
    ├── SB_DetailAccent                   [SizeBox]
    │   └── Border_DetailAccent           [Border] VARIABLE
    ├── Spacer_DetailAccent               [Spacer]
    ├── Text_DetailName                   [TextBlock] VARIABLE
    ├── Spacer_DetailName                 [Spacer]
    ├── Text_DetailDescription            [TextBlock] VARIABLE
    ├── Spacer_DetailDescription          [Spacer]
    ├── HB_DetailFacts                    [HorizontalBox]
    │   ├── Text_DetailLevel              [TextBlock] VARIABLE
    │   ├── Spacer_DetailFacts            [Spacer]
    │   └── Text_DetailCost               [TextBlock] VARIABLE
    ├── Spacer_DetailFactsBottom          [Spacer]
    ├── Text_DetailState                  [TextBlock] VARIABLE
    ├── Spacer_DetailState                [Spacer]
    ├── Text_DetailVariants               [TextBlock] VARIABLE
    ├── Spacer_AcquireTop                 [Spacer] EXISTANT
    ├── Button_AcquireTalent              [Button] VARIABLE, EXISTANT
    │   └── Text_AcquireTalent            [TextBlock]
    ├── Button_ChooseVariant              [Button] VARIABLE, NOUVEAU
    │   └── Text_ChooseVariant            [TextBlock] NOUVEAU
    ├── Combo_VariantChoice               [ComboBoxString] VARIABLE, NOUVEAU
    ├── Spacer_AcquirePrompt              [Spacer] EXISTANT
    ├── Text_AcquirePrompt                [TextBlock] VARIABLE, EXISTANT
    ├── HB_AcquireConfirm                 [HorizontalBox] EXISTANT
    │   ├── Button_ConfirmAcquire         [Button] VARIABLE, EXISTANT
    │   │   └── Text_ConfirmAcquire       [TextBlock]
    │   └── Button_CancelAcquire          [Button] VARIABLE, EXISTANT
    │       └── Text_CancelAcquire        [TextBlock]
    ├── Spacer_AcquireFeedback            [Spacer] EXISTANT
    └── Text_AcquireFeedback              [TextBlock] VARIABLE, EXISTANT
```

## Où ajouter les deux nouveaux widgets

Dans le Designer de `WBP_RPGTalentDetail` :

1. développer `Border_DetailRoot` ;
2. développer `VB_Detail` ;
3. repérer `Button_AcquireTalent` ;
4. insérer **juste après `Button_AcquireTalent`** :
   - `Button_ChooseVariant` ;
   - puis `Combo_VariantChoice` ;
5. laisser `Spacer_AcquirePrompt`, `Text_AcquirePrompt` et `HB_AcquireConfirm` à leur place, après le ComboBox.

Il ne faut déplacer aucun des blocs de description, niveau, coût ou état.

## Button_ChooseVariant

Type :

```text
Button
```

Nom exact :

```text
Button_ChooseVariant
```

Réglages :

```text
Is Variable = ON
Is Enabled  = ON
Visibility  = Collapsed
```

VerticalBox Slot :

```text
Size                 = Auto
Horizontal Alignment = Fill
Vertical Alignment   = Center
Padding              = 0
```

Enfant :

```text
Text_ChooseVariant
```

Texte :

```text
CHOISIR UNE VARIANTE
```

Réglages du TextBlock :

```text
Is Variable = OFF
Visibility  = Not Hit-Testable (Self & All Children)
Justification = Center
```

Le C++ décide quand le bouton apparaît.

## Combo_VariantChoice

Type :

```text
ComboBoxString
```

Nom exact :

```text
Combo_VariantChoice
```

Réglages :

```text
Is Variable = ON
Is Enabled  = ON
Visibility  = Collapsed
Max List Height = 300
```

VerticalBox Slot :

```text
Size                 = Auto
Horizontal Alignment = Fill
Vertical Alignment   = Center
Padding              = 0
```

**Ne saisir aucune option manuellement dans le Designer.**

Le C++ remplit dynamiquement la liste à partir de :

```text
NodeView.Variants[]
```

Le mapping entre texte affiché et `ChoiceId` reste interne au widget C++.

## Visibilité pilotée par le C++

### Talent simple disponible

```text
Button_AcquireTalent = Visible
Button_ChooseVariant = Collapsed
Combo_VariantChoice  = Collapsed
```

### Talent multi-variante disponible

```text
Button_AcquireTalent = Collapsed
Button_ChooseVariant = Visible
Combo_VariantChoice  = Collapsed
```

### Après clic sur CHOISIR UNE VARIANTE

```text
Button_ChooseVariant = Collapsed
Combo_VariantChoice  = Visible
Button_ConfirmAcquire = Visible mais désactivé tant qu’aucune variante n’est choisie
Button_CancelAcquire  = Visible
```

### Après sélection d’une variante

```text
Combo_VariantChoice  = Visible
Button_ConfirmAcquire = Visible et activé
Button_CancelAcquire  = Visible
```

## Exemple : Spécialisation martiale

Avant clic :

```text
Spécialisation martiale
Disponible
Variantes : Tranchant / Perforant / Contondant

[ CHOISIR UNE VARIANTE ]
```

Après clic :

```text
Choisissez une variante pour « Spécialisation martiale ».

[ Tranchant ▼ ]

[ CONFIRMER ] [ ANNULER ]
```

Avant qu’une valeur soit sélectionnée, `CONFIRMER` doit être désactivé.

Après sélection de `Tranchant` :

```text
Confirmer « Tranchant » pour « Spécialisation martiale » ?

[ CONFIRMER ] [ ANNULER ]
```

Après confirmation réussie :

```text
Spécialisation martiale
Acquis
Variante choisie : Tranchant
```

et le compteur de points de talent est rafraîchi.

## Hit-test

Conserver :

```text
Border_DetailRoot
Visibility = Not Hit-Testable (Self Only)
```

et :

```text
VB_Detail
Visibility = Visible
Is Enabled = true
```

Ne jamais mettre `VB_Detail`, `Border_DetailRoot` ou un parent des boutons en :

```text
Not Hit-Testable (Self & All Children)
```

sinon les boutons et le ComboBox cessent d’être interactifs.

## Blueprint

Aucun Event Graph n’est requis.

Ne pas ajouter manuellement :

- `OnClicked` ;
- `OnSelectionChanged` ;
- binding de texte ;
- options de ComboBox.

Le C++ fait tout cela.

## Tests Automation

```text
Grimrock.UI.RPG04.Variants.GenericNSelection
Grimrock.UI.RPG04.Variants.ShortLabels
Grimrock.UI.RPG04.Variants.Transaction
```

Avec les tests RPG04 précédents :

```text
Grimrock.UI.RPG04
→ 12 tests attendus
```

Résultat attendu :

```text
Succeeded              : 12
Succeeded with warnings: 0
Failed                 : 0
```

## Validation PIE

Vérifier :

```text
[ ] talent simple : ACQUÉRIR fonctionne toujours
[ ] talent multi-variante : CHOISIR UNE VARIANTE apparaît
[ ] ComboBox contient toutes les variantes
[ ] libellés courts corrects
[ ] CONFIRMER désactivé avant sélection
[ ] CONFIRMER activé après sélection
[ ] ANNULER ne dépense aucun point
[ ] CONFIRMER dépense le point via TryCommitChoices()
[ ] le nœud devient Acquis
[ ] Variante choisie : <nom> s’affiche
[ ] aucune variante sœur n’est acquise
[ ] compteur de points rafraîchi
```
