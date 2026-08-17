# MON16.8 — Clôture / régression finale du milestone Status Effects

## Statut

**VALIDÉ ET CLOS — 17 août 2026.**

Implémentation MON16.8 :

```text
0244d1dc41d99160d81d5d700ec38408b5c88b0d
Add MON16.8 status effect milestone regression
```

Base MON16.7 :

```text
b4ba41c10e5f38820bbd08ee0abb200dce6a6a92
Close MON16.7 status effect persistence
```

MON16.8 ne crée aucune nouvelle mécanique. Cette étape gèle le contrat du milestone MON16 et valide sa cohérence transversale par une campagne de régression réelle sous UE5.5.4.

## 1. Architecture gelée

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

Aucun second modèle de statut, second lifecycle ou second système de persistance n'a été identifié.

## 2. Contrats gelés

### MON16.1 — modèle runtime

- identité stable `EffectId` ;
- PrimaryAssetId `GridStatusEffect:EffectId` ;
- collection runtime déterministe ;
- état séparé par cible ;
- aucune dépendance UI.

### MON16.2 — lifecycle

- événementiel via le TurnManager ;
- durées `Turns`, `Rounds`, `Permanent` ;
- stacking data-driven ;
- aucune horloge murale ;
- aucun timer ou Tick de statut.

### MON16.3 — dégâts périodiques

- profil de dégâts data-driven ;
- pipeline de dégâts existant réutilisé ;
- tick avant décrément / expiration ;
- aucune branche de gameplay par nom d'effet.

### MON16.4 — initiative

- contribution additive ;
- `InitiativeModifier * StackCount` ;
- saturation int32 ;
- reordering limité aux entrées futures `Waiting`.

### MON16.5 — contrôle

- `bSkipActivation` ;
- `bBlockSpellActions` ;
- `bBlockTranslation` ;
- Stun / Silence / Immobilize sont des configurations de données et non des systèmes parallèles.

### MON16.6 — présentation

- projection UI en lecture seule ;
- HUD alimenté depuis l'état runtime ;
- feedback Apply / Refresh / Tick / Expire ;
- aucun WBP requis pour le contrat C++.

### MON16.7 — persistance

- snapshot stable sans pointeur UObject ;
- rebind de `DefinitionAsset` via l'identité primaire ;
- groupe actif + CharacterPool ;
- monstres via le runtime monster existant ;
- restauration atomique ;
- SaveGame v5 ;
- migration v4 -> v5 conservant la progression MON15.

## 3. Automation MON16.8

Fichier :

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

Résultat observé : **10/10 Success**.

## 4. Baseline MON16 validée

```text
MON16.1 :  7/7
MON16.2 : 10/10
MON16.3 : 11/11
MON16.4 : 11/11
MON16.5 : 11/11
MON16.6 : 10/10
MON16.7 : 11/11
MON16.8 : 10/10
----------------
MON16   : 81/81 Success
```

Le Run 5 du log de validation exécute exactement les 81 tests MON16 et retourne 81 `Success`, sans `Fail` ni `Error`.

## 5. Régressions amont validées

La campagne a également vérifié les milestones dont MON16 dépend directement :

```text
MON15 : 42/42 Success
MON16 : 81/81 Success
----------------------
Run 6 : 123/123 Success
```

Le Run 7 étend la validation à MON14. Le nombre réel de tests MON14 est **21**, et non 19 :

- 19 tests `Grimrock.Monsters.MON14...` ;
- 2 tests éditeur `Grimrock.Editor.MON14.3.1...`.

Résultat final :

```text
MON14 : 21/21 Success
MON15 : 42/42 Success
MON16 : 81/81 Success
----------------------
Total : 144/144 Success
```

Aucun `Result={Fail}` ni `Result={Error}` n'est présent dans les trois campagnes finales.

## 6. Points fonctionnels effectivement couverts

La régression confirme notamment :

- identité primaire stable ;
- stacking et expiration déterministes ;
- dégâts périodiques party/monster ;
- initiative positive/négative, expiration et reordering ;
- Stun / Silence / Immobilize ;
- rotation permise sous immobilisation ;
- translation bloquée sans consommation de ressources ;
- blocage atomique des sorts sous Silence ;
- feedback Apply / Refresh / Tick / Expire ;
- projection HUD ;
- sauvegarde/restauration des status effects ;
- migration de sauvegarde ;
- séparation `Transient` / `SaveGame` ;
- absence de système parallèle et de logique par nom d'effet.

## 7. Correctif de baseline MON14

La prévision initiale de MON16.8 indiquait :

```text
MON14 : 19/19
Total : 142/142
```

Elle était incomplète car elle ne comptait pas les deux tests éditeur MON14.3.1. La baseline correcte, issue de l'exécution réelle, est désormais :

```text
MON14 : 21/21
MON15 : 42/42
MON16 : 81/81
Total : 144/144
```

## 8. Périmètre de clôture

Aucun changement supplémentaire de gameplay, `.uasset`, `.umap` ou WBP n'est requis pour clore MON16.

Le milestone **Status Effects MON16 est gelé**. Toute extension fonctionnelle future des effets de statut doit être traitée dans un milestone ultérieur, en conservant les contrats et la baseline de régression définis ici.
