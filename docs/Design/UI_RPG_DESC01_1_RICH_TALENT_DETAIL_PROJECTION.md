# UI-RPG-DESC01.1 — Rich Talent & Variant Detail Projection

Date : **8 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **SOURCE IMPLÉMENTÉE — validation locale puis matérialisation UMG requises**

## Objectif

Enrichir le panneau de détail des Talents sans introduire une seconde description
gameplay ou un calcul métier dans UMG.

Deux lacunes sont corrigées :

1. les variantes possèdent déjà une description concrète dans
   `FGridTalentVariantView::Description`, mais le panneau n'affichait que la
   description conceptuelle du nœud ;
2. de nombreux Talents actifs ont un `Choice.Description` volontairement
   concis (« Débloque X »), alors que l'action autoritaire contient déjà les
   coûts et la description mécanique réelle.

## Autorités conservées

```text
FRPGClassProgressionChoiceDefinition
    -> nom / description du Talent ou de la variante

URPGClassAsset::CombatActions
    -> coût PA
    -> coût Mana
    -> portée
    -> cooldown
    -> description de l'action

FGridSkillsPageService
    -> projection read-only

UGridTalentDetailWidget
    -> présentation uniquement
```

Aucune valeur numérique n'est recopiée dans un nouvel asset de présentation.

## Read model

Nouveau type :

```text
FGridTalentUnlockedActionView
    ActionId
    DisplayName
    Description
    ActionPointCost
    ManaCost
    RangeCells
    CooldownRounds
```

Chaque `FGridTalentVariantView` expose désormais :

```text
UnlockedActions[]
```

Une action est associée au Choice concret lorsque ses `Requirements`
contiennent :

- le `ChoiceId` lui-même ; ou
- un `GrantedRequirementId` accordé par ce Choice.

Cela couvre notamment les nœuds conceptuels à variantes qui accordent un alias
logique commun.

## Panneau de détail

`UGridTalentDetailWidget` expose trois nouvelles projections :

```text
ResolvedVariantDisplayName
ResolvedVariantDescription
ResolvedActionSummary
```

Comportement :

### Nœud simple

La description principale reste celle du Choice.

Si ce Choice débloque une action, le résumé mécanique est construit depuis
l'action autoritaire.

Exemple conceptuel :

```text
Second souffle

Débloque Second souffle.

Second souffle — 1 PA — Recharge 4 tours
Restaure 20 % des PV maximum.
```

### Nœud à variantes non acquis

Avant choix concret :

```text
description conceptuelle
liste des variantes
aucune description de variante inventée
```

Après sélection d'une variante dans le ComboBox :

```text
Feu
Les sorts de Feu infligent +15 % de dégâts.
```

Le texte vient directement de `FGridTalentVariantView::Description`.

### Nœud à variante acquis

Le `SelectedChoiceId` permet de réafficher automatiquement la variante concrète
et sa description lors des ouvertures suivantes.

## Action summary

Le résumé est purement dérivé :

```text
NomAction — N PA [— N Mana] [— Portée N] [— Recharge N tour(s)]
Description autoritaire de l'action
```

Les segments nuls inutiles sont omis, sauf le coût PA qui reste toujours
explicite.

## UI-RPG-DESC01.2 — matérialisation Designer

Trois bindings optionnels sont préparés dans `WBP_RPGTalentDetail` :

```text
Text_DetailVariantName
Text_DetailVariantDescription
Text_DetailActionSummary
```

Aucun Event Graph n'est requis.

Tant que ces TextBlocks ne sont pas matérialisés, la source reste compatible.

## Authoring futur

UI-RPG-DESC01.1 ne réécrit pas encore les 90 descriptions de production.

Une passe séparée `UI-RPG-DESC01.3` pourra améliorer les descriptions
`ProgressionChoice` encore trop laconiques, en se fondant sur
`RPG_Talents_Mechanics_v0_1.md` et sur les données runtime réellement
authorées.

Cette séparation évite de mélanger :

- projection UI ;
- matérialisation UMG ;
- réauthoring massif des DataAssets.

## Automation

Filtre :

```text
Grimrock.UI.RPG.DESC01
```

Attendu : **5 tests**.

Couverture :

```text
ReadModel.ActionProjection
Detail.SimpleActionSummary
Detail.VariantPreview
Detail.AcquiredVariantPreview
Detail.WidgetContract
```

Commande :

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG.DESC01"
```
