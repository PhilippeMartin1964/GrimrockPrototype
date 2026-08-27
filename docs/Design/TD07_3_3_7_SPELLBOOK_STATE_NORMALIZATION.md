# TD07.3.3.7 — Spellbook State Normalization

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Characterization validée : `b5b017b745914742728be281de65af055cf6ebb9`  
Statut : **VALIDÉ ET CLOS**

## 1. Autorité durable unique

```text
FGridCharacterInventoryState::KnownSpellIds
    autorité durable unique
    canonical SpellId uniquement
    ordre déterministe
```

Le Spellbook voyage naturellement avec le personnage entre ActiveCharacters et CharacterPool.

## 2. Suppressions

```text
UGridPartySpellbookComponent::SpellbookState
FGridPartySpellbookState
FGridCharacterSpellbookSaveState
UGrimrockPartySaveGame::CharacterSpellbookStates
CapturePartySpellbooks()
RestorePartySpellbooks()
ValidateSavedPartySpellbooks()
```

Aucun runtime container indexé par CharacterId ni miroir Save séparé ne subsiste.

## 3. Façade conservée

`UGridPartySpellbookComponent` reste pour les mutations, lectures et notifications. Il résout le personnage dans `PartyInventoryState` et mute directement `KnownSpellIds`.

`FGridCharacterSpellbookState` reste une vue éphémère utilisée par les transactions et l'UI ; elle n'est stockée nulle part comme autorité.

## 4. Validation canonique

`FGridSpellbookPersistence` ne persiste plus rien. Il valide directement :

```text
CharacterId valides et uniques Active + Pool
SpellId non vide
aucun doublon
définition canonique de production existante et valide
ordre lexicographique déterministe
```

La tolérance MON18.8 `Spell_RemovedContent` est volontairement supprimée.

## 5. Hotbar

Le binding Spell reste une référence UI, jamais une connaissance. Une action Spell n'est disponible que si son SpellId figure dans `Character.KnownSpellIds`.

## 6. SaveGame v17

```text
CurrentSaveVersion = 17
v17 -> validation/load
v16 et antérieures -> rejet
aucune migration
```

Le Spellbook est sérialisé dans `PartyInventoryState`.

## 7. Tests dédiés

```text
Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.SchemaAuthority
Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.DirectMutation
Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.ActivePoolDurability
Grimrock.TechnicalDebt.TD07_3_3_7.Normalization.SaveSchemaVersion
```

## 8. Régressions requises

```text
Grimrock.Magic.MON18.2
Grimrock.Magic.MON18.3
Grimrock.Magic.MON18.7a
Grimrock.Magic.MON18.8
Grimrock.UI.UI01.4.3e.2
Grimrock.Save.MON18.9.1
Grimrock.TechnicalDebt.TD07_3_2
Grimrock.TechnicalDebt.TD07_3_3_6.Normalization
```

Puis Win64 Shipping.

## 8.1 Validation locale — build + Normalization

Validation du 27 août 2026 :

```text
Filter                 : Grimrock.TechnicalDebt.TD07_3_3_7.Normalization
Succeeded              : 4
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
Report                 : Saved/Automation/TD04/TD04-20260827-214921
```

Le build UE5.5.4 Development Editor est vert et le gate Normalization est validé.

## 8.2 Validation des régressions post-refactor

Validation locale du 27 août 2026 :

```text
TD07.3.3.7 Characterization     4/4
MON18.2 Spellbook               5/5
MON18.3 Cast transactions       6/6
MON18.7a Spellbook UI           7/7
MON18.8 Persistence            11/11
UI01.4.3e.2 Spell execution     6/6
MON18.9.1 Save policy           6/6
TD07.3.2 Save contract          6/6
TD07.3.3.6 Skills               4/4

Total                          55/55
Warnings                        0
Failures                        0
Not run                         0
```

Références de rapports :

```text
TD04-20260827-215107
TD04-20260827-215120
TD04-20260827-215133
TD04-20260827-215146
TD04-20260827-215158
TD04-20260827-215211
TD04-20260827-215224
TD04-20260827-215237
TD04-20260827-215250
```

Le bloc de régressions post-refactor est entièrement vert. Il ne reste que la validation Win64 Shipping.

## 8.3 Validation Win64 Shipping

Validation finale du 27 août 2026 :

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : Saved/Packaging/TD04/TD04-Shipping-20260827-215448/Windows/GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 905609387
Archive       : Saved/Packaging/TD04/TD04-Shipping-20260827-215448
[OK] Cook / package validated.
```

La stop condition TD07.3.3.7 est entièrement atteinte.

## 9. Stop condition

- [x] KnownSpellIds durable ajouté au personnage ;
- [x] composant runtime owner supprimé ;
- [x] FGridPartySpellbookState supprimé ;
- [x] snapshot Save Spellbook supprimé ;
- [x] capture/restore Spellbook supprimé ;
- [x] façade mutation/notification conservée ;
- [x] validation canonique directe conservée ;
- [x] unknown SpellId legacy rejeté ;
- [x] hotbar indépendante de la connaissance ;
- [x] SaveGame v17 exact-match ;
- [x] tests dédiés ajoutés ;
- [x] build UE5.5.4 vert ;
- [x] Normalization 4/4 ;
- [x] Characterization 4/4 post-refactor ;
- [x] régressions Magic/UI/Save vertes ;
- [x] Shipping Win64 vert.

Prochaine tranche après validation complète :

```text
TD07.3.3.8 — Normalize Status Effects / remaining character snapshots
```
