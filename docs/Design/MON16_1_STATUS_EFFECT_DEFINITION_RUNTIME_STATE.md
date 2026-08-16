# MON16.1 — Status Effect Definition & Runtime State

## Statut

**Implémenté — validation UE5 en attente.**

MON16.1 pose uniquement le vocabulaire, la définition statique et l'état runtime générique des effets de statut. Aucune compilation Unreal Engine ni campagne Automation n'est considérée comme validée tant que le log UE5 correspondant n'a pas été fourni et examiné.

Référence d'audit avant implémentation :

```text
master = 941ae876899c26e5587ed279be50a71a501fa361
Close MON15 XP and level progression
```

Commit logique de code MON16.1 :

```text
10c93ce522fc9cd97dbb927f5234104be6378ae9
Add MON16.1 status effect runtime model
```

---

## 1. Audit et réutilisation de l'existant

L'audit a confirmé les autorités suivantes :

- `FGridCharacterInventoryState` est l'état canonique d'un personnage actif ;
- `FGridPartyInventoryState::ActiveCharacters` possède les personnages actifs ;
- `AGridMonsterActor` possède l'état runtime vivant d'un monstre ;
- `FGridRuntimeMonsterState` est une structure de persistance et n'est donc pas étendue en MON16.1 ;
- `UGridMonsterDefinitionAsset` et les DataAssets RPG donnent le précédent pour une identité `FName` stable et un `PrimaryAssetId` ;
- `FGridCombatantInitiativeEntry::InitiativeModifier` existe déjà et doit rester l'unique point d'intégration de Haste/Slow côté initiative ;
- les `RequirementIds` de MON12/MON15 sont des autorisations/progressions, pas des états temporaires ;
- `EGridDamageType::Poison` et `PoisonResistance` décrivent un type de dégâts et sa résistance, pas l'état runtime `Poison`.

Aucun moteur C++ existant de buff/debuff/status n'a été trouvé. MON16.1 n'en duplique donc aucun.

---

## 2. Architecture retenue

```text
UGridStatusEffectDefinitionAsset
        |
        | construit
        v
FGridStatusEffectRuntimeState
        |
        | stocké dans
        v
FGridStatusEffectCollection
        |
        +--> FGridCharacterInventoryState::StatusEffects
        |
        +--> AGridMonsterActor::StatusEffects
```

Le même format runtime est utilisé pour les personnages et les monstres. Il n'existe pas de classe C++ spécifique à `Poison`, `Burning`, `Haste`, etc.

---

## 3. Définition statique

### `UGridStatusEffectDefinitionAsset`

Fichiers :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectDefinitionAsset.cpp
```

Le DataAsset contient :

- `EffectId` ;
- `DisplayName` ;
- `Description` ;
- `Disposition` : `Neutral`, `Buff`, `Debuff` ;
- `DurationUnit` : `Turns`, `Rounds`, `Permanent` ;
- `DefaultDuration` ;
- `StackPolicy` : `NoStack`, `RefreshDuration`, `AddStacks`, `ReplaceIfStronger` ;
- `MaxStacks` ;
- `InitiativeModifier`.

### Identité stable

`EffectId` est l'identité gameplay stable.

Le `PrimaryAssetId` est :

```text
Type = GridStatusEffect
Name = EffectId
```

Il ne dépend donc pas du nom du fichier `.uasset` ou du nom de l'objet UObject.

Aucun asset de production n'est créé pendant MON16.1.

---

## 4. Validation des définitions

Une définition est rejetée notamment si :

- `EffectId == None` ;
- `DisplayName` est vide ;
- un effet `Turns` ou `Rounds` possède une durée nulle ou négative ;
- un effet `Permanent` possède une durée différente de zéro ;
- `MaxStacks < 1` ;
- `AddStacks` déclare moins de deux stacks ;
- une autre politique que `AddStacks` déclare plus d'un stack dans le contrat MON16.1.

La validation est agrégée comme dans les autres DataAssets du projet : plusieurs erreurs peuvent être retournées en une seule passe.

---

## 5. État runtime commun

### `FGridStatusEffectRuntimeState`

Fichier :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectTypes.h
```

État représenté :

```cpp
EffectId
SourceId
StackCount
DurationUnit
RemainingDuration
```

Exemple logique :

```text
EffectId          = Poison
SourceId          = <Guid stable de la source>
StackCount        = 2
DurationUnit      = Rounds
RemainingDuration = 3
```

MON16.1 ne modifie jamais `RemainingDuration` après la création de l'état.

### Source de l'effet

`SourceId` est un `FGuid`, jamais un pointeur d'Actor ou d'UObject.

Quand une source gameplay possède une identité stable :

- personnage : `CharacterId` ;
- monstre : identité de persistance/résolution existante du monstre ;
- autre objet runtime : son identité stable existante si le système en possède une.

Un `FGuid` invalide représente volontairement une source anonyme, système ou environnementale. Cela évite d'inventer un UObject persistant fragile uniquement pour appliquer un effet.

---

## 6. Collection runtime

### `FGridStatusEffectCollection`

La collection :

- possède les `FGridStatusEffectRuntimeState` ;
- permet la recherche par `EffectId` ;
- crée un état depuis une définition validée ;
- refuse atomiquement une création invalide ;
- refuse en MON16.1 un second état possédant le même `EffectId` ;
- trie les effets par `EffectId` pour garantir une lecture déterministe.

### Pourquoi refuser actuellement un doublon ?

Les politiques de stacking sont déjà **déclarées par données**, mais leur exécution n'appartient pas à MON16.1.

Ainsi :

```text
NoStack
RefreshDuration
AddStacks
ReplaceIfStronger
```

sont représentables, sans que MON16.1 décide encore comment fusionner deux applications concurrentes. Un ajout dupliqué échoue donc sans modifier la collection existante.

La résolution effective sera introduite avec le lifecycle MON16.2.

---

## 7. Stockage sur les cibles

### Personnages

`FGridCharacterInventoryState` reçoit :

```cpp
FGridStatusEffectCollection StatusEffects;
```

La propriété est `Transient` en MON16.1.

Chaque personnage possède sa propre collection par valeur. Aucun état global n'est partagé entre les membres du groupe.

### Monstres

`AGridMonsterActor` reçoit la même structure :

```cpp
FGridStatusEffectCollection StatusEffects;
```

La propriété est également `Transient`.

`InitializeMonster()` remet cette collection à zéro lorsqu'un Actor monstre est réinitialisé.

### Persistance explicitement différée

MON16.1 ne modifie pas :

```text
FGridRuntimeMonsterState
UGameSave / GrimrockPartySaveGame
RPGSaveMigrationService
```

La sauvegarde/restauration des effets appartient à MON16.7.

---

## 8. Initiative et futurs modificateurs

`UGridStatusEffectDefinitionAsset::InitiativeModifier` est une donnée déclarative préparant MON16.4.

MON16.1 ne l'applique nulle part.

MON16.4 devra lire les effets actifs et alimenter le champ existant :

```cpp
FGridCombatantInitiativeEntry::InitiativeModifier
```

Il ne faudra pas introduire une seconde statistique d'initiative parallèle.

De la même manière, les futurs `Strength Buff`, `Armor Buff` ou `Regeneration` devront étendre la définition data-driven et s'insérer dans les pipelines RPG/combat existants, sans classe C++ dédiée par effet et sans système parallèle de statistiques.

---

## 9. Ce que MON16.1 permet déjà de représenter

La même architecture peut porter des identités telles que :

```text
Poison
Bleeding
Burning
Haste
Slow
Stunned
Silenced
Immobilized
Regeneration
StrengthBuff
ArmorBuff
```

MON16.1 ne leur attribue encore aucun comportement gameplay.

---

## 10. Hors périmètre explicite

MON16.1 n'implémente pas :

- décrément par tour ;
- décrément par round ;
- expiration automatique ;
- dégâts périodiques ;
- Poison/Bleeding/Burning fonctionnels ;
- Haste/Slow fonctionnels ;
- modification réelle de l'initiative ;
- Stun/Silence/Immobilize fonctionnels ;
- blocage des actions, sorts ou déplacements ;
- régénération ;
- modification de Strength/Armor ;
- feedback combat ;
- HUD ;
- icône ;
- Widget Blueprint ;
- sauvegarde/restauration ;
- modification `.uasset` ou `.umap`.

---

## 11. Préparation de MON16.2

MON16.2 devra s'appuyer sur les structures ci-dessus et raccorder le lifecycle au `TurnManager` de manière événementielle.

La prochaine étape devra notamment décider précisément :

- à quel événement un effet `Turns` décrémente ;
- à quel événement un effet `Rounds` décrémente ;
- quand l'expiration est évaluée ;
- comment les réapplications exécutent `NoStack`, `RefreshDuration`, `AddStacks`, `ReplaceIfStronger` ;
- comment notifier les changements sans dépendre d'un Widget.

La présentation et les dégâts périodiques resteront hors de MON16.2 si leur sous-jalon dédié n'est pas encore atteint.

---

## 12. Tests Automation MON16.1

Namespace :

```text
Grimrock.RPG.MON16.1
```

Tests :

```text
Grimrock.RPG.MON16.1.DefinitionValidation
Grimrock.RPG.MON16.1.StableIdentity
Grimrock.RPG.MON16.1.RuntimeStateCreation
Grimrock.RPG.MON16.1.TargetIsolation
Grimrock.RPG.MON16.1.DeterministicCollection
Grimrock.RPG.MON16.1.AtomicInvalidAdd
Grimrock.RPG.MON16.1.NoUIDependency
```

Couverture :

- définition valide ;
- `EffectId` invalide ;
- durées et stacking incohérents ;
- identité `PrimaryAssetId` stable ;
- création runtime valide ;
- création atomique ;
- ajout sur personnage ;
- ajout sur vrai `AGridMonsterActor` de test ;
- isolation entre deux personnages ;
- isolation personnage/monstre ;
- collection commune aux deux modèles ;
- ordre déterministe ;
- ajout invalide/dupliqué sans mutation ;
- absence de dépendance source vers UI/UMG/widget.

Commande Automation ciblée :

```text
Automation RunTests Grimrock.RPG.MON16.1
```

MON16.1 ne doit être déclaré **VALIDÉ ET CLOS** qu'après compilation UE5 et succès réel de cette campagne, puis régression appropriée, sur logs fournis par l'utilisateur.
