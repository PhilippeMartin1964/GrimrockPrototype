# MON20.9.4 — Skill Projection / Skills Page Restore Regression

Date : **24 août 2026**  
Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 À FAIRE**

## 1. Objectif

Prouver qu'un rang Skill restauré par la frontière de persistance MON20.9 redevient immédiatement consommable par les systèmes MON20.8, sans snapshot dérivé supplémentaire.

Chaîne validée :

```text
FRPGCharacterSkillSaveState
    -> FRPGSkillPersistence::RestorePartySkills()
    -> FGridCharacterInventoryState::SkillRanks
    -> FRPGSkillRequirementProjectionService
    -> RequirementIds
    -> FGridCombatActionCatalog
    -> FGridSkillsPageService
```

Les `RequirementIds` et le read model UI restent donc entièrement dérivés après chargement.

## 2. Pas de nouveau runtime

MON20.9.4 n'ajoute aucune abstraction de production.

Il réutilise directement :

```text
FRPGSkillPersistence
FRPGSkillService
FRPGSkillRequirementProjectionService
FGridCombatActionCatalog
FGridSkillsPageService
FRPGClassProgressionTransactionService
```

Aucun `RequirementId`, état d'action ou état de page n'est sauvegardé.

## 3. Invariants verrouillés

### 3.1 SkillId

Un rang restauré strictement positif doit immédiatement satisfaire son propre `SkillId`.

```text
Skill_Lockpicking rank 2 restored
    -> Skill_Lockpicking satisfied
```

### 3.2 Seuils de compétence

Les `FRPGSkillRequirementGrant` sont recalculés à partir du rang restauré :

```text
rank >= MinimumRank
    -> GrantedRequirementIds satisfied
```

Un rang inférieur au seuil ne doit jamais sur-déverrouiller une capacité.

### 3.3 Actions

Une action gated par un requirement Skill restauré doit devenir disponible via le catalogue existant. Une action dont le seuil n'est pas atteint reste verrouillée et conserve son diagnostic `MissingRequirements`.

### 3.4 Page Compétences

`FGridSkillsPageService` doit lire directement le rang restauré et afficher :

```text
Rank
MaxRank
bTrained
```

La variante personnage sélectionné doit continuer à suivre `SelectedCharacterIndex`.

### 3.5 Snapshot vide

Le snapshot Skill persistant est autoritaire.

Un restore avec `CharacterSkillStates=[]` supprime donc tout `SkillRanks` transient préexistant. Les consumers doivent ensuite voir :

```text
rank = 0
SkillId non satisfait
bTrained = false
```

### 3.6 Restore invalide

Un restore invalide reste atomique. Les consumers continuent de voir l'état runtime valide antérieur ; aucune projection ni vue partielle ne fuit du snapshot rejeté.

## 4. Tests Automation

Nouveau filtre :

```text
Grimrock.MON20.9.RestoredConsumers
```

Tests :

```text
RestoreProjectsSkillId
RestoreProjectsThresholdRequirement
RestoreUnlocksAction
RestoreBelowThresholdKeepsActionLocked
RestoreUpdatesSkillsPage
RestoreUpdatesSelectedCharacterPage
EmptySnapshotClearsTransientConsumers
InvalidRestorePreservesConsumers
```

## 5. Couverture détaillée

`RestoreProjectsSkillId`
- restaure un rang positif ;
- recalcule le `SkillId` satisfait ;
- préserve les requirements déjà présents.

`RestoreProjectsThresholdRequirement`
- restaure exactement le rang d'un seuil ;
- recalcule le `GrantedRequirementId` correspondant.

`RestoreUnlocksAction`
- restaure un rang atteignant un seuil ;
- projette le requirement ;
- déverrouille une action du catalogue.

`RestoreBelowThresholdKeepsActionLocked`
- restaure un rang positif mais inférieur au seuil ;
- conserve le `SkillId` de base ;
- n'accorde pas le requirement avancé ;
- l'action reste verrouillée avec diagnostic.

`RestoreUpdatesSkillsPage`
- restaure un rang ;
- la page affiche immédiatement le rang et `bTrained=true`.

`RestoreUpdatesSelectedCharacterPage`
- restaure le rang du deuxième personnage ;
- `TryBuildSelectedCharacterView()` suit toujours la sélection.

`EmptySnapshotClearsTransientConsumers`
- part d'un rang transient non sauvegardé ;
- restaure un snapshot vide ;
- supprime le rang et toutes ses projections dérivées.

`InvalidRestorePreservesConsumers`
- tente un rang supérieur à `MaxRank` ;
- le restore échoue atomiquement ;
- le rang, la projection et la page restent sur l'état valide précédent.

## 6. Critères de sortie

```text
rang restauré -> SkillId                  OK
rang restauré -> threshold RequirementId  OK
action gated restaurée                    débloquée
seuil non atteint                         reste verrouillé
page Compétences                          rang restauré visible
personnage sélectionné                    autorité conservée
snapshot vide                             consumers remis à zéro
restore invalide                          consumers antérieurs préservés
```

## 7. Validation demandée

Après compilation UE5.5.4 :

```text
Grimrock.MON20.9.RestoredConsumers
```

Attendu :

```text
8 / 8 Success
0 Fail
0 Error
```

Puis campagne cumulative :

```text
Grimrock.MON20.9
```

Attendu après MON20.9.4 :

```text
SkillPersistence        8/8
ActivePoolPersistence   8/8
RestoredConsumers       8/8
---------------------------
TOTAL                  24/24
```

Aucun PIE n'est requis pour cette tranche : le rendu UMG a déjà été validé en MON20.8.4 et MON20.9.4 ne modifie aucun asset.

## 8. Suite

Après validation :

```text
MON20.9.5 — Automation / PIE Regression & Closure
```
