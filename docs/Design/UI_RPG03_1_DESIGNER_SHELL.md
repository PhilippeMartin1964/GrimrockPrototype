# UI-RPG03.1A — Real WBP_GridSkills Designer Shell Contract

Date : **7 octobre 2026**  
Parent : **UI-RPG03 — Real UMG Talent Tree**  
État : **SOURCE PRÊTE — build local puis modification visuelle du WBP requise**

## Objectif

Permettre à `WBP_GridSkills` de devenir enfin un vrai écran UMG Designer sans être écrasé par le renderer natif MON20.

Le changement est progressif :

```text
WBP legacy sans marker
    -> renderer C++ MON20 conservé

WBP avec Panel_GridSkillsDesignerRoot
    -> renderer natif désactivé
    -> layout UMG réel conservé
    -> header / tabs / titres de branches alimentés par UGridSkillsWidget
```

Le fallback sera supprimé définitivement seulement en UI-RPG03.3, après validation du nouvel écran.

## Noms UMG contractuels

Le Designer doit créer les widgets suivants avec **Is Variable** activé :

```text
Panel_GridSkillsDesignerRoot
Text_CharacterName
Text_ClassLevel
Text_TalentPoints
Button_SkillsTab
Button_TalentsTab
Switcher_SkillsTalents
Text_BranchLeft
Text_BranchCenter
Text_BranchRight
```

Tous sont `BindWidgetOptional` pendant la migration. Le marker qui active le vrai rendu UMG est `Panel_GridSkillsDesignerRoot`.

## Comportement C++

Le bridge remplit automatiquement :

```text
Text_CharacterName  <- FGridSkillsPageView.CharacterName
Text_ClassLevel     <- ClassDisplayName + niveau
Text_TalentPoints   <- RemainingTalentPoints
Text_BranchLeft     <- présentation branche index 0
Text_BranchCenter   <- présentation branche index 1
Text_BranchRight    <- présentation branche index 2
```

Les titres et couleurs viennent de `DA_RPGTalentPresentation`; le matching gameplay reste basé sur `TalentBranchId`.

Les boutons sont reliés en C++ :

```text
Button_SkillsTab  -> Switcher index 0
Button_TalentsTab -> Switcher index 1
```

Aucun Graph Blueprint n'est requis pour ce shell.

## Arbre Designer cible pour UI-RPG03.1B

Conserver le `Border` racine existant de `WBP_GridSkills`, puis construire :

```text
Border (root existant)
└── Overlay / Panel_GridSkillsDesignerRoot
    └── VerticalBox
        ├── Header
        │   ├── Text_CharacterName
        │   ├── Text_ClassLevel
        │   └── Text_TalentPoints
        ├── Tabs
        │   ├── Button_SkillsTab
        │   └── Button_TalentsTab
        └── Switcher_SkillsTalents
            ├── [0] SkillsPage
            └── [1] TalentsPage
                └── HorizontalBox
                    ├── BranchLeft
                    │   ├── Text_BranchLeft
                    │   └── placeholder 5 nodes
                    ├── BranchCenter
                    │   ├── Text_BranchCenter
                    │   └── placeholder 5 nodes
                    └── BranchRight
                        ├── Text_BranchRight
                        └── placeholder 5 nodes
```

UI-RPG03.2 remplacera les placeholders par `WBP_RPGTalentBranch` et `WBP_RPGTalentNode` réutilisables.

## Critères visuels UI-RPG03.1B

En PIE, raccourci K :

- le nouvel écran UMG apparaît à la place de la liste texte MON20 ;
- nom personnage, classe/niveau et Talent Points sont corrects ;
- bouton Talents affiche la page arbre ;
- trois colonnes visibles ;
- titres de branches français et colorés selon la classe ;
- 5 positions de nœuds visibles par colonne, même encore placeholders ;
- changement de personnage rafraîchit le header ;
- navigation globale I/K/G/M/J/H/ESC reste fonctionnelle.

Le fichier WBP `.uasset` ne doit être commité qu'après capture d'écran et validation visuelle.
