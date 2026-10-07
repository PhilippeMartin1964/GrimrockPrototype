# UI-RPG03.2 — WBP_RPGTalentBranch + WBP_RPGTalentNode

Date : **7 octobre 2026**  
État : **03.2A SOURCE PRÊTE — validation locale puis rematérialisation du catalogue requises**

## Objectif

Remplacer les 15 placeholders de UI-RPG03.1 par deux widgets réutilisables :

```text
WBP_RPGTalentBranch
WBP_RPGTalentNode
```

Une seule paire de widgets sert aux **6 classes / 18 branches / 90 talents conceptuels**. UI-RPG03.2 reste read-only ; l'acquisition appartient à UI-RPG04.

## Résolution des noms

Les 86 nœuds simples utilisent directement le texte RPG03 de leur unique Choice. Les 4 familles à variantes utilisent un override conceptuel sparse dans `DA_RPGTalentPresentation` :

```text
Spécialisation martiale
Ennemi juré
Affinité élémentaire
Imprégnation
```

Aucun parsing de `ChoiceId` ou de libellé.

## Validation 03.2A/03.2B

Après pull, exécuter :

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\AuthorUIRPGPresentation.ps1 -EngineRoot D:\UE_5.5
```

Le script doit modifier uniquement :

```text
Content/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation.uasset
```

et valider :

```text
Grimrock.UI.RPG02.ProductionPresentation
Grimrock.UI.RPG02.PresentationData
Grimrock.UI.RPG03.NodeBinding
```

Le nouveau filtre contient **4 tests** :

```text
SimpleNodeUsesChoiceText
VariantNodeRequiresConceptualOverride
OverrideValidation
ProductionNinetyNodeCoverage
```

## WBP_RPGTalentNode

Créer :

```text
/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/RPG/WBP_RPGTalentNode
```

Parent :

```text
UGridTalentNodeWidget
```

Hiérarchie exacte :

```text
SB_NodeRoot                              [SizeBox 68×68]
└── Button_TalentNode                    [Button]      Variable
    └── Overlay_Node                     [Overlay]
        ├── Border_TalentNode            [Border]      Variable
        ├── Text_TalentTier              [TextBlock]   Variable
        ├── Text_TalentLevel             [TextBlock]   Variable
        ├── Text_VariantCount            [TextBlock]   Variable
        └── Text_TalentState             [TextBlock]   Variable
```

### SB_NodeRoot

```text
Width Override  = 68
Height Override = 68
```

### Button_TalentNode

```text
Is Variable          = ON
Horizontal Alignment = Fill
Vertical Alignment   = Fill
```

Aucun `OnClicked` Blueprint.

### Border_TalentNode

```text
Is Variable          = ON
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Padding              = 0
```

Le C++ applique la couleur selon :

```text
Acquired
Available
LockedLevel
LockedPrerequisite
LockedPoints
LockedExclusive
```

### Text_TalentTier

```text
Is Variable = ON
Font        = Alegreya Sans
Size        = 18
Overlay Slot:
  Horizontal = Center
  Vertical   = Center
```

Runtime : I / II / III / IV / V.

### Text_TalentLevel

```text
Is Variable = ON
Font Size   = 9
Overlay Slot:
  Horizontal = Left
  Vertical   = Top
  Padding    = 4 / 3 / 0 / 0
```

Runtime : `Niv. 2`, `Niv. 6`, etc.

### Text_VariantCount

```text
Is Variable = ON
Font Size   = 10
Overlay Slot:
  Horizontal = Right
  Vertical   = Top
  Padding    = 0 / 3 / 4 / 0
```

Runtime : vide pour un nœud simple ; `×3`, `×4`, etc. pour les variantes.

### Text_TalentState

```text
Is Variable = ON
Font Size   = 8
Overlay Slot:
  Horizontal = Center
  Vertical   = Bottom
  Padding Bottom = 3
```

Le nom et la description complets du talent sont fournis automatiquement en tooltip. Le futur panneau Detail les utilisera aussi.

## WBP_RPGTalentBranch

Créer :

```text
/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/RPG/WBP_RPGTalentBranch
```

Parent :

```text
UGridTalentBranchWidget
```

Hiérarchie exacte :

```text
Overlay_BranchRoot
├── Border_BranchBackground
└── VB_Branch
    ├── SB_BranchHeader
    │   └── Border_BranchHeader
    │       └── Overlay_BranchHeader
    │           ├── Border_BranchAccent       [Variable]
    │           ├── Text_BranchName           [Variable]
    │           └── Text_BranchProgress       [Variable]
    ├── Spacer_BranchHeader
    └── Overlay_BranchTree
        ├── Border_BranchSpine
        └── VB_BranchNodes
            ├── Node_Tier1                    [WBP_RPGTalentNode, Variable]
            ├── Spacer_Node12
            ├── Node_Tier2                    [WBP_RPGTalentNode, Variable]
            ├── Spacer_Node23
            ├── Node_Tier3                    [WBP_RPGTalentNode, Variable]
            ├── Spacer_Node34
            ├── Node_Tier4                    [WBP_RPGTalentNode, Variable]
            ├── Spacer_Node45
            └── Node_Tier5                    [WBP_RPGTalentNode, Variable]
```

Les widgets obligatoires `BindWidget` sont :

```text
Text_BranchName
Node_Tier1
Node_Tier2
Node_Tier3
Node_Tier4
Node_Tier5
```

Les noms doivent être exacts.

### Header

```text
SB_BranchHeader.HeightOverride = 54

Text_BranchName:
  Font = Alegreya Sans
  Size = 21
  Justification = Center

Text_BranchProgress:
  Font Size = 10
  Overlay Slot Horizontal = Right
  Overlay Slot Vertical   = Bottom
  Padding = 0 / 0 / 6 / 4
```

`Text_BranchProgress` affiche `0 / 5`, `1 / 5`, etc.

### Tree

`Overlay_BranchTree` dans son VerticalBox Slot :

```text
Size = Fill
Fill Weight = 1.0
Horizontal = Fill
Vertical = Fill
```

`Border_BranchSpine` :

```text
Overlay Slot Horizontal = Center
Overlay Slot Vertical   = Fill
Padding Top/Bottom      = 34
Desired Width           = 3
```

`VB_BranchNodes` :

```text
Overlay Slot Horizontal = Fill
Overlay Slot Vertical   = Fill
```

Chaque `Node_TierX` :

```text
VerticalBox Slot Size = Auto
Horizontal Alignment  = Center
Vertical Alignment    = Center
```

Chaque Spacer :

```text
VerticalBox Slot Size = Fill
Fill Weight           = 1.0
```

## Migration WBP_GridSkills

Conserver le shell UI-RPG03.1.

Dans chaque Border de branche, supprimer seulement l'ancien contenu placeholder et insérer une instance de `WBP_RPGTalentBranch`.

Renommer les trois instances exactement :

```text
Branch_Left
Branch_Center
Branch_Right
```

et activer `Is Variable`.

Hiérarchie finale :

```text
HB_TalentBranches
├── Border_BranchLeft
│   └── Branch_Left
├── Spacer_BranchLC
├── Border_BranchCenter
│   └── Branch_Center
├── Spacer_BranchCR
└── Border_BranchRight
    └── Branch_Right
```

Les anciens `Text_BranchLeft/Center/Right` et les 15 placeholders peuvent alors disparaître du parent.

## Résultat attendu

Pour un Guerrier niveau 1 :

```text
GARDIEN              BRISE-LIGNE           MAÎTRE D'ARMES
 0 / 5                 0 / 5                 0 / 5

 [I] Niv.2            [I] Niv.2             [I] Niv.2 ×3
     Niveau                Niveau                 Niveau
      │                     │                      │
 [II] Niv.6           [II] Niv.6            [II] Niv.6
     Prérequis             Prérequis              Prérequis
      │                     │                      │
 ...
```

Le tooltip d'un nœud simple affiche le vrai nom/description RPG03. Le tooltip d'un nœud à variantes affiche le nom conceptuel puis le nombre de variantes sur le nœud.

## Invariants

```text
6 classes
18 branches
90 nœuds conceptuels
5 nœuds par branche
4 familles de variantes
0 parsing d'identifiants
0 logique gameplay dans UMG
```

## Ordre de fermeture

```text
03.2A : source + tests
03.2B : DA_RPGTalentPresentation rematérialisé
03.2C : WBP_RPGTalentNode + WBP_RPGTalentBranch + WBP_GridSkills
```

Ne committer aucun binaire avant validation locale et capture PIE.
