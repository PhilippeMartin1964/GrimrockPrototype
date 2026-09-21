# UI-ITEM01-CLEAN01 — Remove Legacy Tooltip Projection Duplication

Date : **21 septembre 2026**  
Statut : **CODE PUSHÉ — VALIDATION UE5.5.4 À FAIRE**

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

## Dépendance volontairement conservée

`FGridItemTooltipEquipmentComparison::bHasEquippedItem` est désormais logiquement redondant depuis que les slots vides ne créent plus de comparaison.

Il reste temporairement exposé parce que le WBP local `WBP_ItemTooltipComparisonRow` branche encore son affichage du nom équipé sur ce booléen. Il sera supprimé après migration/sauvegarde de ce Graph, sans conserver de shim legacy.

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
