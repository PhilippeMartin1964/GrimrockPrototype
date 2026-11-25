# Scripts du prototype

Le [guide complet en français](../docs/SCRIPTS_REFERENCE.md) décrit les prérequis, paramètres, commandes, sorties et procédures de validation des scripts versionnés dans ce dossier.

| Besoin | Script | Effet |
| --- | --- | --- |
| Vérifier les plugins déclarés | [CheckProjectDependencies.ps1](CheckProjectDependencies.ps1) | Contrôle sans modification |
| Compiler l’éditeur et/ou lancer des tests Automation | [ValidateUE.ps1](ValidateUE.ps1) | Compilation et rapports sous `Saved/` |
| Installer et maintenir la tâche Windows sans console | [UEValidationSilentTask.ps1](UEValidationSilentTask.ps1) | Migration automatique vers `wscript.exe`, auto-test, contrôle et retour arrière de l'action précédente |
| Valider automatiquement chaque nouveau commit `master` sous Windows | [InstallUEValidationAgent.ps1](InstallUEValidationAgent.ps1), [UEValidationBootstrap.ps1](UEValidationBootstrap.ps1) et [UEValidationAgent.ps1](UEValidationAgent.ps1) | Tâche planifiée invisible, agent mis à jour automatiquement, compilation UE, tests Automation et diagnostics sur GitHub ; [guide français](../docs/Architecture/VALIDATION_AUTO01_WINDOWS_UE_AGENT.md) |
| Capturer des mesures CPU Unreal Insights | [CapturePerfProfile01.ps1](CapturePerfProfile01.ps1) | Lancement standalone, fichier `.utrace` et métadonnées sous `Saved/Profiling/PERF-PROFILE01/` |
| Valider la chaîne UI-RPG-VISUAL01.4 et ses Data Assets sérialisés | [ValidateUIRPGVisual.ps1](ValidateUIRPGVisual.ps1) | Lance la suite ciblée Visual/Assets/Blueprints/DESC01 sans modifier les `.uasset` |
| Vérifier les gardes permanentes | [TestProjectValidationGuards.ps1](TestProjectValidationGuards.ps1) | Lecteurs d’objets et inclusion des matériaux d’huile dans le cook. Ajouter `-SelfTest` pour les cas de refus |
| Comparer les profils d’armes à l’oracle historique | [TestARM_CLEAN06_HistoricalBaseline.ps1](TestARM_CLEAN06_HistoricalBaseline.ps1) | Lecture seule du manifeste, de son reçu et d’un export courant séparé |
| Nettoyer des artefacts régénérables | [CleanProject.ps1](CleanProject.ps1) | Aperçu par défaut, liste blanche appliquée avec `-Apply` |
| Construire le jeu Windows distribuable | [ValidatePackage.ps1](ValidatePackage.ps1) | Compilation, cook et archive |
| Contrôler le format C++ | [CheckCppFormat.ps1](CheckCppFormat.ps1) | Contrôle sans modification |
| Appliquer le format C++ | [FormatCpp.ps1](FormatCpp.ps1) | Réécrit les sources des trois modules du projet |

Depuis PowerShell :

```powershell
Set-Location 'D:\Development\GrimrockPrototype'
.\Scripts\ValidateUE.ps1 -EngineRoot 'D:\UE_5.5' -SkipAutomation
```

Chaque validation automatique compile Development Editor et Shipping, puis lance Grimrock. Elle opère dans le worktree détaché dédié. Les journaux complets sont locaux. Les échecs ne déclenchent aucune écriture d’asset, restauration ou publication de commit.

Les wrappers RPG/UI et de localisation restent disponibles. Consulter la [référence des scripts](../docs/SCRIPTS_REFERENCE.md) avant tout authoring. Les procédures ARM-CLEAN06 ponctuelles se consultent dans Git au commit `29e89de112b6de8796c7ad0a8861437dc76be600`.
