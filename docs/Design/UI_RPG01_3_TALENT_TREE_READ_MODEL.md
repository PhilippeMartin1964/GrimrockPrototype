# UI-RPG01.3 — Talent Tree Read Model Implementation

Date : **6 octobre 2026**  
Parent : **UI-RPG01 — UX Contract & Talent Tree Read Model**  
État : **SOURCE IMPLÉMENTÉ — VALIDATION UE5.5.4 ET MATÉRIALISATION DES 6 DA_Class_* EN ATTENTE**

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

Les `.uasset` de production ne sont pas modifiés à l'aveugle dans ce commit source. Tant que tous les choix d'une classe ont leurs deux métadonnées vides, la page plate existante reste fonctionnelle et `TalentTree` reste vide. Un asset partiellement migré est rejeté atomiquement.

Après build C++ réussi, les six scripts/commandlets d'authoring devront être exécutés localement afin de matérialiser les métadonnées dans les `DA_Class_*`.

## Tests source ajoutés

Filtre :

```text
Grimrock.UI.RPG01.ReadModel
```

Cas : `TreeShape`, `VariantSelection`, `PointsLock`, `PartialMetadataAtomic`, `PreMaterializationBridge`.

UI-RPG01.4 ajoutera la validation globale des assets de production : 6 classes / 18 branches / 90 nœuds conceptuels.

## Validation demandée

```powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG01.ReadModel"
```

Ne pas considérer UI-RPG01.3 validé tant que la sortie locale n'a pas été fournie.
