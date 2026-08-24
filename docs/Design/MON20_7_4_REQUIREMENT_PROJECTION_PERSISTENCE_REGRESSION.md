# MON20.7.4 — Requirement Projection / Persistence Regression

Statut : **VALIDÉ UE5.5.4 — 24/24 CUMULÉS**  
Date : **24 août 2026**  
Jalon parent : **MON20.7 — Talents / Progression Choice Integration**

---

## 1. Objectif

Vérifier que l’intégration Talent de MON20.7 ne casse ni la projection générique des `RequirementIds`, ni la persistance MON15.6 existante.

Cette tranche n’ajoute aucun nouveau runtime de production : elle verrouille par tests les contrats déjà existants.

---

## 2. Contrat de projection

Un talent reste un `FRPGClassProgressionChoiceDefinition`.

Après acquisition :

```text
ChoiceId
    -> devient RequirementId satisfait

GrantedRequirementIds[]
    -> deviennent RequirementIds satisfaits
```

La source autoritaire reste :

```text
FRPGClassProgressionService::CollectSatisfiedRequirements
FRPGClassProgressionTransactionService::AppendRuntimeSatisfiedRequirements
```

MON20.7.4 vérifie que :

- avant acquisition, les requirements du talent sont absents ;
- après acquisition, `ChoiceId` et `GrantedRequirementIds` sont immédiatement projetés ;
- les grants automatiques de niveau restent présents ;
- la projection reste isolée par `CharacterId`.

---

## 3. Contrat de persistance

Aucune donnée Talent séparée n’est introduite.

La persistance reste :

```text
UGrimrockPartySaveGame
    -> ClassProgressionStates[]
        -> FRPGCharacterProgressionSaveState
            -> CharacterId
            -> SelectedChoiceIds[]
```

Les API utilisées restent :

```text
FRPGClassProgressionTransactionService::CapturePersistentState
FRPGClassProgressionTransactionService::RestorePersistentState
```

Le terme Talent ne change donc ni le format, ni la version SaveGame.

`UGrimrockPartySaveGame::CurrentSaveVersion` reste :

```text
7
```

---

## 4. Restore et projection détachée

MON15.6 autorise un état restauré sans `UGridPartyInventoryComponent` vivant immédiatement attaché.

Après `RestorePersistentState()` :

```text
RuntimeState restauré
    -> CharacterId connu
    -> SelectedChoiceIds connus
    -> SatisfiedRequirements reconstruits
    -> InventoryComponent temporairement null
```

`AppendRuntimeSatisfiedRequirements(CharacterId, ...)` doit déjà fonctionner à ce stade.

Lors de la première lecture live, `RefreshCharacterProjection()` rattache ensuite le cache au composant réel sans perdre les choix restaurés.

---

## 5. Atomicité restore

`RestorePersistentState()` reconstruit d’abord tous les états dans une map candidate.

Le commit vers le cache runtime n’a lieu qu’après validation de tous les personnages.

Donc :

```text
snapshot invalide
    -> false
    -> aucun remplacement partiel du cache existant
```

MON20.7.4 teste explicitement ce comportement avec un talent dont le prérequis manque.

---

## 6. CharacterId comme clé persistante

L’ordre du tableau `ClassProgressionStates` n’est pas une autorité.

Le restore fait la correspondance par :

```text
CharacterId
```

MON20.7.4 inverse volontairement l’ordre de deux snapshots avant restore et vérifie que chaque personnage récupère ses propres talents.

---

## 7. Automation

Huit tests MON20.7.4 sont ajoutés au filtre cumulatif :

```text
Grimrock.MON20.7.Talents
```

Tests :

```text
RequirementBeforeTalent
RequirementAfterTalent
RequirementCharacterIsolation
CaptureUsesMON15Snapshot
RestoreTalentReadModel
RestoreDetachedRequirements
InvalidRestoreAtomic
RestoreByCharacterId
```

Validation utilisateur sous UE5.5.4 :

```text
24 / 24 Success
0 Fail
0 Error
```

Les 16 tests MON20.7.2 + MON20.7.3 sont restés verts.

---

## 8. Fichiers

Ajout :

```text
Source/GrimrockPrototype/Private/Tests/RPGMON2074TalentPersistenceRegressionTests.cpp
```

Documentation :

```text
docs/Design/MON20_7_4_REQUIREMENT_PROJECTION_PERSISTENCE_REGRESSION.md
```

Aucun fichier de production n’est modifié pour cette tranche.

Aucun `.uasset`, `.umap` ou changement SaveGame.

---

## 9. Critères de sortie

```text
[OK] projection automatique avant talent
[OK] ChoiceId projeté après acquisition
[OK] GrantedRequirementIds projetés après acquisition
[OK] isolation par CharacterId
[OK] capture MON15.6 conserve SelectedChoiceIds
[OK] SaveVersion reste 7
[OK] restore rend le talent au read model MON20.7
[OK] restore détaché projette déjà les requirements
[OK] restore invalide est atomique
[OK] ordre des snapshots sans importance
[OK] 24/24 Automation cumulés
```

---

## 10. Conclusion

```text
MON20.7.4 — VALIDÉ UE5.5.4 — CLOS
```

La passe cumulative 24/24 sert également de régression finale pour :

```text
MON20.7.5 — Automation Regression / Closure
```
