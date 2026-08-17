# MON16.7 — Validation Checklist

## État

```text
Implémentation C++ : VALIDÉE
Documentation      : VALIDÉE
Compilation UE5    : VALIDÉE
Automation MON16.7 : 11/11 SUCCESS
Régressions        : VALIDÉES
Clôture            : OUI
```

Implémentation : `3d6d1b1600245a79cb0a5262a97a45ecb2237f5c`.
Correctif test de régression : `8d64574f6681c7691a5778451ff3a61dfcdcec8b`.

## Contrat persistant

- [x] `FGridStatusEffectSaveState` ne contient que des données stables
- [x] `DefinitionAsset` absent du snapshot
- [x] runtime `DefinitionAsset` reste `Transient`
- [x] runtime party `StatusEffects` reste `Transient`
- [x] runtime monster `StatusEffects` reste `Transient`
- [x] EffectId reste l'identité persistante
- [x] ordre déterministe par EffectId

## Capture / Restore

- [x] capture refuse un runtime invalide
- [x] capture vérifie la définition canonique runtime
- [x] restore refuse les doublons
- [x] restore refuse les snapshots structurellement invalides
- [x] restore refuse une définition introuvable
- [x] restore vérifie DurationUnit et MaxStacks
- [x] restore reconstruit `DefinitionAsset`
- [x] restore atomique sans mutation partielle

## Groupe

- [x] ActiveCharacters persistés
- [x] CharacterPool persisté
- [x] snapshot indexé par CharacterId stable
- [x] CharacterId invalide/ambigu rejeté
- [x] restauration party atomique
- [x] intégration dans `UGrimrockPartySaveGame::Serialize`

## Monstres

- [x] `FGridRuntimeMonsterState::StatusEffects`
- [x] propriété marquée `SaveGame`
- [x] capture via `AGridMonsterActor::CaptureRuntimeMonsterState`
- [x] restore via `AGridMonsterActor::RestoreRuntimeMonsterState`
- [x] MonsterPlacements couverts par le runtime state existant
- [x] aucun registre parallèle

## Version / migration

- [x] CurrentSaveVersion 4 -> 5
- [x] MinimumCompatibleSaveVersion reste 1
- [x] v4 possède un chemin de migration dédié
- [x] v4 ne repasse pas par la reconstruction v1-v3
- [x] choix MON15.5/15.6 conservés
- [x] pending level-ups conservés
- [x] v4 initialise les status snapshots à vide
- [x] v1-v3 continuent le chemin legacy existant
- [x] tests MON15.6 adaptés au nouveau numéro de version sans perte de couverture

## Compilation UE5.5.4

- [x] compilation / chargement confirmé par exécution des automations utilisateur

## Automation ciblée

Commande :

```text
Automation RunTests Grimrock.RPG.MON16.7
```

- [x] `CollectionCapture` — Success
- [x] `CollectionRestore` — Success
- [x] `AtomicRestoreFailure` — Success
- [x] `DuplicateEffectRejected` — Success
- [x] `DefinitionContractMismatch` — Success
- [x] `PartyActiveAndPoolRoundTrip` — Success
- [x] `PartyAtomicFailure` — Success
- [x] `V4MigrationPreservesProgression` — Success
- [x] `MonsterSnapshotContract` — Success
- [x] `SaveVersionContract` — Success
- [x] `TransientRuntimeBoundary` — Success

Résultat : **11/11 Success**.

## Régressions

Campagnes demandées :

```text
Grimrock.RPG.MON16.6
Grimrock.RPG.MON16.5
Grimrock.RPG.MON16.4
Grimrock.RPG.MON16.3
Grimrock.RPG.MON16.2
Grimrock.RPG.MON16.1
Grimrock.RPG.MON15
Grimrock.Monsters.MON14
```

Résultat : tous les tests fonctionnels demandés sont verts après correction du seul test historique obsolète `Grimrock.RPG.MON15.5.TransientPersistenceBoundary`.

Cause du test obsolète : il imposait `CurrentSaveVersion == 4` alors que MON16.7 définit légitimement la version 5.

Correction appliquée :

```text
8d64574f6681c7691a5778451ff3a61dfcdcec8b
Fix MON16.7 save version regression test
```

Rerun :

```text
Automation RunTests Grimrock.RPG.MON15.5
```

- [x] AtomicBatchCommit — Success
- [x] AtomicFailure — Success
- [x] CharacterIsolation — Success
- [x] CombatCatalogUnlock — Success
- [x] LevelUpNotificationSource — Success
- [x] TransientPersistenceBoundary — Success
- [x] WidgetCancelIsNonMutating — Success
- [x] WidgetConfirmTransaction — Success

Résultat : **8/8 Success**.

- [x] aucun échec résiduel connu dans le périmètre MON14 / MON15 / MON16 demandé

## Vérification manuelle conseillée

Cette vérification reste utile comme contrôle de production, mais n'est plus bloquante pour la clôture automation :

1. appliquer un effet à un personnage ;
2. sauvegarder ;
3. recharger ;
4. vérifier stacks et durée restante ;
5. vérifier que l'effet continue son lifecycle ;
6. répéter avec un monstre persistant si un scénario de test runtime est disponible.

## Clôture

**MON16.7 — VALIDÉ ET CLOS.**

Prochaine étape : `MON16.8 — clôture / régression finale du milestone Status Effects`.
