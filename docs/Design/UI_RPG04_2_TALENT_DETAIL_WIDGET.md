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
