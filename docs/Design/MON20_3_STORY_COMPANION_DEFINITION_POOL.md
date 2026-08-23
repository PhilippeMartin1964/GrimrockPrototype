# MON20.3 — Story Companion Definition & Pool Registration

Statut : **implémenté — validation UE5.5.4 en attente**  
Date : **23 août 2026**  
Référence de départ : `6ebaaf7b3f939a11430c61d5885a881ac801659d` (`Valider MON20.2 recrutement actif`)

## 1. Objectif

Introduire le premier modèle data-driven de compagnon scénarisé sans créer de registre parallèle au groupe et sans mélanger immédiatement modèle, SaveGame v8 et UMG.

La tranche est volontairement plus petite que le cadrage initial :

```text
MON20.3 = Story Companion Definition + CharacterPool registration
MON20.4 = Recruitment UI / Recruter / Refuser
```

Le flux devient :

```text
URPGStoryCompanionAsset
    -> validation
    -> FGridCharacterInventoryState complet
    -> CharacterPool
    -> MON20.2 TryRecruitFromPool()
    -> ActiveCharacters
```

## 2. Identité persistante sans nouveau format SaveGame

MON20.3 n’ajoute pas encore `PartyMemberKind` à `FGridCharacterInventoryState` et ne change pas `CurrentSaveVersion = 7`.

Chaque compagnon possède dans son DataAsset :

```text
CompanionId   = identité lisible de contenu
CharacterId   = GUID stable de runtime/save
```

`CharacterId` est copié dans `FGridCharacterInventoryState`, qui est déjà persisté dans `PartyInventoryState`.

Conséquence : le compagnon reste identifiable après sauvegarde/restauration sans ajouter un second snapshot de compagnons.

Le bouton C++ `GenerateCharacterId()` génère le GUID uniquement s’il est encore invalide. Une fois un compagnon utilisé dans une sauvegarde de production, son `CharacterId` ne doit plus être changé.

`PartyMemberKind` reste une décision MON20 valide, mais son ajout est reporté à la tranche où réserve/persistance en auront réellement besoin afin d’éviter une migration v8 prématurée.

## 3. DataAsset

Fichiers :

```text
Source/GrimrockPrototype/Public/RPG/RPGStoryCompanionAsset.h
Source/GrimrockPrototype/Private/RPG/RPGStoryCompanionAsset.cpp
```

Classe :

```cpp
URPGStoryCompanionAsset : UPrimaryDataAsset
```

Données principales :

```text
CompanionId
CharacterId
DisplayName
ShortDescription
RaceDefinition
ClassDefinition
Level
PortraitGender
PortraitVariantId
Portrait
FullBody
ClassIcon
StartingEquipmentIds
RecruitmentConditionText
```

`StartingEquipmentIds` est déjà authorable mais n’est pas matérialisé en objets runtime dans MON20.3.

## 4. Validation du compagnon

`IsValidDefinition()` exige :

- `CompanionId` non vide ;
- `CharacterId` GUID valide ;
- nom de 1 à 24 caractères ;
- race valide ;
- classe valide ;
- niveau dans les bornes RPG existantes ;
- attributs classe + race dans les bornes existantes ;
- aucun `StartingEquipmentId` vide ou dupliqué.

Aucune règle RPG parallèle n’est introduite.

## 5. Service d’enregistrement

Fichiers :

```text
Source/GrimrockPrototype/Public/RPG/RPGStoryCompanionService.h
Source/GrimrockPrototype/Private/RPG/RPGStoryCompanionService.cpp
```

API :

```cpp
FRPGStoryCompanionService::EnsureCandidateRegistered(
    UGridPartyInventoryComponent* PartyInventoryComponent,
    const URPGStoryCompanionAsset* CompanionDefinition,
    FRPGStoryCompanionRegistrationResult& OutResult);
```

Le service est idempotent :

```text
absent          -> AddedToPool
présent pool    -> AlreadyInPool
présent actif   -> AlreadyActive
collision GUID  -> IdentityCollision
```

Aucune duplication n’est créée lors des appels répétés.

## 6. Construction du candidat

Le candidat utilise uniquement les contrats déjà existants :

```text
CharacterId      <- DataAsset.CharacterId
Race/Class       <- DataAssets existants
Level            <- DataAsset.Level
Experience       <- seuil cumulatif du niveau
Attributes       <- Class.BaseAttributes + Race.AttributeBonuses
DerivedStats     <- RPGCharacterRulesLibrary
InventorySlots   <- DefaultInventorySlotCountPerCharacter
CombatHotbar     <- 10 slots vides MON12
Portrait         <- DataAsset
ClassIcon        <- DataAsset
```

Le candidat est ajouté à `CharacterPool`. MON20.3 ne le recrute pas automatiquement : l’activation reste la transaction MON20.2.

## 7. Tests automatisés

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/RPGStoryCompanionMON203Tests.cpp
```

Suite :

```text
Grimrock.MON20.3.StoryCompanion.DefinitionValidation
Grimrock.MON20.3.StoryCompanion.RegisterCandidate
Grimrock.MON20.3.StoryCompanion.IdempotentRegistration
Grimrock.MON20.3.StoryCompanion.AlreadyActiveRecognition
Grimrock.MON20.3.StoryCompanion.IdentityCollisionReject
Grimrock.MON20.3.StoryCompanion.InvalidDefinitionAtomicReject
```

Attendu :

```text
6/6 Success
```

La suite couvre également l’intégration directe avec `FRPGPartyRecruitmentService::TryRecruitFromPool()` de MON20.2.

## 8. Hors scope

MON20.3 n’ajoute pas encore :

- WBP de recrutement ;
- boutons Recruter / Refuser ;
- dialogue PNJ ;
- équipement de départ matérialisé ;
- sorts de départ spécifiques ;
- coût en or ;
- renvoi ;
- réserve active/réorganisation ;
- `PartyMemberKind` sérialisé ;
- SaveGame v8.

## 9. Validation UE5.5.4 demandée

Compiler :

```text
Development Editor / Win64
```

Puis lancer :

```text
Grimrock.MON20.3.StoryCompanion
```

Attendu :

```text
6/6 Success
```

MON20.3 ne sera marqué validé qu’après retour réel UE5.5.4.

## 10. Suite

Après validation :

```text
MON20.4 — Story Companion Recruitment UI
```

Objectif : écran scénarisé `Recruter / Refuser / Voir la fiche`, branché sur le DataAsset MON20.3 et la transaction MON20.2, sans logique métier dans le Blueprint.
