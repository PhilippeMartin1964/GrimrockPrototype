# MON20.3 — Story Companion Definition & Pool Registration

Statut : **VALIDÉ UE5.5.4 — CLOS**  
Date : **23 août 2026**  
Référence de code après nettoyage d’historique : `0b8bab8f86f3a7f9df4979f1df4259838a93023d` (`Ajouter MON20.3 compagnon scénarisé data-driven`)

## 1. Objectif

Introduire le premier modèle data-driven de compagnon scénarisé sans registre parallèle au groupe et sans migration SaveGame prématurée.

```text
MON20.3 = Story Companion Definition + CharacterPool registration
MON20.4 = Recruitment UI / Recruter / Refuser
```

## 2. Flux

```text
URPGStoryCompanionAsset
    -> validation
    -> FGridCharacterInventoryState complet
    -> CharacterPool
    -> MON20.2 TryRecruitFromPool()
    -> ActiveCharacters
```

## 3. Identité persistante

Le SaveGame reste en version 7. Chaque compagnon possède `CompanionId` (identité lisible de contenu) et `CharacterId` (GUID stable). `CharacterId` est copié dans `FGridCharacterInventoryState`, déjà persisté dans `PartyInventoryState`.

`PartyMemberKind` reste différé jusqu’à un besoin réel de réserve/migration.

## 4. DataAsset

`URPGStoryCompanionAsset : UPrimaryDataAsset` porte notamment :

```text
CompanionId
CharacterId
DisplayName / ShortDescription
RaceDefinition / ClassDefinition / Level
PortraitGender / PortraitVariantId / Portrait / FullBody / ClassIcon
StartingEquipmentIds
RecruitmentConditionText
```

`StartingEquipmentIds` est authorable mais n’est pas encore matérialisé en équipement de `CharacterPool`.

## 5. Service

`FRPGStoryCompanionService::EnsureCandidateRegistered(...)` est idempotent :

```text
absent          -> AddedToPool
présent pool    -> AlreadyInPool
présent actif   -> AlreadyActive
collision GUID  -> IdentityCollision
```

Le candidat réutilise Race/Class, règles d’attributs, niveau/XP, derived stats, slots d’inventaire et hotbar existants.

## 6. Validation UE5.5.4

Suite exécutée :

```text
Grimrock.MON20.3.StoryCompanion
```

Résultats utilisateur :

```text
AlreadyActiveRecognition       Success
DefinitionValidation           Success
IdempotentRegistration         Success
IdentityCollisionReject        Success
InvalidDefinitionAtomicReject  Success
RegisterCandidate              Success

6/6 Success
0 Fail
0 Error
```

MON20.3 est donc **VALIDÉ ET CLOS**.

## 7. Hors scope confirmé

- Recruitment UI ;
- dialogue PNJ ;
- équipement de départ matérialisé ;
- coût, renvoi et réorganisation actif/réserve ;
- `PartyMemberKind` persistant ;
- SaveGame v8.

## 8. Suite

```text
MON20.4 — Story Companion Recruitment UI
```

Ce jalon reste en attente pendant la passe de documentation `docs/Architecture`.
