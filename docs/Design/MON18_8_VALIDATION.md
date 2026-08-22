# MON18.8 — Validation UE5.5.4

Date : **22 août 2026**

Statut : **VALIDÉ ET CLOS**.

## Base validée

```text
cafdb3f8640497680fd75e507ef0dad618d7bc59
Implement MON18.8 spellbook persistence and migration

d25cf26e0d7c052fe30ba772e50e02b7debaa918
Fix MON16.5 status identity regression
```

## Automation ciblée MON18.8

Commande :

```text
Automation RunTests Grimrock.Magic.MON18.8
```

Résultat fourni depuis UE5.5.4 : **12/12 Success**.

Tests validés :

```text
AtomicRestoreFailure
DiskRoundTrip
DuplicateSpellRejected
HotbarSpellRoundTrip
HotbarWithoutKnowledgePreserved
InvalidSpellIdRejected
MultipleCharactersRoundTrip
OrphanCharacterRejected
SingleCharacterRoundTrip
SpellBindingIsNotItemDefinition
UnknownDefinitionPreserved
V5MigrationCreatesEmptySpellbook
```

## Régression MON16.5 détectée puis corrigée

La campagne globale a révélé un unique échec :

```text
Grimrock.RPG.MON16.5.NoParallelSystem
```

Cause : une comparaison directe de `EffectId` dans `GridTurnManagerPlayerActionCatalog.cpp` introduite dans le chemin d'exécution des sorts de statut.

Correction : utilisation de l'identité primaire canonique du Status Effect :

```text
Definition->GetPrimaryAssetId().PrimaryAssetName
```

Commit :

```text
d25cf26e0d7c052fe30ba772e50e02b7debaa918
Fix MON16.5 status identity regression
```

Validation fournie :

```text
Grimrock.RPG.MON16.5.NoParallelSystem     Success
Grimrock.RPG.MON16.5                     9/9 Success
```

## Régression UI / exécution Spellbook

Commande correcte :

```text
Automation RunTests Grimrock.UI.UI01.4.3e.2
```

Résultat fourni depuis UE5.5.4 : **6/6 Success**.

Tests validés :

```text
ArcaneBoltExecution
LesserHealExecution
MissingStatusNoCostCommit
SpellbookCatalogAvailability
SpellbookCatalogExecutorGate
UnknownSpellNoCostCommit
```

## Validation PIE réelle

### Première session

Un mage reçoit les quatre sorts de production via :

```text
Grimrock.Spellbook.SeedProduction
```

Résultat :

```text
Added=4
AlreadyKnown=0
TotalProduction=4
```

La sauvegarde v6 confirme ensuite :

```text
PartySave Saved Slot=GrimrockParty Version=6 Characters=1 Spellbooks=1
```

Le runtime magique reste fonctionnel dans la même session :

```text
Spell_ArcaneBolt
AP=2
Mana=3
Damage=4
```

et :

```text
Spell_CurePoison
AP=2
Mana=4
```

### Arrêt PIE puis Continue

Une nouvelle session PIE est lancée depuis `L_MainMenu`, puis la sauvegarde principale est chargée avec `Continue`.

La désérialisation accepte le snapshot v6 :

```text
Load SourceVersion=6 TargetVersion=6 ... SpellbookCharacters=1 Result=Accepted
```

Le runtime confirme :

```text
PartySave Continued Slot=GrimrockParty CharacterCount=1
```

Après chargement, le même seed est relancé :

```text
Grimrock.Spellbook.SeedProduction
```

Résultat déterminant :

```text
Added=0
AlreadyKnown=4
TotalProduction=4
```

Ce résultat prouve que les quatre `KnownSpellIds` ont été restaurés depuis le SaveGame ; ils ne proviennent pas d'un nouveau seed runtime.

## Hotbar

La persistance des bindings Spell est couverte par les tests automatisés :

```text
HotbarSpellRoundTrip
HotbarWithoutKnowledgePreserved
SpellBindingIsNotItemDefinition
```

MON18.8 ne fait pas de la hotbar une autorité de connaissance et ne transforme jamais `SourceDefinitionId` d'un binding Spell en `ItemDefinitionId`.

## Note sur les messages de validation au menu principal

Le log PIE montre également des messages :

```text
LoadValidation ... Result=Rejected Reason=Le snapshot contient 0 états de progression pour 1 personnages actifs.
```

Ils apparaissent pendant l'énumération des slots du menu principal, alors que la sauvegarde principale chargée ensuite est acceptée et continue correctement.

`UGrimrockGameInstance` sonde le slot principal ainsi que les slots configurés `GrimrockParty_2` et `GrimrockParty_3` afin de construire les informations de sauvegarde. Un ancien slot auxiliaire incompatible peut donc produire ce diagnostic sans invalider le slot principal.

Ce bruit de diagnostic ne bloque pas MON18.8. Il pourra être nettoyé dans la phase de régression/qualité si nécessaire.

## Conclusion

MON18.8 valide désormais :

- SaveGame version 6 ;
- migration explicite v5 -> v6 ;
- persistence sparse `CharacterId + KnownSpellIds` ;
- restauration atomique ;
- Spellbook runtime natif du Pawn ;
- identités de sorts inconnues conservées ;
- hotbar Spell non autoritaire ;
- protection SAVEFIX.1 ;
- comportement Continue réel ;
- round-trip PIE `Save -> arrêt PIE -> Continue` ;
- absence de régression MON16.5 après correction.

**MON18.8 est VALIDÉ ET CLOS sous UE5.5.4.**
