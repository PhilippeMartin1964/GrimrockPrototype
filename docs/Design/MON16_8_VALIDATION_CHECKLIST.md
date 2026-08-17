# MON16.8 — Validation Checklist — Clôture Status Effects

## État final

```text
Audit architecture       : OK
Implémentation MON16.8   : OK
Exécution UE5.5.4        : OK
Automation MON16.8       : 10/10 Success
Régression MON16         : 81/81 Success
Régression MON15         : 42/42 Success
Régression MON14         : 21/21 Success
Campagne finale          : 144/144 Success
Milestone MON16 clôturé  : OUI
```

Commit d'implémentation MON16.8 : `0244d1dc41d99160d81d5d700ec38408b5c88b0d`.

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

- [x] `EffectId` reste l'identité stable
- [x] PrimaryAssetId reste `GridStatusEffect:EffectId`
- [x] runtime party `StatusEffects` reste `Transient`
- [x] runtime monster `StatusEffects` reste `Transient`
- [x] runtime `DefinitionAsset` reste `Transient`
- [x] SaveState n'embarque aucun `DefinitionAsset`
- [x] SaveVersion MON16 reste 5
- [x] MinimumCompatibleSaveVersion reste 1

## Automation MON16.8

Résultat observé : **10/10 Success**.

- [x] `PrimaryAssetIdentityContract`
- [x] `CrossFeatureComposition`
- [x] `PersistenceRoundTripSemantics`
- [x] `DeterministicPersistenceOrder`
- [x] `RuntimeSaveBoundary`
- [x] `SaveVersionContract`
- [x] `LifecycleArchitectureBoundary`
- [x] `NoHardCodedStatusIdentity`
- [x] `RegressionNamespaceCoverage`
- [x] `SingleCanonicalModel`

## Baseline MON16 validée

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

- [x] MON16 : 81/81
- [x] aucun Fail MON16
- [x] aucun Error MON16

## Campagne finale

Résultats réellement observés dans les logs :

```text
Run 5 : MON16                  =  81/81 Success
Run 6 : MON15 + MON16          = 123/123 Success
Run 7 : MON14 + MON15 + MON16  = 144/144 Success
```

Baseline définitive :

```text
MON14 : 21/21 Success
MON15 : 42/42 Success
MON16 : 81/81 Success
----------------------
Total : 144/144 Success
```

MON14 contient :

```text
19 tests Grimrock.Monsters.MON14...
 2 tests Grimrock.Editor.MON14.3.1...
-------------------------------
21 tests
```

La valeur prévisionnelle `19/19` / `142/142` était donc incomplète et est remplacée par cette baseline issue de l'exécution réelle.

- [x] MON14 : 21/21
- [x] MON15 : 42/42
- [x] MON16 : 81/81
- [x] total : 144/144
- [x] aucun `Result={Fail}`
- [x] aucun `Result={Error}`

## Smoke test manuel

Un smoke test manuel status effect reste possible pour la QA de présentation, mais il n'est plus une condition de clôture technique du milestone : les contrats runtime, combat, présentation et persistance sont couverts par l'automation.

## Clôture

**MON16 — Status Effects : VALIDÉ ET CLOS.**

Aucun nouveau mécanisme de status effect ne doit être ajouté dans ce commit de clôture. Toute évolution future doit conserver cette baseline ou documenter explicitement son changement dans un milestone ultérieur.
