# TD07 — Final Quantitative Audit Baseline

Date : **28 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Campagne : **TD07 — Future-Proofing**  
Statut : **BASELINE FINALE POUR AUDITS FUTURS**

## 1. Objet

Ce document fige un bilan chiffré de la campagne TD07 afin de disposer d'un point de comparaison durable lors d'un futur audit de dette technique.

Il complète les documents détaillés TD07.1 à TD07.8. Il ne remplace ni le registre autoritaire de dette, ni les contrats d'architecture.

## 2. Périmètre Git de référence

Fenêtre de comparaison :

```text
Base : b15330c7bbcae2b2d8f45a5bf94ee8f6d05bea5f
Head : 432ec23f345c801de72e331360f34a77865413a1
```

Cette fenêtre couvre la campagne TD07 jusqu'à son état documentaire final après clôture.

Elle inclut la **characterization MON21.4 — Quest Persistence** réalisée pendant la campagne, mais **aucune implémentation MON21.4**.

Métriques GitHub :

```text
Commits                         158
Fichiers concernés             300
  modifiés                     222
  ajoutés                       75
  supprimés                      3

Changements de lignes        13 916
  additions                  8 834
  suppressions               5 082
  delta net                 +3 752
```

Ces métriques englobent code, tests, scripts, documentation et certains assets. Elles mesurent le volume global de transformation du dépôt, pas uniquement les lignes de gameplay.

## 3. Revue initiale des risques — résultat final

La revue précédant TD07 identifiait dix risques principaux.

| # | Risque initial | Traitement TD07 | État final |
|---|---|---|---|
| 1 | Dépendance Meshy non reproductible | TD07.1 | **Résolu** |
| 2 | Accumulation Save / migrations historiques | TD07.3 | **Résolu** |
| 3 | `UGridActivationComponent` très central | TD07.4 | **Caractérisé / surveillé** |
| 4 | API UE dépréciée dans MON17 | TD07.2 | **Résolu** |
| 5 | Tests Receptacle sur branche historique | TD07.5 | **Résolu** |
| 6 | Toolchain de build non stabilisée | TD07.1 / TD-BUILD-002 | **Surveillé** |
| 7 | Compatibilités legacy dans les modèles | TD07.3 | **Résolu** |
| 8 | `LogTemp` massif | TD07.7 | **Réduit / surveillé** |
| 9 | Baseline formatting non propre | TD07.7 | **Résolu** |
| 10 | Branches Git historiques / temporaires | TD07.5 | **Résolu** |

Bilan :

```text
Risques principaux identifiés       10
Complètement résolus                 7
Caractérisés / surveillés            3
Oubliés                              0

Dette P0 active après TD07            0
Dette P1 active après TD07            0
```

Les trois risques volontairement conservés sous surveillance sont :

- `UGridActivationComponent` : aucune duplication d'autorité ni second bus Event -> Command observé ;
- toolchain MSVC : warning de version non préférée, mais Editor et Shipping verts ;
- `LogTemp` résiduel : nettoyage opportuniste par domaine touché, pas de remplacement global artificiel.

## 4. Git / branches

État initial audité :

```text
master + 17 branches historiques / temporaires
```

État final :

```text
master uniquement
```

Résultat :

```text
Branches historiques supprimées : 17
Réduction des branches          : 94,4 % par rapport aux 18 branches initiales
Travail utile perdu             : 0
```

La valeur utile de l'ancienne branche `Test-Receptacle` a été récupérée sous forme de couverture Automation C++ actuelle avant suppression.

## 5. Formatting C++

Périmètre first-party :

```text
Fichiers C++ scannés             568
Fichiers non conformes initiaux  126
Part initialement hors format    22,2 %
Fichiers non conformes finaux      0
Baseline finale                  568 / 568 conforme
clang-format                     19.1.5
```

Résultat : **126 fichiers historiques normalisés** et gate global durable via `Scripts/CheckCppFormat.ps1`.

## 6. Logs

Audit TD07.7 initial :

```text
UE_LOG(LogTemp)                  452 appels / 45 fichiers
ActivationComponent              38 appels LogTemp
```

Après TD07.7 :

```text
UE_LOG(LogTemp)                  414 appels / 44 fichiers
ActivationComponent               0 appel LogTemp
LogGridActivation                38 appels
```

Résultat :

- réduction globale : **38 appels** ;
- réduction dans Activation : **100 %** ;
- les 414 appels restants sont P2 opportunistes et ne constituent pas une dette bloquante.

## 7. Modèle de données / Save

Audit initial TD07.3.1 :

```text
DataAssets scannés                86
Findings                          41
  Conflict                         2
  DuplicateAuthority             23
  LegacyField                    11
  LegacyOnly                      3
  SchemaRename                    2
```

État final :

```text
SaveGame courant                 v22 exact-match
Migrations arrière prototype       0
Candidats de réparation TD07.3.7   0
Assets LFS réparés                12
Nouveaux Item DataAssets           2
  DA_Item_RatMeat
  DA_Item_RatTooth
```

Les autorités Character, Authoring et Combat ont été normalisées et le legacy confirmé inutilisé a été supprimé plutôt que conservé.

## 8. Couverture de validation

### TD07.3

Gate strict :

```text
Grimrock.TechnicalDebt.TD07_3_8.StrictCurrentSchema
5 / 5
```

Campagne de clôture :

```text
34 tests de normalization
 1 CurrentSchemaAssetAudit
 5 tests StrictCurrentSchema
--------------------------------
40 tests / audits verts
```

### TD07.8 final

Automations rejouées dans le gate final :

```text
TD07_3_8.StrictCurrentSchema                       5
TD07_2                                             1
TD07_4.Characterization                            4
TD07_5.Recovery.ReceptacleCommands                 1
TD01_3.EventCommandContract.RuntimeHardening        1
MON21_4.Characterization                            4
----------------------------------------------------
Total                                               16
```

Résultat :

```text
Succeeded with warnings : 0
Failed                  : 0
```

Le gate final couvre en plus :

- branche Git / cleanup ;
- dépendances projet ;
- format C++ global ;
- Development Editor build ;
- Win64 Shipping complet.

## 9. Shipping final de référence

Validation du 28 août 2026 :

```text
BUILD SUCCESSFUL
Cook                    Success - 0 error(s), 0 warning(s)
Pak files               1
Archive files           41
Archive bytes           905 990 059
Archive                 Saved/Packaging/TD04/TD04-Shipping-20260828-125341
```

Ce package constitue la référence Shipping de clôture TD07.

## 10. Résumé chiffré compact

```text
Campagne Git
  158 commits
  300 fichiers concernés
  13 916 changements de lignes

Dette
  10 risques initiaux
   7 résolus
   3 surveillés
   0 oublié
   0 P0 active
   0 P1 active

Git
  17 branches historiques supprimées
  master seule branche finale

Style
  568 fichiers C++ first-party
  126 initialement hors format
    0 hors format final

Logs
  452 -> 414 LogTemp globaux
   38 -> 0 dans Activation

Data / Save
  86 DataAssets audités initialement
  41 findings initiaux
   0 candidat TD07.3.7 final
  SaveGame v22 exact-match
   0 migration arrière prototype

Validation
  40 tests/audits TD07.3 de clôture
  16 Automation dans le gate TD07.8
   0 warning Automation final
   0 échec

Shipping
  0 erreur cook
  0 warning cook
  1 pak
  41 fichiers archive
  905 990 059 octets
```

## 11. Baseline pour le prochain audit

Un futur audit de dette doit comparer au minimum les indicateurs suivants à cette référence :

1. nombre de dettes P0 / P1 actives ;
2. nombre de branches hors `master` ;
3. conformité des 568+ fichiers C++ first-party via `CheckCppFormat.ps1` ;
4. nombre de fichiers et appels `UE_LOG(LogTemp)` ;
5. taille / responsabilités / autorités de `UGridActivationComponent` ;
6. version courante du SaveGame et présence éventuelle de nouvelles migrations ;
7. findings du schéma courant / DataAssets ;
8. warnings first-party de build et cook ;
9. état de la toolchain MSVC ;
10. résultats Editor / Automation / Shipping.

Tout écart n'est pas automatiquement une dette : il doit être qualifié selon le risque réel, conformément aux règles du registre autoritaire.

## 12. Références

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
docs/Design/TD07_1_BUILD_DEPENDENCY_REPRODUCIBILITY.md
docs/Design/TD07_2_UE_DEPRECATION_WARNING_AUDIT.md
docs/Design/TD07_3_8_STRICT_CURRENT_SCHEMA_VALIDATION.md
docs/Design/TD07_4_ACTIVATION_COMPONENT_CHARACTERIZATION.md
docs/Design/TD07_5_SUSPENDED_TEST_INFRASTRUCTURE_BRANCH_RECOVERY.md
docs/Design/TD07_7_TARGETED_LOG_FORMATTING_HYGIENE.md
docs/Design/TD07_8_FUTURE_PROOFING_STOP_CONDITION.md
```
