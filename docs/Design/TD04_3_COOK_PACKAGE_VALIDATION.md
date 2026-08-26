# TD04.3 — Cook / Package Validation

Date : 26 août 2026  
Projet : GrimrockPrototype — Unreal Engine 5.5.4  
Statut : **TERMINÉ / VALIDÉ UE5.5.4 — SHIPPING**

## 1. Objectif

TD04.3 ajoute un harness distinct pour prouver qu'un commit peut produire un build jeu Win64 réellement cooké et packagé.

Nouveau script :

```text
Scripts/ValidatePackage.ps1
```

Cette responsabilité reste séparée de `Scripts/ValidateUE.ps1` :

```text
ValidateUE.ps1       -> Development Editor build + Automation
ValidatePackage.ps1  -> Game target Win64 build + cook + stage + package + archive
```

Un succès Editor/Automation ne vaut donc pas succès Shipping, et inversement.

## 2. Cible réelle du projet

La cible runtime existante est :

```text
Source/GrimrockPrototype.Target.cs
TargetType.Game
ExtraModuleNames.Add("GrimrockPrototype")
```

TD04.3 ne crée pas de nouvelle Target Unreal et ne modifie aucun `.uasset/.umap`.

Le profil de référence est :

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
```

`Development` reste disponible explicitement pour diagnostic, mais ne remplace pas la validation Shipping.

## 3. Pipeline UAT

Le harness utilise :

```text
<EngineRoot>\Engine\Build\BatchFiles\RunUAT.bat
```

avec `BuildCookRun` :

```text
Build
Cook
Stage
Package
Archive
```

Commande logique :

```text
RunUAT.bat BuildCookRun
    -project=<RepoRoot>\GrimrockPrototype.uproject
    -noP4
    -utf8output
    -platform=Win64
    -target=GrimrockPrototype
    -clientconfig=Shipping
    -build
    -cook
    -stage
    -package
    -pak
    -archive
    -archivedirectory=<session>
```

## 4. Résolution portable de l'environnement

La racine UE est fournie par :

```powershell
-EngineRoot <chemin>
```

ou par `UE_ROOT`. La racine projet est déduite de l'emplacement du script.

La sortie par défaut est :

```text
Saved\Packaging\TD04\TD04-Shipping-yyyyMMdd-HHmmss\
```

`Saved/` étant ignoré par Git, les archives de validation ne polluent pas le dépôt.

## 5. Critères de succès

Le script exige :

```text
1. code UAT = 0 ;
2. présence d'un GrimrockPrototype.exe dans l'archive ;
3. présence d'au moins un fichier .pak ;
4. archive contenant réellement des fichiers.
```

En cas d'échec :

```text
UAT non nul       -> échec immédiat
exécutable absent -> exit 2
aucun .pak        -> exit 3
```

## 6. Configuration projet observée

Le projet définit actuellement :

```text
GameDefaultMap=/Game/GrimrockPrototype/Maps/L_MainMenu.L_MainMenu
GlobalDefaultGameMode=/Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockGameMode.BP_GrimrockGameMode_C
GameInstanceClass=/Game/GrimrockPrototype/Blueprints/System/BP_GrimrockGameInstance.BP_GrimrockGameInstance_C
```

TD04.3 ne force pas `-allmaps` et ne modifie pas automatiquement les Packaging Settings. Le cook exerce les références/configurations réelles du projet.

## 7. Validation réelle du 26 août 2026

Commande exécutée sur l'environnement de développement UE5.5.4 :

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Résultat communiqué après exécution réelle :

```text
=== Package summary ===
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : D:\Development\GrimrockPrototype\Saved\Packaging\TD04\TD04-Shipping-20260826-141330\Windows\GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 905582948
Archive       : D:\Development\GrimrockPrototype\Saved\Packaging\TD04\TD04-Shipping-20260826-141330
[OK] Cook / package validated.

TD04.3 validation completed successfully.
```

La stop condition est donc satisfaite :

```text
BuildCookRun              Success
Win64 Shipping executable présent
.pak                       présent
archive                    non vide
code final                 0
```

## 8. Commits

```text
4722e3d3d77d32a9722aa075dfce2f00823a8d35  Add TD04.3 cook package validation harness
```

## 9. Non-objectifs

TD04.3 ne couvre pas :

- lancement automatisé du jeu packagé ;
- test visuel du menu ou d'une partie ;
- signature/installer ;
- distribution ;
- GitHub Actions ;
- installation automatique des prérequis UE/Visual Studio.

Ces points ne doivent pas être confondus avec la preuve désormais acquise de `build + cook + stage + package + archive` Shipping.

## 10. Décision après validation

TD04.3 est **TERMINÉ / VALIDÉ UE5.5.4**.

TD04.4 — CI UE réelle reste conditionnel. Aucun workflow ne doit être créé tant qu'un runner capable d'exécuter réellement UE5.5.4, `Scripts/ValidateUE.ps1` et `Scripts/ValidatePackage.ps1` n'est pas provisionné et vérifiable.

En l'absence d'un tel runner, la bonne stop condition est de conserver les deux harness locaux versionnés comme autorité reproductible et de reprendre la roadmap produit plutôt que d'introduire une pseudo-CI.