# MON20.2 — Active Party Recruitment Foundation

Statut : **VALIDÉ UE5.5.4 — CLOS**  
Date : **23 août 2026**  
Référence de départ : `77bcee01c5d4deda9b20786f93548a9381312746` (`Definir l audit et le contrat MON20.1`)

## 1. Objectif

Ajouter la première vraie transaction de recrutement sans créer de registre de groupe parallèle.

L’audit approfondi de MON20.1 a révélé que `FGridPartyInventoryState` possède déjà :

```cpp
TArray<FGridCharacterInventoryState> ActiveCharacters;
TArray<FGridCharacterEquipmentState> ActiveEquipment;
TArray<FGridCharacterInventoryState> CharacterPool;
```

`CharacterPool` est donc la réserve-like existante à réutiliser.

Le flux MON20.2 est :

```text
CharacterPool
    -> validation atomique
    -> ActiveCharacters
    -> ActiveEquipment aligné
    -> ownership validé
    -> notification après commit
```

## 2. Révision de scope après audit

Le contrat initial MON20.1 envisageait d’introduire immédiatement `ERPGPartyMemberKind` dans l’état sérialisé du personnage.

MON20.2 le diffère volontairement : ajouter maintenant un nouveau champ persistant ferait évoluer le contrat SaveGame avant même que les définitions de compagnons scénarisés soient figées.

L’audit de MON20.3 a confirmé qu’un compagnon scénarisé peut conserver une identité durable en réutilisant le `CharacterId` déjà persistant. `PartyMemberKind` est donc reporté à la tranche où réserve/persistance en auront réellement besoin, au lieu de forcer une migration SaveGame prématurée.

Cette décision conserve les premières tranches MON20 petites et réversibles.

## 3. Nouveau service

Fichiers :

```text
Source/GrimrockPrototype/Public/RPG/RPGPartyRecruitmentService.h
Source/GrimrockPrototype/Private/RPG/RPGPartyRecruitmentService.cpp
```

API :

```cpp
FRPGPartyRecruitmentService::TryRecruitFromPool(
    UGridPartyInventoryComponent* PartyInventoryComponent,
    const FGuid& CharacterId,
    FRPGPartyRecruitmentResult& OutResult);
```

Le service n’est pas un UObject et ne possède aucun état durable. L’autorité reste `UGridPartyInventoryComponent::PartyInventoryState`.

## 4. Validation avant mutation

La transaction rejette proprement :

- composant absent ;
- création initiale non terminée ;
- état actif incohérent ;
- `CharacterId` invalide ;
- identité déjà active ;
- groupe plein ;
- candidat absent ;
- plusieurs candidats portant le même `CharacterId` ;
- candidat incomplet ;
- hotbar structurellement invalide.

Aucune mutation n’a lieu avant ces contrôles.

## 5. Commit atomique

Pour un candidat valide :

1. copier le candidat depuis `CharacterPool` ;
2. initialiser les slots d’inventaire s’ils sont absents ;
3. initialiser une hotbar vide si elle est absente ;
4. normaliser l’ownership des objets d’inventaire vers le nouvel index actif ;
5. recalculer poids courant et capacité de port ;
6. retirer le candidat du pool ;
7. l’ajouter à `ActiveCharacters` ;
8. ajouter un `FGridCharacterEquipmentState` vide afin de conserver l’alignement ;
9. exécuter `ValidateInventoryOwnership()` ;
10. rollback complet si l’ownership échoue ;
11. émettre `NotifyPartyInventoryChanged(NewCharacterIndex)` uniquement après commit réussi.

MON20.2 ne crée pas d’équipement de départ : `CharacterPool` ne possède actuellement pas de tableau d’équipement associé. Ce point appartient au modèle de compagnon/recrue ultérieur.

## 6. Validation UE5.5.4

Validation utilisateur du 23 août 2026.

Suite exécutée :

```text
Grimrock.MON20.2.Recruitment
```

Résultats :

```text
DuplicateIdentityReject           Success
FullPartyAtomicReject             Success
InventoryOwnershipNormalization   Success
MissingCandidateAtomicReject      Success
OwnershipCollisionRollback        Success
ValidPoolRecruitment              Success
```

Bilan :

```text
6/6 Success
0 Fail
0 Error
```

Le test `OwnershipCollisionRollback` confirme notamment qu’une collision de `RuntimeObjectId` provoque bien un refus atomique sans laisser la recrue partiellement intégrée au groupe.

Le namespace de tests est explicitement nommé `GridMON202RecruitmentTests` afin d’éviter les collisions Unity Build rencontrées lors de MON19.8.

## 7. Hors scope

MON20.2 n’ajoute pas encore :

- `PartyMemberKind` persistant ;
- DataAsset de compagnon scénarisé ;
- widget Recruter / Refuser ;
- dialogues PNJ ;
- recrutement payant ;
- équipement de départ de la réserve ;
- renvoi d’un compagnon ;
- réorganisation active/réserve ;
- compétences ;
- nouveaux talents.

## 8. Conclusion

MON20.2 est **VALIDÉ ET CLOS**.

La fondation de recrutement réutilise bien l’autorité existante `CharacterPool -> ActiveCharacters` et conserve les invariants d’ownership et d’alignement du groupe.

La suite autoritaire est :

```text
MON20.3 — Story Companion Definition & Pool Registration
```

Cette tranche introduit :

- une définition data-driven du compagnon ;
- un `CharacterId` stable défini dans le DataAsset ;
- la construction d’un candidat complet dans `CharacterPool` ;
- l’enregistrement idempotent et la détection de collision ;
- l’intégration avec la transaction MON20.2.

L’écran de recrutement scénarisé est séparé en MON20.4.
