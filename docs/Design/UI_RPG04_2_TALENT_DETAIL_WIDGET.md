# UI-RPG04.2A — Talent Detail Widget Contract

Date : **7 octobre 2026**  
État : **SOURCE PRÊTE — WBP à matérialiser après validation locale**

## Objectif

Créer le panneau read-only qui présente le Talent conceptuel sélectionné.

Aucune acquisition n'est introduite.

## Classe C++

```text
UGridTalentDetailWidget
```

Initialisation :

```text
InitializeTalentDetail(NodeView, BranchPresentation)
```

Affiche :

```text
nom
description
niveau requis
coût
état
variantes éventuelles
accent de branche
```

## Résolution de texte

La règle déjà utilisée par `UGridTalentNodeWidget` devient partagée :

```text
UGridTalentNodeWidget::ResolvePresentationText(...)
```

Ainsi le carré du Talent et le panneau de détail utilisent exactement la même identité UI.

## Intégration WBP_GridSkills

`UGridSkillsWidget` possède temporairement :

```text
Detail_Talent [BindWidgetOptional]
```

Le binding reste optionnel uniquement pendant 04.2A pour permettre au C++ de compiler avant la création du nouvel asset UMG.

Après matérialisation et validation de `WBP_RPGTalentDetail`, il pourra devenir `BindWidget` obligatoire.

## Hiérarchie Designer requise pour WBP_RPGTalentDetail

Parent C++ :

```text
UGridTalentDetailWidget
```

Hiérarchie minimale :

```text
Border_DetailRoot
└── VB_Detail
    ├── Border_DetailAccent          Variable
    ├── Text_DetailName              Variable
    ├── Text_DetailDescription       Variable
    ├── Text_DetailLevel             Variable
    ├── Text_DetailCost              Variable
    ├── Text_DetailState             Variable
    └── Text_DetailVariants          Variable
```

Pour 04.2A/04.2B, aucun bouton d'achat n'est ajouté.

## Intégration dans WBP_GridSkills

Ajouter une instance :

```text
Detail_Talent : WBP_RPGTalentDetail
```

dans la page Talents, à côté ou sous les trois branches selon la composition finale choisie.

Le C++ remplit automatiquement le détail lors du clic sur un nœud.

## Tests

```text
Grimrock.UI.RPG04.Detail.SimpleNode
Grimrock.UI.RPG04.Detail.VariantNode
Grimrock.UI.RPG04.Detail.SharedResolver
```

Validation :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG04.Detail"
```

Attendu : 3/3, 0 warning, 0 failed.


# UI-RPG04.2B — MATÉRIALISATION DESIGNER EXACTE

## 1. Asset à créer

Créer :

```text
/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/RPG/WBP_RPGTalentDetail
```

Parent Class :

```text
UGridTalentDetailWidget
```

Ne pas créer de Graph Blueprint.

Le widget est strictement de présentation.

## 2. Preview Designer

Pour faciliter l'édition :

```text
Screen Size / Custom : 380 × 620
```

La taille de preview n'est pas une contrainte runtime.

## 3. Hiérarchie complète WBP_RPGTalentDetail

Construire exactement :

```text
Border_DetailRoot                         [Border]
└── VB_Detail                             [VerticalBox]
    ├── SB_DetailAccent                   [SizeBox]
    │   └── Border_DetailAccent           [Border] VARIABLE = YES
    │
    ├── Spacer_DetailAccent               [Spacer]
    ├── Text_DetailName                   [TextBlock] VARIABLE = YES
    ├── Spacer_DetailName                 [Spacer]
    ├── Text_DetailDescription            [TextBlock] VARIABLE = YES
    ├── Spacer_DetailDescription          [Spacer]
    │
    ├── HB_DetailFacts                    [HorizontalBox]
    │   ├── Text_DetailLevel              [TextBlock] VARIABLE = YES
    │   ├── Spacer_DetailFacts            [Spacer]
    │   └── Text_DetailCost               [TextBlock] VARIABLE = YES
    │
    ├── Spacer_DetailFactsBottom          [Spacer]
    ├── Text_DetailState                  [TextBlock] VARIABLE = YES
    ├── Spacer_DetailState                [Spacer]
    └── Text_DetailVariants               [TextBlock] VARIABLE = YES
```

Tous les widgets marqués VARIABLE doivent avoir **Is Variable = ON**.

## 4. Border_DetailRoot

Réglages :

```text
Padding              : 18 / 16 / 18 / 16
Brush Color          : R 0.025 / G 0.028 / B 0.032 / A 0.96
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Visibility           : Not Hit-Testable (Self Only)
Is Enabled           : true
```

Important :

```text
Not Hit-Testable (Self Only)
```

et surtout pas :

```text
Not Hit-Testable (Self & All Children)
```

Le panneau est read-only aujourd'hui, mais UI-RPG04.3 ajoutera des contrôles interactifs dedans.

## 5. VB_Detail

Dans le Border Slot :

```text
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Padding              : 0
```

Visibility :

```text
Visible
```

## 6. Accent de branche

### SB_DetailAccent

```text
Height Override : 4
VerticalBox Slot: Auto
```

### Border_DetailAccent

```text
Is Variable  : ON
Brush Color  : R 0.45 / G 0.35 / B 0.16 / A 1
Visibility   : Not Hit-Testable (Self & All Children)
```

La couleur fallback n'a pas d'autorité. Le runtime la remplace avec :

```text
BranchPresentation.AccentColor
```

### Spacer_DetailAccent

```text
Size Y            : 12
VerticalBox Slot  : Auto
```

## 7. Nom du Talent

### Text_DetailName

```text
Is Variable      : ON
Text             : Talent sélectionné
Font             : Alegreya Sans
Font Size        : 24
Justification    : Left
Auto Wrap Text   : ON
Color            : R 0.92 / G 0.90 / B 0.84 / A 1
Visibility       : Not Hit-Testable (Self & All Children)
VerticalBox Slot : Auto
```

Le texte Designer est uniquement un placeholder.

### Spacer_DetailName

```text
Size Y : 10
Slot   : Auto
```

## 8. Description

### Text_DetailDescription

```text
Is Variable      : ON
Text             : Description du talent.
Font             : Alegreya Sans
Font Size        : 16
Justification    : Left
Auto Wrap Text   : ON
Wrapping Policy  : Default
Color            : R 0.78 / G 0.78 / B 0.75 / A 1
Visibility       : Not Hit-Testable (Self & All Children)
VerticalBox Slot : Auto
```

### Spacer_DetailDescription

```text
Size Y : 18
Slot   : Auto
```

## 9. Ligne Niveau / Coût

### HB_DetailFacts

```text
VerticalBox Slot
    Size                 : Auto
    Horizontal Alignment : Fill
    Vertical Alignment   : Center
```

### Text_DetailLevel

```text
Is Variable   : ON
Text          : Niveau requis : 2
Font Size     : 15
Auto Wrap     : OFF
Visibility    : Not Hit-Testable (Self & All Children)

HorizontalBox Slot
    Size                 : Auto
    Horizontal Alignment : Left
    Vertical Alignment   : Center
```

### Spacer_DetailFacts

```text
HorizontalBox Slot
    Size        : Fill
    Fill Weight : 1.0
```

### Text_DetailCost

```text
Is Variable   : ON
Text          : Coût : 1 point
Font Size     : 15
Auto Wrap     : OFF
Visibility    : Not Hit-Testable (Self & All Children)

HorizontalBox Slot
    Size                 : Auto
    Horizontal Alignment : Right
    Vertical Alignment   : Center
```

### Spacer_DetailFactsBottom

```text
Size Y : 12
Slot   : Auto
```

## 10. État

### Text_DetailState

```text
Is Variable      : ON
Text             : Disponible
Font Size        : 16
Auto Wrap        : OFF
Color            : R 0.88 / G 0.82 / B 0.62 / A 1
Visibility       : Not Hit-Testable (Self & All Children)
VerticalBox Slot : Auto
```

Le runtime peut afficher :

```text
Acquis
Disponible
Niveau requis
Prérequis manquant
Points insuffisants
Choix exclusif
```

### Spacer_DetailState

```text
Size Y : 14
Slot   : Auto
```

## 11. Variantes

### Text_DetailVariants

```text
Is Variable      : ON
Text             : Variantes : Tranchant / Perforant / Contondant
Font Size        : 15
Auto Wrap Text   : ON
Color            : R 0.72 / G 0.72 / B 0.70 / A 1
Visibility       : Not Hit-Testable (Self & All Children)
VerticalBox Slot : Auto
```

Pour un Talent simple, le runtime met ce texte à vide.

Ne pas créer de boutons de variantes dans UI-RPG04.2B.

## 12. Intégration exacte dans WBP_GridSkills

Dans :

```text
Border_TalentsPage
└── Overlay_TalentsPage
```

la structure actuelle contient :

```text
Border_TalentsInner
HB_TalentBranches
```

Conserver `Border_TalentsInner`.

Créer un nouveau :

```text
HB_TalentsContent [HorizontalBox]
```

et déplacer l'actuel `HB_TalentBranches` dedans.

Résultat :

```text
Border_TalentsPage
└── Overlay_TalentsPage
    ├── Border_TalentsInner
    └── HB_TalentsContent                  [HorizontalBox]
        ├── HB_TalentBranches              [HorizontalBox] EXISTANT
        │   ├── Border_BranchLeft
        │   │   └── Branch_Left
        │   ├── ...
        │   ├── Border_BranchCenter
        │   │   └── Branch_Center
        │   ├── ...
        │   └── Border_BranchRight
        │       └── Branch_Right
        │
        ├── Spacer_TalentDetail            [Spacer]
        └── SB_TalentDetail                [SizeBox]
            └── Detail_Talent              [WBP_RPGTalentDetail] VARIABLE = YES
```

Ne pas reconstruire les trois branches.

Ne pas renommer :

```text
Branch_Left
Branch_Center
Branch_Right
```

## 13. HB_TalentsContent

Dans Overlay Slot :

```text
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Padding              : 12
```

Visibility :

```text
Visible
```

## 14. HB_TalentBranches existant

Dans son nouveau HorizontalBox Slot :

```text
Size                 : Fill
Fill Weight          : 1.0
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Padding              : 0
```

Sa structure interne ne change pas.

## 15. Spacer_TalentDetail

```text
Size X : 14
```

HorizontalBox Slot :

```text
Size : Auto
```

## 16. SB_TalentDetail

```text
Width Override : 380
```

HorizontalBox Slot :

```text
Size                 : Auto
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Padding              : 0
```

## 17. Detail_Talent

Type :

```text
WBP_RPGTalentDetail
```

Nom exact :

```text
Detail_Talent
```

Réglages :

```text
Is Variable           : ON
Visibility            : Visible
Horizontal Alignment  : Fill
Vertical Alignment    : Fill
```

Aucun binding Blueprint.

Aucun Event Graph.

Le C++ de `UGridSkillsWidget` l'alimente automatiquement.

## 18. Comportement attendu en PIE

À l'ouverture :

```text
Detail_Talent vide
```

Cliquer sur un Talent simple :

```text
nom              = nom réel
description      = description réelle
niveau requis    = valeur du read model
coût             = valeur du read model
état             = état réel
variantes        = vide
```

Cliquer sur `Spécialisation martiale` :

```text
nom       = Spécialisation martiale
variantes = Tranchant / Perforant / Contondant
```

Cliquer sur un Talent verrouillé doit aussi afficher son détail.

Aucun clic de Talent ne dépense de point dans 04.2B.

## 19. Vérification hit-test

Compte tenu du correctif précédent, contrôler explicitement :

```text
Panel_GridSkillsDesignerRoot
    Is Enabled = true

HB_TalentsContent
    Visibility = Visible

HB_TalentBranches
    Visibility = Visible

WBP_RPGTalentDetail / Border_DetailRoot
    Visibility = Not Hit-Testable (Self Only)
```

Ne jamais mettre un parent des `WBP_RPGTalentNode` en :

```text
Not Hit-Testable (Self & All Children)
```

## 20. Validation UI-RPG04.2B

Après Compile + Save :

```text
[ ] K ouvre Skills
[ ] TALENTS fonctionne
[ ] les 15 nœuds restent cliquables
[ ] clic nœud simple -> détail correct
[ ] clic nœud multi-variante -> détail conceptuel correct
[ ] clic nœud verrouillé -> détail visible
[ ] aucun Talent Point dépensé
[ ] les 3 branches restent lisibles
[ ] aucun Graph Blueprint ajouté
```

Puis :

```powershell
git status --short --untracked-files=all
```

Les fichiers attendus pour 04.2B sont uniquement :

```text
Content/GrimrockPrototype/Blueprints/UI/InGameMenu/RPG/WBP_RPGTalentDetail.uasset
Content/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSkills.uasset
```

Ne pas inclure `L_Dungeon.umap`.
