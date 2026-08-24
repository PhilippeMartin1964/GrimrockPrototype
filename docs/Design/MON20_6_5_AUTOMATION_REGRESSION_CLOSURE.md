# MON20.6.5 — Automation Regression / Closure

Statut : **VALIDÉ UE5.5.4 — CLOS**  
Date : **24 août 2026**  
Jalon parent : **MON20.6 — Skills Data Model & Runtime**

---

## 1. Objectif

Clore MON20.6 après validation cumulative du modèle Skill, de la résolution déterministe et de l'accès runtime par personnage explicite ou sélectionné.

Aucun code supplémentaire n'est ajouté dans MON20.6.5 : les 24 tests cumulés couvrent déjà la régression de l'ensemble des tranches MON20.6.2 à MON20.6.4.

---

## 2. Validation finale UE5.5.4

Filtre exécuté :

```text
Grimrock.MON20.6.Skills
```

Résultat fourni le 24 août 2026 :

```text
24 / 24 Success
0 Fail
0 Error
```

Les familles couvertes sont :

```text
MON20.6.2 — Skill Definition + Character Runtime Ranks      8 tests
MON20.6.3 — Deterministic Skill Check Resolution            8 tests
MON20.6.4 — Runtime Access / Character Selection API        8 tests
                                                        --------
TOTAL                                                      24 tests
```

Aucun PIE n'est requis pour MON20.6 : aucune tranche n'introduit d'UI, d'interaction monde ou d'asset d'authoring.

---

## 3. Architecture finale MON20.6

```text
URPGSkillAsset
    -> SkillId
    -> DisplayName
    -> Description
    -> GoverningAttribute
    -> MaxRank
    -> bAllowUntrainedChecks

FGridCharacterInventoryState
    -> SkillRanks : TArray<FRPGSkillRank> [Transient]

FRPGSkillService
    -> ValidateSkillState
    -> GetSkillRank
    -> TrySetSkillRank
    -> TryIncreaseSkillRank

FRPGSkillCheckService
    -> TryResolveSkillCheck
    -> d20 + Rank + AttributeModifier >= Difficulty
    -> FRandomStream fourni par le caller

FRPGSkillRuntimeService
    -> CharacterIndex explicite
    -> SelectedCharacterIndex existant
    -> lecture / mutation / check
    -> NotifyPartyInventoryChanged uniquement si mutation réelle
```

L'autorité de personnage reste :

```text
UGridPartyInventoryComponent
    -> FGridPartyInventoryState
        -> ActiveCharacters
            -> FGridCharacterInventoryState
                -> SkillRanks
```

Aucun registre parallèle `CharacterId -> Skills` n'existe.

---

## 4. Formule de test figée

```text
Total = d20 + SkillRank + AttributeModifier
Success = Total >= Difficulty
```

avec :

```text
AttributeModifier = floor((AttributeValue - 10) / 2)
```

Règles :

- pas d'échec automatique sur 1 naturel ;
- pas de réussite automatique sur 20 naturel ;
- compétence non entraînée autorisée seulement si `bAllowUntrainedChecks` ;
- les rejets de validation ne consomment pas le `FRandomStream`.

---

## 5. Invariants validés

```text
SkillId valide
MaxRank > 0
rang absent -> 0
rang > 0 -> une entrée sparse
rang = 0 -> suppression de l'entrée
pas de SkillId dupliqué
rang hors plage -> rejet atomique
index personnage invalide -> rejet atomique
sélection personnage -> API existante uniquement
mutation réelle -> notification inventaire
mutation idempotente -> pas de notification artificielle
check invalide -> aucun RNG consommé
même seed -> même résultat
```

---

## 6. Persistance volontairement différée

`SkillRanks` reste `Transient` à la clôture de MON20.6.

Donc :

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 7
aucune migration SaveGame MON20.6
aucun snapshot Skill persistant
```

La persistance keyed by `CharacterId` reste réservée à MON20.9.

---

## 7. Hors scope confirmé

MON20.6 ne contient volontairement pas :

- talents ;
- arbre de talents parallèle ;
- monnaie de talents ;
- RequirementIds issus des Skills ;
- lockpicking réel ;
- pièges / dialogues / quêtes ;
- Lua / Event -> Command Skill ;
- UI de dépense de points ;
- SaveGame v8 ;
- catalogue production de compétences `.uasset`.

Ces intégrations restent dans MON20.7 à MON20.9 et dans les consommateurs ultérieurs.

---

## 8. Fichiers de référence

```text
docs/Design/MON20_6_1_SKILLS_ARCHITECTURE_CONTRACT.md
docs/Design/MON20_6_2_SKILL_DEFINITION_RUNTIME_RANKS.md
docs/Design/MON20_6_3_DETERMINISTIC_SKILL_CHECK_RESOLUTION.md
docs/Design/MON20_6_4_RUNTIME_ACCESS_CHARACTER_SELECTION.md
docs/Design/MON20_6_5_AUTOMATION_REGRESSION_CLOSURE.md
```

---

## 9. Conclusion

```text
MON20.6 — Skills Data Model & Runtime
VALIDÉ UE5.5.4 — CLOS — 24/24
```

Prochain jalon :

```text
MON20.7 — Talents / Progression Choice Integration
```

MON20.7 doit réutiliser les `ProgressionChoices` de MON15 au lieu d'introduire un second système de talents.
