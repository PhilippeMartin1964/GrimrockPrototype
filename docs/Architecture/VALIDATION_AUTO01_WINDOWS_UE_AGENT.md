# VALIDATION-AUTO01 — Agent de validation locale UE 5.5.4

**Statut : infrastructure versionnee, activation et premiere validation Windows encore necessaires.**

## Pourquoi un agent local plutot qu'un GitHub Actions runner ?

`GrimrockPrototype` est un depot **public**. GitHub deconseille les runners auto-heberges persistants pour les depots publics : du code non fiable provenant de PR peut compromettre la machine. Nous ne connectons **pas** le poste Windows comme un runner Actions.

L'agent est une **tache planifiee Windows**, tournant sous le compte de l'utilisateur, qui effectue uniquement des sorties HTTPS vers GitHub (Git / gh CLI). Il surveille **`origin/master` exclusivement**, puis compile et execute les tests. Il n'accepte pas de commandes entrantes de GitHub Actions et ne declenche pas de workflow issu d'une PR. Les commits publies sur master restent des programmes executes localement : conserver un controle strict sur les droits d'ecriture de master.

## Architecture et garanties

- Point de depart utilisateur : `D:\Development\GrimrockPrototype` (jamais `pull`, `checkout`, `reset`, `clean`, `stash`, `add`, `commit` sur ce checkout).
- Lecture distante : `git fetch --no-tags origin master` puis lecture de `origin/master`. `git fetch` modifie les references de `.git`, pas les fichiers utilisateur du checkout.
- Source des tests : un **worktree Git dedie** `D:\Development\GrimrockPrototype_UEValidation`, cree a part. Ne pas y travailler manuellement.
- Au prochain commit, l'agent **refuse** tout checkout si ce worktree possede des modifications non committees ou fichiers non suivis. Aucun `git reset --hard`, `git clean` ou suppression automatises.
- Recuperation des vrais assets Git LFS avec `git lfs pull` pour le worktree ; la premiere execution peut etre longue/volumineuse.
- Compilation et Automation : reemploi de `Scripts/ValidateUE.ps1` dans le worktree, EngineRoot `D:\UE_5.5`, filtre par defaut `Grimrock` (suite complete).
- Rapports locaux (jamais commits) : `%LOCALAPPDATA%\GrimrockPrototype\UEValidationAgent\Runs\...`.
- Etat GitHub par commit : `UE5.5.4/LocalAuto` (`pending`, `success`, `failure`) via GitHub CLI authentifie. Sur echec, issue GitHub automatique avec extraits **filtres et tronques** ; jamais les logs complets ni les secrets.
- L'agent traite chaque SHA une seule fois, meme en cas d'echec. Une correction publiee sur `master` declenche automatiquement une nouvelle validation.
- Une seule execution simultanee : verrou Windows + tache `IgnoreNew`.
- Aucune modification de traduction anglaise, de DataAsset ou de contenu utilisateur.

## Prerequis (une fois)

1. Windows 11 + UE 5.5.4 deja installe dans `D:\UE_5.5`, outils Visual Studio C++ pour Unreal.
2. `git`, Git LFS et **GitHub CLI** (`gh`) disponibles dans le PATH **de votre session Windows**.
3. Authentification initiale GitHub CLI (une fois) : `gh auth login -h github.com` ; il faut des droits de creation des statuts de commit et des issues sur le depot. Le token de compte ne doit jamais etre copie dans le projet.
4. La session Windows du proprietaire de la tache doit etre ouverte, et l'ordinateur allume. La tache s'execute sous le compte utilisateur interactif, sans privilege administrateur demande.
5. Assez d'espace disque pour le worktree de validation, les assets LFS, Intermediate, Binaries et rapports.

## Activation (intervention unique sur Windows)

Dans PowerShell :

```powershell
cd D:\Development\GrimrockPrototype
# Ne pas ecraser les modifications locales : inspection avant mise a jour.
git status --short
git fetch origin master
# Mettre a disposition les nouveaux Scripts par fast-forward seulement, apres
# avoir protege les changements locaux si necessaire.
git pull --ff-only origin master
.\Scripts\InstallUEValidationAgent.ps1 -EngineRoot D:\UE_5.5
```

L'installateur effectue une verification des outils et de `gh auth`, installe une tache planifiee toutes les **3 minutes** et declenche la premiere verification. Aucune intervention n'est necessaire pour les commits suivants. Il ne telecharge pas de binaire opaque et ne modifie pas les comptes ou droits Windows.

Verifier :

```powershell
Get-ScheduledTask -TaskName 'GrimrockPrototype-UEValidation' | Get-ScheduledTaskInfo
Get-Content "$env:LOCALAPPDATA\GrimrockPrototype\UEValidationAgent\state.json"
```

Si `gh` n'est pas present, installer la CLI GitHub depuis l'installateur officiel (`winget install --id GitHub.cli --exact --source winget`) puis lancer `gh auth login -h github.com`. Le navigateur peut demander une autorisation initiale, jamais a chaque commit. Si PowerShell n'autorise pas `Register-ScheduledTask` pour le compte actuel, effectuer l'installation une fois dans une session adaptee ; ne pas passer la tache en SYSTEM.

## Exploitation / incidents

- **Travail utilisateur modifie** : le checkout `D:\Development\GrimrockPrototype` n'est jamais reinitialise et les modifications `*.uasset`, `*.umap`, etc. restent intactes.
- **Agent en echec** : ouvrir l'issue automatique `[UEValidation] Echec du commit ...`, et corriger le code dans GitHub. La correction publiee declenche une nouvelle execution.
- **Plusieurs commits pendant une execution** : `origin/master` sera relu au prochain cycle ; l'agent valide le dernier etat (pas chaque commit intermediaire).
- **Machine eteinte** : aucune execution tant que le poste est eteint ; elle reprend lors de la prochaine ouverture de session.
- **Verrouillage** : une validation en cours est terminee avant d'en declencher une nouvelle.
- **Nettoyage** : desactiver la tache avec `Disable-ScheduledTask -TaskName GrimrockPrototype-UEValidation` ou desinstaller via `Scripts/UninstallUEValidationAgent.ps1`. Aucun dossier ni rapport n'est supprime automatiquement.
- **Validation PIE visuelle** : le build et l'Automation sont automatiques, mais **la verification visuelle humaine** ne peut pas etre declaree reussie sans capture/test approprie. `-UseRHI` peut activer la RHI pour des tests concus a cet effet, mais ne garantit pas une recette PIE visuelle.

## Regle de publication

`VALIDATION-AUTO01` est un **jalon d'infrastructure autonome**, sans modification du moteur de combat. Son commit installe les scripts et la documentation ; aucune verification UE ne peut encore etre declaree reussie tant que la tache Windows n'a pas tourne. Ne pas qualifier ARMEMENT01.17.1.4 de valide avant son propre build, les tests et sa recette visuelle.

References :
- [GitHub : Ajouter un runner auto-heberge](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/add-runners)
- [GitHub : Securiser GitHub Actions](https://docs.github.com/en/actions/reference/security/secure-use)