# UI-RPG-TALENT-CODE-AUDIT02.4 — Dedicated Talent detail read-model

Date : 17 octobre 2026  
Statut : **IMPLÉMENTÉ — EN ATTENTE DE VALIDATION LOCALE**

## Problème constaté

Le read-model de l'arbre `FGridTalentNodeView` était également détourné comme
read-model de la fiche. Pour une variante, `BuildSelectedTalentDetailView()`
copiait le nœud conceptuel puis écrasait :

```text
DisplayName
State
Type
Principle
Effects
Usage
SimpleChoiceId
bCanAcquireSimple
bHasExclusiveVariants
Variants
SelectedChoiceId
```

Le résultat n'était donc plus réellement un node d'arbre, mais un faux node
fabriqué pour satisfaire `UGridTalentDetailWidget`.

## Contrat retenu

```text
FGridTalentNodeView
    identité conceptuelle
    topologie de l'arbre
    état du node
    Variants[]

FGridTalentDetailView
    TalentNodeId
    TalentBranchId
    ChoiceId
    DisplayName
    State
    Type
    Principle
    Effects[]
    Usage[]
    Acquisition
    bCanAcquire
```

Le detail read-model ne transporte aucune topologie :

```text
ABSENT :
Tier
PreviousNodeId
PreviousNodeDisplayName
SelectedChoiceId
SimpleChoiceId
bCanAcquireSimple
bHasExclusiveVariants
Variants[]
```

## Projection

`FGridTalentDetailView::TryBuildFromNode()` est une projection pure.

Talent simple :

```text
SimpleChoiceId -> Detail.ChoiceId
Node content   -> Detail content
```

Variante concrète :

```text
TalentNodeId / TalentBranchId -> identité conceptuelle conservée
Variant.ChoiceId              -> Detail.ChoiceId
Variant content               -> Detail content
Node.Acquisition              -> Detail.Acquisition
```

Vue conceptuelle de diagnostic :

```text
ChoiceId = None
bCanAcquire = false
```

Elle ne contient toujours aucun tableau de variantes.

## Presenter

`UGridTalentDetailWidget` stocke maintenant :

```text
FGridTalentDetailView DetailView
```

et non plus :

```text
FGridTalentNodeView NodeView
```

Le bouton d'acquisition utilise exclusivement `DetailView.ChoiceId`.

## Périmètre inchangé

Aucun changement de :

- `URPGClassAsset` ;
- règles de progression ;
- transactions ;
- SaveGame ;
- DataAssets ;
- WBP / .uasset ;
- structure 3 branches × 5 nodes ;
- flyout des variantes.

## Validation

Filtre principal :

```text
Grimrock.UI.RPG.CODEAUDIT02
```

Nouveaux tests 02.4 :

```text
TalentDetailReadModel.Shape
TalentDetailReadModel.Projection
TalentDetailReadModel.Conceptual
```

Régressions ciblées :

```text
Grimrock.UI.RPG04.Detail
Grimrock.UI.RPG04.Acquisition
Grimrock.UI.RPG.DESC01
Grimrock.UI.RPG.CODEAUDIT01.BlueprintCompatibility
Grimrock.UI.RPG01.ProductionAssets
```
