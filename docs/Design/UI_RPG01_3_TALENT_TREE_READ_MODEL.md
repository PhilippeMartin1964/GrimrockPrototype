# UI-RPG01.3 — Talent Tree Read Model Implementation

Date : **6 octobre 2026**  
Parent : **UI-RPG01 — UX Contract & Talent Tree Read Model**  
État : **VALIDÉ — Automation `Grimrock.UI.RPG01.ReadModel` 5/5 ; production matérialisée ensuite par UI-RPG01.4**

## Objectif

Implémenter le contrat UI-RPG01.2 sans créer de nouvelle autorité RPG et sans toucher encore à l'UMG final.

`FRPGClassProgressionChoiceDefinition` porte désormais `TalentBranchId` et `TalentNodeId`. Ces champs structurent seulement la projection UI : `ChoiceId` et `SelectedClassProgressionChoiceIds` restent les identités et l'autorité durables.

Les six authorings renseignent explicitement leurs trois branches. Les nœuds à variantes regroupent leurs ChoiceIds sous un même `TalentNodeId` : Spécialisation martiale, Ennemi juré, Affinité élémentaire et Imprégnation. Aucun parsing de ChoiceId n'est ajouté au runtime.

## Read model

`GridSkillsUiTypes.h` ajoute :

```text
EGridTalentNodeState
FGridTalentVariantView
FGridTalentNodeView
FGridTalentBranchView
FGridTalentTreeView
```

`FGridSkillsPageView` expose aussi `CharacterLevel`, `ClassId`, `ClassDisplayName` et `TalentTree`. La projection plate `Talents[]` est conservée temporairement jusqu'à UI-RPG03 pour préserver le renderer natif minimal.

`FGridSkillsPageService` reste le service read-only unique. Il groupe par branche/nœud, délègue la disponibilité à `FRPGClassProgressionService::GetChoiceAvailability()`, dérive les paliers depuis 2/6/10/14/18, valide les variantes et vérifie la chaîne conceptuelle des prérequis.

## Migration des assets

Le commit UI-RPG01.3 n'a modifié aucun `.uasset` à l'aveugle. Le pont de pré-matérialisation a permis de conserver la page plate tant que les métadonnées étaient entièrement absentes et de rejeter atomiquement un asset partiellement migré.

UI-RPG01.4 a ensuite matérialisé proprement les six `DA_Class_*` via Unreal Editor ; le pont reste documenté comme garde de migration mais les assets de production actuels portent tous `TalentBranchId` et `TalentNodeId`.

## Tests source ajoutés

Filtre :

```text
Grimrock.UI.RPG01.ReadModel
```

Cas : `TreeShape`, `VariantSelection`, `PointsLock`, `PartialMetadataAtomic`, `PreMaterializationBridge`.

UI-RPG01.4 a ensuite validé les invariants globaux de production : **6 classes / 18 branches / 90 nœuds conceptuels**.

## Validation obtenue

```text
Filter                  : Grimrock.UI.RPG01.ReadModel
Succeeded               : 5
Succeeded with warnings : 0
Failed                  : 0
Process exit code       : 0
```

Commit source : `7906028fad27a4cc2e36b57979454aa384c73242`.
