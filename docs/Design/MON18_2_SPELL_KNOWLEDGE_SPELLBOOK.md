# MON18.2 — Spell Knowledge / Spellbook

Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
Date : **21 août 2026**

## Objectif

Introduire une notion générique de sorts connus par personnage et un propriétaire runtime de Spellbook, sans exécuter de sort, sans payer de mana/PA et sans anticiper la persistance MON18.8.

## Décisions d'architecture

### 1. Identité stable uniquement

Le Spellbook stocke des `SpellId` (`FName`) définis par MON18.1. Il ne stocke ni `UGridSpellDefinitionAsset*`, ni copie de `FGridSpellDefinition`, ni pointeur d'acteur.

La résolution `SpellId -> UGridSpellDefinitionAsset` sera réalisée par les couches qui en ont besoin.

### 2. Un Spellbook par `CharacterId`

`FGridCharacterSpellbookState` associe :

```text
CharacterId
KnownSpellIds[]
```

Deux personnages ne partagent jamais implicitement leur connaissance magique.

### 3. Propriétaire runtime dédié

`UGridPartySpellbookComponent` possède `FGridPartySpellbookState` et expose :

```text
EnsureCharacterSpellbook
RemoveCharacterSpellbook
LearnSpell
ForgetSpell
KnowsSpell
GetKnownSpellIds
ResetAllSpellbooks
ValidateSpellbookState
```

L'enregistrement d'un personnage est explicite. `LearnSpell` ne crée jamais silencieusement un Spellbook pour un `CharacterId` inconnu.

### 4. Pas de duplication

Les doublons de `SpellId` sont refusés avec `AlreadyKnown`. `NAME_None` est refusé. Un même `CharacterId` ne peut avoir qu'un seul Spellbook valide.

### 5. Aucune ressource gameplay

MON18.2 ne lit et ne modifie :

- ni mana ;
- ni PA ;
- ni cooldown ;
- ni ciblage ;
- ni dégâts ;
- ni Status Effects ;
- ni inventaire ;
- ni hotbar.

Ces responsabilités restent dans leurs systèmes existants.

### 6. Persistance explicitement différée

`UGridPartySpellbookComponent::SpellbookState` porte `Transient`.

C'est volontaire : MON18.2 construit le modèle de connaissance runtime, tandis que MON18.8 ajoutera le snapshot, la migration de SaveGame et la restauration. Cela évite de modifier silencieusement le format de sauvegarde v5 pendant MON18.2.

## Fichiers

```text
Source/GrimrockPrototype/Public/Magic/GridSpellbookTypes.h
Source/GrimrockPrototype/Public/Magic/GridPartySpellbookComponent.h
Source/GrimrockPrototype/Private/Magic/GridPartySpellbookComponent.cpp
Source/GrimrockPrototype/Private/Tests/GridMagicMON182SpellbookTests.cpp
docs/Design/MON18_2_SPELL_KNOWLEDGE_SPELLBOOK.md
```

## Tests Automation

```text
Grimrock.Magic.MON18.2.CharacterRegistration
Grimrock.Magic.MON18.2.LearnForget
Grimrock.Magic.MON18.2.CharacterIsolation
Grimrock.Magic.MON18.2.StableIdentity
Grimrock.Magic.MON18.2.TransientContract
```

Attendu : **5/5 Success**.

## Hors périmètre

MON18.2 ne raccorde pas encore ce composant au `AGrimrockPartyPawn` ni au catalogue d'actions. Ce branchement appartient à MON18.3, qui doit vérifier qu'un personnage connaît réellement `SpellId` avant d'autoriser le cast et d'engager la transaction PA/mana.

Aucun Blueprint, DataAsset, WBP ou `.uasset` n'est requis pour cette étape.
