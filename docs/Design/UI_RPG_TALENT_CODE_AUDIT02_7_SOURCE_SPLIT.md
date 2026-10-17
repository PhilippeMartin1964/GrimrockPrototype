# UI-RPG-TALENT-CODE-AUDIT02.7 — Split oversized Skills/Talents sources

Date : 17 octobre 2026  
Statut : **IMPLÉMENTÉ — EN ATTENTE DE VALIDATION LOCALE**

## Constat

Deux fichiers concentraient des responsabilités devenues indépendantes :

```text
GridSkillsWidget.cpp      ~1556 lignes
GridSkillsPageService.cpp ~1556 lignes
```

Le premier mélangeait cycle de vie UMG, session Skills, mutations, sélection
Talent, flyout, rendu des branches et tri. Le second contenait environ 1240
lignes de helpers Talent avant cinq méthodes publiques.

## Découpage UGridSkillsWidget

```text
GridSkillsWidget.cpp
    InitializeSkillsWidget
    NativeConstruct / NativeDestruct
    BindDesignerShell / UnbindDesignerShell
    NativeOnPreviewMouseButtonDown
    ClearView
    ResolveCanonicalSkillDefinition
    RefreshSkills
    FindTalentNode
    PublishProgressionNotification
    HandlePartyInventoryChanged

GridSkillsWidgetSkills.cpp
    session d'allocation
    détail Skill
    increase / decrease
    résumé
    tri
    rows
    sélection Skill

GridSkillsWidgetTalents.cpp
    presentation catalogue Talent
    recherche node widget
    variant flyout
    sélection node / variante
    acquisition
    détail Talent
    callbacks Talent

GridSkillsWidgetPresentation.cpp
    ApplyDesignerPresentation
    ResolveCurrentClassVisual
    ApplyClassVisualIdentity
    tabs
```

Les helpers de tri Skills restent locaux au fichier Skills. Les helpers de
formatage/branche restent locaux au fichier Presentation.

## Découpage FGridSkillsPageService

```text
GridSkillsPageService.cpp
    ValidateAndSortDefinitions
    ResolveClassDefinition
    TryBuildCharacterView
    TryBuildSelectedCharacterView
    ResolveSkillSelection
    TryBuildSkillDetailView
    ResolveCanonicalSkillDefinitions

GridSkillsPageServiceTalentTree.cpp
    tier
    selected Choice set
    semantic node state
    topology validation
    BuildTalentNode
    BuildTalentTree

GridSkillsPageServiceTalentDetail.cpp
    structured effect rows
    reaction/action/status projection
    acquisition
    usage
    conceptual presentation

GridSkillsPageServiceInternal.h
    declarations privées strictement nécessaires entre TalentTree et TalentDetail
```

Aucun symbole n'est ajouté à `Public/`.

## Invariants

- aucun changement de signature publique ;
- aucun changement de `UFUNCTION` / `UPROPERTY` ;
- aucun changement de read-model ;
- aucun changement d'asset ou de config ;
- aucun changement de règles de progression ;
- les corps de méthodes déplacés sont conservés ;
- chaque fichier fonctionnel reste sous 900 lignes après split.

## Régressions 02.7

Le filtre `Grimrock.UI.RPG.CODEAUDIT02` ajoute :

```text
SourceSplit.FileSizes
SourceSplit.Responsibilities
```

Les tests vérifient les sept unités fonctionnelles, leur taille maximale et
l'ownership des principales responsabilités.
