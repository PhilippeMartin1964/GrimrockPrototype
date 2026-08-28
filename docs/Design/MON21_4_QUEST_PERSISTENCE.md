# MON21.4 — Quest Persistence

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Statut : CHARACTERIZATION VALIDÉE — SUSPENDU JUSQU'À CLÔTURE TD07

## 1. Reprise après TD07.3

TD07.3 — Prototype Data Model Reset est validé et clos.

MON21.4 reprend sur le contrat prototype courant :

- SaveGame exact-match ;
- aucune migration arrière ;
- identités stables comme autorité persistante ;
- données dérivées/transient reconstruites ;
- aucune compatibilité legacy.

## 2. Autorité existante

```text
UGridQuestSubsystem : UGameInstanceSubsystem
    CampaignState : FGridCampaignQuestRuntimeState
    QuestDefinitionsById : transient registry
```

L'état runtime stocke seulement :

```text
QuestId
Quest Status
ObjectiveId
Objective Status
```

Aucun pointeur `UGridQuestDefinitionAsset*` n'est une autorité runtime.

## 3. Contrat de persistance cible

Le snapshot SaveGame devra contenir une copie durable de :

```text
FGridCampaignQuestRuntimeState
```

avec :

- snapshot par `QuestId` / `ObjectiveId` ;
- restauration atomique ;
- validation complète avant mutation du subsystem ;
- QuestId inconnu -> rejet ;
- ObjectiveId inconnu / ordre incompatible -> rejet ;
- aucun pointeur de définition sauvegardé ;
- Event -> Command inchangé.

Toute modification du SaveGame ouvre une **nouvelle génération exact-match** après v22. Aucune migration v22 -> nouvelle version ne sera ajoutée.

## 4. Risque cross-level identifié

Aujourd'hui, les définitions sont enregistrées par :

```text
UGridActivationComponent::RegisterCurrentLevelQuestDefinitions()
    -> RuntimeActor->LevelAsset->QuestDefinitions
```

Ce registre est donc seulement enrichi par les niveaux chargés pendant la session.

Une sauvegarde peut contenir une quête commencée dans un autre niveau. Lors d'un chargement frais, la validation stricte échouerait si cette définition n'est pas enregistrée avant restauration.

### Contrat MON21.4

Avant de restaurer le snapshot Quest, le pipeline de load devra enregistrer les définitions Quest de **tous les niveaux activés du UGridDungeonAsset**.

Les collisions de `QuestId` restent rejetées par `UGridQuestSubsystem::RegisterQuestDefinition`.

## 5. Caractérisation

Filtre :

```text
Grimrock.Quests.MON21_4.Characterization
```

Sous-tests :

```text
SaveEnvelopeGap
RuntimeAuthority
SnapshotValidationContract
SaveLoadPipelineGap
```

Baseline attendue :

- SaveGame v22 ;
- aucun `CampaignQuestState` persistant ;
- `CampaignState` transient ;
- validator runtime déjà capable de rejeter QuestId/ObjectId incompatibles ;
- Pawn Save/Load ne capture/restaure pas encore les quêtes ;
- registre des définitions actuellement level-scoped.

## 6. Découpage proposé

```text
MON21.4.1  Characterization                         <- présent
MON21.4.2  Save schema + subsystem snapshot API
MON21.4.3  Dungeon-wide definition registration
MON21.4.4  Pawn Save/Load atomic integration
MON21.4.5  Round-trip / invalid snapshot regressions / Shipping
```

Aucun changement .uasset/.umap n'est requis pour MON21.4.1.


## 7. Characterization validée puis suspension

Validation locale du 28 août 2026 :

```text
Grimrock.Quests.MON21_4.Characterization
Succeeded              : 4
Failed                 : 0
```

La caractérisation est conservée comme baseline prête pour la reprise future.

**Aucune implémentation MON21.4.2+ ne doit commencer avant la clôture complète de TD07.**

La campagne technique reprend donc dans cet ordre :

```text
TD07.4  ActivationComponent characterization
TD07.5  Suspended test infrastructure / branch recovery
TD07.6  Legacy asset/API cleanup audit          ABSORBÉ PAR TD07.3
TD07.7  Targeted log / formatting hygiene
TD07.8  Future-proofing re-audit / stop condition
```
