# TD07.1 — Build / Dependency Reproducibility

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Baseline GitHub : `b15330c7bbcae2b2d8f45a5bf94ee8f6d05bea5f`  
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE REQUISE**

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

## 9. Validation requise

Après `git pull` :

```powershell
.\Scripts\CheckProjectDependencies.ps1 -EngineRoot D:\UE_5.5
```

Résultat attendu :

```text
ModelingToolsEditorMode  -> trouvé dans Engine/Plugins
meshy                    -> optional disabled ; présent ou absent accepté
[OK] Project dependency contract validated.
```

Puis build/Automation :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_8"
```

Enfin, pour prouver que Meshy n'est pas requis par le produit Shipping :

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Critères TD07.1 :

```text
Dependency check = success
Editor build      = success
Automation TD06_8 = 1 Success / 0 warning / 0 Failed
Shipping package  = success
```

Les warnings Meshy précédemment observés ne doivent plus apparaître puisque le plugin est désactivé par défaut.

## 10. Stop condition

TD07.1 est clos lorsque les trois validations ci-dessus sont vertes.

Le plugin Meshy ne doit alors plus être classé comme dette bloquante. La prochaine tranche de future-proofing devient :

```text
TD07.2 — UE deprecation cleanup / compiler warning audit
```
