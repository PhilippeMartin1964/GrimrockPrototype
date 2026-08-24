# MON20.9.1 — Persistence / Migration — Audit & Architecture Contract

Date : **24 août 2026**  
Statut : **TERMINÉ — contrat d’implémentation défini**

## 1. Objectif

MON20.9 rend persistantes les données MON20 qui restent runtime-only sans dupliquer les systèmes déjà sauvegardés.

Le besoin concret restant est `FGridCharacterInventoryState::SkillRanks` :

```text
FGridCharacterInventoryState
    -> SkillRanks : TArray<FRPGSkillRank>
       UPROPERTY(..., Transient, ...)
```

Les talents ne sont pas concernés par une nouvelle persistance : MON15.6/MON20.7 les sauvegardent déjà via `ClassProgressionStates` et `SelectedChoiceIds`.

## 2. Audit de l’existant

### 2.1 SaveGame courant

`UGrimrockPartySaveGame` possède actuellement :

```text
CurrentSaveVersion = 7
MinimumCompatibleSaveVersion = 1
PartyInventoryState
ClassProgressionStates
PendingLevelUpNotifications
CharacterStatusEffectStates
CharacterSpellbookStates
DungeonRuntimeState
```

`Serialize()` applique déjà une politique stricte :

```text
Capture runtime snapshots
    -> ValidateCurrentSave()
    -> Super::Serialize()

Load
    -> Super::Serialize()
    -> PrepareLoadedSave()
    -> restore runtime snapshots
```

### 2.2 Patterns de persistance existants

Le projet possède déjà deux patterns pertinents :

- Status Effects : état runtime `Transient`, snapshot séparé keyed by `CharacterId`, restore canonique ;
- Spellbook : snapshot sparse séparé keyed by `CharacterId`, couvrant personnages actifs + réserve, ordre déterministe et restore atomique.

Le pattern Spellbook est le plus proche des Skills.

### 2.3 Décision MON20.6 déjà prise

Le contrat MON20.6.1 avait explicitement réservé :

```text
MON20.9
    -> snapshot Skill keyed by CharacterId
```

Décision confirmée : **ne pas retirer simplement `Transient` de `SkillRanks` pour s’appuyer implicitement sur `PartyInventoryState`**.

La frontière SaveGame doit rester explicite et versionnée.

## 3. Contrat de persistance retenu

Créer une représentation SaveGame dédiée :

```text
FRPGSkillRankSaveState
    SkillId
    Rank

FRPGCharacterSkillSaveState
    CharacterId
    SkillRanks[]
```

et dans `UGrimrockPartySaveGame` :

```text
CharacterSkillStates : TArray<FRPGCharacterSkillSaveState>
```

Les structures runtime restent inchangées :

```text
FGridCharacterInventoryState::SkillRanks
    reste Transient
```

Le snapshot est une frontière de sérialisation, pas un second état runtime.

## 4. Service de persistance

Créer un service pur :

```text
FRPGSkillPersistence
```

API prévue :

```text
CapturePartySkills(...)
ValidateSavedPartySkills(...)
RestorePartySkills(...)
ResolveDefinitionBySkillId(...)
```

### Capture

La capture doit :

- couvrir `ActiveCharacters` et `CharacterPool` ;
- utiliser `CharacterId` comme seule identité de rattachement ;
- omettre les personnages sans rang entraîné ;
- trier les snapshots par `CharacterId` ;
- trier les rangs par `SkillId` ;
- rejeter `CharacterId` invalide/dupliqué/ambigu ;
- rejeter un état Skill runtime structurellement invalide ;
- rejeter un rang qui ne respecte plus la définition canonique.

### Restore

Le restore doit :

- travailler sur une copie candidate de `FGridPartyInventoryState` ;
- effacer les `SkillRanks` runtime de la candidate avant reconstruction ;
- résoudre chaque snapshot par `CharacterId`, indépendamment de l’ordre active/pool ;
- résoudre chaque `SkillId` via `RPGSkill:<SkillId>` ;
- vérifier `1 <= Rank <= Definition.MaxRank` ;
- reconstruire les rangs via le contrat `FRPGSkillService` ;
- n’appliquer la candidate qu’après validation complète.

Échec = **aucune mutation partielle**.

## 5. Politique sur les définitions manquantes

Contrairement au Spellbook, un Skill possède une contrainte numérique dépendante de sa définition (`MaxRank`).

Décision : un snapshot Skill non vide dont la définition canonique est absente ou invalide provoque un rejet du restore.

Raison : restaurer silencieusement un rang impossible rendrait incohérents :

```text
Skill checks
Requirement projection
Combat action gating
Skills page
```

La politique est donc fail-closed pour l’état Skill persistant.

## 6. SaveGame v8

MON20.9.2 incrémentera :

```text
UGrimrockPartySaveGame::CurrentSaveVersion
7 -> 8
```

`MinimumCompatibleSaveVersion` reste `1`.

### Migration v7 -> v8

Une sauvegarde v7 ne possède aucun snapshot Skill autoritaire puisque `SkillRanks` était `Transient`.

La migration correcte est donc :

```text
v7 domains existants
    -> validation inchangée
CharacterSkillStates
    -> vide
SaveVersion
    -> 8
ValidateCurrentSave(v8)
```

Aucun rang ne peut être inventé pendant la migration.

### Migrations v1-v6

Les chemins legacy existants continuent à reconstruire leurs domaines actuels et doivent également initialiser :

```text
CharacterSkillStates = []
```

avant la validation finale v8.

## 7. Validation SaveGame v8

`FRPGSaveMigrationService::ValidateCurrentSave()` devra valider en plus :

```text
FRPGSkillPersistence::ValidateSavedPartySkills(
    PartyInventoryState,
    CharacterSkillStates)
```

La validation doit couvrir personnages actifs et réserve.

## 8. Ordre de save/load retenu

### Save

```text
Party runtime
    -> CaptureStatusEffectState()
    -> Capture class progression / pending level up
    -> Capture Skill snapshots
    -> ValidateCurrentSave(v8)
    -> serialize
```

### Load

```text
Deserialize
    -> PrepareLoadedSave() / migrate to v8
    -> RestoreStatusEffectState()
    -> Restore class progression
    -> Restore Skill snapshots
    -> restore remaining runtime domains
```

Le restore Skill doit être terminé avant qu’un consommateur MON20.8 recalcule :

```text
SatisfiedRequirements
Combat actions
Skills page
```

## 9. Hors scope MON20.9

Ne pas introduire :

- nouvelle persistance Talent ;
- duplication de `ClassProgressionStates` ;
- persistance des `RequirementIds` dérivés ;
- persistance de la Skills page ;
- nouvelle monnaie de points de compétence ;
- assets de production Skill ;
- refactor global du SaveGame.

## 10. Découpage MON20.9

```text
MON20.9.1 — Audit & Architecture Contract                     TERMINÉ
MON20.9.2 — Skill Rank Save Snapshot + v8 Migration           PROCHAIN
MON20.9.3 — Active/Pool Character Persistence Regression
MON20.9.4 — Skill Projection / Skills Page Restore Regression
MON20.9.5 — Automation / PIE Regression & Closure
```

## 11. Critères de sortie MON20.9.2

```text
capture personnage actif entraîné        -> snapshot créé
capture personnage de réserve entraîné   -> snapshot créé
personnage sans Skill                     -> snapshot omis
ordre runtime différent                   -> snapshot déterministe
CharacterId invalide/dupliqué             -> rejet atomique
SkillId invalide/dupliqué                 -> rejet atomique
rang > MaxRank                            -> rejet atomique
définition Skill absente                  -> rejet atomique
restore snapshot valide                   -> SkillRanks reconstruits
ordre snapshots différent                 -> restore par CharacterId
migration v7                              -> Skill snapshots vides, v8
save courant                              -> CurrentSaveVersion = 8
RequirementIds                            -> toujours dérivés, jamais sauvegardés
```

## 12. Conclusion

MON20.9 n’a pas besoin d’un nouveau système de personnage ni d’un nouveau système de progression.

Le contrat retenu est :

```text
SkillRanks runtime Transient
        ↓ capture
FRPGCharacterSkillSaveState keyed by CharacterId
        ↓ SaveGame v8
migration / validation
        ↓ restore atomique
SkillRanks runtime
        ↓
MON20.8 Requirement projection / actions / UI
```

La prochaine étape est **MON20.9.2 — Skill Rank Save Snapshot + v8 Migration**.
