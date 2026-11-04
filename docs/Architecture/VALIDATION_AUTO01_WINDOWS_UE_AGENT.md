# VALIDATION-AUTO01 — Validation locale automatisée UE 5.5.4

**État au 4 novembre 2026 : opérationnel.** Le commit `32d2a305` a obtenu le statut GitHub **success — Build + Automation OK (Grimrock)**. La suite complète a été exécutée dans le worktree Windows, y compris la dépendance Lua 5.4.8. Cet état ne dispense pas de valider les futurs jalons individuellement.

## Objectif et principes

Ce dispositif exécute, sans intervention manuelle quotidienne, la compilation Unreal Engine 5.5.4 et les tests Automation du projet. Le dépôt est public ; nous n'utilisons **pas** de runner GitHub Actions auto-hébergé sur votre ordinateur personnel.

L'exécuteur est une **tâche planifiée Windows**, sous votre compte habituel. Il contacte seulement `origin/master` et GitHub par les outils `git` et `gh`. Il n'accepte aucune instruction entrante provenant d'une pull request.

Les engagements sont les suivants :

- **Une seule branche de développement :** `master`.
- **Sources de jeu en C++ ;** Unreal et les assets constituent la présentation et les données de production.
- **Aucune modification de votre copie de développement** `D:\Development\GrimrockPrototype` par l'agent, hormis les références Git mises à jour par `fetch`.
- **Worktree de validation indépendant :** `D:\Development\GrimrockPrototype_UEValidation`.
- **Pas de commande destructive** `git reset --hard` ou `git clean` ; refus des worktrees contenant des modifications imprévues.
- **Résultats publiés sur GitHub :** statut `UE5.5.4/LocalAuto` pour chaque commit testé et issue en cas d'échec.
- **Confidentialité :** journaux complets conservés localement. Les issues du dépôt public ne reçoivent que des informations techniques extraites d'une liste blanche.

## Fonctionnement automatique

1. Le Planificateur de tâches déclenche `Scripts/UEValidationBootstrap.ps1` environ toutes les trois minutes, tant que l'ordinateur est allumé et que votre session est ouverte.
2. Le bootstrap vérifie le dépôt d'origine, exécute `git fetch --no-tags origin master`, extrait `UEValidationAgent.ps1` depuis **le SHA exact de `master`**, puis contrôle sa syntaxe avant de l'exécuter.
3. L'agent protège le worktree, récupère le commit demandé et initialise `ThirdParty/Lua54` par `git submodule update --init`.
4. Il vérifie la présence des sources Lua `onelua.c`, `lua.h`, `lauxlib.h` et `lualib.h`, puis récupère les assets Git LFS.
5. Il lance `Scripts/ValidateUE.ps1` dans un processus PowerShell distinct, **sans fenêtre persistante**, pour compiler l'éditeur puis exécuter les tests `Grimrock`.
6. Il publie `pending`, puis `success` ou `failure` sur GitHub, avec un rapport limité en cas d'échec.
7. Un même commit déjà validé n'est pas relancé inutilement. Les erreurs d'infrastructure transitoires sont réessayées au prochain cycle.

**Important :** l'agent applique automatiquement les nouvelles versions de `UEValidationAgent.ps1`. La mise à jour du bootstrap lui-même ou des paramètres de la tâche peut exceptionnellement nécessiter une réinstallation.

## Prérequis

- Windows 11, Unreal Engine **5.5.4** sous `D:\UE_5.5`, outils C++ Visual Studio et Windows SDK adéquats.
- `git`, Git LFS et GitHub CLI (`gh`) accessibles depuis la session Windows exécutant la tâche.
- Authentification GitHub CLI avec les autorisations nécessaires aux statuts, issues et commits explicitement autorisés.
- Espace disque suffisant pour les assets Git LFS, les binaires, les données intermédiaires et les rapports.
- Ordinateur allumé et session Windows ouverte ; pas de privilège SYSTEM.

### Installation initiale — déjà réalisée

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\InstallUEValidationAgent.ps1 -EngineRoot D:\UE_5.5
```

L'installateur contrôle les prérequis, la syntaxe PowerShell et l'appartenance de la tâche, puis l'enregistre de manière idempotente. Il détecte les options réellement disponibles de `New-ScheduledTaskSettingsSet` et n'utilise plus les paramètres de batterie incorrects. Le bootstrap s'exécute en arrière-plan.

Cette procédure n'est **pas** à répéter lors de chaque nouveau commit.

### Consulter le dispositif

```powershell
Get-ScheduledTask -TaskName 'GrimrockPrototype-UEValidation' | Get-ScheduledTaskInfo
Get-Content "$env:LOCALAPPDATA\GrimrockPrototype\UEValidationAgent\Bootstrap.latest.log" -Tail 40
Get-Content "$env:LOCALAPPDATA\GrimrockPrototype\UEValidationAgent\Agent.latest.log" -Tail 70
```

Les fichiers des sessions sont dans `%LOCALAPPDATA%\GrimrockPrototype\UEValidationAgent\Runs`. Les journaux comprennent les erreurs du compilateur, de l'éditeur Unreal et des tests Automation. Le tableau de bord GitHub affiche `UE5.5.4/LocalAuto`, accompagné d'une issue `[UEValidation]` lorsqu'un commit échoue.

## Diagnostics et sécurité

En cas d'erreur de compilation, l'issue publique peut afficher **le nom de fichier source, le numéro de ligne et un code MSVC**. Pour Automation, seuls des noms de tests `Grimrock.*` contrôlés et des compteurs d'échecs sont autorisés. Le diagnostic de la compétence `Skill_Traps` utilise une liste blanche des identifiants de compétences et des catégories de contrôles.

Les chemins personnels, noms de répertoire utilisateur, extraits bruts de journaux, messages libres et secrets **ne sont pas publiés**. Les codes d'erreurs de préparation distinguent notamment `WORKTREE_REGISTRATION`, `WORKTREE_DIRTY`, `LFS`, `GITHUB_AUTH`, `GIT` et `OTHER`.

L'agent refuse un checkout si le worktree n'appartient pas au bon dépôt ou présente des changements inattendus. Un échec d'authentification ou de réseau n'autorise pas à supprimer les données pour réessayer.

**Attention :** le bootstrap exécute les fichiers PowerShell présents sur `master`. Les droits d'écriture de cette branche doivent donc rester strictement protégés. Aucune PR non fiable ne doit être exécutée.

## Matérialisation contrôlée des assets Unreal

Deux exceptions explicitement limitées peuvent produire des **commits d'assets natifs** depuis le worktree de validation, après vérification des tests :

- **VALIDATION-AUTO01.7 / Compétences :** `RepairCanonicalSkillTraps.ps1` a rétabli uniquement `DA_Skill_Traps.uasset`. L'asset a été publié automatiquement au commit `32d2a305`, suivi d'une validation complète réussie.
- **ARMEMENT01.17.1.4 / Huile au sol :** `AuthorOilSurfaceMaterial.ps1` n'agit que si le seul échec est `Grimrock.RPG.ARMEMENT01.17.1.4.ProductionMaterial`. Il génère, par un commandlet UE, uniquement `M_Surface_Oil.uasset` et `MI_Surface_Oil.uasset`, vérifie Git LFS, le SHA, la liste exacte des fichiers modifiés et la possibilité d'un push non forcé. Le résultat doit ensuite repasser l'intégralité des tests.

Ces réparations ne constituent pas une permission générale d'écrire des assets. Elles n'agissent jamais sur le checkout principal et n'utilisent jamais `git add -A`. Un fichier imprévu provoque l'arrêt de la publication.

## Historique des correctifs du 4 novembre 2026

| Jalon | Problème résolu |
|---|---|
| VALIDATION-AUTO01.1 | Paramètres de batterie incorrects ; installation idempotente, contrôle syntaxique et journalisation. |
| VALIDATION-AUTO01.2 | Suppression des deux fenêtres PowerShell persistantes. |
| VALIDATION-AUTO01.3 | Publication de diagnostics techniques filtrés, sans fuite des journaux. |
| VALIDATION-AUTO01.4 | Normalisation des chemins Git Windows et bootstrap de mise à jour automatique. |
| VALIDATION-AUTO01.5 | Initialisation automatique du sous-module Lua 5.4.8 avant compilation. |
| VALIDATION-AUTO01.6 / .6A | Réalignement de deux tests historiques et correction du script de diagnostic des compétences. |
| VALIDATION-AUTO01.7 | Réparation ciblée du DataAsset `Skill_Traps`, puis retour à une validation UE complète réussie. |
| ARMEMENT01.17.1.4 | Projection des surfaces d'huile et génération automatique du matériau ; validation propre au jalon encore nécessaire. |

## Limites de la validation

Une compilation et des tests Automation réussis prouvent le respect des assertions exécutées, **pas** l'apparence finale d'un effet graphique. Le test visuel de l'huile nécessite soit une capture automatisée avec RHI, soit une recette PIE représentative. La vue 3D, l'éclairage des torches, les reliefs du sol et les lisières doivent encore être examinés avant de déclarer l'apparence artistique définitive.

L'agent ne doit ni lancer automatiquement une traduction anglaise ni réécrire des matériaux existants sans jalon explicite.

## Arrêt et désinstallation

```powershell
Disable-ScheduledTask -TaskName 'GrimrockPrototype-UEValidation'
.\Scripts\UninstallUEValidationAgent.ps1
```

Les journaux et données de travail ne sont pas effacés automatiquement.

## Références

- [Configuration des runners GitHub](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/add-runners).
- [Sécurisation de GitHub Actions](https://docs.github.com/en/actions/reference/security/secure-use).
- `Scripts/ValidateUE.ps1` et `Scripts/README.md` — référence opérationnelle du projet.
