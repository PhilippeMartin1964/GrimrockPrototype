# MON16.7 — Validation Checklist

## État

```text
Implémentation C++ : préparée
Documentation      : préparée
Compilation UE5    : EN ATTENTE
Automation MON16.7 : EN ATTENTE
Régressions        : EN ATTENTE
Clôture            : NON
```

Base : `71f38ea0638bdd99c81e268b26afc768fb196f57`.

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

Attendu : 0 erreur C++, UHT ou link.

- [ ] compilation / chargement confirmé par log utilisateur

## Automation ciblée

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16.7
```

- [ ] `CollectionCapture` — Success
- [ ] `CollectionRestore` — Success
- [ ] `AtomicRestoreFailure` — Success
- [ ] `DuplicateEffectRejected` — Success
- [ ] `DefinitionContractMismatch` — Success
- [ ] `PartyActiveAndPoolRoundTrip` — Success
- [ ] `PartyAtomicFailure` — Success
- [ ] `V4MigrationPreservesProgression` — Success
- [ ] `MonsterSnapshotContract` — Success
- [ ] `SaveVersionContract` — Success
- [ ] `TransientRuntimeBoundary` — Success

Attendu : **11/11 Success**.

## Régressions minimales

Après MON16.7 vert :

```text
Automation RunTests Grimrock.RPG.MON16.6
Automation RunTests Grimrock.RPG.MON16.5
Automation RunTests Grimrock.RPG.MON16.4
Automation RunTests Grimrock.RPG.MON16.3
Automation RunTests Grimrock.RPG.MON16.2
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

Attendus :

```text
MON16.6 : 10/10
MON16.5 : 11/11
MON16.4 : 11/11
MON16.3 : 11/11
MON16.2 : 10/10
MON16.1 :  7/7
MON15   : 42/42
MON14   : 19/19
```

- [ ] MON16.6 : 10/10 Success
- [ ] MON16.5 : 11/11 Success
- [ ] MON16.4 : 11/11 Success
- [ ] MON16.3 : 11/11 Success
- [ ] MON16.2 : 10/10 Success
- [ ] MON16.1 : 7/7 Success
- [ ] MON15 : 42/42 Success
- [ ] MON14 : 19/19 Success

## Vérification manuelle conseillée

Après les automations :

1. appliquer un effet à un personnage ;
2. sauvegarder ;
3. recharger ;
4. vérifier stacks et durée restante ;
5. vérifier que l'effet continue son lifecycle ;
6. répéter avec un monstre persistant si un scénario de test runtime est disponible.

Cette vérification manuelle n'est pas un substitut aux automations mais confirme le wiring SaveGame réel avec des DataAssets de production.

## Clôture

MON16.7 sera marqué **VALIDÉ ET CLOS** uniquement après compilation/chargement UE5.5.4, 11/11 MON16.7 et régressions appropriées sans nouvel échec.

Prochaine étape : `MON16.8 — clôture / régression finale du milestone Status Effects`.
