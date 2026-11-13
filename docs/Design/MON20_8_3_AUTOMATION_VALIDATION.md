# MON20.8.3 — Automation Validation

> **ARM-CLEAN06 — MODÈLE D'OBJETS APPROUVÉ (13.11.2026).** [Contrat normatif](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [plan de migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les comportements de ce jalon restent une référence fonctionnelle ; seules ses prescriptions concernant les anciennes structures d'items/armes, copies de données et références d'assets sont remplacées. **Modèle cible non encore implémenté ; zéro legacy final.**


Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — 8/8 ciblés — 16/16 MON20.8 cumulés**

Validation fournie depuis Unreal Engine 5.5.4 :

```text
Grimrock.MON20.8.ActionRequirements
8 / 8 Success
0 Fail
0 Error
```

Campagne cumulative au même passage :

```text
Grimrock.MON20.8.SkillRequirements   8 / 8 Success
Grimrock.MON20.8.ActionRequirements  8 / 8 Success
-----------------------------------------------
TOTAL                               16 / 16 Success
```

Les tests validés couvrent :

```text
MissingRequirementDiagnostic
MissingRequirementsDeterministicOrder
NonRequirementFailureKeepsDiagnostic
RequirementsSatisfied
SkillAndExistingRequirementsCompose
SkillIdUnlocksAction
SkillProjectionFailureAtomic
SkillThresholdUnlocksAction
```

Cette note complète `MON20_8_3_COMBAT_ACTION_REQUIREMENT_INTEGRATION_DIAGNOSTICS.md` sans modifier le contrat technique. Le prochain travail autoritaire est MON20.8.4.
