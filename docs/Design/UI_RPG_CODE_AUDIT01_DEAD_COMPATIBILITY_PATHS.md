# UI-RPG-CODE-AUDIT01 — Remove Dead Talent/Skill Compatibility Paths

Date : **9 octobre 2026**  
État : **VALIDÉ / CLOS — 9 octobre 2026**

## Objectif

Supprimer les façades et projections historiques qui doublonnaient les autorités
actuelles de progression Skills/Talents, sans introduire de nouvelle couche.

## Suppressions Talent

La projection plate MON20.7/UI-RPG historique est supprimée :

```text
FRPGTalentRuntimeView
FRPGTalentPointBalance
FRPGTalentRuntimeService

FGridTalentEntryView
FGridSkillsPageView::Talents
UGridSkillsWidget::GetTalentEntryCount()
UGridSkillsWidget::GetTalentEntry()
```

Le read-model ne construit plus deux représentations des Talents à chaque
refresh. La seule projection UI Talent restante est :

```text
URPGClassAsset::ProgressionChoices
    -> FRPGClassProgressionService
    -> FRPGClassProgressionTransactionService
    -> FGridSkillsPageService
    -> FGridTalentTreeView
       -> FGridTalentBranchView
          -> FGridTalentNodeView
             -> FGridTalentVariantView
```

Le solde de Talent Points est lu directement depuis
`FRPGClassProgressionTransactionService::TryGetChoicePointBalance()`.

Le test MON20.7 dédié à la façade supprimée est retiré. Les tests de persistance,
PIE et page Skills/Talents sont migrés vers les autorités canoniques.

## Suppressions Skill

`FRPGSkillRuntimeService` et son test auto-référentiel MON20.6.4 sont supprimés.

Cette façade n'avait plus aucun consommateur C++ de production et exposait des
mutations directes de rang qui contournaient l'économie RPG-SKILL01.

Les responsabilités restantes sont explicites :

```text
FRPGSkillService
    -> primitive pure de lecture/mutation du tableau SkillRanks

FRPGSkillCheckService
    -> résolution d'un Skill Check

FRPGSkillPointService
    -> unique économie joueur des Skill Points
    -> RankCap
    -> achat
    -> Safe Undo de session

FGridSkillsPageService
    -> read-model UI
```

## Nettoyage progression

La surcharge historique pré-MON15.5 :

```cpp
FRPGClassProgressionService::CollectAutomaticSatisfiedRequirements(...)
```

est supprimée. Les appels utilisent désormais directement
`CollectSatisfiedRequirements(..., EmptySelection, ...)`.

## Garde-fou Blueprint

Les symboles Talent plats supprimés étaient réfléchis par Unreal. GitHub LFS ne
permet pas d'inspecter statiquement le bytecode de `WBP_GridSkills`.

Un test Editor dédié est donc ajouté :

```text
Grimrock.UI.RPG.CODEAUDIT01.BlueprintCompatibility
```

Il vérifie :

- absence réfléchie de `GetTalentEntry` ;
- absence réfléchie de `GetTalentEntryCount` ;
- absence de la propriété `FGridSkillsPageView::Talents` ;
- chargement du `WBP_GridSkills` de production ;
- recompilation du Blueprint sans `BS_Error`.

Aucun `.uasset` n'est modifié par le ticket.

## Éléments volontairement non supprimés

Les helpers `BlueprintPure` tels que `HasVariants()`,
`GetTalentNodeState()`, `GetTalentNodeWidgetForTier()` ou les champs
`PreviousNodeId` / `PreviousNodeDisplayName` ne sont pas supprimés ici.

Ils ont peu ou pas de consommateurs C++ mais restent exposés à Blueprint. Leur
suppression nécessite une preuve séparée de non-référence binaire ; elle ne doit
pas être faite à l'aveugle.

## Validation finale

Validation locale fournie par l'utilisateur le **9 octobre 2026**.

Le build complet a précédé les Automation ; les filtres suivants ont tous
terminé avec **Process exit code = 0**, sans warning ni échec :

```text
Grimrock.UI.RPG.CODEAUDIT01
    Succeeded              1
    Succeeded with warnings 0
    Failed                 0

Grimrock.UI.RPG.DESC01
    Succeeded             19
    Succeeded with warnings 0
    Failed                 0

Grimrock.MON20.8.SkillsPage
    Succeeded              8
    Succeeded with warnings 0
    Failed                 0

Grimrock.RPG.MON15.4
    Succeeded              7
    Succeeded with warnings 0
    Failed                 0

Grimrock.RPG.RPG03.10.PIE.SixClassTalentRuntime
    Succeeded              1
    Succeeded with warnings 0
    Failed                 0
```

Le garde-fou Editor a donc confirmé que `WBP_GridSkills` charge et recompile
sans référence aux symboles Talent plats supprimés.

La régression DESC01 confirme que le contrat final des 90 Talents reste intact.
La régression MON20.8 confirme que le read-model Skills/Talents canonique reste
opérationnel. MON15.4 confirme que le nettoyage du helper pré-transaction n'a
pas modifié les règles de progression. Enfin, le test PIE six classes confirme
le fonctionnement dans un vrai monde PIE.

**UI-RPG-CODE-AUDIT01 est VALIDÉ / CLOS.**
