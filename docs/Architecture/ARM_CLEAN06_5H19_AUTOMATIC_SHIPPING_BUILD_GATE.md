# ARM-CLEAN06.5H.19 — Compilation Shipping dans la validation Windows automatique

## Justification : vrai Cook/Shipping désormais vérifié

Sur le SHA `e042c9585165b7f5f695698639b9b5bc0bd0d128`, le journal de `Scripts/ValidatePackage.ps1 -EngineRoot D:\UE_5.5 -Configuration Shipping` transmis le **21 novembre 2026** atteste :

| Étape | Résultat |
|---|---|
| `GrimrockPrototypeEditor Win64 Development` | Build réussi |
| `GrimrockPrototype Win64 Shipping` | Build réussi, erreurs `C4702` disparues |
| Cook complet Windows | `UnrealEditor-Cmd.exe ExitCode=0` |
| Stage, Package, Archive | Terminés |
| AutomationTool | `ExitCode=0 (Success)` |
| Présentation RPG et compétences | `DA_RPGTalentPresentation` présent, 25/25 `DA_Skill_*` |
| Archive | `TD04-Shipping-20261121-160515`, 41 fichiers, 2 342 400 785 octets |

Le journal mentionne uniquement l'avertissement MSVC « version non préférée » durant la compilation ; il ne signale pas d'erreur de compilation, Cook ou Packaging. Ce résultat établit **la production du binaire**, et non le résultat d'un essai visuel PIE ou d'une partie sauvegardée/reprise.

## Risque éliminé par le lot

La validation `UE5.5.4/LocalAuto` compilait seulement le target **Editor Development**, où `WITH_DEV_AUTOMATION_TESTS` expose des blocs de compatibilité anciens. Le compilateur optimisé **Shipping**, exécuté manuellement plus tard, a révélé des `C4702` inaccessibles au build Editor.

La nouvelle étape de compilation, exécutée par le même agent Windows dans le worktree dédié `GrimrockPrototype_UEValidation`, réduit cet écart de couverture :

1. `ValidateUE.ps1 -BuildShipping` lance `Build.bat GrimrockPrototypeEditor Win64 Development`, puis `Build.bat GrimrockPrototype Win64 Shipping`, puis `Automation RunTest Grimrock`.
2. `UEValidationAgent.ps1` passe systématiquement `-BuildShipping` dans sa commande pour chaque SHA nouveau.
3. En cas d'échec Editor **ou** Shipping, le stage devient `BUILD_FAILED` et `LocalAuto=success` **n'est pas publié** ; les diagnostics restent privés et la publication d'incident reste filtrée par la whitelist existante.
4. En cas de succès des trois étapes, le contexte `UE5.5.4/LocalAuto` rapporte `Editor + Shipping + Automation OK (Grimrock)` ; `UE5.5.4/SilentTask` conserve son rôle indépendant de surveillance de la tâche planifiée.

La compilation Shipping **n'effectue pas** de Cook, Stage, Package, Archive, de démarrage de jeu ou de tests de rendu. Les preuves de packaging restent celles du rapport opérateur (et des validations futures `ValidatePackage.ps1`). `-BuildShipping` est **optionnel** pour le harness manuel afin de conserver ses usages existants.

## Sécurité et régression

Aucune modification des paramètres globaux de l'outil MSVC ; `C4702` reste fatal en Shipping. Le SDK Windows choisi par l'agent est transmis aux deux builds avec les mêmes paramètres validés. Le `WaitMutex` est utilisé pour éviter les collisions avec d'autres invocations de build. Le worktree de validation est séparé du répertoire de développement et des sept `.sav` privées.

**Validation du lot H.19 :** publication d'un commit atomique (harness + agent + documentation), puis attente obligatoire des deux statuts `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA exact. Le statut `LocalAuto` de ce lot prouve effectivement que le nouveau chemin Shipping a été exécuté. La validation visuelle PIE / Save-Continue reste à effectuer sur le poste de développement avant de clôturer la migration.
