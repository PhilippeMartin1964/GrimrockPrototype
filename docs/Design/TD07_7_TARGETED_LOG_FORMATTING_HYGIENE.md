# TD07.7 — Targeted Log / Formatting Hygiene

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Dettes : TD-LOG-001 / TD-STYLE-001
Statut : LOG NORMALIZED — FORMAT BASELINE GREEN — RUNTIME VALIDATION PENDING

## 1. Objectif

Traiter uniquement les deux écarts de hygiene encore autoritaires :

- taxonomie de logs encore partiellement `LogTemp` ;
- baseline globale `clang-format 19.1.5` non encore prouvée verte.

TD07.7 n'est **pas** un remplacement global aveugle de `LogTemp` et n'est pas un prétexte à un refactor fonctionnel.

## 2. Règle logs

Le registre TD-LOG-001 impose :

> continuer uniquement dans les domaines réellement touchés ; aucun remplacement global de LogTemp.

Le domaine prioritaire est donc :

```text
Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp
```

car TD07.4 vient de le caractériser en profondeur.

Baseline GitHub :

```text
GridActivationComponent.cpp
    1 542 lignes
    38 appels UE_LOG(LogTemp)
    LogGridRecruitmentOffer existe déjà pour le sous-domaine recrutement
```

La cible probable est une catégorie dédiée `LogGridActivation` pour les 38 logs génériques du composant, sans modifier `LogGridRecruitmentOffer`.

## 3. Règle formatting

Le contrat d'outil est déjà résolu :

```text
.clang-format
.editorconfig
.gitattributes
Scripts/FormatCpp.ps1
Scripts/CheckCppFormat.ps1
clang-format 19.1.5
```

La dette restante est uniquement la **baseline first-party globale**, historiquement signalée comme non entièrement verte.

Avant tout formatage :

1. exécuter le check global ;
2. obtenir la liste exacte des fichiers non conformes ;
3. ne formater que les fichiers nécessaires à la fermeture de la dette ;
4. vérifier le diff ;
5. ne toucher à aucun comportement.

## 4. Characterization audit

Script read-only :

```text
Scripts/AuditTD077Hygiene.ps1
```

Il :

- scanne les trois modules first-party ;
- compte les `UE_LOG(LogTemp)` par fichier ;
- mesure explicitement `GridActivationComponent.cpp` ;
- exécute `Scripts/CheckCppFormat.ps1` dans un sous-processus ;
- enregistre le résultat dans :
  `Saved/Diagnostics/TD07/TD07_7_HygieneAudit.txt` ;
- ne modifie aucun source.

## 5. Stop condition TD07.7

- [x] audit de caractérisation exécuté ;
- [x] liste exacte des violations clang-format connue ;
- [x] `GridActivationComponent.cpp` ne dépend plus de `LogTemp` ;
- [x] aucune substitution globale hors périmètre justifié ;
- [x] baseline `Scripts/CheckCppFormat.ps1` verte ;
- [ ] build Editor vert ;
- [ ] régressions ciblées Activation/Event->Command/Receptacle vertes ;
- [ ] script one-shot TD07.7 supprimé après usage.

Après TD07.7, la campagne passe à **TD07.8 — Future-proofing re-audit / stop condition**.


## 6. Characterization exécutée

Audit local du 28 août 2026 :

```text
First-party files scanned: 568
Files containing UE_LOG(LogTemp): 45
Total UE_LOG(LogTemp) calls: 452
GridActivationComponent UE_LOG(LogTemp): 38
```

Les autres `LogTemp` restent hors périmètre TD07.7 tant qu'aucun domaine n'est spécifiquement touché.

Le premier check format a confirmé une baseline non verte mais a aussi révélé un défaut du harness : sous Windows PowerShell 5, le stderr natif de `clang-format --dry-run --Werror` interrompait la boucle au premier fichier, `GridLevelAsset.cpp`.

`Scripts/CheckCppFormat.ps1` est donc corrigé pour :

- supprimer le stderr natif attendu de chaque dry-run ;
- utiliser uniquement l'exit code par fichier ;
- continuer sur les 568 fichiers ;
- produire une liste complète `[FORMAT] ...` ;
- retourner ensuite 1 si la baseline globale reste non conforme.

## 7. Log normalization ciblée

`GridActivationComponent.cpp` possède maintenant :

```cpp
DEFINE_LOG_CATEGORY_STATIC(LogGridActivation, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogGridRecruitmentOffer, Log, All);
```

Les 38 appels génériques utilisent désormais `LogGridActivation`.

`LogGridRecruitmentOffer` reste inchangé pour son sous-domaine dédié.

Aucun autre fichier contenant `LogTemp` n'est modifié par cette étape.


## 8. Baseline formatting complète

Le second audit local a parcouru les **568 fichiers C++ first-party** et produit la liste complète :

```text
126 fichiers non conformes à Grimrock C++ Style v1
CheckCppFormat exit code : 1
clang-format              : 19.1.5
```

Cette dérive est historique et transverse : Core, Magic, Quest, RPG, Runtime, tests et quelques headers publics sont concernés.

À ce stade, corriger fichier par fichier n'apporte aucune valeur et accroît le risque d'oubli. La bonne opération est une normalisation mécanique unique via l'outil autoritaire existant.

## 9. Normalisation mécanique préparée

Script one-shot :

```text
Scripts/ApplyTD077FormatBaseline.ps1
```

Garanties :

1. exige la branche locale `master` ;
2. exige un working tree tracked propre ;
3. exige `HEAD == origin/master` ;
4. relance `CheckCppFormat.ps1` avant toute mutation ;
5. exige exactement **126** fichiers non conformes ;
6. exécute `FormatCpp.ps1` avec clang-format 19.1.5 ;
7. exige que les fichiers modifiés soient exactement les fichiers signalés avant formatage ;
8. exige ensuite une baseline globale `CheckCppFormat.ps1 = 0` ;
9. exécute `git diff --check` ;
10. committe et pousse un unique commit mécanique ;
11. restaure les changements si le périmètre formaté diffère de la baseline caractérisée.

Aucun changement comportemental n'est attendu.


## 10. Normalisation format exécutée

Validation locale du 28 août 2026 :

```text
Pre-format:
    568 fichiers scannés
    126 non conformes

FormatCpp.ps1:
    clang-format 19.1.5
    126 fichiers modifiés exactement

Post-format:
    568 fichiers scannés
    Format C++ conforme

Commit:
    dc24867191252821ad013e9c868b85b3a3174344
    Normalize TD07.7 first-party C++ formatting
```

Le script de garde-fou a confirmé que les fichiers modifiés correspondaient exactement aux 126 fichiers caractérisés avant formatage.

TD07.7 n'est pas encore clos : il reste la validation Editor/Automation après cette normalisation mécanique.


## 11. Final single-file format gate

Après l'adaptation du test TD07.4 au nouveau contrat de logs, le contrôle global a signalé un unique écart :

```text
Source/GrimrockPrototype/Private/Tests/GridTD074ActivationComponentCharacterizationTests.cpp
```

Les validations UE sont déjà vertes :

```text
TD07.4 Characterization                    4 Success / 0 warning / 0 Failed
TD01.3 EventCommand RuntimeHardening       1 Success / 0 warning / 0 Failed
TD07.5 ReceptacleCommands                  1 Success / 0 warning / 0 Failed
```

Le script `Scripts/FinalizeTD077Format.ps1` délègue maintenant le dernier changement au clang-format 19.1.5 autoritaire et refuse toute modification hors de ce seul fichier.
