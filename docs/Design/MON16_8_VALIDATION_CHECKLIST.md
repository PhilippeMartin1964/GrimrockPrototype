# MON16.8 — Validation Checklist — Clôture Status Effects

## État

```text
Audit architecture       : TERMINÉ
Implémentation MON16.8   : PRÉPARÉE
Compilation UE5.5.4      : EN ATTENTE
Automation MON16.8       : EN ATTENTE
Régression MON16         : EN ATTENTE
Régression MON15         : EN ATTENTE
Régression MON14         : EN ATTENTE
Milestone MON16 clôturé  : NON
```

Base : `b4ba41c10e5f38820bbd08ee0abb200dce6a6a92`.

## Audit de cohérence

- [x] un seul `FGridStatusEffectRuntimeState`
- [x] une seule `FGridStatusEffectCollection`
- [x] un seul `UGridStatusEffectLifecycleSubsystem`
- [x] un seul `FGridStatusEffectPersistence`
- [x] initiative projetée par le résolveur MON16.4
- [x] contrôles projetés par le résolveur MON16.5
- [x] présentation projetée par le builder MON16.6
- [x] persistance réutilise SaveGame + runtime monster existants
- [x] aucun WBP/.uasset/.umap requis par MON16.8
- [x] aucun refactor gameplay nécessaire à la clôture

## Contrats de données

- [x] EffectId reste l'identité stable
- [x] PrimaryAssetId reste `GridStatusEffect:EffectId`
- [x] runtime party StatusEffects reste `Transient`
- [x] runtime monster StatusEffects reste `Transient`
- [x] runtime DefinitionAsset reste `Transient`
- [x] SaveState n'embarque aucun DefinitionAsset
- [x] SaveVersion MON16 reste 5
- [x] MinimumCompatibleSaveVersion reste 1

## Automation ciblée MON16.8

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16.8
```

Attendu : **10/10 Success**.

- [ ] `PrimaryAssetIdentityContract`
- [ ] `CrossFeatureComposition`
- [ ] `PersistenceRoundTripSemantics`
- [ ] `DeterministicPersistenceOrder`
- [ ] `RuntimeSaveBoundary`
- [ ] `SaveVersionContract`
- [ ] `LifecycleArchitectureBoundary`
- [ ] `NoHardCodedStatusIdentity`
- [ ] `RegressionNamespaceCoverage`
- [ ] `SingleCanonicalModel`

## Baseline MON16 figée

Les namespaces existants doivent rester :

```text
MON16.1 :  7
MON16.2 : 10
MON16.3 : 11
MON16.4 : 11
MON16.5 : 11
MON16.6 : 10
MON16.7 : 11
MON16.8 : 10
----------------
MON16   : 81
```

## Campagne finale

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

Attendu :

```text
MON16 : 81/81 Success
MON15 : 42/42 Success
MON14 : 19/19 Success
Total : 142/142 Success
```

- [ ] MON16 : 81/81
- [ ] MON15 : 42/42
- [ ] MON14 : 19/19
- [ ] aucun Fail
- [ ] aucun Error

## Smoke test manuel optionnel

Ce contrôle n'est pas un substitut aux automations :

1. appliquer un effet comportant durée + contrôle ou DoT ;
2. vérifier le résumé HUD / feedback ;
3. sauvegarder ;
4. recharger ;
5. vérifier stacks et durée restante ;
6. vérifier que l'effet continue à modifier combat / mouvement / initiative selon sa définition.

- [ ] smoke test effectué si souhaité

## Clôture

MON16 ne doit être marqué **VALIDÉ ET CLOS** qu'après confirmation des campagnes ci-dessus.

Aucun nouveau mécanisme de status effect ne doit être ajouté dans ce commit de clôture.
