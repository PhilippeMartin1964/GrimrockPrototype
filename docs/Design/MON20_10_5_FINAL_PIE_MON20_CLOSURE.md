# MON20.10.5 — Final PIE / MON20 Closure

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — MON20 CLOS**  
Jalon parent : **MON20.10 — Balance / Regression / Closure**

---

## 1. Objectif

Clore MON20 — Recruitment / Skills / Talents après :

- audit de balance et de régression ;
- correction du restore de monstres morts ;
- classification des diagnostics connus ;
- campagne Automation complète ;
- smoke test PIE réel Save -> Continue -> UI -> Save.

Aucun nouveau système de production n'est ajouté dans MON20.10.5.

## 2. Baseline finale

Le HEAD validé avant cette clôture est :

```text
41884f2994e85065558ef27872acd07a12370dc0
Fix MON20.7 SaveVersion regression assertion
```

La correction MON20.7 rend le test de persistance compatible avec les versions SaveGame ultérieures sans attribuer à MON20.7 la montée v8 introduite par MON20.9.

## 3. MON20.10 — découpage final

```text
MON20.10.1 — Balance / Regression Audit                 CLOS
MON20.10.2 — Dead Monster Restore Occupancy Hardening   VALIDÉ UE5.5.4 — CLOS
MON20.10.3 — Log Hygiene / Known Diagnostics            CLOS
MON20.10.4 — Full MON20 Automation Regression           VALIDÉ UE5.5.4
MON20.10.5 — Final PIE / MON20 Closure                  VALIDÉ UE5.5.4 — CLOS
```

## 4. Automation finale MON20.10.4

Filtre :

```text
Grimrock.MON20
```

Résultat final fourni depuis UE5.5.4 :

```text
MON20.2       6
MON20.3       6
MON20.4      18
MON20.5      23
MON20.6      24
MON20.7      24
MON20.8      24
MON20.9      24
MON20.10.2    2
----------------
TOTAL       151/151 Success
0 Fail
0 Error
```

Le test précédemment obsolète :

```text
Grimrock.MON20.7.Talents.CaptureUsesMON15Snapshot
```

est désormais `Success` avec le contrat invariant :

```text
CurrentSaveVersion >= 7
```

au lieu d'une égalité historique figée sur 7.

## 5. PIE final — Continue réel

Le test final part de `L_MainMenu`, demande explicitement le slot :

```text
GrimrockParty
```

et suit le vrai chemin :

```text
GrimrockGameInstance PendingLoadSlot Set Slot=GrimrockParty UserIndex=0
GrimrockGameInstance PendingStartupMode Set Mode=1
GrimrockGameInstance LoadSlot Requested Slot=GrimrockParty UserIndex=0
-> L_GrimrockEditor
-> GrimrockStartupMode AppliedSaveSlot
-> Load SaveGame
-> Apply DungeonRuntimeState
-> PartySave Continued
```

La migration est acceptée :

```text
[GridSaveMigration]
SourceVersion=8
TargetVersion=8
Migrated=false
Choices=2
PendingLevelUps=0
StatusCharacters=0
SpellbookCharacters=1
SkillCharacters=0
Result=Accepted
```

`SkillCharacters=0` reste valide : la sauvegarde ne contient aucun Skill de production entraîné. Les snapshots Skill non vides sont couverts par MON20.9 Automation.

## 6. Validation réelle du correctif Dead Restore

La sauvegarde finale contient :

```text
Monsters=4
DeadMonsters=2
```

Les deux Gobelins lanceurs morts sont restaurés avec :

```text
RestoreDead ... Cell=(29,26) ... PresentationHidden=true
RestoreDead ... Cell=(26,25) ... PresentationHidden=true
```

Puis le runtime confirme :

```text
GridRuntimeState Apply Level=Into_The_Dark ... Monsters=4 DeadMonsters=2
PartySave Continued Slot=GrimrockParty CharacterCount=2
```

Les deux Rats géants vivants sont restaurés avec `RestoreAlive`.

Aucun des diagnostics bloquants historiques ne réapparaît pour les monstres morts :

```text
PartyOccupiesCell
MissingActor
```

Le runtime capture ensuite de nouveau les deux Gobelins avec :

```text
State=Dead HP=0 Dead=true
```

Ce test ferme la régression MON20.10.2 à la frontière SaveGame réelle, au-delà du harness Automation.

## 7. UI / SelectedCharacter / équipement

Après Continue :

```text
GrimrockMenu UI Shown Pawn=BP_GrimrockPartyPawn_C_0
GridInventory SelectedCharacter Changed Old=1 New=0 Result=true
GridInventory SelectedCharacter Changed Old=0 New=1 Result=true
```

Le menu reste fonctionnel après restore et l'autorité `SelectedCharacterIndex` demeure cohérente.

Le test a également manipulé la torche persistée :

```text
Unequip Item_Torch -> Result=true
Equip Item_Torch OffHand -> Result=true
GridEquipmentLight ... OffLight=true Result=true
Held item equipped: Item_Torch
```

Cela constitue un smoke test supplémentaire de l'inventaire/équipement après Continue.

## 8. Save après Continue

La fermeture du menu hors combat recapture le runtime :

```text
GridRuntimeState Capture ... Monsters=4 DeadMonsters=2
```

puis sauvegarde avec succès :

```text
PartySave Saved Slot=GrimrockParty Version=8 Characters=2 Spellbooks=1
```

Le Save -> Continue -> Save v8 est donc validé sans perte du statut mort des Gobelins.

## 9. Diagnostics connus acceptés

### CustomRecruiter data-only

```text
Runtime object skipped: archetype CustomRecruiter_Service has no RuntimeActorClass.
```

NON BLOQUANT. `CustomRecruiter` est un target logique/data-only par contrat MON20.5.

### Slot secondaire GrimrockParty_2

Un ancien slot incohérent peut être rejeté par le probe fail-closed. Cela ne concerne pas le slot `GrimrockParty` utilisé par Continuer.

### Save pendant combat

```text
PartySave SaveRejected ... Reason=CombatStateNotSaveable
```

est attendu et conforme à MON18.9.1. Les sauvegardes normales restent interdites pendant un combat actif.

## 10. Architecture finale MON20

```text
FGridPartyInventoryState
├── ActiveCharacters
├── ActiveEquipment
├── CharacterPool
└── FGridCharacterInventoryState
    └── SkillRanks (Transient)

Story Companion / Custom Recruit
    -> CharacterPool
    -> FRPGPartyRecruitmentService
    -> ActiveCharacters

Skills
    -> URPGSkillAsset
    -> FRPGSkillService / Check / Runtime
    -> SkillRank
    -> Requirement projection

Talents
    -> MON15 ProgressionChoices
    -> ChoiceId / ChoicePoints
    -> GrantedRequirementIds

Class + Equipment + Talent + Skill requirements
    -> FGridCombatActionCatalog
    -> MissingRequirements
    -> HUD / hotbar

SaveGame v8
    ├── PartyInventoryState
    ├── ClassProgressionStates
    ├── CharacterSkillStates keyed by CharacterId
    └── DungeonRuntimeState
```

Décisions structurantes maintenues :

- aucune seconde autorité de groupe ;
- maximum actif = 6 ;
- recrutement transactionnel et atomique ;
- aucun domaine Talent parallèle ;
- Skills sparse et data-driven ;
- `RequirementIds` dérivés, jamais sauvegardés ;
- rattachement persistant par `CharacterId` ;
- SaveGame v8 fail-closed ;
- actors morts persistants retenus sans occupation.

## 11. État final des sous-jalons MON20

```text
MON20.1   Audit & Architecture Contract                  CLOS
MON20.2   Active Party Recruitment Foundation            6/6
MON20.3   Story Companion Definition / Pool              6/6
MON20.4   Story Companion Recruitment UI                18/18
MON20.5   Custom Recruit / Wizard Reuse                 23/23 + PIE
MON20.6   Skills Data Model & Runtime                    24/24
MON20.7   Talents / Progression Choice                   24/24
MON20.8   Cross-System Requirements / Actions / UI       24/24 + PIE
MON20.9   Persistence / Migration                        24/24 + PIE
MON20.10  Balance / Regression / Closure                151/151 global + final PIE
```

## 12. Critères de clôture MON20

```text
[OK] Recruitment actif atomique
[OK] Story Companion
[OK] Custom Recruit
[OK] Skills runtime
[OK] deterministic Skill checks
[OK] Talents intégrés à MON15
[OK] Requirement projection
[OK] combat action gating
[OK] Skills/Talents UI
[OK] SaveGame v8 / migration
[OK] active + CharacterPool persistence
[OK] 151/151 Automation
[OK] Continue réel v8
[OK] 2 DeadMonsters restaurés correctement
[OK] menu / SelectedCharacter après restore
[OK] Save après Continue
[OK] aucun diagnostic MON20 bloquant restant
```

## 13. Conclusion

```text
MON20 — Recruitment / Skills / Talents
VALIDÉ UE5.5.4 — CLOS
```

Suite autoritaire :

```text
MON21 — Quests / Journal / Map / Codex
```

MON21 doit d'abord auditer les surfaces existantes `Journal`, `Map` et `Codex`, puis définir une architecture data-driven réutilisant Event -> Command, variables/Lua, `CharacterId`/ObjectId, SaveGame et le menu existant avant d'ajouter de nouvelles abstractions.
