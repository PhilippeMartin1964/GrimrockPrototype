# TD07.3.3 — Character State Normalization — Closure

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Statut : **VALIDÉ ET CLOS**  
Schéma Save final : **v20 exact-match**

## 1. Résultat

TD07.3.3 remplace les autorités historiques parallèles du personnage par un contrat unique.

```text
Durable authority
    CharacterId / identity
    Attributes
    Experience
    Resources
    SelectedClassProgressionChoiceIds
    SkillRanks
    KnownSpellIds
    StatusEffects
    LastAcknowledgedLevel
    InventorySlots
    CombatHotbarSlots

Transient / reconstructed
    Level
    DerivedStats
    StatusEffect DefinitionAsset
    class progression runtime projection
    UI / combat read models
```

## 2. Nettoyages réalisés

```text
TD07.3.3.2  legacy Strength / bRPGAttributesInitialized removed
TD07.3.3.3  DerivedStats separated from mutable Resources
TD07.3.3.4  CurrentWeight / MaxCarryWeight caches removed
TD07.3.3.5  Experience authoritative; Level transient; choices durable
TD07.3.3.6  SkillRanks durable; separate Skill snapshot removed
TD07.3.3.7  KnownSpellIds durable; Spellbook mirror removed
TD07.3.3.8  StatusEffects durable; DefinitionAsset transient
TD07.3.3.9  LastAcknowledgedLevel replaces persisted UI queue
TD07.3.3.10 DerivedStats transient; SaveGame v20
```

## 3. Snapshots parallèles supprimés

```text
ClassProgressionStates
CharacterSkillStates
CharacterSpellbookStates
CharacterStatusEffectStates
PendingLevelUpNotifications
```

## 4. Save contract

```text
CurrentSaveVersion = 20
v20 accepted
v19 and earlier rejected
no backward migration
```

Load projection order:

```text
deserialize durable state
-> rebuild Level from Experience
-> rebuild DerivedStats from Attributes + ClassDefinition + Level
-> validate current schema
-> rehydrate transient definition caches
-> rebuild runtime progression projections
```

## 5. Validation

Final core filter:

```text
Grimrock.TechnicalDebt.TD07_3_3
Succeeded              : 71
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved/Automation/TD04/TD04-20260827-232129
```

Final cross-system campaign:

```text
TD07.3.3 core                          71
TD07.3.2 SaveContract                   6
MON15 progression                      34
MON16 Status Effects                   80
MON20 Skills                           47
MON18 Magic / Spellbook / Save         40
CharacterCreation CC5                   1
MON20 Recruitment                      35
-----------------------------------------
TOTAL                                 314

Warnings                                0
Failures                                0
Not run                                 0
```

## 6. Shipping

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Pak files     : 1
Archive files : 41
Archive bytes : 905473595
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260827-232723
[OK] Cook / package validated.
```

## 7. Stop condition

- [x] one durable authority per character gameplay datum;
- [x] derived state reconstructed;
- [x] mutable resources preserved;
- [x] ActiveCharacters and CharacterPool covered;
- [x] parallel Save snapshots removed;
- [x] exact-match v20;
- [x] core tests 71/71;
- [x] cross-system tests 314/314;
- [x] zero Automation warnings;
- [x] Shipping Win64 green.

## 8. Next

```text
TD07.3.4 — Authoring Identity Normalization
```

Scope remains the identity/presentation duplication deliberately deferred from TD07.3.3:

```text
ClassId + ClassDefinition + ClassDisplayName
RaceId + RaceDisplayName
PortraitVariantId + Portrait
ClassId + ClassIcon
```
