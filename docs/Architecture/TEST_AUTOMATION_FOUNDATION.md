# Tests Automation et validation — Fondation d’architecture

> **DOC-ARCH01 — REVALIDÉ LE 9 OCTOBRE 2026.** Comparé à la baseline C++ `6f98a0ef`. Ce document reste une fondation courante. Le harness local reste l'autorité de validation exécutée par l'utilisateur.

Date de référence : **27 août 2026**

## Niveaux de validation

1. **Tests unitaires/contractuels C++** pour données et services purs.
2. **Tests runtime avec UWorld léger** pour intégrations gameplay.
3. **Tests Editor-only** pour authoring et validation du Grid Editor.
4. **PIE** lorsque le comportement dépend de Blueprints, meshes, UMG ou DataAssets binaires.
5. **Build UE5.5.4 réel** avant de déclarer une tranche compilée.
6. **Cook/package Shipping réel** lorsque la distribution est concernée.

## Organisation

Les suites historiques portent généralement le jalon dans leur chemin (`Grimrock.MONxx...`). Les campagnes de dette utilisent `Grimrock.TechnicalDebt.TDxx...`.

Les nouveaux helpers de tests doivent être nommés de façon globalement unique. Unreal peut regrouper plusieurs `.cpp` dans une Unity Build ; un namespace anonyme ne protège pas contre les collisions entre fichiers réunis dans la même unité de traduction.

## Harness local Editor + Automation

TD04.2 a ajouté et validé :

```text
Scripts/ValidateUE.ps1
```

Le script :

- résout la racine projet depuis son emplacement ;
- reçoit la racine UE via `-EngineRoot` ou `UE_ROOT` ;
- construit `GrimrockPrototypeEditor Win64 Development` ;
- lance un filtre Automation explicite via `UnrealEditor-Cmd.exe` ;
- exporte et lit `index.json` ;
- échoue si aucun test n’est exécuté ou si `failed > 0`.
- redirige par défaut la sortie brute de `UnrealEditor-Cmd.exe` vers `Automation.console.log` ;
- conserve le log UE complet dans `Automation.log` ;
- génère `Automation.summary.txt` avec les compteurs et, en cas d’échec/warning, les diagnostics `LogAutomationController` pertinents ;
- affiche ce même résumé compact dans le terminal ;
- accepte `-ShowAutomationOutput` pour restaurer ponctuellement la sortie Unreal temps réel.

Validation réelle TD04.2 :

```text
Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

## Harness local Shipping

TD04.3 a ajouté et validé :

```text
Scripts/ValidatePackage.ps1
```

Le script exécute `RunUAT.bat BuildCookRun` avec :

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Build + Cook + Stage + Package + Pak + Archive
```

Validation réelle TD04.3 :

```text
Pak files     : 1
Archive files : 41
Archive bytes : 905582948
[OK] Cook / package validated.
```

## Warnings intentionnels

Certains tests négatifs provoquent volontairement des warnings ou errors attendus puis terminent en `Success`. Le résultat Automation et les assertions du contrat, pas la seule présence d’un message de log, décident du verdict.

## Règles de projet

- ne pas annoncer compilation/test UE comme validé sans résultat réellement exécuté ;
- ne pas modifier un `.uasset/.umap` pour un test C++ si une fixture transiente suffit ;
- un sous-jalon = un commit logique autant que possible ;
- caractériser avant extraction/changement structurel ;
- après caractérisation, changement de production + adaptation du test = même commit logique ;
- après une évolution transversale, lancer la suite ciblée puis la régression du domaine ;
- PIE reste requis quand Automation ne couvre pas fidèlement le contrat visuel/asset.

## Manques actuels

- **CI distante UE5.5.4** : différée jusqu’à disponibilité d’un vrai runner Windows + UE5.5.4 + toolchain ;
- scénarios de campagne longs ;
- validation automatisée exhaustive des assets binaires.

La validation locale build/Automation et la validation Win64 Shipping ne sont plus des manques : elles sont versionnées et ont été exécutées avec succès.


## Rebaseline DOC-ARCH01 — validations globales

La dernière campagne **globale** reçue reste :

```text
Runtime/content baseline : 9045ef2db75c09997db4fc65dbf99d4598f4df5c
Grimrock                 : 1026/1026
Warnings                 : 0
Failed                   : 0
Not run                  : 0
Win64 Shipping           : VALIDÉ
Cook                     : 0 error / 0 warning
```

Les modifications postérieures disposent de validations ciblées. Une
documentation-only rebaseline ne transforme pas ces validations ciblées en
nouvelle campagne globale.
