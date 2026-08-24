# MON20.5.2 — Context + Custom Recruit Transaction

Statut : **VALIDÉ UE5.5.4 — CLOS**  
Date : **24 août 2026**  
Jalon parent : **MON20.5 — Custom Recruit / Wizard Context Reuse**

---

## 1. Objectif

Installer le socle runtime d’une recrue personnalisée avant d’adapter le wizard UMG.

MON20.5.2 n’ouvre aucun écran et ne modifie aucun Blueprint. Il ajoute :

```text
ERPGCharacterCreationContext
FRPGCustomRecruitService
Automation Tests
```

Le flux autoritaire devient :

```text
FRPGCharacterCreationRequest
    -> validation CustomRecruit
    -> candidat temporaire CharacterPool
    -> FRPGPartyRecruitmentService::TryRecruitFromPool
    -> ActiveCharacters
```

---

## 2. Contexte de création

`RPGCharacterTypes.h` expose désormais :

```cpp
ERPGCharacterCreationContext::NewGameMainHero
ERPGCharacterCreationContext::CustomRecruit
```

Le contexte est volontairement transitoire. Aucun champ n’est ajouté à `FGridCharacterInventoryState` et aucune migration SaveGame n’est nécessaire.

---

## 3. Nouveau service

Fichiers :

```text
Source/GrimrockPrototype/Public/RPG/RPGCustomRecruitService.h
Source/GrimrockPrototype/Private/RPG/RPGCustomRecruitService.cpp
```

API :

```cpp
FRPGCustomRecruitService::TryCreateAndRecruit(
    UGridPartyInventoryComponent* PartyInventoryComponent,
    const FRPGCharacterCreationRequest& Request,
    FRPGCustomRecruitResult& OutResult);
```

Le service est sans état durable.

---

## 4. Validations avant mutation

La transaction vérifie avant création du candidat :

- composant d’inventaire présent ;
- création initiale terminée ;
- au moins un héros actif ;
- alignement `ActiveCharacters / ActiveEquipment` ;
- sélection active valide ;
- limite du groupe cohérente ;
- place disponible dans le groupe actif ;
- nom de 1 à 24 caractères après trim ;
- race valide ;
- classe de création valide ;
- classe canonique d’actions valide et de même `ClassId` ;
- attributs finaux compris dans les bornes RPG existantes.

Un groupe plein est rejeté avant toute création de candidat : MON20.5 ne transforme pas encore `CharacterPool` en réserve visible pour les mercenaires personnalisés.

---

## 5. Construction du candidat

Le candidat reçoit :

```text
CharacterId         nouveau GUID unique actif + pool
DisplayName         nom normalisé
Race                définition choisie
Class               définition choisie
ClassDefinition     asset canonique des actions
Level               1
Experience          0
Attributes          allocation du wizard + bonus de race
DerivedStats        calcul existant
PortraitGender      choix du wizard
PortraitVariantId   choix du wizard
Portrait            choix du wizard
ClassIcon           classe choisie
InventorySlots      taille standard du composant
CombatHotbarSlots   10 slots structurellement valides et vides
```

La classe transitoire utilisée par le wizard pour porter l’allocation des attributs n’est pas conservée dans l’état persistant. `ClassDefinition` pointe sur la source canonique fournie par `CombatActionSourceClassDefinition`.

---

## 6. Réutilisation de MON20.2

Le service CustomRecruit ne réimplémente pas l’activation du groupe.

Après construction :

```text
State.CharacterPool.Add(Candidate)
    -> FRPGPartyRecruitmentService::TryRecruitFromPool(CharacterId)
```

MON20.2 reste donc responsable :

- du déplacement pool -> actif ;
- de l’alignement de l’équipement ;
- de la normalisation d’ownership ;
- de la validation finale ;
- de la notification après commit.

---

## 7. Rollback extérieur

L’état complet du groupe est capturé avant le staging temporaire.

Si MON20.2 rejette l’activation :

```text
State = PreviousState
```

Le candidat CustomRecruit temporaire disparaît donc aussi du `CharacterPool`.

Cette règle distingue volontairement la recrue personnalisée du Story Companion MON20.3 : un compagnon scénarisé peut rester connu dans le pool, tandis qu’un mercenaire qui n’a pas été engagé ne doit pas devenir une réserve cachée.

---

## 8. Hors scope

MON20.5.2 n’ajoute pas :

- modification du wizard ;
- modal runtime ;
- recruteur dans le donjon ;
- coût en or ;
- équipement de départ ;
- niveau dynamique ;
- `PartyMemberKind` persistant ;
- migration SaveGame ;
- réserve UI.

---

## 9. Automation Tests

Filtre :

```text
Grimrock.MON20.5.CustomRecruit
```

Tests ajoutés :

```text
AllocatedAttributesPreserved
ContextContract
InitialHeroStatePreserved
InvalidRequestAtomicReject
PartyFullAtomicReject
RecruitmentRollbackLeavesNoPoolCandidate
UniqueCharacterIdentity
ValidCreateAndRecruit
VisualSelectionPreserved
```

Validation utilisateur UE5.5.4 du 24 août 2026 :

```text
AllocatedAttributesPreserved                 Success
ContextContract                              Success
InitialHeroStatePreserved                    Success
InvalidRequestAtomicReject                   Success
PartyFullAtomicReject                        Success
RecruitmentRollbackLeavesNoPoolCandidate     Success
UniqueCharacterIdentity                      Success
ValidCreateAndRecruit                        Success
VisualSelectionPreserved                     Success
```

Bilan :

```text
9 / 9 Success
0 Fail
0 Error
```

MON20.5.2 est donc validé et clos.

---

## 10. Suite

```text
MON20.5.3 — Wizard Context Reuse
```

Cette tranche adapte `URPGCharacterCreationWizardWidget` au contexte `CustomRecruit` sans créer de second WBP.
