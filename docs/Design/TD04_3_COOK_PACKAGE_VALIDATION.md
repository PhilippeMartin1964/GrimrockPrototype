# TD04.3 — Cook / Package Validation

Date : 26 août 2026  
Projet : GrimrockPrototype — Unreal Engine 5.5.4  
Statut : **IMPLÉMENTÉ — VALIDATION LOCALE À EFFECTUER**

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

Le profil par défaut du harness est :

```text
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
```

`Development` reste disponible explicitement pour diagnostic, mais la validation de référence TD04.3 est Shipping.

## 3. Pipeline UAT

Le harness utilise :

```text
<EngineRoot>\Engine\Build\BatchFiles\RunUAT.bat
```

avec le pipeline Unreal Automation Tool `BuildCookRun` :

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

Ce découpage correspond au pipeline Build/Cook/Stage/Package documenté par Unreal Automation Tool.

## 4. Résolution portable de l'environnement

Comme TD04.2, le script ne contient aucun chemin machine obligatoire.

La racine UE est fournie par :

```powershell
-EngineRoot <chemin>
```

ou :

```text
UE_ROOT
```

La racine projet est déduite de l'emplacement du script.

La sortie par défaut est :

```text
Saved\Packaging\TD04\TD04-Shipping-yyyyMMdd-HHmmss\
```

`Saved/` étant déjà ignoré par Git, les archives de validation ne polluent pas le dépôt.

Un autre dossier peut être fourni avec :

```powershell
-ArchiveRoot <chemin>
```

## 5. Critères de succès

TD04.3 ne se contente pas du code retour UAT.

Après `BuildCookRun`, le script exige aussi :

```text
1. code UAT = 0 ;
2. présence d'un GrimrockPrototype.exe dans l'archive ;
3. présence d'au moins un fichier .pak ;
4. archive contenant réellement des fichiers.
```

En cas d'échec :

```text
UAT non nul              -> échec immédiat
exécutable absent        -> exit 2
aucun .pak               -> exit 3
```

Le résumé final affiche notamment :

```text
Target
Platform
Configuration
Executable
Pak files
Archive files
Archive bytes
Archive path
```

## 6. Configuration projet observée

Le projet définit actuellement :

```text
GameDefaultMap=/Game/GrimrockPrototype/Maps/L_MainMenu.L_MainMenu
GlobalDefaultGameMode=/Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockGameMode.BP_GrimrockGameMode_C
GameInstanceClass=/Game/GrimrockPrototype/Blueprints/System/BP_GrimrockGameInstance.BP_GrimrockGameInstance_C
```

TD04.3 ne force pas une liste artificielle de maps dans le script. Le cook doit exercer les références/configurations réelles du projet et révéler les assets ou maps insuffisamment inclus plutôt que masquer le problème par `-allmaps`.

## 7. Commande de validation TD04.3

Fermer Unreal Editor avant le test, puis :

```powershell
cd D:\Development\GrimrockPrototype
git pull origin master

.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5
```

Résultat attendu :

```text
=== UE5.5.4 Win64 Shipping BuildCookRun ===
...

=== Package summary ===
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Executable    : <...>\GrimrockPrototype.exe
Pak files     : >= 1
Archive files : > 0
Archive bytes : > 0
Archive       : <...>\Saved\Packaging\TD04\TD04-Shipping-...
[OK] Cook / package validated.

TD04.3 validation completed successfully.
```

## 8. Diagnostic Development optionnel

Si Shipping échoue et qu'il faut distinguer un problème général de packaging d'un problème propre à Shipping :

```powershell
.\Scripts\ValidatePackage.ps1 `
    -EngineRoot D:\UE_5.5 `
    -Configuration Development
```

Ce passage Development est un diagnostic ; il ne remplace pas la validation Shipping demandée par TD04.3.

## 9. Non-objectifs

TD04.3 ne fait pas encore :

- lancement automatisé du jeu packagé ;
- test visuel du menu ou d'une partie ;
- signature/installer ;
- distribution ;
- GitHub Actions ;
- installation automatique des prérequis UE/Visual Studio ;
- modification automatique des Packaging Settings.

Toute erreur de cook liée à une référence asset/map manquante doit d'abord être comprise comme un résultat de validation, pas contournée automatiquement.

## 10. Stop condition

TD04.3 sera marqué **RÉALISÉ / VALIDÉ UE5.5.4** uniquement après une exécution réelle du harness en `Shipping` avec :

```text
BuildCookRun = succès
GrimrockPrototype.exe = présent
.pak = présent
archive non vide
code final = 0
```

Après cette validation, TD04 pourra décider s'il est rentable de poursuivre vers une vraie CI UE (`TD04.4`) ou si l'absence de runner UE5.5.4 impose une stop condition locale.
