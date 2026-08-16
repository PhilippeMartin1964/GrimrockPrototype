# MON16.2 — Duration / Turn / Round Lifecycle

## Statut

**VALIDÉ ET CLOS — 16 août 2026.**

Base :

```text
83f2630c213ad8e1c0583c326085be4df73de71a
Close MON16.1 status effect runtime model
```

Commit logique d'implémentation MON16.2 :

```text
e038df582acc25bd990d924eec689d7a2b09d231
Add MON16.2 status effect lifecycle
```

MON16.2 ajoute le lifecycle logique des effets de statut sans introduire leur comportement spécifique. Les durées restent exprimées en tours ou rounds, jamais en secondes.

## Périmètre

Implémenté : décrément `Turns`, décrément `Rounds`, expiration à zéro, conservation `Permanent`, réapplication et politiques `NoStack`, `RefreshDuration`, `AddStacks`, `ReplaceIfStronger`, raccordement événementiel au TurnManager, comportement commun personnages/monstres.

Hors périmètre : DoT, Poison/Bleeding/Burning fonctionnels, Haste/Slow effectifs, Stun/Silence/Immobilize effectifs, HUD, icônes, WBP, sauvegarde/restauration.

## Autorité temporelle

`UGridStatusEffectLifecycleSubsystem` est un `UWorldSubsystem` runtime. Il se lie au `UGridTurnManagerComponent` existant et écoute :

```text
OnCombatantStateChanged -> Completed/Incapacitated -> AdvanceDuration(Turns)
OnRoundStarted          -> transition N vers N+1    -> AdvanceDuration(Rounds)
OnCombatEnded           -> reset de la baseline round
```

Il n'utilise ni tick de durée, ni timer, ni horloge de présentation.

## Sémantique

`Turns` décrémente une fois lorsqu'une cible consomme une activation. `Completed` et `Incapacitated` comptent comme une activation consommée. Seule la collection de la cible avance.

`Rounds` décrémente à chaque frontière entre deux manches. `OnRoundStarted(1)` établit la baseline ; `Round 1 -> Round 2` consomme une unité. Si plusieurs numéros sont franchis, toutes les frontières sont consommées déterministiquement.

`Permanent` conserve `RemainingDuration = 0` et ne décrémente jamais.

Un effet présent au moment de sa frontière logique est décrémenté : il n'existe aucun timestamp caché.

## Expiration

`FGridStatusEffectCollection::AdvanceDuration()` traite uniquement l'unité demandée, décrémente d'une unité, supprime à zéro et retourne :

```cpp
FGridStatusEffectAdvanceResult
DurationUnit
AdvancedEffectIds
ExpiredEffectIds
```

L'ordre reste déterministe par `EffectId`.

## Stacking

`TryAdd()` reste le chemin strict MON16.1 : un doublon est rejeté.

MON16.2 ajoute `TryApply()` :

- `NoStack` : rejette sans mutation ;
- `RefreshDuration` : remplace source, durée et potency par le candidat ;
- `AddStacks` : ajoute jusqu'à `MaxStacks`, rafraîchit la durée, garde la source récente et la potency maximale ;
- `ReplaceIfStronger` : remplace seulement si `Candidate.Potency > Existing.Potency`.

## Potency

MON16.1 déclarait `ReplaceIfStronger` sans critère générique de puissance. MON16.2 ajoute :

```cpp
UGridStatusEffectDefinitionAsset::DefaultPotency
FGridStatusEffectRuntimeState::Potency
```

`Potency` est uniquement une valeur de précédence de réapplication. Ce n'est ni l'attribut RPG Strength, ni un bonus de dégâts, d'armure ou d'initiative. `TryApply()` accepte un `PotencyOverride` pour les futurs sorts/objets/compétences.

## Atomicité

Toute définition, durée, stack, potency ou réapplication incohérente laisse la collection existante inchangée. Une réapplication du même `EffectId` avec un autre `DurationUnit` est rejetée.

## Intégration

Le subsystem ne duplique aucun état de combat. Il lit `PartyPawn`, `CombatMonsters` et `FGridCombatantInitiativeEntry` depuis le TurnManager et agit sur les collections existantes. Aucun Widget n'est impliqué.

## Fichiers

Modifiés :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectTypes.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectTypes.cpp
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectDefinitionAsset.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectDefinitionAsset.cpp
```

Ajoutés :

```text
Source/GrimrockPrototype/Public/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.h
Source/GrimrockPrototype/Private/RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp
Source/GrimrockPrototype/Private/Tests/RPGMON162StatusEffectLifecycleTests.cpp
docs/Design/MON16_2_DURATION_TURN_ROUND_LIFECYCLE.md
docs/Design/MON16_2_VALIDATION_CHECKLIST.md
```

Aucun `.uasset`, `.umap` ou WBP.

## Validation Automation

Résultats fournis et analysés le 16 août 2026 :

```text
Grimrock.RPG.MON16.2.TurnDurationLifecycle          Success
Grimrock.RPG.MON16.2.RoundDurationLifecycle         Success
Grimrock.RPG.MON16.2.PermanentDurationLifecycle     Success
Grimrock.RPG.MON16.2.NoStackAndRefresh              Success
Grimrock.RPG.MON16.2.AddStacks                      Success
Grimrock.RPG.MON16.2.ReplaceIfStronger              Success
Grimrock.RPG.MON16.2.AtomicFailure                   Success
Grimrock.RPG.MON16.2.DeterministicExpiration        Success
Grimrock.RPG.MON16.2.TurnManagerEventIntegration    Success
Grimrock.RPG.MON16.2.NoUIDependency                  Success
```

Bilan ciblé : **10/10 Success**.

Régressions exécutées dans la même validation :

```text
Grimrock.RPG.MON16.1        7/7 Success
Grimrock.RPG.MON15         42/42 Success
Grimrock.Monsters.MON14    19/19 Success
```

Bilan de la campagne fournie : **78/78 Success**.

Aucun `Result={Fail}` ni erreur Automation n'est présent dans le log analysé. Les warnings de rendu `FlushRenderingCommands called recursively` observés pendant la campagne ciblée n'ont provoqué aucun échec et ne concernent pas les règles du lifecycle.

Le fait que les nouveaux tests C++ MON16.2 soient découverts, chargés et exécutés par UE5.5.4 confirme que le code MON16.2 correspondant est compilé et chargé dans l'éditeur utilisé pour la validation.

## Contrat gelé à la clôture

MON16.2 est **VALIDÉ ET CLOS** avec les règles suivantes :

- les durées gameplay restent exclusivement en `Turns`, `Rounds` ou `Permanent` ;
- les frontières temporelles sont celles du TurnManager existant ;
- aucun timer ni tick de statut ;
- `Turns` avance à la consommation d'une activation de la cible ;
- `Rounds` avance aux frontières de round ;
- expiration déterministe à zéro ;
- réapplication via `TryApply()` et politiques data-driven ;
- `Potency` sert uniquement à `ReplaceIfStronger` ;
- aucune logique spécifique Poison/Bleeding/Burning en MON16.2 ;
- aucune modification effective d'initiative en MON16.2 ;
- aucune dépendance UI ;
- aucune persistance avant MON16.7.

Prochaine étape : **MON16.3 — DoT Poison / Bleeding / Burning**.
