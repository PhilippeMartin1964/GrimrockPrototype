# UI-RPG03.2 — WBP_RPGTalentBranch + WBP_RPGTalentNode

Date : **7 octobre 2026**  
État : **03.2A/03.2B VALIDÉS — 03.2C construction des WBP en cours**

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

### Hiérarchie exacte ET ordre de peinture

```text
SB_NodeRoot                              [SizeBox 68×68]
└── Button_TalentNode                    [Button]      Variable
    └── Overlay_Node                     [Overlay]
        ├── Border_TalentNode            [Border]      Variable   <- DOIT ÊTRE LE 1er ENFANT
        ├── Text_TalentTier              [TextBlock]   Variable
        ├── Text_TalentLevel             [TextBlock]   Variable
        ├── Text_VariantCount            [TextBlock]   Variable
        └── Text_TalentState             [TextBlock]   Variable
```

Dans un `Overlay`, les enfants placés plus bas dans la hiérarchie sont peints au-dessus des précédents.  
**Si `Border_TalentNode` est placé après les TextBlock, il les recouvre et ils deviennent invisibles.**

### Visibilité UE5.5.4 — nomenclature exacte

Ne pas chercher le texte `Self Hit Test Invisible` : selon l'affichage de UE5.5.4, le champ **Visibility** propose les libellés conviviaux :

```text
Visible
Collapsed
Hidden
Not Hit-Testable (Self & All Children)
Not Hit-Testable (Self Only)
```

Pour UI-RPG03.2C :

```text
Button_TalentNode       = Visible
Border_TalentNode       = Not Hit-Testable (Self & All Children)
Text_TalentTier         = Not Hit-Testable (Self & All Children)
Text_TalentLevel        = Not Hit-Testable (Self & All Children)
Text_VariantCount       = Not Hit-Testable (Self & All Children)
Text_TalentState        = Not Hit-Testable (Self & All Children)
```

Le Button doit rester `Visible`, sinon il ne recevra pas les clics.

### SB_NodeRoot

```text
Width Override  = 68
Height Override = 68
```

Important : ces valeurs donnent la **Desired Size** du UserWidget. Dans son futur `VerticalBox Slot` de branche, l'instance devra aussi être en :

```text
Size                 = Auto
Horizontal Alignment = Center
Vertical Alignment   = Center
```

C'est ce slot parent qui garantit que le nœud ne s'étire pas sur toute la largeur de la branche.

### Button_TalentNode — rendre le Button transparent

```text
Is Variable          = ON
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Visibility           = Visible
```

Aucun `OnClicked` Blueprint.

Le Button ne doit pas fournir le fond visuel du talent : c'est `Border_TalentNode` qui le fait.

Dans **Style** du Button, mettre les brushes de fond à transparent / sans dessin. La solution la plus sûre dans UE5.5.4 est :

```text
Style > Normal   > Draw As = No Draw Type
Style > Hovered  > Draw As = No Draw Type
Style > Pressed  > Draw As = No Draw Type
Style > Disabled > Draw As = No Draw Type
```

Si l'éditeur ne propose pas `No Draw Type` pour l'un de ces brushes, mettre son `Tint` avec Alpha = 0.

Le contenu de l'Overlay restera visible et le Button continuera à recevoir le clic.

### Overlay_Node

```text
Horizontal Alignment = Fill
Vertical Alignment   = Fill
```

Ne pas modifier son Visibility.

### Border_TalentNode — correction du rectangle blanc

```text
Is Variable          = ON
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Padding              = 0
Visibility           = Not Hit-Testable (Self & All Children)
```

Dans le Designer, choisir un **fallback sombre explicite** :

```text
Brush Color:
R = 0.08
G = 0.08
B = 0.09
A = 1.00
```

Ne jamais laisser le Brush blanc par défaut.

Le C++ remplacera ensuite cette couleur au runtime selon :

```text
Acquired
Available
LockedLevel
LockedPrerequisite
LockedPoints
LockedExclusive
```

Si un grand rectangle blanc apparaît dans le Designer, vérifier dans cet ordre :

```text
1. Button Style n'est pas encore transparent ;
2. Border_TalentNode possède encore son Brush blanc par défaut ;
3. Border_TalentNode n'est pas le premier enfant de Overlay_Node ;
4. le UserWidget est prévisualisé en Fill Screen — ce point n'affecte pas sa taille réelle dans WBP_RPGTalentBranch.
```

### Couleur commune des quatre TextBlock

Pour rendre le montage vérifiable dans le Designer :

```text
Color and Opacity = blanc cassé
R = 0.90
G = 0.90
B = 0.88
A = 1.00

Shadow Offset  = 1 / 1
Shadow Color   = noir avec Alpha ~= 0.75
```

Ils doivent tous apparaître **après `Border_TalentNode`** dans `Overlay_Node`.

### Text_TalentTier

```text
Is Variable = ON
Text        = III          <- placeholder de montage
Font        = Alegreya Sans
Font Size   = 18
Justification = Center
Visibility  = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Center
  Vertical   = Center
  Padding    = 0
```

Le placeholder `III` permet de vérifier immédiatement le centrage.  
Runtime : I / II / III / IV / V.

### Text_TalentLevel

```text
Is Variable = ON
Text        = Niv. 10      <- placeholder de montage
Font        = Alegreya Sans
Font Size   = 9
Visibility  = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Left
  Vertical   = Top
  Padding:
    Left   = 4
    Top    = 3
    Right  = 0
    Bottom = 0
```

Runtime : `Niv. 2`, `Niv. 6`, `Niv. 10`, `Niv. 14`, `Niv. 18`.

### Text_VariantCount

Pendant la construction, **ne pas le laisser vide**, sinon son emplacement est impossible à contrôler visuellement.

```text
Is Variable = ON
Text        = ×4           <- placeholder de montage uniquement
Font        = Alegreya Sans
Font Size   = 10
Visibility  = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Right
  Vertical   = Top
  Padding:
    Left   = 0
    Top    = 3
    Right  = 4
    Bottom = 0
```

Runtime :

```text
nœud simple -> texte vide
3 variantes -> ×3
4 variantes -> ×4
N variantes -> ×N
```

### Text_TalentState

```text
Is Variable = ON
Text        = Prérequis    <- placeholder de montage
Font        = Alegreya Sans
Font Size   = 8
Justification = Center
Visibility  = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Center
  Vertical   = Bottom
  Padding:
    Left   = 1
    Top    = 0
    Right  = 1
    Bottom = 3
```

Runtime :

```text
Acquis
Disponible
Niveau
Prérequis
Points
Exclusif
```

### Aspect que le Designer doit montrer AVANT de créer WBP_RPGTalentBranch

Avec les placeholders de montage, le carré 68×68 doit ressembler approximativement à :

```text
┌──────────────────┐
│ Niv.10       ×4  │
│                  │
│       III        │
│                  │
│    Prérequis     │
└──────────────────┘
```

Si vous ne voyez pas **ces quatre textes** dans le Designer, ne poursuivez pas vers `WBP_RPGTalentBranch`. Corriger d'abord l'ordre de l'Overlay, le Brush du Border et les couleurs de texte.

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
