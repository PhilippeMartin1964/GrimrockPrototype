# Référence des scripts PowerShell du prototype

Audit du **21 septembre 2026**, sur `master`, commit de référence `4b6f974390f0bc6de0982af898a5e49fb877519b` (UI-ITEM01). Cette référence décrit les fichiers versionnés dans `Scripts/`, correspondant à `D:\Development\GrimrockPrototype\Scripts` après synchronisation du dépôt. Les éventuels scripts locaux non versionnés ne font pas partie de cet audit.

## Résultat de l’audit documentaire

Il existait des explications utiles, mais **pas de référence complète et centralisée des six scripts**. Les guides TD04.2, TD04.3, TD07.1, STYLE01 et MIG08 documentaient leurs jalons respectifs. Il manquait notamment un inventaire actuel, des tableaux exhaustifs des paramètres, un dépannage commun et une distinction explicite entre outils courants et migration historique.

L’audit a lu intégralement les six `.ps1` et vérifié leurs références dans `Source/` et la documentation. Il révèle deux limites à connaître :

- `MigrateWorldObjectAssets.ps1` existe encore, mais le commandlet `GridWorldObjectMIG08` qu’il appelle n’existe plus dans le code source courant. Ce lanceur ne constitue donc pas une procédure utilisable sur un build propre de cette révision.
- Certains codes d’échec annoncés dans les anciens guides ne sont pas garantis : plusieurs branches appellent `Write-Error` avec `$ErrorActionPreference = 'Stop'` avant leur instruction `exit`. Voir [résultats et codes de sortie](#resultats-et-codes-de-sortie).

Cette mise à jour porte uniquement sur la documentation. Elle ne réactive pas la migration et ne change pas le comportement des scripts.

## Sommaire

- [Choisir le bon script](#choisir-le-bon-script)
- [Préparer PowerShell et les chemins](#preparer-powershell-et-les-chemins)
- [CheckProjectDependencies.ps1](#checkprojectdependenciesps1)
- [ValidateUE.ps1](#validateueps1)
- [ValidatePackage.ps1](#validatepackageps1)
- [CheckCppFormat.ps1 et FormatCpp.ps1](#checkcppformatps1-et-formatcppps1)
- [MigrateWorldObjectAssets.ps1](#migrateworldobjectassetsps1)
- [Résultats et codes de sortie](#resultats-et-codes-de-sortie)
- [Procédures usuelles](#procedures-usuelles)
- [Dépannage](#depannage)
- [Documentation associée et maintenance](#documentation-associee-et-maintenance)

<a id="choisir-le-bon-script"></a>
## Choisir le bon script

| Script | Utilisation | Écritures et portée |
| --- | --- | --- |
| `CheckProjectDependencies.ps1` | Vérifier la présence des plugins déclarés dans le `.uproject` | Lecture seule ; n’installe aucun plugin |
| `ValidateUE.ps1` | Compiler `GrimrockPrototypeEditor` en Win64 Development, puis exécuter un filtre Automation | Produits de compilation et rapports ; les effets propres aux tests sélectionnés dépendent de ces tests |
| `ValidatePackage.ps1` | Produire le jeu Win64, en Shipping par défaut | Produits de build/cook/stage et archive complète du jeu |
| `CheckCppFormat.ps1` | Contrôler la conformité à `.clang-format` | Lecture seule des sources |
| `FormatCpp.ps1` | Appliquer `.clang-format` | Réécriture en place de tous les fichiers C++ admissibles, pas seulement des fichiers modifiés dans Git |
| `MigrateWorldObjectAssets.ps1` | Ancien lanceur WORLDOBJ-MIG08 | Historique, indisponible avec le code courant ; ancien mode `-Apply` destiné à réenregistrer des assets |

Ce dossier contient des outils de développement Windows. Il ne correspond pas aux scripts Lua de gameplay et n’a pas à être copié à côté de l’exécutable distribué.

<a id="preparer-powershell-et-les-chemins"></a>
## Préparer PowerShell et les chemins

Ouvrir **PowerShell** ou un terminal PowerShell dans Visual Studio, puis :

```powershell
Set-Location 'D:\Development\GrimrockPrototype'
git status --short
$env:UE_ROOT = 'D:\UE_5.5'
```

`UE_ROOT` désigne le dossier **contenant** `Engine`, donc `D:\UE_5.5`, pas `D:\UE_5.5\Engine`. Cette affectation vaut pour la session PowerShell courante et ses processus enfants. Vous pouvez aussi fournir `-EngineRoot 'D:\UE_5.5'` à chaque appel ; ce paramètre remplace la valeur de `UE_ROOT` pour cet appel.

Tous les scripts déduisent la racine du dépôt du dossier parent de leur propre emplacement (`$PSScriptRoot`). Ils ciblent `GrimrockPrototype.uproject` lorsque nécessaire ; aucun paramètre `-Project` n’est exposé. Les exemples supposent le terminal placé à la racine. Utiliser des chemins absolus entre apostrophes pour les emplacements contenant des espaces et pour `-ReportRoot`/`-ArchiveRoot`.

Prérequis selon l’opération :

- build, Automation et packaging : installation UE **5.5.4**, outils de compilation C++/Windows SDK compatibles avec cet environnement, sources et contenu du projet disponibles ;
- contrôle des dépendances : installation UE et `.uproject` disponibles ; pas de compilation effectuée ;
- formatage : **clang-format 19.1.5**, `.clang-format` du dépôt et les trois dossiers `Source/GrimrockPrototype`, `Source/GrimrockPrototypeEditor`, `Source/GrimrockLua` ; UE n’est pas nécessaire ;
- Git : nécessaire aux commandes de suivi proposées ici ; le lanceur de migration l’appelle aussi après `-Apply`.

Les scripts ne vérifient pas eux-mêmes le numéro exact de version d’Unreal. Ils ne sélectionnent pas une version de Visual Studio pour UBT. Voir le [guide d’environnement](Design/DEVELOPMENT_ENVIRONMENT_SETUP.md) pour les validations historiques de la toolchain. La détection de **clang-format**, elle, cible explicitement VS 2022 : la marche à suivre pour une installation VS 2026 est précisée plus bas.

Avant une compilation complète ou un packaging, fermer l’éditeur et les sessions de jeu utilisant les binaires concernés, et attendre la fin des autres compilations. Cela évite les verrouillages de DLL et les conflits avec Live Coding.

Les options `[switch]` s’écrivent simplement `-SkipBuild`, sans ajouter `true`. Dans les exemples multilignes, le caractère de continuation PowerShell est l’accent grave `` ` `` et doit être le **dernier caractère de la ligne**, sans espace après. Aucun des six scripts ne contient actuellement une aide détaillée intégrée de type `Get-Help -Full` ; ce guide fournit leur mode d’emploi.

<a id="checkprojectdependenciesps1"></a>
## CheckProjectDependencies.ps1

**But :** contrôler les plugins activés dans `GrimrockPrototype.uproject`, avant d’ouvrir ou de compiler le projet sur une machine.

| Paramètre | Type | Défaut | Explication |
| --- | --- | --- | --- |
| `-EngineRoot` | chaîne | `$env:UE_ROOT` | Racine d’une installation UE ; obligatoire en pratique si `UE_ROOT` est vide |

```powershell
.\Scripts\CheckProjectDependencies.ps1 -EngineRoot 'D:\UE_5.5'
```

Pour chaque plugin activé, le script cherche d’abord `Plugins\<Nom>\<Nom>.uplugin` dans le projet, puis cherche récursivement `<Nom>.uplugin` dans `<EngineRoot>\Engine\Plugins`. Un descripteur introuvable provoque une erreur bloquante. Le chemin trouvé est affiché avec `[OK]`.

Les plugins explicitement désactivés **et** optionnels sont seulement signalés comme installés localement ou absents. Au moment de l’audit, `ModelingToolsEditorMode` est activé et `meshy` est désactivé et optionnel. L’absence de Meshy est donc autorisée.

Le succès se termine par `[OK] Project dependency contract validated.` et `exit 0`. Aucun rapport fichier n’est créé par ce script.

**Limites :** ce contrôle prouve la présence du descripteur, pas la compatibilité de version, la compilation des modules, la validité des dépendances transitives ou l’intégrité des assets. Pour les plugins projet, seule la disposition exacte `Plugins\<Nom>\<Nom>.uplugin` est recherchée ; un plugin rangé dans un sous-dossier supplémentaire peut ne pas être trouvé. Ce contrôle n’est pas lancé automatiquement par `ValidateUE.ps1` ou `ValidatePackage.ps1`.

<a id="validateueps1"></a>
## ValidateUE.ps1

**But :** compiler l’éditeur et exécuter des tests Automation ciblés. Par défaut, les deux étapes sont prévues : le filtre est donc obligatoire.

| Paramètre | Type | Défaut | Explication |
| --- | --- | --- | --- |
| `-EngineRoot` | chaîne | `$env:UE_ROOT` | Racine UE |
| `-AutomationFilter` | chaîne | Aucun | Nom ou préfixe de tests ; obligatoire sauf avec `-SkipAutomation` |
| `-ReportRoot` | chaîne | `<Repo>\Saved\Automation\TD04` | Dossier parent des rapports ; un sous-dossier horodaté est ajouté |
| `-SkipBuild` | switch | Désactivé | Ne recompile pas ; utilise les binaires Editor déjà disponibles |
| `-SkipAutomation` | switch | Désactivé | Compile seulement ; aucun rapport Automation créé |
| `-UseRHI` | switch | Désactivé | Omet `-NullRHI` pour les tests nécessitant un rendu réel |
| `-ShowAutomationOutput` | switch | Désactivé | Affiche la sortie Unreal en direct au lieu de la rediriger dans `Automation.console.log` |

`-SkipBuild` et `-SkipAutomation` sont incompatibles ensemble. `-ReportRoot`, `-UseRHI` et `-ShowAutomationOutput` n’interviennent que dans l’étape Automation. La cible `GrimrockPrototypeEditor`, la plateforme `Win64` et la configuration `Development` sont fixes.

Le script exige le `.uproject`, `Engine\Build\BatchFiles\Build.bat` et `Engine\Binaries\Win64\UnrealEditor-Cmd.exe`, **même si une des deux étapes est ignorée**. Le contrôle du filtre intervient après la compilation : un appel sans filtre ni `-SkipAutomation` peut compiler avant de signaler l’oubli.

### Compiler uniquement

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot 'D:\UE_5.5' -SkipAutomation
```

Cette commande utilise `Build.bat GrimrockPrototypeEditor Win64 Development`, avec `-Project`, `-WaitMutex` et `-NoHotReloadFromIDE`. La sortie de compilation reste affichée dans le terminal.

### Compiler et tester

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot 'D:\UE_5.5' `
    -AutomationFilter 'Grimrock.UI.Clean04'
```

Ce filtre existe dans la révision auditée ; choisissez ensuite le filtre correspondant à votre modification. Le script transmet la valeur à `Automation RunTest <filtre>;Quit`. Il ne choisit pas automatiquement les tests et ne maintient pas un catalogue de filtres. Les noms sont déclarés dans les tests C++ et consultables dans la fenêtre Automation d’Unreal.

### Relancer les tests sans recompiler

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot 'D:\UE_5.5' `
    -SkipBuild `
    -AutomationFilter 'Grimrock.UI.Clean04' `
    -ReportRoot 'D:\Development\GrimrockPrototype\Saved\Automation\UI'
```

`-SkipBuild` n’est approprié que si les binaires correspondent au code que vous voulez tester. Un changement C++, un changement de branche ou une mise à jour du dépôt peut nécessiter une nouvelle compilation.

Pour diagnostiquer un lancement, ajouter `-ShowAutomationOutput` au même appel. Ajouter `-UseRHI` uniquement si la suite nécessite le rendu : par défaut, le script utilise `-NullRHI`. `-UseRHI` ne lance pas à lui seul une validation visuelle PIE.

### Rapports et interprétation

Sortie par défaut : `Saved\Automation\TD04\TD04-yyyyMMdd-HHmmss\`.

| Fichier | Utilisation |
| --- | --- |
| `index.json` | Rapport exporté par Unreal ; indispensable au verdict du script |
| `Automation.summary.txt` | Résumé compact à lire ou à transmettre, compteurs et diagnostics pertinents |
| `Automation.log` | Journal Unreal complet demandé avec `-abslog` |
| `Automation.console.log` | Sortie console redirigée par défaut ; non créée par cette redirection avec `-ShowAutomationOutput` |

Un `index.json` absent provoque un échec et un résumé de secours. Un rapport présent mais illisible peut interrompre le script avant la production du résumé normal.

| Champ du résumé | Sens et effet |
| --- | --- |
| `Succeeded` | Tests réussis sans avertissement |
| `Succeeded with warnings` | Tests réussis avec avertissements ; ne provoque pas à lui seul un échec |
| `Failed` | Tests échoués ; toute valeur supérieure à zéro fait échouer la validation |
| `Not run` | Tests non exécutés ; affiché mais ne fait pas à lui seul échouer le script |
| `Process exit code` | Code du processus Unreal, distinct du code de sortie du script PowerShell |

Le nombre exécuté est `Succeeded + Succeeded with warnings + Failed`. Le succès exige un rapport lisible, au moins un test exécuté, `Failed = 0` et un code du processus Unreal égal à `0`. Ainsi, un succès du script peut coexister avec `Not run > 0` : pour une validation complète de la sélection, vérifier aussi ce compteur. Les avertissements doivent être examinés, même s’ils sont attendus dans certains tests négatifs.

Le résumé extrait certaines lignes `LogAutomationController` en cas d’échec ou de succès avec avertissements. Il ne remplace pas le journal complet pour diagnostiquer un crash ou une erreur hors Automation.

<a id="validatepackageps1"></a>
## ValidatePackage.ps1

**But :** construire et préparer le jeu Windows autonome. Une compilation Editor réussie ne suffit pas à valider ce parcours.

| Paramètre | Type | Défaut | Explication |
| --- | --- | --- | --- |
| `-EngineRoot` | chaîne | `$env:UE_ROOT` | Racine UE contenant `Engine\Build\BatchFiles\RunUAT.bat` |
| `-ArchiveRoot` | chaîne | `<Repo>\Saved\Packaging\TD04` | Dossier parent de l’archive ; un sous-dossier de session est toujours ajouté |
| `-Configuration` | chaîne | `Shipping` | Valeurs acceptées uniquement : `Development` ou `Shipping` |

Package de référence :

```powershell
.\Scripts\ValidatePackage.ps1 -EngineRoot 'D:\UE_5.5'
```

Package Development pour diagnostic, avec destination explicite :

```powershell
.\Scripts\ValidatePackage.ps1 `
    -EngineRoot 'D:\UE_5.5' `
    -Configuration Development `
    -ArchiveRoot 'D:\Development\GrimrockPrototype\Saved\Packaging\Diagnostics'
```

Le script appelle `RunUAT.bat BuildCookRun` pour la cible **GrimrockPrototype**, plateforme **Win64**, avec `-build -cook -stage -package -pak -archive`. Il ne propose ni `-SkipBuild`, ni filtre Automation, ni sélection de maps. Les maps et références à cuire dépendent de la configuration du projet ; il ne force pas `-allmaps`.

L’archive se trouve dans `<ArchiveRoot>\TD04-<Configuration>-yyyyMMdd-HHmmss\`. Par défaut, pour Shipping : `Saved\Packaging\TD04\TD04-Shipping-yyyyMMdd-HHmmss\`.

Après un code UAT égal à zéro, le script cherche récursivement `GrimrockPrototype.exe` puis au moins un `.pak`. Il affiche le premier exécutable trouvé, le nombre de `.pak`, le nombre de fichiers, leur taille totale en octets et le dossier d’archive. Il n’impose pas de seuil de taille supplémentaire. La sortie finale de succès est `[OK] Cook / package validated.`

Distribuer **l’ensemble du dossier de jeu produit**, avec ses sous-dossiers et fichiers de données ; l’exécutable seul ne suffit pas. Le chemin exact de l’exécutable est celui du résumé, pas un chemin relatif supposé.

Le script ne lance pas le jeu packagé, n’exécute pas de tests Automation, ne crée pas d’installateur, ne signe pas l’exécutable et ne publie rien. Tester ensuite le menu, une nouvelle partie et le chargement du donjon dans le jeu produit. Aucun `Package.summary.txt` ni journal personnalisé horodaté n’est créé par ce wrapper : consulter la console et les chemins de logs indiqués par UAT.

<a id="checkcppformatps1-et-formatcppps1"></a>
## CheckCppFormat.ps1 et FormatCpp.ps1

Les deux scripts emploient la même configuration `.clang-format`, la même version attendue **19.1.5** et le même périmètre. `CheckCppFormat.ps1` vérifie ; `FormatCpp.ps1` modifie les fichiers.

### Paramètre commun et recherche de l’outil

| Script | Paramètre | Type | Défaut | Explication |
| --- | --- | --- | --- | --- |
| `CheckCppFormat.ps1` | `-ClangFormatPath` | chaîne | Recherche automatique | Chemin complet de `clang-format.exe`, pas de son dossier |
| `FormatCpp.ps1` | `-ClangFormatPath` | chaîne | Recherche automatique | Même résolution et même contrôle de version |

Ordre de recherche :

1. Chemin fourni par `-ClangFormatPath`, s’il est renseigné.
2. Installation Visual Studio détectée par `vswhere`, plage `[17.0,18.0)` (**VS 2022**), puis `VC\Tools\Llvm\x64\bin\clang-format.exe`.
3. Chemin fixe `C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe`.
4. Première application `clang-format` trouvée dans le `PATH`.

Le premier exécutable trouvé est ensuite interrogé avec `--version`. Le texte doit commencer par `clang-format version 19.1.5` (comparaison sans distinction de casse). Une version différente provoque une erreur ; le script ne poursuit pas la recherche d’un second exécutable compatible.

**Avec Visual Studio 2026 :** l’installation n’est pas couverte par la recherche `vswhere` ciblée sur VS 2022. Fournir le chemin d’un exécutable **19.1.5** déjà installé, ou le rendre accessible dans le `PATH`. Le numéro de version du formatter reste requis, indépendamment de l’IDE utilisé pour compiler.

### Périmètre exact

- Dossiers parcourus récursivement : `Source/GrimrockPrototype`, `Source/GrimrockPrototypeEditor`, `Source/GrimrockLua` ; l’absence d’un de ces dossiers fait échouer le script.
- Extensions admises : `.h`, `.cpp`, `.inl`.
- Fichiers `*.generated.h` exclus.
- Répertoires `ThirdParty`, `Intermediate`, `Binaries`, `Saved`, `DerivedDataCache` exclus.
- Les `.inl` sont présentés au formatter comme des fichiers C++ via `--assume-filename=<fichier>.cpp`.

Les fichiers `.Build.cs`, `.Target.cs`, les assets et les sources hors de ces trois modules ne font pas partie du périmètre. Il n’existe pas d’option de sélection par fichier, par diff Git ou de mode `-WhatIf`.

### Contrôler sans modifier

```powershell
.\Scripts\CheckCppFormat.ps1
```

Le script appelle `clang-format --dry-run --Werror --style=file`, vérifie tous les fichiers et affiche `[FORMAT] <chemin>` pour chaque retour non nul. Il masque volontairement les diagnostics natifs du formatter pour conserver une sortie concise. Le code explicite est `0` si tout est conforme et `1` si au moins un fichier est signalé. Un retour non nul du formatter peut aussi traduire une erreur de configuration ; ne pas supposer systématiquement un simple problème d’indentation.

Exemple avec l’emplacement VS 2022 Community historique, si cet exécutable existe sur votre machine :

```powershell
.\Scripts\CheckCppFormat.ps1 -ClangFormatPath 'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe'
```

### Appliquer le format

```powershell
git status --short
.\Scripts\FormatCpp.ps1
git diff --stat
git diff --check
.\Scripts\CheckCppFormat.ps1
```

`FormatCpp.ps1` lance le formatter avec `-i --style=file --Werror` sur **tout le périmètre**. Il s’arrête au premier échec ; des fichiers précédents peuvent déjà avoir été réécrits. Examiner le diff avant de conserver les changements, particulièrement si des travaux étaient déjà en cours. Le script ne sauvegarde pas une copie des sources, ne crée pas de commit et ne compile pas le projet.

Son message final est `Formatage C++ termine.` ; contrairement au contrôleur, il n’a pas d’instruction finale `exit 0`. Une exécution normale dans un processus PowerShell dédié se termine avec succès. Les deux scripts ne génèrent pas de rapport fichier.

<a id="migrateworldobjectassetsps1"></a>
## MigrateWorldObjectAssets.ps1

**Statut : historique, non opérationnel sur un build propre du code audité.**

Le fichier lance `UnrealEditor-Cmd.exe -run=GridWorldObjectMIG08`. L’audit de `Source/` ne trouve ni ce commandlet, ni l’ancienne implémentation MIG08. Recompiler le projet courant ne le recréera pas. Des binaires anciens éventuellement conservés sur une machine ne constituent pas une validation du code actuel.

Ne pas utiliser ce script comme commande de maintenance régulière et ne pas lancer `-Apply` sur le contenu courant en se fondant sur l’ancien guide. Si une nouvelle migration devient nécessaire, elle devra être définie et validée pour le modèle de données actuel. L’ancien [document MIG08](Architecture/WORLDOBJ_MIG08_ASSET_MIGRATION.md) conserve les objectifs et résultats historiques ; ses commandes ne valent pas procédure actuelle.

### Interface du lanceur conservé

| Paramètre | Type | Défaut | Effet codé dans le `.ps1` |
| --- | --- | --- | --- |
| `-EngineRoot` | chaîne | `$env:UE_ROOT` | Installation UE contenant `UnrealEditor-Cmd.exe` |
| `-RootPath` | chaîne | `/Game/GrimrockPrototype` | Racine virtuelle Unreal transmise par `-Root`, pas un chemin Windows |
| `-ReportRoot` | chaîne | `<Repo>\Saved\Automation\MIG08` | Dossier parent des sessions |
| `-Apply` | switch | Désactivé | Ajoute `-Apply` au commandlet ; ancien mode de sauvegarde des packages |
| `-SkipBuild` | switch | Désactivé | Omet l’appel préalable à `ValidateUE.ps1 -SkipAutomation` |

Sans `-SkipBuild`, le wrapper compile d’abord l’éditeur. Il crée ensuite `<ReportRoot>\MIG08-yyyyMMdd-HHmmss\`, prévoit `MIG08.report.txt` et `MIG08.log`, puis demande le lancement du commandlet avec `-Unattended -NoSplash -NoP4 -NoSound -NullRHI -UTF8Output`.

Sans `-Apply`, le mode affiché est `DRY-RUN` ; avec `-Apply`, il devient `APPLY`. Le wrapper affiche le rapport **s’il existe**, puis échoue si le processus Unreal a retourné un code non nul. Il n’exige pas lui-même l’existence du rapport pour déclarer son succès. Après `-Apply`, il appelle `git status --short -- Content` ; il ne crée pas de commit.

Ces détails décrivent le lanceur encore versionné, sans garantir les effets d’un commandlet absent. Aucun exemple d’application sur le projet courant n’est proposé pour cette raison.

<a id="resultats-et-codes-de-sortie"></a>
## Résultats et codes de sortie

Tous les scripts utilisent `$ErrorActionPreference = 'Stop'` et `Set-StrictMode -Version Latest`. Une erreur bloquante doit interrompre la validation ; ne pas poursuivre simplement parce qu’un ancien log comportait `[OK]`.

| Script | Succès | Échec |
| --- | --- | --- |
| `CheckProjectDependencies.ps1` | `exit 0` explicite | Exception en cas de prérequis ou de plugin manquant |
| `CheckCppFormat.ps1` | `exit 0` explicite | `exit 1` si des fichiers sont signalés ; exception si outil/version/périmètre invalide |
| `FormatCpp.ps1` | Fin normale après le message de réussite | Exception sur erreur de résolution, de version ou de formatage |
| `ValidateUE.ps1` | `exit 0` explicite | Exception sur prérequis, compilation, rapport ou résultat Automation invalide |
| `ValidatePackage.ps1` | `exit 0` explicite | Exception sur prérequis, UAT ou artefacts attendus absents |
| `MigrateWorldObjectAssets.ps1` | Fin normale du wrapper si processus réussi | Exception sur prérequis/build/processus ; commandlet manquant dans le code courant |

**Particularité des codes détaillés :** `ValidateUE.ps1` contient `exit 2` (aucun test), `exit 3` (tests échoués), `exit 4` (processus Unreal non nul). `ValidatePackage.ps1` contient `exit 2` (exécutable absent) et `exit 3` (`.pak` absent). Mais chaque instruction est précédée de `Write-Error`, qui est bloquant avec la préférence `Stop`. Ces valeurs ne constituent donc pas un contrat fiable pour un programme appelant. Se baser sur succès/échec, le message et les rapports. La présente documentation n’a pas corrigé ce comportement.

Pour obtenir le code **du processus PowerShell complet**, lancer un script dans un processus distinct depuis PowerShell, puis relever immédiatement `$LASTEXITCODE` :

```powershell
powershell.exe -NoProfile -File '.\Scripts\ValidateUE.ps1' -EngineRoot 'D:\UE_5.5' -SkipAutomation
$validationExitCode = $LASTEXITCODE
Write-Host "Code de validation : $validationExitCode"
```

Cette séparation est utile pour une future orchestration : dans une session interactive, `$LASTEXITCODE` peut représenter le dernier programme natif appelé, notamment après une exception PowerShell. Un orchestrateur doit arrêter la chaîne au premier échec et ne pas assimiler le code du processus Unreal affiché dans un résumé au code global du script.

<a id="procedures-usuelles"></a>
## Procédures usuelles

### Après récupération d’une mise à jour C++

Exécuter ces étapes **une par une**, en vérifiant le résultat avant de passer à la suivante :

1. `git status --short` : identifier les modifications locales à préserver.
2. `CheckProjectDependencies.ps1 -EngineRoot 'D:\UE_5.5'` si l’environnement ou les plugins ont changé.
3. `ValidateUE.ps1 -EngineRoot 'D:\UE_5.5' -AutomationFilter '<filtre du jalon>'` via le chemin `.\Scripts\` ; remplacer le texte entre chevrons par le filtre réel.
4. Tester dans l’éditeur le comportement concerné si la modification touche l’interface, un Blueprint ou des assets.

Pour simplement recompiler avant d’ouvrir l’éditeur, employer `-SkipAutomation`. Pour réexécuter le même test après un build déjà validé et inchangé, employer `-SkipBuild`.

### Avant de livrer une version jouable

1. Valider la compilation Editor et les tests du périmètre modifié.
2. Lancer `ValidatePackage.ps1 -EngineRoot 'D:\UE_5.5'` via `.\Scripts\`.
3. Relever le dossier et l’exécutable du résumé.
4. Lancer le jeu produit et contrôler les parcours concernés.

Le contrôle de format est utile lorsque les sources C++ changent. Le reformatage global et l’ancienne migration ne sont pas des étapes systématiques de validation.

### Conservation des sorties

Les rapports et packages par défaut sont sous `Saved/`, ignoré par Git. Conserver le résumé et les logs utiles avant de nettoyer ce dossier. Les scripts ne suppriment pas les anciennes sessions. Les noms de session n’ont qu’une précision d’une seconde : éviter deux lancements simultanés du même outil vers la même racine de sortie. Une destination personnalisée peut se trouver hors des dossiers ignorés ; ne pas ajouter accidentellement les rapports ou archives au dépôt.

<a id="depannage"></a>
## Dépannage

| Symptôme | Vérification et action |
| --- | --- |
| Le `.ps1` est introuvable | Vérifier `Set-Location`, utiliser `.\Scripts\Nom.ps1`, et vérifier que la copie locale est à jour |
| PowerShell bloque l’exécution des scripts | Consulter `Get-ExecutionPolicy -List`. Sur votre poste, si la politique locale le permet, définir `Set-ExecutionPolicy -Scope Process -ExecutionPolicy RemoteSigned` pour cette session ; une politique d’organisation garde priorité |
| `Racine Unreal Engine non renseignee` | Fournir `-EngineRoot` ou définir `$env:UE_ROOT` dans le terminal utilisé |
| `Build.bat`, `RunUAT.bat` ou `UnrealEditor-Cmd.exe` introuvable | Vérifier la racine UE et le sous-dossier `Engine` ; `ValidateUE` exige ses deux exécutables même avec une étape ignorée |
| `Plugin active introuvable` | Vérifier le nom et l’emplacement du `.uplugin` ; installer/rétablir la dépendance voulue avant de compiler |
| Build refusé avec Live Coding, DLL verrouillée ou autre build actif | Fermer l’éditeur et les processus concernés, puis relancer ; consulter la première erreur de compilation |
| Erreur d’accès ou de rotation du journal UBT | Lire le chemin exact dans l’erreur, vérifier les permissions et les processus qui utilisent le journal ; ce n’est pas nécessairement une erreur C++ |
| `AutomationFilter est obligatoire` | Fournir le filtre réel, ou `-SkipAutomation` si vous voulez seulement compiler |
| Aucun test exécuté | Vérifier le nom du test dans le code/la fenêtre Automation et recompiler si les binaires sont anciens ; ce résultat n’est pas une validation |
| `index.json` absent | Lire `Automation.summary.txt`, `Automation.log` et la sortie console ; rechercher un démarrage raté ou un crash avant l’export |
| Terminal silencieux pendant Automation | Mode concis normal : lire les logs ou relancer avec `-ShowAutomationOutput` pour voir la sortie en direct |
| Test dépendant du rendu en échec sous NullRHI | Utiliser `-UseRHI` si ce besoin est confirmé par le test ; vérifier que l’environnement graphique est disponible |
| Tests réussis avec warnings ou tests non exécutés | Examiner les diagnostics et le compteur `Not run` ; le succès global ne garantit pas leur absence |
| Package échoué alors que l’Editor compile | Lire la première erreur UAT/cook ; vérifier les références et la configuration de packaging du projet |
| Exécutable ou `.pak` absent | Vérifier l’archive affichée et les logs UAT ; le script exige ces deux types d’artefacts |
| `clang-format` introuvable ou version incompatible | Fournir explicitement `-ClangFormatPath` vers une version 19.1.5 ; VS 2026 n’est pas détecté par la recherche dédiée VS 2022 |
| `[FORMAT]` sans diagnostic détaillé | Interroger le même outil sur un fichier : `& 'C:\chemin\clang-format.exe' --dry-run --Werror --style=file 'Source\GrimrockPrototype\GrimrockPrototype.cpp'`, en remplaçant le chemin de l’outil |
| Nombreux fichiers changés après `FormatCpp.ps1` | Le script couvre les trois modules complets ; examiner le diff et préserver les modifications utilisateur existantes |
| `GridWorldObjectMIG08` introuvable | Ancien commandlet absent du code courant ; ne pas contourner ce constat en utilisant des binaires anciens ou en lançant `-Apply` |

<a id="documentation-associee-et-maintenance"></a>
## Documentation associée et maintenance

Les documents ci-dessous expliquent les décisions et validations de leurs jalons. Pour les paramètres et précautions d’utilisation courante des scripts, commencer par la présente référence et les `.ps1` concernés.

| Domaine | Documentation complémentaire |
| --- | --- |
| Installation et plugins | [Environnement de développement](Design/DEVELOPMENT_ENVIRONMENT_SETUP.md), [TD07.1 — Dépendances](Design/TD07_1_BUILD_DEPENDENCY_REPRODUCIBILITY.md) |
| Editor et Automation | [TD04.2 — Validation locale](Design/TD04_2_LOCAL_UE_VALIDATION_HARNESS.md), [Fondation des tests](Architecture/TEST_AUTOMATION_FOUNDATION.md) |
| Packaging | [TD04.3 — Cook/package](Design/TD04_3_COOK_PACKAGE_VALIDATION.md) |
| Formatage | [STYLE01 — Baseline C++](Design/STYLE01_CPP_FORMATTING_BASELINE.md) |
| Migration historique | [WORLDOBJ-MIG08](Architecture/WORLDOBJ_MIG08_ASSET_MIGRATION.md) |

D’autres documents citent des scripts ponctuels d’audit ou de réparation, par exemple `RepairItemThrowShurikenAuthority.ps1` ou `ValidateTD078StopCondition.ps1`. Ils sont absents du dossier `Scripts/` audité : ces mentions historiques ne doivent pas être interprétées comme un catalogue d’outils encore disponibles.

À chaque ajout, suppression ou modification d’un script, mettre à jour son statut, ses paramètres et valeurs par défaut, ses prérequis, ses effets sur les fichiers, ses critères de succès, ses exemples et les index. Vérifier aussi que les commandlets et filtres cités existent toujours. Ne pas annoncer une validation UE, un packaging ou une migration uniquement sur la base d’une lecture du code.

**Vérification de cette documentation :** comparaison statique avec les six scripts et recherche de leurs dépendances dans la révision auditée. Aucune exécution PowerShell/UE5.5.4, compilation, migration ou génération de package n’a été effectuée pour cet audit documentaire.
