# UI-RPG03.3 — Remove MON20 Native Skills Renderer

Date : **7 octobre 2026**  
État : **SOURCE PRÊTE — validation locale UE5.5.4 requise**

## Objectif

Après validation de UI-RPG03.2C, `WBP_GridSkills` est la seule présentation runtime autorisée de la surface Compétences / Talents.

UI-RPG03.3 supprime le renderer natif provisoire MON20 et la logique de bascule qui permettait à C++ de remplacer le contenu du WBP.

## Avant

```text
UGridSkillsWidget
├── WBP Designer si Panel_GridSkillsDesignerRoot existe
└── sinon
    └── construction runtime C++
        ├── NativeSkillsScroll
        ├── NativeSkillsContent
        └── TextBlocks dynamiques
```

Deux autorités de présentation existaient donc encore.

## Après

```text
FGridSkillsPageView
        ↓
UGridSkillsWidget
        ↓
WBP_GridSkills
├── Text_CharacterName
├── Text_ClassLevel
├── Text_TalentPoints
├── Button_SkillsTab
├── Button_TalentsTab
├── Switcher_SkillsTalents
├── Branch_Left
├── Branch_Center
└── Branch_Right
        ↓
WBP_RPGTalentBranch
        ↓
WBP_RPGTalentNode
```

Il n'existe plus aucun chemin alternatif de construction visuelle.

## Supprimé

```text
IsUsingDesignerPresentation()
RebuildPresentation()

Panel_GridSkillsDesignerRoot comme marker C++
Text_BranchLeft
Text_BranchCenter
Text_BranchRight

NativeScrollBox
NativeContentBox

WidgetTree runtime construction
GridSkillsWidgetPrivate::GetAttributeLabel()
GridSkillsWidgetPrivate::AddText()
GridSkillsWidgetPrivate::BranchTitleByIndex()

includes Border / ScrollBox / VerticalBox / VerticalBoxSlot
log de fallback MON20
```

## Contrat WBP désormais obligatoire

Les widgets suivants passent de `BindWidgetOptional` à `BindWidget` :

```text
Text_CharacterName
Text_ClassLevel
Text_TalentPoints
Button_SkillsTab
Button_TalentsTab
Switcher_SkillsTalents
Branch_Left
Branch_Center
Branch_Right
```

Une modification future de `WBP_GridSkills` qui supprime ou renomme un de ces widgets doit donc provoquer une erreur de compilation Blueprint au lieu de réactiver silencieusement un fallback.

## Ce qui n'est PAS supprimé dans ce ticket

Le read model n'est pas changé.

Restent volontairement :

```text
FGridSkillsPageView::Skills
FGridSkillsPageView::Talents
FGridTalentTreeView

GetSkillEntryCount()
GetSkillEntry()

GetTalentEntryCount()
GetTalentEntry()
```

Les getters Skills restent utiles au futur renderer détaillé de l'onglet Compétences.

La projection plate `Talents` est une dette séparée : elle est encore construite par `FGridSkillsPageService` et couverte par les tests MON20. Sa suppression doit être un ticket dédié, avec migration des tests/read model, pas un effet collatéral de UI-RPG03.3.

## Validation locale requise

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipAutomation
```

Puis :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG03.NodeBinding"
```

Et en PIE :

```text
K ouvre WBP_GridSkills
Compétences/Talents commute
header correct
3 branches correctes
5 nœuds par branche
tooltips corrects
changement de personnage refresh
aucun renderer texte MON20
```

## Critère de clôture

UI-RPG03.3 est fermé lorsque :

```text
build Editor = OK
NodeBinding = 4/4
PIE = validé
aucun NativeSkillsScroll / NativeSkillsContent dans le code
aucune construction WidgetTree dans UGridSkillsWidget
```
