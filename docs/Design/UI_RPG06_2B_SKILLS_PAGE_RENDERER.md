# UI-RPG06.2B — Renderer COMPÉTENCES

Date : **7 octobre 2026**  
Parent : **UI-RPG06 — Unification Compétences + Talents UX**  
État : **SOURCE 7/7 + MATÉRIALISATION UMG/PIE VALIDÉES — finalisation UI-RPG06.3A en cours**

## 1. Objectif

Remplacer le placeholder de la page `COMPÉTENCES` de `WBP_GridSkills` par une
liste réelle des 25 Skills de production.

Le flux reste unique :

```text
URPGSkillAsset
        +
FGridCharacterInventoryState::SkillRanks
        ↓
FGridSkillsPageService
        ↓
FGridSkillEntryView[]
        ↓
UGridSkillsWidget
        ↓
UGridSkillEntryWidget
        ↓
WBP_RPGSkillEntry
```

Aucun état gameplay n'est créé dans UMG.

## 2. Données affichées

Une ligne expose uniquement les données existantes :

```text
DisplayName
GoverningAttribute
Rank / MaxRank
bTrained
bAllowUntrainedChecks
Description si non vide
```

Il n'existe toujours pas dans le modèle courant :

- d'XP de Skill ;
- de barre de progression vers le prochain rang ;
- de solde de Skill Points ;
- de transaction d'achat de rang.

Ces éléments ne doivent donc pas être dessinés dans la page.

## 3. Widget réutilisable

Créer manuellement :

```text
/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/RPG/WBP_RPGSkillEntry
```

Parent C++ :

```text
UGridSkillEntryWidget
```

### Hiérarchie exacte

```text
SB_SkillEntryRoot                         [SizeBox]
└── Border_SkillEntryRoot                 [Border]
    └── VB_SkillEntry                     [VerticalBox]
        ├── HB_SkillSummary               [HorizontalBox]
        │   ├── Text_SkillName            [TextBlock] VARIABLE = YES
        │   ├── SB_SkillAttribute         [SizeBox WidthOverride=160]
        │   │   └── Text_SkillAttribute   [TextBlock] VARIABLE = YES
        │   ├── SB_SkillRank              [SizeBox WidthOverride=110]
        │   │   └── Text_SkillRank        [TextBlock] VARIABLE = YES
        │   └── SB_SkillTrainingPolicy    [SizeBox WidthOverride=240]
        │       └── Text_SkillTrainingPolicy [TextBlock] VARIABLE = YES
        └── Text_SkillDescription         [TextBlock] VARIABLE = YES
```

Réglages :

- `SB_SkillEntryRoot.Min Desired Height = 52` ;
- `Border_SkillEntryRoot.Padding = 10, 6` ;
- `Text_SkillName` : slot HorizontalBox = **Fill 1.0**, alignement vertical Center, Font Size **18** ;
- `Text_SkillAttribute` : Font Size **16** ;
- `Text_SkillRank` : Font Size **16**, justification Center ;
- `Text_SkillTrainingPolicy` : Font Size **15** ;
- `Text_SkillDescription` : Font Size **14**, slot VerticalBox = Auto, padding haut 4, Auto Wrap Text = true ;
- les trois `SizeBox` de droite : slot = **Auto**, alignement vertical Center ;
- les quatre textes de synthèse : Auto Wrap Text = false ;
- tous les TextBlocks : **Not Hit-Testable (Self & All Children)** ;
- aucune variable supplémentaire n'est nécessaire ;
- aucun Event Graph n'est nécessaire.

Le C++ masque automatiquement l'attribut si `GoverningAttribute=None` et la
description si elle est vide.

## 4. Migration de WBP_GridSkills

Asset :

```text
/Game/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridSkills
```

Dans la page index 0 du `Switcher_SkillsTalents`, conserver :

```text
Border_SkillsPage
└── Overlay_SkillsPage
    ├── Border_SkillsInner
    └── VB_SkillsPage
```

Dans `VB_SkillsPage`, remplacer uniquement `Text_SkillsPlaceholder` par :

```text
HB_SkillsColumns                         [HorizontalBox]
├── Text_HeaderSkill                     [TextBlock]
├── SB_HeaderAttribute                   [SizeBox WidthOverride=160]
│   └── Text_HeaderAttribute             [TextBlock]
├── SB_HeaderRank                        [SizeBox WidthOverride=110]
│   └── Text_HeaderRank                  [TextBlock]
└── SB_HeaderStatus                      [SizeBox WidthOverride=240]
    └── Text_HeaderStatus                [TextBlock]

Overlay_SkillsListArea                   [Overlay]
├── ScrollBox_Skills                     [ScrollBox]
│   └── Panel_SkillEntries               [VerticalBox] VARIABLE = YES
└── Text_EmptySkills                     [TextBlock] VARIABLE = YES
```

Textes de colonnes :

```text
COMPÉTENCE
ATTRIBUT
RANG
STATUT
```

Slots et rendu validés en PIE :

- `HB_SkillsColumns` = Auto dans `VB_SkillsPage` ;
- `Text_HeaderSkill` = Fill 1.0, Font Size **18** ;
- `Text_HeaderAttribute` = Font Size **16** ;
- `Text_HeaderRank` = Font Size **16** ;
- `Text_HeaderStatus` = Font Size **16** ;
- les trois SizeBox d'en-tête = Auto ;
- `Overlay_SkillsListArea` = **Fill 1.0** dans `VB_SkillsPage` ;
- `ScrollBox_Skills` = **Fill Horizontal + Fill Vertical** dans son slot Overlay ;
- `ScrollBox_Skills.Always Show Scrollbar = true` ;
- scrollbar visible en PIE ;
- `Panel_SkillEntries` = unique enfant du ScrollBox, Horizontal Alignment = Fill ;
- `Text_EmptySkills` = Center/Center, Font Size **16**, texte `Aucune compétence disponible.` ;
- `Text_EmptySkills` = Not Hit-Testable ;
- ne pas ajouter de bouton d'achat ou de `ProgressBar`.

Dans **Class Defaults** de `WBP_GridSkills` :

```text
RPG | Skills | UI | Presentation
Skill Entry Widget Class = WBP_RPGSkillEntry
```

## 5. Présentation d'une ligne

Le C++ produit :

```text
Crochetage        Dextérité      Rang 0 / 5      Entraînement requis
Perception        Sagesse        Rang 0 / 5      Test possible sans entraînement
```

Pour un rang acquis :

```text
Perception        Sagesse        Rang 2 / 5      Entraînée
```

La description apparaît sous la ligne uniquement si le DataAsset en contient
une. Les 25 assets canoniques actuels n'inventent aucune description.

## 6. Validation

Après pull :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG06.Skills"
```

Le filtre doit couvrir désormais :

- catalogue canonique ;
- préservation des extensions d'authoring ;
- politique entraînée/non entraînée du read model ;
- chargement des 25 DataAssets de production ;
- présentation d'une ligne ;
- rejet des projections incohérentes ;
- contrat du renderer.

Après matérialisation UMG, PIE :

```text
K ouvre WBP_GridSkills
COMPÉTENCES = index 0
25 lignes visibles
changement de personnage -> refresh automatique
aucune barre XP / aucun Skill Point / aucun bouton d'achat
TALENTS reste inchangé et fonctionnel
```

## 7. Suite

UI-RPG06.2C est matérialisé et validé en PIE.

UI-RPG06.3A rend le contrat Designer obligatoire et remplace l'ordre technique
par `SkillId` par un ordre de présentation alphabétique sur `DisplayName`.
`SkillId` reste le tie-break déterministe.

La projection Talent plate historique n'est pas supprimée dans cette tranche :
elle est Blueprint-readable et peut encore être référencée par des assets
binaires. Sa suppression exige un audit dédié des références Blueprint.

UI-RPG06.3B exécute la régression finale Skills/Talents et clôt les statuts
documentaires devenus obsolètes.
