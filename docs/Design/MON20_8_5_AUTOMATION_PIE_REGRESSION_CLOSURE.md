# MON20.8.5 — Automation / PIE Regression & Closure

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — CLOS — 24/24 + PIE**

## Validation finale

Campagne cumulative exécutée dans Unreal Engine 5.5.4 :

```text
Grimrock.MON20.8
24 / 24 Success
0 Fail
0 Error
```

Répartition :

```text
Grimrock.MON20.8.SkillRequirements    8/8 Success
Grimrock.MON20.8.ActionRequirements   8/8 Success
Grimrock.MON20.8.SkillsPage           8/8 Success
```

La campagne finale confirme dans une même passe :

- identité canonique `RPGSkill:<SkillId>` ;
- projection atomique des `RequirementIds` issus des Skills ;
- déverrouillage d'actions par `SkillId` et seuil de rang ;
- composition avec les requirements classe, équipement et talents ;
- diagnostics structurés `MissingRequirements[]` ;
- read model Skills/Talents déterministe ;
- autorité du personnage sélectionné ;
- rejets atomiques en cas de définition ou rang incohérent.

## Validation PIE

`WBP_GridSkills` a été reparenté sur `UGridSkillsWidget`, compilé et sauvegardé.

La présentation native MON20.8.4 remplace correctement le placeholder au runtime et affiche :

```text
Compétences & talents
Nom du personnage
Points de talent
Compétences
Talents acquis
```

Le changement de personnage a été validé en PIE avec deux personnages distincts : la page passe correctement de `Elias` à `Elarion` et reconstruit la vue à partir de `SelectedCharacterIndex`.

L'absence actuelle de compétences et talents de production sur ces personnages n'est pas une anomalie : le read model et l'UI affichent correctement les états vides.

## Architecture finale MON20.8

```text
URPGSkillAsset
    -> SkillId / rank thresholds
    -> FRPGSkillRequirementProjectionService
    -> SatisfiedRequirements

MON15 / MON20.7 talents
    -> ChoiceId + GrantedRequirementIds
    -> SatisfiedRequirements

ClassId + ItemTags + Talent Requirements + Skill Requirements
    -> FGridCombatActionCatalog
    -> MissingRequirements[] / Enabled
    -> HUD / hotbar existants

SelectedCharacterIndex
    -> FGridSkillsPageService
    -> UGridSkillsWidget
    -> WBP_GridSkills
```

Décisions confirmées :

- aucun système Talent parallèle ;
- aucun second catalogue d'actions ;
- aucun second registre de personnages ;
- les `RequirementIds` restent dérivés et non persistés ;
- la page Compétences reste read-only pour la progression dans MON20.8 ;
- `SkillRanks` reste encore `Transient` à la clôture de MON20.8 ; sa persistance est explicitement reportée à MON20.9.

## Suite

```text
MON20.9 — Persistence / Migration
```

Premier travail : auditer le contrat SaveGame actuel, définir la persistance des `SkillRanks` par `CharacterId` et la migration depuis la version 7 sans dupliquer l'état de progression MON15/MON20.7.
