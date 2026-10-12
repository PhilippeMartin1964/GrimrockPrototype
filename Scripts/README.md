# Scripts du prototype

Le [guide complet en français](../docs/SCRIPTS_REFERENCE.md) décrit les prérequis, paramètres, commandes, sorties et procédures de validation des scripts versionnés dans ce dossier.

| Besoin | Script | Effet |
| --- | --- | --- |
| Vérifier les plugins déclarés | [CheckProjectDependencies.ps1](CheckProjectDependencies.ps1) | Contrôle sans modification |
| Compiler l’éditeur et/ou lancer des tests Automation | [ValidateUE.ps1](ValidateUE.ps1) | Compilation et rapports sous `Saved/` |
| Valider la chaîne UI-RPG-VISUAL01.4 et ses Data Assets sérialisés | [ValidateUIRPGVisual.ps1](ValidateUIRPGVisual.ps1) | Lance la suite ciblée Visual/Assets/Blueprints/DESC01 sans modifier les `.uasset` |
| Construire le jeu Windows distribuable | [ValidatePackage.ps1](ValidatePackage.ps1) | Compilation, cook et archive |
| Contrôler le format C++ | [CheckCppFormat.ps1](CheckCppFormat.ps1) | Contrôle sans modification |
| Appliquer le format C++ | [FormatCpp.ps1](FormatCpp.ps1) | Réécrit les sources des trois modules du projet |
| Ancienne migration WORLDOBJ-MIG08 | [MigrateWorldObjectAssets.ps1](MigrateWorldObjectAssets.ps1) | **Historique : commandlet absent du code courant** |

Depuis PowerShell :

```powershell
Set-Location 'D:\Development\GrimrockPrototype'
.\Scripts\ValidateUE.ps1 -EngineRoot 'D:\UE_5.5' -SkipAutomation
```

Pour la migration, consulter impérativement son [statut actuel](../docs/SCRIPTS_REFERENCE.md#migrateworldobjectassetsps1) : la présence du fichier `.ps1` ne signifie pas que la migration est encore disponible.
