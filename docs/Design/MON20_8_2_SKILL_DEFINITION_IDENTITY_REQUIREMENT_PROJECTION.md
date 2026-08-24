# MON20.8.2 — Skill Definition Identity & Requirement Projection

Statut : **VALIDÉ UE5.5.4 — 8/8 AUTOMATION SUCCESS**  
Date : **24 août 2026**  
Jalon parent : **MON20.8 — Cross-System Requirements / Actions / UI**

---

## 1. Objectif

Donner aux Skills MON20.6 une identité PrimaryAsset canonique et projeter leurs rangs numériques vers le vocabulaire binaire `RequirementId` déjà consommé par MON12/MON15/MON20.7.

Cette tranche ne branche pas encore ces requirements dans le TurnManager : cette intégration appartient à MON20.8.3.

---

## 2. Identité canonique

`URPGSkillAsset` conserve `SkillId` comme identité métier et surcharge désormais :

```text
GetPrimaryAssetId()
    -> Type = RPGSkill
    -> Name = SkillId
```

Le nom physique du `.uasset` n'est donc pas l'autorité.

Le service `FRPGSkillRequirementProjectionService::ResolveDefinitionBySkillId()` résout :

```text
SkillId
    -> FPrimaryAssetId(RPGSkill, SkillId)
    -> UAssetManager
    -> URPGSkillAsset canonique
```

En l'absence d'un scan préalable, le resolver peut scanner `/Game` pour le type `RPGSkill`, sur le même principe que le resolver des Status Effects.

---

## 3. Grants de requirements par rang

`URPGSkillAsset` expose maintenant :

```text
RequirementGrants[]
    FRPGSkillRequirementGrant
        MinimumRank
        GrantedRequirementIds[]
```

Validation :

```text
MinimumRank >= 1
MinimumRank <= MaxRank
GrantedRequirementIds non vide
aucun RequirementId = None
aucun doublon dans un même grant
```

Un asset sans `RequirementGrants` reste parfaitement valide pour préserver les Skills MON20.6 existants.

---

## 4. Règles de projection

Pour chaque entrée sparse `FRPGSkillRank` positive :

```text
SkillRank.Rank > 0
    -> SkillId satisfait automatiquement

pour chaque RequirementGrant
    si SkillRank.Rank >= MinimumRank
        -> GrantedRequirementIds satisfaits
```

Exemple :

```text
SkillId = Skill_Lockpicking
Rank = 3

Grant rank 2 -> Req_Lockpicking_Advanced
Grant rank 4 -> Req_Lockpicking_Master

Résultat :
Skill_Lockpicking
Req_Lockpicking_Advanced
```

`Req_Lockpicking_Master` n'est pas encore satisfait.

---

## 5. Service de projection

Nouveau service sans état :

```text
FRPGSkillRequirementProjectionService
```

API :

```text
AppendSatisfiedRequirements(CharacterState, OutRequirements, OutError)
AppendSatisfiedRequirements(CharacterState, DefinitionResolver, OutRequirements, OutError)
ResolveDefinitionBySkillId(SkillId)
```

Le second overload permet des tests purs avec resolver injecté, sans dépendre de l'AssetManager.

### Atomicité

La projection construit une copie candidate du `TSet` fourni.

Si un Skill runtime :

- ne possède pas de définition canonique ;
- résout une définition invalide ;
- résout une définition avec un autre `SkillId` ;
- possède un rang supérieur au `MaxRank` canonique ;
- appartient à un état sparse structurellement invalide ;

alors :

```text
false
OutError renseigné
OutRequirements inchangé
```

Cela garantit que les requirements déjà produits par classe, équipement ou talents ne sont jamais partiellement contaminés par une projection Skill invalide.

---

## 6. Séparation Skill Requirement / Skill Check

MON20.8.2 ne modifie pas `FRPGSkillCheckService`.

Les deux concepts restent distincts :

```text
RequirementId
    -> capacité binaire / autorisation

Skill Check
    -> d20 + SkillRank + AttributeModifier >= Difficulty
```

Un rang de Crochetage peut donc autoriser l'action `Crocheter`, tandis que le jet détermine ensuite si la tentative réussit.

---

## 7. Persistance

Aucun changement SaveGame.

```text
CurrentSaveVersion = 7
SkillRanks restent Transient jusqu'à MON20.9
RequirementIds dérivés ne sont jamais persistés
```

Après la future restauration des `SkillRanks`, les requirements seront simplement recalculés.

---

## 8. Fichiers

```text
Source/GrimrockPrototype/Public/RPG/RPGSkillAsset.h
Source/GrimrockPrototype/Private/RPG/RPGSkillAsset.cpp
Source/GrimrockPrototype/Public/RPG/RPGSkillRequirementProjectionService.h
Source/GrimrockPrototype/Private/RPG/RPGSkillRequirementProjectionService.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON2082SkillRequirementProjectionTests.cpp
```

Aucun `.uasset` / `.umap` n'est requis pour cette tranche.

---

## 9. Automation

Filtre :

```text
Grimrock.MON20.8.SkillRequirements
```

Résultat validé sous UE5.5.4 le 24 août 2026 :

```text
PrimaryAssetIdentity          Success
RequirementGrantValidation   Success
UntrainedNoRequirements      Success
TrainedSkillId               Success
ThresholdRequirements        Success
MultipleSkillsProjection     Success
InvalidRankAtomic            Success
MissingDefinitionAtomic      Success

8 / 8 Success
0 Fail
0 Error
```

---

## 10. Critères de sortie

```text
[OK] GrimrockPrototypeEditor compile sous UE5.5.4
[OK] 8/8 Automation Success
[OK] identité RPGSkill:<SkillId> confirmée
[OK] SkillId projeté pour tout rang positif
[OK] seuils atteints projetés
[OK] seuils non atteints absents
[OK] plusieurs Skills combinés sans collision
[OK] définition absente -> rejet atomique
[OK] rang > MaxRank canonique -> rejet atomique
[OK] aucune migration SaveGame
```

MON20.8.2 est **VALIDÉ**.

Prochain travail : **MON20.8.3 — Combat Action Requirement Integration & Diagnostics**.
