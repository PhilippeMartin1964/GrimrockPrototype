# MON20.8.4 — Automation Validation

Date : **24 août 2026**  
Statut : **AUTOMATION VALIDÉE UE5.5.4 — 8/8 ciblés — ASSET/PIE EN ATTENTE**

Validation fournie depuis Unreal Engine 5.5.4 :

```text
Grimrock.MON20.8.SkillsPage
8 / 8 Success
0 Fail
0 Error
```

Tests validés :

```text
DeterministicSkillOrder
DuplicateDefinitionAtomic
MissingRankDefinitionAtomic
SelectedCharacterAuthority
SelectedCharacterIdentity
SkillRanks
TalentPointBalance
TalentProjection
```

Les 16 tests MON20.8.2 / MON20.8.3 avaient déjà été validés lors des passes précédentes. À ce stade, les 24 tests MON20.8 ont donc chacun été validés au moins une fois, mais la campagne cumulative `Grimrock.MON20.8 = 24/24` sera relancée dans MON20.8.5 avant clôture.

MON20.8.4 n'est pas encore clos : il reste la validation asset/PIE :

```text
WBP_GridSkills
    -> Reparent Blueprint
    -> UGridSkillsWidget
    -> Compile / Save
    -> PIE onglet Compétences
    -> changement de personnage
    -> vérification Skills / Talents / points
```

Aucune conclusion de compilation autonome n'est enregistrée ici, le journal fourni étant un journal Automation.
