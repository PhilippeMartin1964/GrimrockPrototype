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

### Hiérarchie exacte ET ordre de peinture

```text
Overlay_BranchRoot
├── Border_BranchBackground                [Border]      <- 1er enfant : fond
│
└── VB_Branch                              [VerticalBox] <- 2e enfant : contenu
    │
    ├── SB_BranchHeader                    [SizeBox]
    │   └── Border_BranchHeader            [Border]
    │       └── Overlay_BranchHeader       [Overlay]
    │           ├── Border_BranchAccent    [Border]      Variable
    │           ├── Text_BranchName        [TextBlock]   Variable
    │           └── Text_BranchProgress    [TextBlock]   Variable
    │
    ├── Spacer_BranchHeader                [Spacer]
    │
    └── Overlay_BranchTree                 [Overlay]
        ├── Border_BranchSpine             [Border]      <- 1er enfant : derrière
        │
        └── VB_BranchNodes                 [VerticalBox] <- 2e enfant : devant
            ├── Node_Tier1                 [WBP_RPGTalentNode] Variable
            ├── Spacer_Node12              [Spacer]
            ├── Node_Tier2                 [WBP_RPGTalentNode] Variable
            ├── Spacer_Node23              [Spacer]
            ├── Node_Tier3                 [WBP_RPGTalentNode] Variable
            ├── Spacer_Node34              [Spacer]
            ├── Node_Tier4                 [WBP_RPGTalentNode] Variable
            ├── Spacer_Node45              [Spacer]
            └── Node_Tier5                 [WBP_RPGTalentNode] Variable
```

L'ordre de peinture est important dans les deux Overlays :

```text
Overlay_BranchRoot
  Background d'abord
  VB_Branch ensuite

Overlay_BranchTree
  Spine d'abord
  Nodes ensuite
```

Sinon le fond ou la spine peut recouvrir le contenu.

### Widgets obligatoires côté C++

Ces widgets sont des `BindWidget` et leurs noms doivent être exacts :

```text
Text_BranchName
Node_Tier1
Node_Tier2
Node_Tier3
Node_Tier4
Node_Tier5
```

Ces widgets sont `BindWidgetOptional`, mais doivent être ajoutés dans UI-RPG03.2C :

```text
Text_BranchProgress
Border_BranchAccent
```

### Visibility UE5.5.4

Utiliser les libellés UE5.5.4 suivants :

```text
Overlay_BranchRoot       = Visible
Border_BranchBackground  = Not Hit-Testable (Self & All Children)
VB_Branch                = Visible

Border_BranchHeader      = Not Hit-Testable (Self & All Children)
Overlay_BranchHeader     = Visible
Border_BranchAccent      = Not Hit-Testable (Self & All Children)
Text_BranchName          = Not Hit-Testable (Self & All Children)
Text_BranchProgress      = Not Hit-Testable (Self & All Children)

Overlay_BranchTree       = Visible
Border_BranchSpine       = Not Hit-Testable (Self & All Children)
VB_BranchNodes           = Visible
```

Les cinq `Node_TierX` restent `Visible`, car leurs boutons internes doivent recevoir les clics.

---

### Overlay_BranchRoot

Le root n'a pas de taille fixe : il doit remplir l'espace donné par `WBP_GridSkills`.

Dans le Designer du widget isolé, utiliser une Preview Size confortable, par exemple :

```text
Desired / Custom Preview Size ≈ 380 × 620
```

Cette taille n'est qu'une prévisualisation. Dans `WBP_GridSkills`, la branche sera étirée par son Border parent.

`Border_BranchBackground` :

```text
Overlay Slot:
  Horizontal = Fill
  Vertical   = Fill
  Padding    = 0

Brush Color fallback:
  R = 0.025
  G = 0.028
  B = 0.032
  A = 0.92

Visibility = Not Hit-Testable (Self & All Children)
```

Ne pas laisser un Brush blanc par défaut.

`VB_Branch` :

```text
Overlay Slot:
  Horizontal = Fill
  Vertical   = Fill
  Padding    = 12
```

---

### SB_BranchHeader

```text
Height Override = 54
```

Dans son VerticalBox Slot :

```text
Size                 = Auto
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Padding              = 0
```

Enfant unique :

```text
Border_BranchHeader
```

---

### Border_BranchHeader

```text
Padding     = 0
Brush Color:
  R = 0.055
  G = 0.058
  B = 0.065
  A = 1.00

Visibility = Not Hit-Testable (Self & All Children)
```

Attention : le Border contient `Overlay_BranchHeader`. Le Border ne doit pas bloquer le hit test des futurs nœuds, même s'il est hors de leur zone.

---

### Overlay_BranchHeader

Ordre exact :

```text
Overlay_BranchHeader
├── Border_BranchAccent
├── Text_BranchName
└── Text_BranchProgress
```

#### Border_BranchAccent

Cette ligne est purement décorative.

```text
Is Variable = ON
Visibility  = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Fill
  Vertical   = Bottom
  Padding:
    Left   = 8
    Top    = 0
    Right  = 8
    Bottom = 2

Desired Height / Min Desired Height = 3

Brush Color fallback:
  R = 0.45
  G = 0.35
  B = 0.16
  A = 1.00
```

Le C++ remplace cette couleur par `BranchPresentation.AccentColor`.

#### Text_BranchName

```text
Is Variable = ON
Text        = GARDIEN             <- placeholder de montage
Font        = Alegreya Sans
Font Size   = 21
Justification = Center
Color and Opacity:
  R = 0.90
  G = 0.90
  B = 0.88
  A = 1.00
Visibility  = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Fill
  Vertical   = Center
  Padding:
    Left   = 8
    Top    = 0
    Right  = 48
    Bottom = 0
```

Le padding Right = 48 réserve la place du compteur de progression.

Le runtime remplace le placeholder par :

```text
Gardien
Brise-ligne
Maître d'armes
etc.
```

et applique la couleur de branche.

#### Text_BranchProgress

```text
Is Variable = ON
Text        = 3 / 5               <- placeholder de montage
Font        = Alegreya Sans
Font Size   = 10
Justification = Right
Color and Opacity:
  R = 0.78
  G = 0.78
  B = 0.76
  A = 1.00
Visibility  = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Right
  Vertical   = Bottom
  Padding:
    Left   = 0
    Top    = 0
    Right  = 6
    Bottom = 5
```

Runtime : `0 / 5` à `5 / 5`.

---

### Spacer_BranchHeader

Widget Spacer :

```text
Size X = 0
Size Y = 8
```

VerticalBox Slot :

```text
Size = Auto
```

---

### Overlay_BranchTree

Dans le VerticalBox Slot de `VB_Branch` :

```text
Size                 = Fill
Fill Weight          = 1.0
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Padding              = 0
```

Ordre exact :

```text
Overlay_BranchTree
├── Border_BranchSpine
└── VB_BranchNodes
```

La spine doit être derrière les nœuds.

---

### Border_BranchSpine

```text
Visibility = Not Hit-Testable (Self & All Children)

Overlay Slot:
  Horizontal = Center
  Vertical   = Fill
  Padding:
    Left   = 0
    Top    = 34
    Right  = 0
    Bottom = 34

Min Desired Width / Width = 3
```

Fallback conseillé :

```text
Brush Color:
  R = 0.26
  G = 0.26
  B = 0.25
  A = 1.00
```

Ne pas mettre la spine en blanc pur.

---

### VB_BranchNodes

Dans son Overlay Slot :

```text
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Padding              = 0
```

Hiérarchie stricte :

```text
Node_Tier1
Spacer_Node12
Node_Tier2
Spacer_Node23
Node_Tier3
Spacer_Node34
Node_Tier4
Spacer_Node45
Node_Tier5
```

---

### Les cinq Node_TierX

Les cinq widgets sont des instances de :

```text
WBP_RPGTalentNode
```

Noms exacts :

```text
Node_Tier1
Node_Tier2
Node_Tier3
Node_Tier4
Node_Tier5
```

Pour chacun :

```text
Is Variable = ON
Visibility  = Visible
```

Dans son **Vertical Box Slot** :

```text
Size                 = Auto
Horizontal Alignment = Center
Vertical Alignment   = Center
Padding              = 0
```

Ne pas mettre `Fill` sur les Node_TierX.

Sinon le `WBP_RPGTalentNode` 68×68 sera étiré horizontalement et reproduira le problème des gros rectangles de UI-RPG03.1.

Comme `WBP_RPGTalentNode` a un root SizeBox 68×68, l'affichage attendu est un carré centré sur la spine.

---

### Les quatre Spacer_NodeXX

```text
Spacer_Node12
Spacer_Node23
Spacer_Node34
Spacer_Node45
```

Pour chacun, dans son **Vertical Box Slot** :

```text
Size                 = Fill
Fill Weight          = 1.0
Horizontal Alignment = Fill
Vertical Alignment   = Fill
Padding              = 0
```

La propriété `Size` du widget Spacer lui-même peut rester à 0×0.

C'est **le Fill du VerticalBox Slot** qui répartit les cinq nœuds sur toute la hauteur.

---

### Ce que doit montrer le Designer de WBP_RPGTalentBranch

Avant de l'insérer dans `WBP_GridSkills`, avec les placeholders du Node :

```text
┌──────────────────────────────────────┐
│              GARDIEN          3 / 5 │
│        ─────────────────────────     │
│                                      │
│                ┌──────┐              │
│                │ III  │              │
│                └──────┘              │
│                   │                  │
│                   │                  │
│                ┌──────┐              │
│                │ III  │              │
│                └──────┘              │
│                   │                  │
│                  ...                 │
│                   │                  │
│                ┌──────┐              │
│                │ III  │              │
│                └──────┘              │
└──────────────────────────────────────┘
```

Les cinq instances affichent encore les mêmes placeholders tant qu'elles ne sont pas alimentées par le runtime. C'est normal.

La branche ne doit afficher :

- aucun grand rectangle blanc ;
- aucune spine blanche dominante ;
- aucun nœud étiré sur la largeur ;
- aucun texte masqué.

---

### Diagnostic rapide si WBP_RPGTalentBranch est incorrect

```text
GROS RECTANGLE BLANC
  -> vérifier Border_BranchBackground.Brush Color
  -> vérifier Border_BranchHeader.Brush Color
  -> vérifier l'ordre dans Overlay_BranchRoot

NŒUDS EN BARRES HORIZONTALES
  -> Node_TierX > VerticalBox Slot > Size doit être Auto
  -> Horizontal Alignment doit être Center

NŒUDS COLLÉS EN HAUT
  -> Spacer_NodeXX > VerticalBox Slot > Size = Fill 1.0
  -> Overlay_BranchTree > VerticalBox Slot > Size = Fill 1.0

SPINE DEVANT LES NŒUDS
  -> Border_BranchSpine doit être le 1er enfant de Overlay_BranchTree
  -> VB_BranchNodes doit être le 2e enfant

TITRE / 3/5 INVISIBLES
  -> vérifier l'ordre de Overlay_BranchHeader
  -> vérifier leur Color and Opacity
  -> utiliser les placeholders GARDIEN et 3 / 5 pendant le montage

CLICS FUTURS BLOQUÉS
  -> Border/Texts décoratifs en Not Hit-Testable (Self & All Children)
  -> Node_TierX reste Visible
  -> Button_TalentNode interne reste Visible
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
