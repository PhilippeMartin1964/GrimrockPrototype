# TD07.7 — Targeted Log / Formatting Hygiene

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Dettes : TD-LOG-001 / TD-STYLE-001
Statut : CHARACTERIZATION PREPARED — À VALIDER

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

- [ ] audit de caractérisation exécuté ;
- [ ] liste exacte des violations clang-format connue ;
- [ ] `GridActivationComponent.cpp` ne dépend plus de `LogTemp` ;
- [ ] aucune substitution globale hors périmètre justifié ;
- [ ] baseline `Scripts/CheckCppFormat.ps1` verte ;
- [ ] build Editor vert ;
- [ ] régressions ciblées Activation/Event->Command/Receptacle vertes ;
- [ ] script one-shot TD07.7 supprimé après usage.

Après TD07.7, la campagne passe à **TD07.8 — Future-proofing re-audit / stop condition**.
