# MON20.9.5 — Automation / PIE Regression & Closure

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — CLOS — 24/24 + PIE**

## 1. Objectif

Clore MON20.9 en consolidant la validation Automation et le smoke test PIE de la frontière réelle SaveGame v8 / Continue.

MON20.9.5 n'ajoute aucune abstraction de production et ne modifie aucun `.uasset/.umap`.

## 2. Validation Automation finale

La campagne cumulative MON20.9 est entièrement verte :

```text
Grimrock.MON20.9.SkillPersistence        8/8 Success
Grimrock.MON20.9.ActivePoolPersistence   8/8 Success
Grimrock.MON20.9.RestoredConsumers       8/8 Success
---------------------------------------------------
TOTAL                                   24/24 Success
0 Fail
0 Error
```

Cette campagne couvre :

```text
Save snapshot Skill v8
migration v7 -> v8
active + CharacterPool
restore par CharacterId
projection SkillId
projection des seuils
combat action gating
MissingRequirements
Skills page
SelectedCharacterIndex
snapshot vide autoritaire
restore invalide atomique
```

## 3. Frontière SaveGame réelle

`UGrimrockPartySaveGame::Serialize()` utilise le pipeline final MON20.9.

### Save

```text
PartyInventoryState
    -> CaptureStatusEffectState()
    -> ClassProgression / PendingLevelUps
    -> FRPGSkillPersistence::CapturePartySkills()
    -> CharacterSkillStates
    -> FRPGSaveMigrationService::ValidateCurrentSave()
    -> Super::Serialize()
```

### Load

```text
Super::Serialize()
    -> PrepareLoadedSave() / migration v8
    -> RestoreStatusEffectState()
    -> Restore class progression
    -> FRPGSkillPersistence::RestorePartySkills()
    -> SkillRanks runtime
    -> consommateurs MON20.8
```

Les `RequirementIds`, l'état des actions et le read model de la page restent dérivés et ne sont jamais sauvegardés.

## 4. Validation PIE — Continue

Le smoke test PIE du 24 août 2026 valide le slot réellement utilisé par **Continuer** :

```text
GrimrockParty
```

Le menu principal prépare explicitement ce slot :

```text
GrimrockGameInstance PendingLoadSlot Set Slot=GrimrockParty UserIndex=0
GrimrockGameInstance LoadSlot Requested Slot=GrimrockParty UserIndex=0
```

Le chargement SaveGame v8 est accepté :

```text
[GridSaveMigration] Load SourceVersion=8 TargetVersion=8
Migrated=false
Choices=2
PendingLevelUps=0
StatusCharacters=0
SpellbookCharacters=0
SkillCharacters=0
Result=Accepted
```

`SkillCharacters=0` est valide dans cette sauvegarde : aucun Skill de production n'est actuellement entraîné. Les cas non vides sont couverts par les tests Automation MON20.9.2 à MON20.9.4.

Le runtime confirme ensuite la reprise effective :

```text
PartySave Continued Slot=GrimrockParty CharacterCount=2
```

Le groupe de deux personnages est donc restauré et le jeu ne retourne pas au menu principal pour erreur de SaveGame.

## 5. Validation UI après Continue

Après chargement, le menu Grimrock s'ouvre correctement :

```text
GrimrockMenu UI Shown Pawn=BP_GrimrockPartyPawn_C_0
```

La sélection de personnage reste fonctionnelle :

```text
GridInventory SelectedCharacter Changed Old=1 New=0 Result=true
GridInventory SelectedCharacter Changed Old=0 New=1 Result=true
GridInventory SelectedCharacter Changed Old=1 New=0 Result=true
```

Cela confirme la stabilité du `SelectedCharacterIndex` et du chemin UI utilisé par la page Compétences après Continue.

Aucune erreur `GridSkillPersistence` n'est présente sur le chargement validé.

## 6. Slot secondaire rejeté — non bloquant pour MON20.9

Le même log contient des rejets concernant un autre slot :

```text
Slot=GrimrockParty_2
```

Le probe MON18.9.3 le rejette avec :

```text
Result=Rejected
Reason=IncompatibleSave
Detail=Le snapshot contient 0 états de progression pour 1 personnages actifs.
```

Ce slot n'est **pas** celui demandé par Continuer dans le test MON20.9.5. Le comportement est au contraire conforme au contrat fail-closed : un snapshot incohérent est détecté et rejeté.

`GrimrockParty_2` peut être supprimé ultérieurement s'il ne correspond plus à une sauvegarde utile.

## 7. Autres warnings observés — hors périmètre MON20.9

Le log contient également :

```text
Runtime object skipped: archetype CustomRecruiter_Service has no RuntimeActorClass.
[GridMonsterSpawn] ... Reason=PartyOccupiesCell
[GridMonsterState] MissingActor ...
```

Ces messages ne concernent ni la migration Skill v8 ni la restauration du groupe. Ils n'empêchent pas la validation MON20.9, mais doivent rester visibles dans la passe globale de régression MON20.10.

## 8. Critères de clôture

```text
[OK] SkillPersistence        8/8 Automation
[OK] ActivePoolPersistence   8/8 Automation
[OK] RestoredConsumers       8/8 Automation
[OK] TOTAL                  24/24 Automation
[OK] PIE Save/Continue v8 accepté sur GrimrockParty
[OK] groupe restauré : CharacterCount=2
[OK] menu / page Compétences utilisables après load
[OK] changement de personnage après load
[OK] aucune erreur GridSkillPersistence sur le slot validé
[OK] slot incohérent secondaire rejeté fail-closed
```

## 9. Architecture finale MON20.9

```text
FGridCharacterInventoryState::SkillRanks (Transient)
        ↓ capture
FRPGCharacterSkillSaveState keyed by CharacterId
        ↓
UGrimrockPartySaveGame::CharacterSkillStates
SaveVersion = 8
        ↓ load / migrate / validate
FRPGSkillPersistence::RestorePartySkills()
        ↓
SkillRanks runtime
        ├── Skill checks
        ├── Requirement projection
        ├── Combat action gating
        └── Skills page
```

Décisions finales :

- aucun second registre de personnages ;
- aucun état Talent parallèle ;
- `SkillRanks` reste `Transient` ;
- `CharacterSkillStates` est la frontière SaveGame explicite ;
- identité de rattachement par `CharacterId` ;
- actifs + réserve couverts ;
- restore atomique et fail-closed ;
- migration v7 -> v8 sans inventer de rang ;
- `RequirementIds` toujours dérivés ;
- consommateurs MON20.8 reconstruits après restore.

## 10. Conclusion

```text
MON20.9 — Persistence / Migration
VALIDÉ UE5.5.4 — CLOS — 24/24 + PIE
```

Suite autoritaire :

```text
MON20.10 — Balance / Regression / Closure
```
