# MON16.8 — Clôture / régression finale du milestone Status Effects

## Statut

**IMPLEMENTED — validation finale UE5.5.4 en attente.**

Base :

```text
b4ba41c10e5f38820bbd08ee0abb200dce6a6a92
Close MON16.7 status effect persistence
```

MON16.8 ne crée aucune nouvelle mécanique de status effect. Cette étape gèle le contrat du milestone MON16, vérifie sa cohérence transversale et ajoute une campagne de régression dédiée à l'architecture complète MON16.1–MON16.7.

## 1. Résultat de l'audit

La revue de clôture confirme une architecture unique et orientée données :

```text
UGridStatusEffectDefinitionAsset
        |
        v
FGridStatusEffectRuntimeState
        |
        v
FGridStatusEffectCollection
        |
        +--> UGridStatusEffectLifecycleSubsystem
        |       +--> durée / expiration / stacking
        |       +--> periodic damage
        |       +--> initiative refresh
        |       +--> feedback
        |
        +--> FGridStatusEffectInitiativeResolver
        +--> FGridStatusEffectControlResolver
        +--> FGridStatusEffectPresentationBuilder
        +--> FGridStatusEffectPersistence
```

Aucun second modèle de statut, second lifecycle, second système de persistance ou second registre de monstres n'a été identifié.

MON16.8 ne modifie donc pas le gameplay de production : il ajoute des tests contractuels et la documentation de gel du milestone.

## 2. Contrats gelés

### MON16.1 — modèle runtime

- identité stable `EffectId` ;
- PrimaryAssetId `GridStatusEffect:EffectId` ;
- collection runtime déterministe ;
- état séparé par cible ;
- pas de dépendance UI.

### MON16.2 — lifecycle

- événementiel via le TurnManager ;
- durée `Turns`, `Rounds`, `Permanent` ;
- stacking data-driven ;
- aucune horloge murale ;
- aucun timer ou Tick de statut.

### MON16.3 — dégâts périodiques

- profil de dégâts data-driven ;
- pipeline de dégâts existant réutilisé ;
- tick avant décrément / expiration ;
- aucune branche `Poison/Burning/...` par EffectId.

### MON16.4 — initiative

- contribution additive ;
- `InitiativeModifier * StackCount` ;
- saturation int32 ;
- reordering limité aux entrées futures `Waiting`.

### MON16.5 — contrôle

- `bSkipActivation` ;
- `bBlockSpellActions` ;
- `bBlockTranslation` ;
- Stun/Silence/Immobilize restent des configurations de données, jamais des classes parallèles.

### MON16.6 — présentation

- snapshot UI en lecture seule ;
- HUD alimenté depuis l'état runtime ;
- feedback Apply / Refresh / Tick / Expire ;
- un seul CombatLog autoritatif ;
- aucun WBP requis pour le contrat C++.

### MON16.7 — persistance

- snapshot stable sans pointeur UObject ;
- rebind de `DefinitionAsset` via PrimaryAssetId ;
- groupe actif + CharacterPool ;
- monstres via `FGridRuntimeMonsterState` ;
- restauration atomique ;
- SaveGame v5 ;
- migration v4 -> v5 conservant la progression MON15.

## 3. Automation MON16.8

Nouveau fichier :

```text
Source/GrimrockPrototype/Private/Tests/RPGMON168StatusEffectMilestoneTests.cpp
```

Namespace :

```text
Grimrock.RPG.MON16.8
```

Tests :

```text
PrimaryAssetIdentityContract
CrossFeatureComposition
PersistenceRoundTripSemantics
DeterministicPersistenceOrder
RuntimeSaveBoundary
SaveVersionContract
LifecycleArchitectureBoundary
NoHardCodedStatusIdentity
RegressionNamespaceCoverage
SingleCanonicalModel
```

Attendu : **10/10 Success**.

## 4. Ce que vérifient les tests de clôture

### Identité

`PrimaryAssetIdentityContract` fige :

```text
GridStatusEffect:EffectId
```

comme identité canonique.

### Composition

`CrossFeatureComposition` applique un même effet comportant simultanément :

- stacks ;
- potency ;
- initiative ;
- dégâts périodiques ;
- blocage de sorts ;
- blocage de translation.

Les résolveurs et la présentation doivent tous projeter le même état runtime.

### Round-trip sémantique

`PersistenceRoundTripSemantics` vérifie que capture puis restore conservent :

- EffectId ;
- SourceId ;
- StackCount ;
- DurationUnit ;
- RemainingDuration ;
- Potency ;
- initiative ;
- contrôles ;
- présentation ;
- periodic damage.

`DefinitionAsset` est réattaché au restore et n'est pas sérialisé.

### Déterminisme

`DeterministicPersistenceOrder` vérifie que runtime, capture et restore convergent vers l'ordre stable par EffectId.

### Frontière runtime / SaveGame

`RuntimeSaveBoundary` vérifie statiquement que :

- les collections de statut des personnages restent `Transient` ;
- les collections de statut des monstres restent `Transient` ;
- `DefinitionAsset` reste `Transient` ;
- le snapshot sauvegardé ne contient pas `DefinitionAsset` ;
- les champs persistants sont `SaveGame`.

### Lifecycle

`LifecycleArchitectureBoundary` interdit dans le lifecycle :

- `TickComponent` ;
- `SetTimer` / `FTimerManager` ;
- `FDateTime` ;
- `FPlatformTime` ;
- dépendance `UUserWidget` / `WBP_`.

Il vérifie les bindings événementiels au TurnManager.

### Data-driven

`NoHardCodedStatusIdentity` protège les sources de production MON16 contre des branches par identité textuelle de Poison, Bleeding, Burning, Haste, Slow, Stun, Silence ou Immobilize.

### Baseline de régression

`RegressionNamespaceCoverage` gèle le nombre de tests préexistants :

```text
MON16.1 :  7
MON16.2 : 10
MON16.3 : 11
MON16.4 : 11
MON16.5 : 11
MON16.6 : 10
MON16.7 : 11
----------------
Baseline : 71
```

Avec les 10 tests MON16.8 :

```text
Grimrock.RPG.MON16 : 81 tests attendus
```

## 5. Régression finale du milestone

Après compilation :

```text
Automation RunTests Grimrock.RPG.MON16.8
```

Puis :

```text
Automation RunTests Grimrock.RPG.MON16
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

Attendus :

```text
MON16 : 81/81
MON15 : 42/42
MON14 : 19/19
----------------
Total : 142/142
```

Ces nombres sont les attentes de la campagne, pas une validation anticipée.

## 6. Hors périmètre

MON16.8 n'ajoute ni :

- nouvel EffectId de production ;
- nouvelle règle de combat ;
- nouveau widget ;
- modification `.uasset` / `.umap` ;
- nouveau système de sauvegarde ;
- nouveau système de monstres ;
- refactor massif.

## 7. Condition de clôture

Le milestone MON16 sera marqué **VALIDÉ ET CLOS** uniquement après :

1. compilation UE5.5.4 confirmée ;
2. MON16.8 : 10/10 Success ;
3. MON16 : 81/81 Success ;
4. MON15 : 42/42 Success ;
5. MON14 : 19/19 Success ;
6. aucun Fail/Error résiduel attribuable au milestone.

Après cette validation, MON16 est gelé et les évolutions de gameplay devront appartenir au milestone suivant.
