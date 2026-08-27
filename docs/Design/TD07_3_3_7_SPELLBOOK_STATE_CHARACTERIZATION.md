# TD07.3.3.7 — Spellbook State Characterization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline : `dc53cc1ab4a9443adca022aff412ebc98b3e6ce2`  
Statut : **CHARACTERIZATION VALIDÉE — NORMALIZATION ACTIVE**

## 1. Objet

TD07.3.3.7 doit supprimer la duplication d'autorité du Spellbook sans changer les règles de gameplay MON18.

La question d'architecture est simple :

```text
Quels sorts un personnage connaît-il ?
```

Cette information ne doit avoir qu'une seule autorité durable.

## 2. Autorité runtime actuelle

Aujourd'hui, l'autorité immédiate est :

```text
UGridPartySpellbookComponent
    -> SpellbookState               Transient
        -> FGridPartySpellbookState
            -> CharacterSpellbooks[]
                -> CharacterId
                -> KnownSpellIds[]
```

`LearnSpell()`, `ForgetSpell()`, `KnowsSpell()` et l'UI lisent ou mutent ce conteneur.

`FGridCharacterInventoryState` ne possède actuellement aucun `KnownSpellIds`.

## 3. Miroir Save actuel

MON18.8 contourne le caractère transient via une seconde représentation :

```text
FGridCharacterSpellbookSaveState
    CharacterId
    KnownSpellIds[]

UGrimrockPartySaveGame::CharacterSpellbookStates[]
```

Le pipeline est :

```text
UGridPartySpellbookComponent::SpellbookState
    -> CapturePartySpellbooks()
    -> CharacterSpellbookStates
    -> SaveGame
    -> RestorePartySpellbooks()
    -> UGridPartySpellbookComponent::SpellbookState
```

Il existe donc deux structures représentant le même état métier.

## 4. Snapshot sparse et déterministe

`CapturePartySpellbooks()` :

1. valide les CharacterId Active + CharacterPool ;
2. valide le runtime Spellbook ;
3. omet les personnages dont le Spellbook est vide ;
4. trie chaque `KnownSpellIds` par SpellId ;
5. trie les snapshots par CharacterId.

Le runtime `LearnSpell()` conserve actuellement l'ordre d'insertion ; le tri déterministe n'apparaît qu'à la frontière de persistance.

## 5. Restore de remplacement

`RestorePartySpellbooks()` construit un nouveau `FGridPartySpellbookState` complet :

```text
tous les personnages Active
+ tous les personnages CharacterPool
    -> un conteneur runtime
```

Un personnage absent du snapshot sparse retrouve un Spellbook vide.

Le candidat n'est commité qu'après validation complète. Une restauration invalide ne doit pas écraser le runtime précédent.

## 6. Hotbar : référence, jamais connaissance

Le binding :

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
```

n'enseigne jamais le sort.

Le contrat à préserver est :

```text
Hotbar contient Spell_ArcaneBolt
KnownSpellIds ne contient pas Spell_ArcaneBolt
    -> le sort reste inconnu
```

La hotbar n'est donc pas une autorité Spellbook.

## 7. Tolérance legacy encore présente

MON18.8 accepte volontairement un identifiant tel que :

```text
Spell_RemovedContent
```

même si aucune définition canonique n'existe.

La validation actuelle exige seulement :

```text
SpellId != NAME_None
aucun doublon
```

Cette tolérance provenait d'une politique de récupération de contenu temporairement manquant.

TD07.3 ayant adopté :

```text
exact-match
aucune migration arrière
aucune compatibilité prototype
```

cette tolérance sera réévaluée après validation du gate. La direction cible est de rejeter un SpellId qui ne résout plus une définition canonique courante.

## 8. Tests ajoutés

Filtre :

```text
Grimrock.TechnicalDebt.TD07_3_3_7.Characterization
```

Tests :

```text
RuntimeAuthorityBoundary
SparsePersistenceMirror
RestoreReplacementBoundary
LegacyToleranceAndHotbarIndependence
```

Attendu :

```text
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## 9. Direction de normalisation envisagée

Après validation du gate :

```text
FGridCharacterInventoryState
    -> KnownSpellIds[]
        autorité durable unique
```

Puis supprimer ce qui devient redondant :

```text
FGridCharacterSpellbookSaveState
UGrimrockPartySaveGame::CharacterSpellbookStates
CapturePartySpellbooks()
RestorePartySpellbooks()
FGridPartySpellbookState / FGridCharacterSpellbookState
    si aucune responsabilité utile ne subsiste
```

`UGridPartySpellbookComponent` ne sera conservé que s'il apporte encore une vraie valeur comme façade de mutations / notifications. Il ne doit plus posséder une seconde autorité.

`FGridSpellbookPersistence` sera supprimé ou réduit à une validation canonique directe du state durable.

Le changement de layout ouvrira une nouvelle génération SaveGame exact-match, vraisemblablement v17, sans migration.

## 10. Invariants à préserver

```text
Learn / Forget semantics
Character isolation
Active / CharacterPool preservation
Hotbar does not teach spells
Spell hotbar is never an ItemDefinition
UI reads the selected character's spell knowledge
combat catalogue/execution requires known spell
atomic save/load failure behavior
deterministic durable ordering
```

## 11. Hors périmètre du gate

```text
Status Effects
Skills
Class / Race identity
Quest state
DataAssets
Blueprints
maps
```

Aucune suppression de runtime ou de Save mirror ne commence avant validation de ce gate.

## 12. Validation

À exécuter :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD07_3_3_7.Characterization"
```

## 13. Stop condition du gate

- [x] autorité runtime actuelle documentée ;
- [x] miroir Save documenté ;
- [x] Active + CharacterPool documentés ;
- [x] snapshot sparse / déterministe documenté ;
- [x] restore de remplacement / atomicité documenté ;
- [x] indépendance Hotbar / connaissance documentée ;
- [x] tolérance unknown SpellId documentée ;
- [x] 4 tests de caractérisation ajoutés ;
- [x] compilation UE5.5.4 verte ;
- [x] 4/4 tests verts.

Validation locale du 27 août 2026 :

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_3_7.Characterization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le gate est atteint. La normalisation peut commencer.
