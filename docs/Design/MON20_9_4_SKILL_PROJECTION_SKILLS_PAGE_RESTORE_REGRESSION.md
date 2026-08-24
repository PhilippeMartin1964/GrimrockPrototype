# MON20.9.4 — Skill Projection / Skills Page Restore Regression

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — 8/8 SUCCESS — 24/24 CUMULÉS MON20.9**

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

Un rang restauré strictement positif satisfait immédiatement son propre `SkillId`.

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

Un rang inférieur au seuil ne sur-déverrouille jamais une capacité.

### 3.3 Actions

Une action gated par un requirement Skill restauré devient disponible via le catalogue existant. Une action dont le seuil n'est pas atteint reste verrouillée et conserve son diagnostic `MissingRequirements`.

### 3.4 Page Compétences

`FGridSkillsPageService` lit directement le rang restauré et affiche :

```text
Rank
MaxRank
bTrained
```

La variante personnage sélectionné continue à suivre `SelectedCharacterIndex`.

### 3.5 Snapshot vide

Le snapshot Skill persistant est autoritaire.

Un restore avec `CharacterSkillStates=[]` supprime donc tout `SkillRanks` transient préexistant. Les consumers voient ensuite :

```text
rank = 0
SkillId non satisfait
bTrained = false
```

### 3.6 Restore invalide

Un restore invalide reste atomique. Les consumers continuent de voir l'état runtime valide antérieur ; aucune projection ni vue partielle ne fuit du snapshot rejeté.

## 4. Tests Automation

Filtre :

```text
Grimrock.MON20.9.RestoredConsumers
```

Tests :

```text
EmptySnapshotClearsTransientConsumers
InvalidRestorePreservesConsumers
RestoreBelowThresholdKeepsActionLocked
RestoreProjectsSkillId
RestoreProjectsThresholdRequirement
RestoreUnlocksAction
RestoreUpdatesSelectedCharacterPage
RestoreUpdatesSkillsPage
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

## 6. Validation UE5.5.4 fournie

Campagne exécutée le **24 août 2026**.

Résultat ciblé :

```text
Grimrock.MON20.9.RestoredConsumers

8 / 8 Success
0 Fail
0 Error
```

Résultat cumulatif :

```text
Grimrock.MON20.9

SkillPersistence        8/8 Success
ActivePoolPersistence   8/8 Success
RestoredConsumers       8/8 Success
---------------------------
TOTAL                   24/24 Success
0 Fail
0 Error
```

Cette validation confirme simultanément :

```text
rang restauré -> SkillId                  OK
rang restauré -> threshold RequirementId  OK
action gated restaurée                    débloquée
seuil non atteint                         reste verrouillé
MissingRequirements                       cohérent
page Compétences                          rang restauré visible
personnage sélectionné                    autorité conservée
snapshot vide                             consumers remis à zéro
restore invalide                          consumers antérieurs préservés
```

Aucun PIE n'était requis pour MON20.9.4 : aucun asset ni comportement visuel n'a été modifié dans cette tranche.

## 7. Conclusion

MON20.9.4 est **VALIDÉ UE5.5.4 — 8/8**.

La campagne cumulative MON20.9 est désormais :

```text
24 / 24 Success
```

Aucune nouvelle abstraction parallèle n'a été introduite et les `RequirementIds` restent strictement dérivés.

## 8. Suite

```text
MON20.9.5 — Automation / PIE Regression & Closure
```

MON20.9.5 doit consolider la campagne Automation déjà verte et effectuer le dernier smoke test PIE de la frontière Save/Continue avant fermeture de MON20.9.
