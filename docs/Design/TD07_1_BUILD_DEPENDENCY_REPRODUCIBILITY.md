# TD07.1 — Build / Dependency Reproducibility

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Baseline GitHub : `b15330c7bbcae2b2d8f45a5bf94ee8f6d05bea5f`  
Statut : **VALIDÉ — TD-BUILD-001 RÉSOLU**

## 1. Objet

TD07.1 ouvre la passe de future-proofing après les stop conditions TD05/TD06.

Le premier risque traité est la reproductibilité d'un clone propre : le projet versionné activait Meshy alors que tout `/Plugins/` était ignoré par Git et qu'aucun fichier Meshy n'existe sur `master`.

## 2. Constat avant TD07.1

`GrimrockPrototype.uproject` contenait :

```text
meshy
Enabled = true
SupportedTargetPlatforms = Win64, Mac
```

mais `.gitignore` contenait :

```text
/Plugins/
```

Le repository ne versionne aucun fichier sous `Plugins/`.

Les recherches first-party ne montrent aucune dépendance C++, module ou API à Meshy. Les seules références versionnées étaient le descripteur `.uproject` et une mention de production d'assets dans la documentation d'équipement.

Les builds locaux affichaient en outre plusieurs warnings provenant du `meshy.Build.cs` installé localement, avec des chemins Editor UE inexistants.

## 3. Décision

Meshy est **un outil de développement optionnel**, pas une dépendance du projet.

Le contrat versionné devient :

```text
Name     = meshy
Enabled  = false
Optional = true
```

Conséquences :

- un clone propre n'a pas besoin de Meshy ;
- Meshy n'est plus chargé/compilé par défaut ;
- un développeur peut l'installer localement pour une tranche de production d'assets ;
- le plugin lui-même n'est pas versionné ;
- les assets livrés ne doivent pas dépendre de classes/modules/assets Meshy.

## 4. Politique Git des plugins

Avant TD07.1 :

```text
/Plugins/
```

Après TD07.1 :

```text
/Plugins/meshy/
/Plugins/**/Binaries/
/Plugins/**/Intermediate/
/Plugins/**/Saved/
```

Cela permet de versionner un futur plugin first-party sans embarquer ses sorties générées.

## 5. Contrôle de dépendances

Nouveau script :

```text
Scripts/CheckProjectDependencies.ps1
```

Il :

1. lit `GrimrockPrototype.uproject` ;
2. examine les références de plugins ;
3. vérifie que chaque plugin **Enabled=true** existe dans `<Repo>/Plugins` ou `<EngineRoot>/Engine/Plugins` ;
4. accepte explicitement un plugin `Enabled=false, Optional=true` absent ;
5. signale si Meshy est présent localement mais désactivé.

Ce contrôle reste séparé de `ValidateUE.ps1` afin de ne pas modifier le contrat TD04.2 déjà validé.

## 6. Environnement de développement

Nouveau document :

```text
docs/Design/DEVELOPMENT_ENVIRONMENT_SETUP.md
```

Il fixe :

- UE5.5.4 ;
- Visual Studio 2022 ;
- utilisation de `-EngineRoot` / `UE_ROOT` ;
- politique des plugins optionnels ;
- procédure Meshy pour un tiers ;
- toolchain MSVC observée ;
- harness Editor / Automation / Shipping.

## 7. Toolchain observée

Le build du 26 août 2026 utilisait :

```text
Visual Studio 2022
MSVC 14.44.35227
Windows SDK 10.0.26100.0
```

UBT avertit que `14.44.35227` n'est pas sa version préférée et cite `14.38.33130`.

TD07.1 ne force pas de downgrade : les builds Editor sont actuellement verts. Le risque devient **surveillé** et doit être réévalué si la toolchain change ou si Shipping échoue.

## 8. Fichiers modifiés

```text
GrimrockPrototype.uproject
.gitignore
README.md
Scripts/CheckProjectDependencies.ps1
docs/Design/DEVELOPMENT_ENVIRONMENT_SETUP.md
docs/Design/TD07_1_BUILD_DEPENDENCY_REPRODUCIBILITY.md
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
```

Aucun C++, Blueprint, DataAsset, map ou SaveGame n'est modifié.

## 9. Validation réelle

Validation locale fournie le **27 août 2026** :

```text
CheckProjectDependencies.ps1
    Enabled plugins validated : 1
    Optional disabled plugins : 1
    [OK] Project dependency contract validated.

Grimrock.TechnicalDebt.TD06_8
    Succeeded              : 1
    Succeeded with warnings: 0
    Failed                 : 0
    Not run                : 0

Win64 Shipping
    BUILD SUCCESSFUL
    Pak files     : 1
    Archive files : 41
    Archive bytes : 906089915
```

Le contrôle de dépendances résout correctement :

```text
ModelingToolsEditorMode -> Engine/Plugins/Editor/ModelingToolsEditorMode
meshy                  -> optional disabled, installé localement mais non requis
```

Le build/cook/package ne charge plus Meshy et les warnings provenant de `Plugins/meshy/Source/meshy/meshy.Build.cs` ont disparu.

Le cook expose encore un warning first-party distinct, concernant le nom Python de `EGridItemTransferResult` / `FGridItemTransferResult`. Il est transféré à TD07.2.

## 10. Stop condition

Les trois validations sont vertes. **TD07.1 est clos et TD-BUILD-001 est résolu.**

Meshy reste un outil local optionnel. Toute future dépendance dure à Meshy doit rouvrir TD-BUILD-001.

La prochaine tranche de future-proofing est :

```text
TD07.2 — UE deprecation cleanup / compiler warning audit
```
