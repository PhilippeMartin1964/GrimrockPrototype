# TD04.2 — Local UE Validation Harness

Date : 26 août 2026  
Projet : GrimrockPrototype — Unreal Engine 5.5.4  
Statut : **RÉALISÉ / VALIDÉ UE5.5.4**

## 1. Objectif

TD04.2 transforme la procédure locale de validation Unreal en un harness versionné et reproductible, sans introduire encore de CI distante.

Nouveau script :

```text
Scripts/ValidateUE.ps1
```

Le script automatise deux responsabilités indépendantes :

```text
1. build GrimrockPrototypeEditor Win64 Development ;
2. exécution d'un filtre Automation explicite via UnrealEditor-Cmd.exe.
```

Il ne remplace pas PIE lorsqu'un jalon touche une présentation, un asset, un Blueprint/UMG binding ou un workflow visuel.

## 2. Résolution de l'environnement

Le chemin du projet n'est pas codé en dur : il est déduit de l'emplacement du script.

Le moteur doit être fourni par :

```text
-EngineRoot <chemin>
```

ou par la variable d'environnement :

```text
UE_ROOT
```

Le script refuse explicitement de poursuivre si les fichiers suivants n'existent pas :

```text
<EngineRoot>\Engine\Build\BatchFiles\Build.bat
<EngineRoot>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe
<RepoRoot>\GrimrockPrototype.uproject
```

Aucun emplacement machine tel que `D:\Development\GrimrockPrototype` ou `D:\UE_5.5` n'est intégré au script.

## 3. Build

Commande logique exécutée :

```text
Build.bat GrimrockPrototypeEditor Win64 Development
          -Project=<RepoRoot>\GrimrockPrototype.uproject
          -WaitMutex
          -NoHotReloadFromIDE
```

Toute sortie native non nulle interrompt le harness avec une erreur.

Le build peut être ignoré explicitement avec :

```powershell
-SkipBuild
```

## 4. Automation

Sauf `-SkipAutomation`, le filtre est obligatoire :

```powershell
-AutomationFilter "Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract"
```

Le lancement suit le contrat Automation en ligne de commande d'Unreal : un Editor/Client reçoit une commande `Automation RunTest ...;Quit`, avec export de rapport.

Le harness lance notamment :

```text
UnrealEditor-Cmd.exe <uproject>
    -Unattended
    -NoSplash
    -NoP4
    -NoSound
    -ExecCmds="Automation RunTest <filter>;Quit"
    -ReportExportPath=<session>
    -log
    -abslog=<session>\Automation.log
    -NullRHI
```

`-NullRHI` est utilisé par défaut pour les tests ne nécessitant pas de rendu. Pour un test déclaré ou connu comme dépendant d'un RHI réel :

```powershell
-UseRHI
```

## 5. Détection du résultat

Le script ne considère pas le simple retour du processus Editor comme une preuve suffisante.

Après l'exécution, il exige :

```text
<ReportExportPath>\index.json
```

Puis il lit :

```text
succeeded
succeededWithWarnings
failed
notRun
```

Règles :

```text
processus natif non nul -> échec
index.json absent        -> échec
0 test exécuté           -> échec
failed > 0               -> échec
sinon                    -> succès
```

Les rapports sont stockés hors source, sous :

```text
Saved\Automation\TD04\TD04-yyyyMMdd-HHmmss\
```

par défaut.

## 6. Validation réelle du 26 août 2026

Commande exécutée sur la machine de développement UE5.5.4 :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract"
```

Résultat transmis par la validation locale :

```text
=== Automation summary ===
Filter                 : Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260826-133532
Log                    : D:\Development\GrimrockPrototype\Saved\Automation\TD04\TD04-20260826-133532\Automation.log
[OK] Automation filter validated.

TD04.2 validation completed successfully.
```

Le passage complet valide donc :

```text
Development Editor build : OK
Automation exécutée       : 1
Automation échouée        : 0
rapport index.json        : présent et interprété
code final harness        : 0
```

Cette validation constitue la preuve de fonctionnement réelle du harness TD04.2 sous UE5.5.4.

## 7. Utilisations courantes

### Build seul

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
```

### Automation seule après build déjà validé

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract"
```

### Variable d'environnement

```powershell
$env:UE_ROOT = 'D:\UE_5.5'
.\Scripts\ValidateUE.ps1 -AutomationFilter "Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract"
```

## 8. Non-objectifs

TD04.2 ne fait pas :

- cook/package ;
- smoke test d'un build packagé ;
- GitHub Actions ;
- installation automatique d'Unreal Engine ;
- remplacement des validations PIE ;
- exécution implicite de toutes les Automation du projet.

Le filtre reste explicite afin d'éviter qu'un script de validation devienne un second catalogue de tests ou lance une suite très large sans intention.

## 9. Stop condition

La stop condition TD04.2 est atteinte :

- harness versionné ;
- résolution portable du moteur/projet ;
- build Development Editor réellement exécuté ;
- Automation réellement exécutée ;
- rapport exporté et interprété ;
- code final de succès obtenu sous UE5.5.4.

**Décision : TD04.2 est RÉALISÉ / VALIDÉ. La suite est TD04.3 — Cook / Package validation.**
