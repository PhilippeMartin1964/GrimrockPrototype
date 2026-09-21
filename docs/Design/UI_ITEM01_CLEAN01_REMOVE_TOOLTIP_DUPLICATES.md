# UI-ITEM01-CLEAN01 — Remove Legacy Tooltip Projection Duplication

Date : **21 septembre 2026**  
Statut : **VALIDÉ UE5.5.4 — 21 septembre 2026 — 4/4 tests**

## Objectif

Réduire UI-ITEM01 à une seule projection structurée sans déplacer la présentation UMG vers le C++.

Le ticket supprime :

- les anciens helpers Blueprint de tooltip remplacés par `GetTooltipView()` ;
- les résumés textuels parallèles `UsageSummary`, `StatSummary`, `ComparisonSummary` ;
- `bIntegerValue`, qui n'était plus lu après formatage de `ValueText` / `DeltaText` ;
- `bCanEquipToSlot`, redondant dans une comparaison déjà issue d'un slot compatible.

`GetTooltipText()` reste utilisé par l'action **Examiner** et construit désormais ses chaînes à la demande à partir de `StatLines[]` et `EquipmentComparisons[]`.

## Autorité conservée

~~~text
C++
  GetTooltipView()
  -> FGridItemTooltipView
     -> StatLines[]
     -> EquipmentComparisons[]

UMG
  -> SetText / Visibility
  -> création des widgets de lignes
  -> couleurs / icônes / typographie
~~~

Aucune autorité gameplay n'est déplacée vers Blueprint.

## Helpers supprimés

~~~text
GetDisplayNameText
GetQuantityText
GetItemTypeDisplayText
GetCompatibleEquipmentSlotsText
GetLightTooltipText
~~~

La construction du texte des slots compatibles reste interne au fichier C++ et alimente `CompatibleSlotsText`.

## État de `bHasEquippedItem`

`FGridItemTooltipEquipmentComparison::bHasEquippedItem` reste une donnée utile du read model.

UI-ITEM01 conserve volontairement les comparaisons avec un slot compatible vide : dans ce cas `bHasEquippedItem=false`, `EquippedItemName` reste vide et les bonus/résistances du candidat sont comparés à zéro. Pour un slot occupé, `bHasEquippedItem=true` et `EquippedItemName` identifie l'objet de référence.

Ce booléen ne doit donc pas être supprimé par un cleanup ultérieur tant que ce contrat existe.

## Automation

Filtre dédié :

~~~text
Grimrock.UI.Item01.Clean01
~~~

Test :

~~~text
Grimrock.UI.Item01.Clean01.NoLegacyTooltipProjection
~~~

Le test vérifie l'absence des anciens helpers et propriétés, ainsi que la présence de `GetTooltipView()`, `GetTooltipText()`, `StatLines[]` et `EquipmentComparisons[]`.


## Validation locale

Validation reçue le 21 septembre 2026 :

~~~text
Filter                 : Grimrock.UI.Item01
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
~~~

UI-ITEM01-CLEAN01 est validé avec l'ensemble du filtre UI-ITEM01.
