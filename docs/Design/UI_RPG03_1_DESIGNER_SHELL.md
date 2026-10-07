# UI-RPG03.1 — WBP_GridSkills Designer Specification

Date : **7 octobre 2026**  
Parent : **UI-RPG03 — Real UMG Talent Tree**  
État : **SPEC DESIGNER COMPLÈTE — WBP À CONSTRUIRE MANUELLEMENT DANS UE5.5.4**

---

## 1. Objectif

Ce document est la **spécification de construction exacte** du premier vrai écran UMG `WBP_GridSkills`.

Il remplace les descriptions sommaires précédentes.

UI-RPG03.1 doit produire un écran réellement visible et exploitable en PIE, avec :

- header personnage / classe / niveau / points de talent ;
- onglets `COMPÉTENCES` et `TALENTS` ;
- page Talents avec **3 branches simultanées** ;
- **5 positions de nœuds par branche** ;
- titres de branches récupérés depuis `DA_RPGTalentPresentation` ;
- couleurs de branche récupérées depuis `DA_RPGTalentPresentation` ;
- aucune logique métier dans le Graph Blueprint ;
- maintien temporaire du renderer natif MON20 uniquement comme fallback tant que le marker Designer n'existe pas.

Le vrai écran UMG est activé dès que le widget suivant existe dans le Designer :

```text
Panel_GridSkillsDesignerRoot
```

---

## 2. Asset à modifier

```text
/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSkills
```

Classe parente :

```text
UGridSkillsWidget
```

Le `Border` racine existant doit être **conservé**.

Ne pas créer un second `WBP_GridSkills`.

Ne pas changer de parent Blueprint.

Ne pas ajouter de Graph Blueprint pour les boutons d'onglet : le C++ les gère déjà.

---

## 3. Principe de layout

La page est structurée verticalement :

```text
HEADER
TABS
CONTENT
```

La zone CONTENT contient un `WidgetSwitcher` :

```text
index 0 = Compétences
index 1 = Talents
```

La page Talents contient trois colonnes égales :

```text
GAUCHE          CENTRE          DROITE
Branche 1       Branche 2       Branche 3
   │               │               │
   I               I               I
   │               │               │
  II              II              II
   │               │               │
 III             III             III
   │               │               │
  IV              IV              IV
   │               │               │
   V               V               V
```

---

# 4. HIÉRARCHIE COMPLÈTE DU WBP

La hiérarchie ci-dessous est la hiérarchie à reproduire dans le **Hierarchy panel** du Designer Unreal.

```text
Border_Root                                  [Border] EXISTANT — NE PAS SUPPRIMER
└── Panel_GridSkillsDesignerRoot             [Overlay] VARIABLE = YES
    ├── Border_Background                    [Border]
    │
    └── VB_Main                              [VerticalBox]
        │
        ├── SB_Header                        [SizeBox]
        │   └── Border_Header                [Border]
        │       └── HB_Header                [HorizontalBox]
        │           │
        │           ├── SB_Identity          [SizeBox]
        │           │   └── VB_Identity      [VerticalBox]
        │           │       ├── Text_CharacterName   [TextBlock] VARIABLE = YES
        │           │       └── Text_ClassLevel      [TextBlock] VARIABLE = YES
        │           │
        │           ├── Spacer_HeaderFlex    [Spacer]
        │           │
        │           └── SB_TalentPoints      [SizeBox]
        │               └── Border_TalentPoints
        │                   └── Text_TalentPoints    [TextBlock] VARIABLE = YES
        │
        ├── SB_Tabs                          [SizeBox]
        │   └── Border_Tabs                  [Border]
        │       └── HB_Tabs                  [HorizontalBox]
        │           │
        │           ├── Spacer_TabLeft       [Spacer]
        │           │
        │           ├── Button_SkillsTab     [Button] VARIABLE = YES
        │           │   └── Text_SkillsTab   [TextBlock]
        │           │
        │           ├── Spacer_TabMiddle     [Spacer]
        │           │
        │           ├── Button_TalentsTab    [Button] VARIABLE = YES
        │           │   └── Text_TalentsTab  [TextBlock]
        │           │
        │           └── Spacer_TabRight      [Spacer]
        │
        └── Border_Content                   [Border]
            └── Switcher_SkillsTalents       [WidgetSwitcher] VARIABLE = YES
                │
                ├── Border_SkillsPage        [Border] INDEX 0
                │   └── Overlay_SkillsPage   [Overlay]
                │       ├── Border_SkillsInner
                │       └── VB_SkillsPage    [VerticalBox]
                │           ├── Text_SkillsPageTitle
                │           ├── Spacer_SkillsTitle
                │           └── Text_SkillsPlaceholder
                │
                └── Border_TalentsPage       [Border] INDEX 1
                    └── Overlay_TalentsPage   [Overlay]
                        ├── Border_TalentsInner
                        │
                        └── HB_TalentBranches [HorizontalBox]
                            │
                            ├── Border_BranchLeft
                            │   └── VB_BranchLeft
                            │       ├── SB_BranchLeftHeader
                            │       │   └── Border_BranchLeftHeader
                            │       │       └── Text_BranchLeft        [TextBlock] VARIABLE = YES
                            │       ├── Spacer_BranchLeftHeader
                            │       └── Overlay_BranchLeftTree
                            │           ├── Border_BranchLeftSpine
                            │           └── VB_BranchLeftNodes
                            │               ├── SB_Left_Tier1
                            │               │   └── Border_Left_Tier1
                            │               │       └── Text_Left_Tier1
                            │               ├── Spacer_Left_12
                            │               ├── SB_Left_Tier2
                            │               │   └── Border_Left_Tier2
                            │               │       └── Text_Left_Tier2
                            │               ├── Spacer_Left_23
                            │               ├── SB_Left_Tier3
                            │               │   └── Border_Left_Tier3
                            │               │       └── Text_Left_Tier3
                            │               ├── Spacer_Left_34
                            │               ├── SB_Left_Tier4
                            │               │   └── Border_Left_Tier4
                            │               │       └── Text_Left_Tier4
                            │               ├── Spacer_Left_45
                            │               └── SB_Left_Tier5
                            │                   └── Border_Left_Tier5
                            │                       └── Text_Left_Tier5
                            │
                            ├── Spacer_BranchLC
                            │
                            ├── Border_BranchCenter
                            │   └── VB_BranchCenter
                            │       ├── SB_BranchCenterHeader
                            │       │   └── Border_BranchCenterHeader
                            │       │       └── Text_BranchCenter      [TextBlock] VARIABLE = YES
                            │       ├── Spacer_BranchCenterHeader
                            │       └── Overlay_BranchCenterTree
                            │           ├── Border_BranchCenterSpine
                            │           └── VB_BranchCenterNodes
                            │               ├── SB_Center_Tier1
                            │               │   └── Border_Center_Tier1
                            │               │       └── Text_Center_Tier1
                            │               ├── Spacer_Center_12
                            │               ├── SB_Center_Tier2
                            │               │   └── Border_Center_Tier2
                            │               │       └── Text_Center_Tier2
                            │               ├── Spacer_Center_23
                            │               ├── SB_Center_Tier3
                            │               │   └── Border_Center_Tier3
                            │               │       └── Text_Center_Tier3
                            │               ├── Spacer_Center_34
                            │               ├── SB_Center_Tier4
                            │               │   └── Border_Center_Tier4
                            │               │       └── Text_Center_Tier4
                            │               ├── Spacer_Center_45
                            │               └── SB_Center_Tier5
                            │                   └── Border_Center_Tier5
                            │                       └── Text_Center_Tier5
                            │
                            ├── Spacer_BranchCR
                            │
                            └── Border_BranchRight
                                └── VB_BranchRight
                                    ├── SB_BranchRightHeader
                                    │   └── Border_BranchRightHeader
                                    │       └── Text_BranchRight       [TextBlock] VARIABLE = YES
                                    ├── Spacer_BranchRightHeader
                                    └── Overlay_BranchRightTree
                                        ├── Border_BranchRightSpine
                                        └── VB_BranchRightNodes
                                            ├── SB_Right_Tier1
                                            │   └── Border_Right_Tier1
                                            │       └── Text_Right_Tier1
                                            ├── Spacer_Right_12
                                            ├── SB_Right_Tier2
                                            │   └── Border_Right_Tier2
                                            │       └── Text_Right_Tier2
                                            ├── Spacer_Right_23
                                            ├── SB_Right_Tier3
                                            │   └── Border_Right_Tier3
                                            │       └── Text_Right_Tier3
                                            ├── Spacer_Right_34
                                            ├── SB_Right_Tier4
                                            │   └── Border_Right_Tier4
                                            │       └── Text_Right_Tier4
                                            ├── Spacer_Right_45
                                            └── SB_Right_Tier5
                                                └── Border_Right_Tier5
                                                    └── Text_Right_Tier5
```

---

# 5. Widgets CONTRACTUELS C++

Les widgets suivants doivent avoir **exactement** ces noms et **Is Variable = ON**.

| Nom | Type | Obligatoire pour UI-RPG03.1 | Utilisation |
|---|---|---:|---|
| `Panel_GridSkillsDesignerRoot` | Overlay | oui | marker qui désactive le renderer natif MON20 |
| `Text_CharacterName` | TextBlock | oui | nom du personnage sélectionné |
| `Text_ClassLevel` | TextBlock | oui | classe + niveau |
| `Text_TalentPoints` | TextBlock | oui | Talent Points disponibles |
| `Button_SkillsTab` | Button | oui | active index 0 |
| `Button_TalentsTab` | Button | oui | active index 1 |
| `Switcher_SkillsTalents` | WidgetSwitcher | oui | commute Compétences / Talents |
| `Text_BranchLeft` | TextBlock | oui | branche visuelle index 0 |
| `Text_BranchCenter` | TextBlock | oui | branche visuelle index 1 |
| `Text_BranchRight` | TextBlock | oui | branche visuelle index 2 |

Tous les autres noms de cette spécification sont recommandés et doivent être suivis pour garder un Designer lisible, mais ils ne sont pas encore liés au C++.

---

# 6. RÉGLAGES DU ROOT ET DU PANEL PRINCIPAL

## 6.1 Border_Root

Widget existant.

Réglages :

```text
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Padding              : 0
Brush Color          : conserver temporairement le style existant
```

Son unique enfant devient :

```text
Panel_GridSkillsDesignerRoot
```

## 6.2 Panel_GridSkillsDesignerRoot

Type :

```text
Overlay
```

Réglages :

```text
Is Variable           : ON
Horizontal Alignment  : Fill
Vertical Alignment    : Fill
Padding               : 0
Visibility            : Visible
```

Il contient deux enfants superposés :

```text
Border_Background
VB_Main
```

Le background est placé en premier pour rester derrière le contenu.

## 6.3 Border_Background

Réglages provisoires recommandés :

```text
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Brush Color          : (0.015, 0.018, 0.022, 0.96)
Padding              : 0
Hit Test              : Self Hit Test Invisible
```

Ne pas chercher le matériau final dans UI-RPG03.1.

---

# 7. VB_Main — STRUCTURE VERTICALE

`VB_Main` occupe toute la surface.

Réglages Overlay Slot :

```text
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Padding              : 24 / 20 / 24 / 20
```

Ses trois enfants sont :

```text
SB_Header
SB_Tabs
Border_Content
```

Réglages des slots VerticalBox :

| Enfant | Size | Padding |
|---|---|---|
| `SB_Header` | Auto | 0,0,0,8 |
| `SB_Tabs` | Auto | 0,0,0,10 |
| `Border_Content` | Fill 1.0 | 0 |

---

# 8. HEADER COMPLET

## 8.1 SB_Header

```text
Type          : SizeBox
HeightOverride: 92
```

Enfant :

```text
Border_Header
```

## 8.2 Border_Header

Réglages :

```text
Padding              : 18 / 10 / 18 / 10
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Brush Color          : (0.025, 0.028, 0.032, 0.90)
```

Enfant :

```text
HB_Header
```

## 8.3 HB_Header

Trois enfants :

```text
SB_Identity
Spacer_HeaderFlex
SB_TalentPoints
```

### SB_Identity

```text
WidthOverride : 520
```

Enfant :

```text
VB_Identity
```

### VB_Identity

Deux enfants :

```text
Text_CharacterName
Text_ClassLevel
```

### Text_CharacterName

```text
Is Variable      : ON
Text             : PERSONNAGE
Font             : Alegreya Sans
Font Size        : 28
Justification    : Left
Auto Wrap        : OFF
Color            : blanc chaud
Vertical slot    : Auto
```

Le texte du Designer est seulement un placeholder. Le runtime le remplace.

### Text_ClassLevel

```text
Is Variable      : ON
Text             : Classe — Niveau 1
Font             : Alegreya Sans
Font Size        : 18
Justification    : Left
Auto Wrap        : OFF
Color            : gris clair provisoire
Vertical slot    : Auto
Padding Top      : 2
```

La couleur est remplacée en runtime par `ClassPresentation.AccentColor`.

### Spacer_HeaderFlex

```text
HorizontalBox Slot Size : Fill 1.0
```

### SB_TalentPoints

```text
WidthOverride : 300
```

Slot HorizontalBox :

```text
Horizontal Alignment : Right
Vertical Alignment   : Center
Size                 : Auto
```

### Border_TalentPoints

```text
Padding     : 14 / 8 / 14 / 8
Brush Color: (0.06, 0.055, 0.035, 0.95)
```

### Text_TalentPoints

```text
Is Variable   : ON
Text          : Points de talent : 0
Font          : Alegreya Sans
Font Size     : 19
Justification : Center
Auto Wrap     : OFF
Color         : or pâle
```

---

# 9. BARRE D'ONGLETS COMPLÈTE

## 9.1 SB_Tabs

```text
HeightOverride : 52
```

Enfant :

```text
Border_Tabs
```

## 9.2 Border_Tabs

```text
Padding     : 8 / 5 / 8 / 5
Brush Color: (0.02, 0.022, 0.026, 0.90)
```

Enfant :

```text
HB_Tabs
```

## 9.3 HB_Tabs

Enfants :

```text
Spacer_TabLeft
Button_SkillsTab
Spacer_TabMiddle
Button_TalentsTab
Spacer_TabRight
```

Réglages :

```text
Spacer_TabLeft   : Fill 1.0
Spacer_TabMiddle : Width = 16
Spacer_TabRight  : Fill 1.0
```

## 9.4 Button_SkillsTab

```text
Is Variable     : ON
Desired Width   : 190
Desired Height  : 38
```

Enfant :

```text
Text_SkillsTab
```

### Text_SkillsTab

```text
Text          : COMPÉTENCES
Font          : Alegreya Sans
Font Size     : 17
Justification : Center
```

## 9.5 Button_TalentsTab

Même taille.

Enfant :

```text
Text_TalentsTab
```

### Text_TalentsTab

```text
Text          : TALENTS
Font          : Alegreya Sans
Font Size     : 17
Justification : Center
```

Aucun événement Blueprint.

Le C++ effectue :

```text
Button_SkillsTab  -> ShowSkillsTab()  -> index 0
Button_TalentsTab -> ShowTalentsTab() -> index 1
```

---

# 10. ZONE CONTENT

## 10.1 Border_Content

Slot VerticalBox :

```text
Size                 : Fill 1.0
Horizontal Alignment : Fill
Vertical Alignment   : Fill
```

Réglages :

```text
Padding     : 8
Brush Color: (0.012, 0.014, 0.018, 0.88)
```

Enfant :

```text
Switcher_SkillsTalents
```

## 10.2 Switcher_SkillsTalents

```text
Is Variable        : ON
ActiveWidgetIndex  : 1 pendant UI-RPG03.1
```

Deux enfants seulement :

```text
index 0 = Border_SkillsPage
index 1 = Border_TalentsPage
```

---

# 11. PAGE COMPÉTENCES — INDEX 0

UI-RPG03.1 ne refond pas encore le contenu Skills. Il faut néanmoins construire une page propre pour que l'onglet fonctionne.

## Hiérarchie

```text
Border_SkillsPage
└── Overlay_SkillsPage
    ├── Border_SkillsInner
    └── VB_SkillsPage
        ├── Text_SkillsPageTitle
        ├── Spacer_SkillsTitle
        └── Text_SkillsPlaceholder
```

## Border_SkillsPage

```text
Padding : 14
```

## Border_SkillsInner

```text
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Brush Color          : (0.02, 0.024, 0.028, 0.80)
```

## VB_SkillsPage

Overlay Slot :

```text
Horizontal Alignment : Fill
Vertical Alignment   : Top
Padding              : 20
```

## Text_SkillsPageTitle

```text
Text      : COMPÉTENCES
Font Size : 24
```

## Spacer_SkillsTitle

```text
Size Y : 16
```

## Text_SkillsPlaceholder

```text
Text :
"La présentation détaillée des compétences sera intégrée
dans la tranche UI-SKILLS. Ce jalon valide le shell commun."

Font Size : 16
Auto Wrap : ON
```

Ce placeholder est intentionnel et temporaire.

---

# 12. PAGE TALENTS — INDEX 1

## Hiérarchie de premier niveau

```text
Border_TalentsPage
└── Overlay_TalentsPage
    ├── Border_TalentsInner
    └── HB_TalentBranches
        ├── Border_BranchLeft
        ├── Spacer_BranchLC
        ├── Border_BranchCenter
        ├── Spacer_BranchCR
        └── Border_BranchRight
```

## Border_TalentsPage

```text
Padding : 12
```

## Border_TalentsInner

```text
Brush Color : (0.018, 0.020, 0.024, 0.86)
```

## HB_TalentBranches

Overlay Slot :

```text
Horizontal Alignment : Fill
Vertical Alignment   : Fill
Padding              : 10
```

Slots :

```text
Border_BranchLeft   : Fill 1.0
Spacer_BranchLC     : Width 14
Border_BranchCenter : Fill 1.0
Spacer_BranchCR     : Width 14
Border_BranchRight  : Fill 1.0
```

Les trois colonnes ont donc exactement la même largeur.

---

# 13. COLONNE GAUCHE — HIÉRARCHIE ET RÉGLAGES

## 13.1 Border_BranchLeft

```text
Padding     : 12
Brush Color: (0.025, 0.028, 0.032, 0.92)
```

Enfant :

```text
VB_BranchLeft
```

## 13.2 VB_BranchLeft

```text
SB_BranchLeftHeader
Spacer_BranchLeftHeader
Overlay_BranchLeftTree
```

Slots :

```text
SB_BranchLeftHeader       : Auto
Spacer_BranchLeftHeader   : Auto
Overlay_BranchLeftTree    : Fill 1.0
```

## 13.3 SB_BranchLeftHeader

```text
HeightOverride : 54
```

Enfant :

```text
Border_BranchLeftHeader
```

### Border_BranchLeftHeader

```text
Padding     : 8
Brush Color: transparent sombre
```

Enfant :

```text
Text_BranchLeft
```

### Text_BranchLeft

```text
Is Variable   : ON
Text          : BRANCHE GAUCHE
Font          : Alegreya Sans
Font Size     : 21
Justification : Center
Auto Wrap     : ON
```

Runtime :

```text
index 0 de DA_RPGTalentPresentation
```

Pour Warrior, le résultat attendu est `Gardien`.

## 13.4 Spacer_BranchLeftHeader

```text
Size Y : 8
```

## 13.5 Overlay_BranchLeftTree

Deux enfants superposés :

```text
Border_BranchLeftSpine
VB_BranchLeftNodes
```

### Border_BranchLeftSpine

Overlay Slot :

```text
Horizontal Alignment : Center
Vertical Alignment   : Fill
Padding Top/Bottom   : 32
```

Réglages :

```text
Desired Width : 3
Brush Color   : gris/bronze sombre
Hit Test      : Self Hit Test Invisible
```

Cette ligne est uniquement décorative.

### VB_BranchLeftNodes

Overlay Slot :

```text
Horizontal Alignment : Center
Vertical Alignment   : Fill
```

Hiérarchie :

```text
SB_Left_Tier1
Spacer_Left_12
SB_Left_Tier2
Spacer_Left_23
SB_Left_Tier3
Spacer_Left_34
SB_Left_Tier4
Spacer_Left_45
SB_Left_Tier5
```

Tous les `SB_Left_TierX` :

```text
WidthOverride  : 68
HeightOverride : 68
```

Chaque SizeBox contient un Border.

Chaque Border :

```text
Padding     : 0
Brush Color: (0.06, 0.065, 0.07, 1.0)
```

Chaque Border contient un TextBlock centré.

Textes :

```text
Text_Left_Tier1 : I
Text_Left_Tier2 : II
Text_Left_Tier3 : III
Text_Left_Tier4 : IV
Text_Left_Tier5 : V
```

Chaque TextBlock :

```text
Font          : Alegreya Sans
Font Size     : 18
Justification : Center
Color         : gris clair
```

Spacers verticaux :

```text
Spacer_Left_12 : Fill 1.0
Spacer_Left_23 : Fill 1.0
Spacer_Left_34 : Fill 1.0
Spacer_Left_45 : Fill 1.0
```

Le but est de répartir automatiquement les 5 nœuds sur toute la hauteur.

---

# 14. COLONNE CENTRALE

Même structure que la gauche.

```text
Border_BranchCenter
└── VB_BranchCenter
    ├── SB_BranchCenterHeader
    │   └── Border_BranchCenterHeader
    │       └── Text_BranchCenter
    ├── Spacer_BranchCenterHeader
    └── Overlay_BranchCenterTree
        ├── Border_BranchCenterSpine
        └── VB_BranchCenterNodes
            ├── SB_Center_Tier1
            │   └── Border_Center_Tier1
            │       └── Text_Center_Tier1
            ├── Spacer_Center_12
            ├── SB_Center_Tier2
            │   └── Border_Center_Tier2
            │       └── Text_Center_Tier2
            ├── Spacer_Center_23
            ├── SB_Center_Tier3
            │   └── Border_Center_Tier3
            │       └── Text_Center_Tier3
            ├── Spacer_Center_34
            ├── SB_Center_Tier4
            │   └── Border_Center_Tier4
            │       └── Text_Center_Tier4
            ├── Spacer_Center_45
            └── SB_Center_Tier5
                └── Border_Center_Tier5
                    └── Text_Center_Tier5
```

Textes :

```text
I / II / III / IV / V
```

`Text_BranchCenter` :

```text
Is Variable : ON
```

Runtime :

```text
index 1 de DA_RPGTalentPresentation
```

Pour Warrior : `Brise-ligne`.

---

# 15. COLONNE DROITE

Même structure.

```text
Border_BranchRight
└── VB_BranchRight
    ├── SB_BranchRightHeader
    │   └── Border_BranchRightHeader
    │       └── Text_BranchRight
    ├── Spacer_BranchRightHeader
    └── Overlay_BranchRightTree
        ├── Border_BranchRightSpine
        └── VB_BranchRightNodes
            ├── SB_Right_Tier1
            │   └── Border_Right_Tier1
            │       └── Text_Right_Tier1
            ├── Spacer_Right_12
            ├── SB_Right_Tier2
            │   └── Border_Right_Tier2
            │       └── Text_Right_Tier2
            ├── Spacer_Right_23
            ├── SB_Right_Tier3
            │   └── Border_Right_Tier3
            │       └── Text_Right_Tier3
            ├── Spacer_Right_34
            ├── SB_Right_Tier4
            │   └── Border_Right_Tier4
            │       └── Text_Right_Tier4
            ├── Spacer_Right_45
            └── SB_Right_Tier5
                └── Border_Right_Tier5
                    └── Text_Right_Tier5
```

`Text_BranchRight` :

```text
Is Variable : ON
```

Runtime :

```text
index 2 de DA_RPGTalentPresentation
```

Pour Warrior : `Maître d'armes`.

---

# 16. RÉCAPITULATIF DES 15 PLACEHOLDERS

Ces widgets sont provisoires et seront remplacés par `WBP_RPGTalentNode` en UI-RPG03.2.

| Colonne | Tier | SizeBox | Border | Text |
|---|---:|---|---|---|
| Left | I | `SB_Left_Tier1` | `Border_Left_Tier1` | `Text_Left_Tier1` |
| Left | II | `SB_Left_Tier2` | `Border_Left_Tier2` | `Text_Left_Tier2` |
| Left | III | `SB_Left_Tier3` | `Border_Left_Tier3` | `Text_Left_Tier3` |
| Left | IV | `SB_Left_Tier4` | `Border_Left_Tier4` | `Text_Left_Tier4` |
| Left | V | `SB_Left_Tier5` | `Border_Left_Tier5` | `Text_Left_Tier5` |
| Center | I | `SB_Center_Tier1` | `Border_Center_Tier1` | `Text_Center_Tier1` |
| Center | II | `SB_Center_Tier2` | `Border_Center_Tier2` | `Text_Center_Tier2` |
| Center | III | `SB_Center_Tier3` | `Border_Center_Tier3` | `Text_Center_Tier3` |
| Center | IV | `SB_Center_Tier4` | `Border_Center_Tier4` | `Text_Center_Tier4` |
| Center | V | `SB_Center_Tier5` | `Border_Center_Tier5` | `Text_Center_Tier5` |
| Right | I | `SB_Right_Tier1` | `Border_Right_Tier1` | `Text_Right_Tier1` |
| Right | II | `SB_Right_Tier2` | `Border_Right_Tier2` | `Text_Right_Tier2` |
| Right | III | `SB_Right_Tier3` | `Border_Right_Tier3` | `Text_Right_Tier3` |
| Right | IV | `SB_Right_Tier4` | `Border_Right_Tier4` | `Text_Right_Tier4` |
| Right | V | `SB_Right_Tier5` | `Border_Right_Tier5` | `Text_Right_Tier5` |

Aucun de ces 45 widgets placeholder n'a besoin de `Is Variable` dans UI-RPG03.1.

---

# 17. RÉGLAGES TYPOGRAPHIQUES

Utiliser la fonte UI déjà adoptée dans le projet :

```text
Alegreya Sans
```

Tailles recommandées :

```text
CharacterName  : 28
ClassLevel     : 18
TalentPoints   : 19
Tabs           : 17
Branch titles  : 21
Tier labels    : 18
Skills title   : 24
Skills text    : 16
```

Ne pas mélanger plusieurs familles de fontes dans ce WBP.

---

# 18. RÈGLES DE COULEUR

Les couleurs finales de classe/branche ne doivent pas être hardcodées dans le Blueprint.

Le C++ fournit déjà :

```text
Text_ClassLevel    <- ClassPresentation.AccentColor
Text_BranchLeft    <- Branch[0].AccentColor
Text_BranchCenter  <- Branch[1].AccentColor
Text_BranchRight   <- Branch[2].AccentColor
```

Le Designer doit donc utiliser uniquement des couleurs neutres de fallback.

Les `Border_Branch*` peuvent rester gris sombre dans UI-RPG03.1.

La coloration complète des cadres/nœuds arrive en UI-RPG03.2/UI-RPG03.3.

---

# 19. RÈGLES DE TAILLE

Le layout doit être responsive à la taille disponible du panneau.

Ne pas positionner les éléments avec un Canvas absolu.

Utiliser :

```text
VerticalBox
HorizontalBox
Overlay
SizeBox
Spacer
Border
WidgetSwitcher
```

Le seul dimensionnement explicite requis pour UI-RPG03.1 est :

```text
Header               : 92 px
Tabs                 : 52 px
Talent node          : 68 x 68
Talent Points block  : 300 px largeur
Identity block       : 520 px largeur
Branch gaps          : 14 px
```

Les trois branches utilisent `Fill 1.0`.

Les espacements verticaux entre les cinq nœuds utilisent eux aussi `Fill 1.0`, ce qui évite d'avoir à calculer manuellement les positions.

---

# 20. CE QU'IL NE FAUT PAS FAIRE

Ne pas :

- supprimer le `Border` racine existant ;
- utiliser un `CanvasPanel` pour placer les 15 nœuds à la main ;
- taper manuellement `Gardien / Brise-ligne / Maître d'armes` dans les trois titres ;
- créer six versions du WBP selon la classe ;
- ajouter du gameplay dans le Graph Blueprint ;
- ajouter des boutons d'achat de talent maintenant ;
- ajouter de logique de prérequis maintenant ;
- construire les vrais nœuds maintenant ;
- committer le `.uasset` avant validation PIE et capture.

---

# 21. ORDRE DE CONSTRUCTION CONSEILLÉ DANS LE DESIGNER

Pour éviter de se perdre :

1. ouvrir `WBP_GridSkills` ;
2. conserver `Border_Root` ;
3. supprimer uniquement l'ancien placeholder enfant ;
4. créer `Panel_GridSkillsDesignerRoot` ;
5. créer `Border_Background` et `VB_Main` ;
6. construire entièrement le Header ;
7. construire entièrement la barre Tabs ;
8. créer `Border_Content` et `Switcher_SkillsTalents` ;
9. créer la page Skills index 0 ;
10. créer la page Talents index 1 ;
11. terminer toute la branche Left ;
12. dupliquer conceptuellement sa structure pour Center ;
13. dupliquer conceptuellement sa structure pour Right ;
14. vérifier les 10 widgets contractuels `Is Variable = ON` ;
15. mettre `Switcher_SkillsTalents.ActiveWidgetIndex = 1` ;
16. Compile ;
17. Save ;
18. PIE ;
19. K ;
20. capture d'écran.

---

# 22. CHECKLIST AVANT COMPILE

La hiérarchie doit contenir exactement ces éléments contractuels :

```text
[ ] Panel_GridSkillsDesignerRoot     Overlay       Variable
[ ] Text_CharacterName               TextBlock     Variable
[ ] Text_ClassLevel                  TextBlock     Variable
[ ] Text_TalentPoints                TextBlock     Variable
[ ] Button_SkillsTab                 Button        Variable
[ ] Button_TalentsTab                Button        Variable
[ ] Switcher_SkillsTalents           WidgetSwitcher Variable
[ ] Text_BranchLeft                  TextBlock     Variable
[ ] Text_BranchCenter                TextBlock     Variable
[ ] Text_BranchRight                 TextBlock     Variable
```

Puis :

```text
[ ] Switcher child 0 = Border_SkillsPage
[ ] Switcher child 1 = Border_TalentsPage
[ ] Left   = 5 placeholders
[ ] Center = 5 placeholders
[ ] Right  = 5 placeholders
[ ] aucune logique Blueprint ajoutée
[ ] root Border conservé
```

---

# 23. RÉSULTAT VISUEL ATTENDU EN PIE

Pour un Guerrier :

```text
┌──────────────────────────────────────────────────────────────────────────┐
│ ELIAS                                      Points de talent : 4          │
│ Guerrier — Niveau 12                                                   │
├──────────────────────────────────────────────────────────────────────────┤
│                    COMPÉTENCES        TALENTS                           │
├──────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│      GARDIEN               BRISE-LIGNE           MAÎTRE D'ARMES          │
│                                                                          │
│        [ I ]                  [ I ]                  [ I ]                 │
│          │                      │                      │                   │
│       [ II ]                 [ II ]                 [ II ]                │
│          │                      │                      │                   │
│      [ III ]                [ III ]                [ III ]                │
│          │                      │                      │                   │
│       [ IV ]                 [ IV ]                 [ IV ]                │
│          │                      │                      │                   │
│        [ V ]                  [ V ]                  [ V ]                 │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

À ce stade les nœuds sont encore des placeholders.

Le résultat attendu de UI-RPG03.1 est **la structure réelle de l'écran**, pas son polish final.

---

# 24. VALIDATION PIE UI-RPG03.1B

Après Compile + Save :

1. lancer PIE ;
2. appuyer sur `K` ;
3. vérifier que l'ancienne liste texte MON20 n'apparaît plus ;
4. vérifier le nom du personnage ;
5. vérifier classe + niveau ;
6. vérifier les Talent Points ;
7. cliquer `COMPÉTENCES` ;
8. vérifier passage index 0 ;
9. cliquer `TALENTS` ;
10. vérifier passage index 1 ;
11. vérifier les trois titres de branches ;
12. vérifier leur couleur ;
13. vérifier 5 nœuds par branche ;
14. changer de personnage ;
15. vérifier refresh du header ;
16. vérifier navigation globale I/K/G/M/J/H/ESC.

Le `.uasset` sera commité dans **UI-RPG03.1B** uniquement après cette validation visuelle.

---

# 25. SUITE APRÈS UI-RPG03.1

```text
UI-RPG03.2
    WBP_RPGTalentBranch
    WBP_RPGTalentNode
    WBP_RPGTalentDetail
    binding réel des 3 x 5 nœuds

UI-RPG03.3
    suppression du renderer natif MON20
    suppression de la projection plate legacy
    polish du vrai écran
```
