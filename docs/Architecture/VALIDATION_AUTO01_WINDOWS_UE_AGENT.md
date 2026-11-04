# VALIDATION-AUTO01 — Agent de validation locale UE 5.5.4

**Statut : VALIDATION-AUTO01.5 — sous-module Lua54 automatiquement initialise avant validation UE ; validation Windows du correctif a confirmer.**

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
- Rapports locaux (jamais commits) : `%LOCALAPPDATA%\GrimrockPrototype\UEValidationAgent\Runs\...` ; journal de demarrage `%LOCALAPPDATA%\GrimrockPrototype\UEValidationAgent\Agent.latest.log` (PowerShell Transcript) et stderr du harness conserve dans `Agent.stderr.log`. Le harness `ValidateUE.ps1` est execute dans un **second processus PowerShell invisible** (`-WindowStyle Hidden`), avec code de sortie fiable. Le processus agent planifie est egalement lance avec `-WindowStyle Hidden` : pas de fenetre PowerShell persistante lors des prochaines executions.
- Etat GitHub par commit : `UE5.5.4/LocalAuto` (`pending`, `success`, `failure`) via GitHub CLI authentifie. Sur echec des tests, issue GitHub automatique avec **uniquement des diagnostics structures limites** (noms de fichiers sources, lignes, codes compilateur et chemins de tests Grimrock strictement valides), jamais des lignes brutes ou chemins absolus. Le journal complet reste local.
- L'agent traite un SHA une seule fois en cas de succes ou d'echec de compilation/Automation ; une panne d'infrastructure Git, LFS ou de publication est **reessayee** au cycle suivant. Un changement de filtre redemarre aussi la validation.
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

L'installateur effectue une verification des outils, du parseur PowerShell 5.1 et de `gh auth`, installe une tache planifiee toutes les **3 minutes** et declenche la premiere verification en tache de fond invisible. Lors de la reinstallation, si une validation est deja en cours, il conserve cette execution et ne declenche pas une seconde validation. Il est **idempotent** et refuse de remplacer une tache de meme nom qui ne correspond pas a cet agent. Aucune intervention n'est necessaire pour les commits suivants. Le correctif .1 n'utilise plus les parametres invalides `-DisallowStartIfOnBatteries`/`-StopIfGoingOnBatteries` : les options disponibles sont detectees avant appel, et le mode batterie est degrade avec avertissement si une option est absente. Il ne telecharge pas de binaire opaque et ne modifie pas les comptes ou droits Windows.

Preflight optionnel, sans ecriture de tache :

```powershell
.\Scripts\InstallUEValidationAgent.ps1 -EngineRoot D:\UE_5.5 -CheckOnly
```

Verifier l'installation et la premiere execution (le fichier `state.json` n'apparait qu'apres qu'un commit a ete traite) :

```powershell
Get-ScheduledTask -TaskName 'GrimrockPrototype-UEValidation' | Get-ScheduledTaskInfo
Get-Content "$env:LOCALAPPDATA\GrimrockPrototype\UEValidationAgent\Agent.latest.log" -Tail 70
Get-Content "$env:LOCALAPPDATA\GrimrockPrototype\UEValidationAgent\state.json" -ErrorAction SilentlyContinue
```

Si `gh` n'est pas present, installer la CLI GitHub depuis l'installateur officiel (`winget install --id GitHub.cli --exact --source winget`) puis lancer `gh auth login -h github.com`. Le navigateur peut demander une autorisation initiale, jamais a chaque commit. Si PowerShell n'autorise pas `Register-ScheduledTask` pour le compte actuel, effectuer l'installation une fois dans une session adaptee ; ne pas passer la tache en SYSTEM.

## Exploitation / incidents

- **Travail utilisateur modifie** : le checkout `D:\Development\GrimrockPrototype` n'est jamais reinitialise et les modifications `*.uasset`, `*.umap`, etc. restent intactes.
- **Agent en echec au demarrage** : consulter `Agent.latest.log`, puis le `LastTaskResult` du Planificateur. Si Git/LFS est indisponible, l'agent retente au prochain cycle. Si la compilation/Automation echoue, consulter l'issue `[UEValidation] Echec du commit ...` (sans secrets/logs) et les fichiers locaux sous `Runs/`. Une correction publiee declenche une nouvelle validation.
- **Plusieurs commits pendant une execution** : `origin/master` sera relu au prochain cycle ; l'agent valide le dernier etat (pas chaque commit intermediaire).
- **Machine eteinte** : aucune execution tant que le poste est eteint ; elle reprend lors de la prochaine ouverture de session.
- **Verrouillage** : une validation en cours est terminee avant d'en declencher une nouvelle.
- **Nettoyage** : desactiver la tache avec `Disable-ScheduledTask -TaskName GrimrockPrototype-UEValidation` ou desinstaller via `Scripts/UninstallUEValidationAgent.ps1`. Aucun dossier ni rapport n'est supprime automatiquement.
- **Validation PIE visuelle** : le build et l'Automation sont automatiques, mais **la verification visuelle humaine** ne peut pas etre declaree reussie sans capture/test approprie. `-UseRHI` peut activer la RHI pour des tests concus a cet effet, mais ne garantit pas une recette PIE visuelle.

## Correctifs VALIDATION-AUTO01.1 (4 novembre 2026)

- Remplacement de la mauvaise API du Planificateur (`-DisallowStartIfOnBatteries`, `-StopIfGoingOnBatteries`) par detection de `-AllowStartIfOnBatteries` et `-DontStopIfGoingOnBatteries`. References : documentation Microsoft `New-ScheduledTaskSettingsSet`.
- Choix explicite de Windows PowerShell 5.1 dans la tache planifiee, y compris si l'installation a ete lancee sous PowerShell 7.
- Test de parsing natif des scripts avant inscription, `-CheckOnly`, installation repetable et refus de remplacer une tache homonyme etrangere.
- Transcript local pour diagnostiquer les erreurs de preflight, validation dans un processus distinct, vrais codes de sortie, retry des erreurs d'infrastructure.
- Verifications du chemin et de l'appartenance du worktree a ce depot. Aucun `reset --hard`, `clean` ni suppression.
- Aucune validation Windows/UE declaree tant que la premiere execution n'a pas termine avec succes.

## Regle de publication

`VALIDATION-AUTO01` est un **jalon d'infrastructure autonome**, sans modification du moteur de combat. Son commit installe les scripts et la documentation ; aucune verification UE ne peut encore etre declaree reussie tant que la tache Windows n'a pas tourne. Ne pas qualifier ARMEMENT01.17.1.4 de valide avant son propre build, les tests et sa recette visuelle.

References :
- [GitHub : Ajouter un runner auto-heberge](https://docs.github.com/en/actions/how-tos/manage-runners/self-hosted-runners/add-runners)
- [GitHub : Securiser GitHub Actions](https://docs.github.com/en/actions/reference/security/secure-use)

## Correctif VALIDATION-AUTO01.2 : deux fenetres PowerShell (4 novembre 2026)

La premiere version lancait `powershell.exe` depuis le Planificateur dans la session interactive, puis `Start-Process powershell.exe` pour le harness. Windows pouvait donc afficher deux consoles, dont l'une pouvait rester vide pendant un long build Unreal. Ce comportement n'indiquait pas necessairement un blocage.

- Tache planifiee : `powershell.exe -WindowStyle Hidden -NoProfile -NonInteractive ...`.
- Harness : `Start-Process -WindowStyle Hidden -RedirectStandardOutput ... -RedirectStandardError ... -Wait -PassThru`.
- Aucune redirection des logs n'est perdue ; suivre `Agent.latest.log` et `Runs/.../Agent.log`, ainsi que le contexte GitHub `UE5.5.4/LocalAuto`.
- L'installation est reentrante : elle ne declenche pas une nouvelle validation si la tache tourne deja.
- Une execution deja lancee avec la version .1 reste visible jusqu'a sa fin, car les parametres de lancement d'un processus existant ne peuvent pas etre modifies retroactivement.

Mise a jour de la tache : `git pull --ff-only origin master` depuis le checkout propre, puis `.\Scripts\InstallUEValidationAgent.ps1 -EngineRoot D:\UE_5.5`. Cette operation ne touche ni aux assets ni au worktree. Si une validation reste active, laisser cette execution terminer ; les prochains lancements seront invisibles.

**Limitations :** le demarrage de binaires natifs Windows peut produire un tres bref flash sur certaines configurations ; cela ne doit jamais laisser deux consoles persistantes. Seul le poste Windows peut verifier ce comportement.


## Correctif VALIDATION-AUTO01.3 : diagnostic automatique des echecs (4 novembre 2026)

Le premier run du commit a57a7eaa a echoue (code UE 1), et son issue publique #9 ne contient pas de detail technique. Pour eviter toute copie manuelle recurrente, les futurs echecs remontent maintenant un bilan structure minimal en provenance du journal de validation et, si present, du rapport Automation JSON.

Seuls sont publies les fichiers sources de type cpp/h/cs/inl avec leur numero de ligne et un code compilateur reconnu, un compte d'echecs et les identifiants de tests de la forme Grimrock.x.y. Toute ligne libre, message brut, chemin absolu et donnee personnelle sont exclus de l'issue publique. Si le format du log ne permet pas un extrait fiable, le bilan mentionne explicitement ses limites. Les rapports complets restent locaux. Cette modification ne peut pas modifier retroactivement l'issue du premier run, qui precede ce correctif.

L'agent et le planificateur restent independants du checkout de developpement. Pour passer le planificateur en mode cache, mettre a jour master puis relancer une fois InstallUEValidationAgent.ps1.


## Correctif VALIDATION-AUTO01.4 : bootstrap auto-actualise et chemin Git Windows

Les essais du commit 705d02b0 ont retourne plusieurs erreurs `setup` avant l'execution UE. Une cause possible est le controle du worktree par comparaison de chemins : Git for Windows utilise parfois `D:/...`, Windows `D:\...`. Les chemins sont maintenant normalises en chemins Windows absolus avant comparaison.

Le planificateur execute maintenant `Scripts/UEValidationBootstrap.ps1`, un petit lanceur stable qui verifie le depot `origin`, effectue `git fetch --no-tags origin master`, extrait la version de `Scripts/UEValidationAgent.ps1` associee au SHA exact du commit dans `%LOCALAPPDATA%\GrimrockPrototype\UEValidationAgent\AgentVersions`, verifie sa syntaxe et l'execute.

Le bootstrap ne fait jamais `git pull`, `git reset`, `git clean` ou `checkout` sur la copie de developpement ; seuls les objets et references Git sont mis a jour par fetch. Les fichiers de travail et assets locaux restent proteges. Les futures corrections ordinaires de `UEValidationAgent.ps1` sur `master` sont prises automatiquement, sans reinstaller la tache. En revanche, un changement du bootstrap ou de sa configuration planifiee necessiterait une mise a jour explicite.

Le statut GitHub d'un echec en phase `setup` inclut un code borne : WORKTREE_REGISTRATION, WORKTREE_DIRTY, WORKTREE_LOCATION, LFS, GITHUB_AUTH, GIT, HARNESS_MISSING ou OTHER ; jamais le texte libre d'une erreur Windows. Les journaux complets restent uniquement locaux : Bootstrap.latest.log, Agent.latest.log et Runs/.

**Mise en route de la nouvelle tache (une seule fois) :**
```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\InstallUEValidationAgent.ps1 -EngineRoot D:\UE_5.5
```

**Securite :** le bootstrap execute volontairement le code versionne de `master` sur votre poste. Proteger strictement les droits d'ecriture de master, car tout code malveillant pousse sur cette branche pourrait aussi s'executer sous votre compte Windows.


## Correctif VALIDATION-AUTO01.5 — sous-module Lua54 obligatoire

La premiere compilation reellement lancee par l'agent sur le commit `b7b9e5a6` a echoue avec le code MSVC `C1083` : `Lua54.cpp` (include `onelua.c`) et `GridLuaVm.cpp` (include `lua.h`). Le depot contient `ThirdParty/Lua54` comme **sous-module Git** vers `https://github.com/lua/lua.git`, epingle au commit `6e22fedb74cf0c9b6656e9fce8b7331db847c605`. Un `git worktree add` n'initialise pas automatiquement ce sous-module ; `git lfs pull` ne le restaure pas non plus.

L'agent execute desormais, sur **le seul worktree de validation**, avant `git lfs pull` et avant la compilation :

```powershell
git -C D:\Development\GrimrockPrototype_UEValidation submodule update --init -- ThirdParty/Lua54
```

Il controle ensuite la presence de `onelua.c`, `lua.h`, `lauxlib.h` et `lualib.h`. Cette etape est repetable sur les prochains commits. Une panne reseau Git/submodule laisse un echec d'infrastructure reessayable ; aucun reset, clean ou changement de la copie de developpement n'est effectue.

La version de l'agent est prise automatiquement depuis `origin/master` par `UEValidationBootstrap.ps1`. **Aucun nouveau `git pull` ni reinstallation du Planificateur ne sont requis** pour cette correction de l'agent. Il faut simplement laisser la tache automatique effectuer son prochain cycle sur la machine Windows allumee, session utilisateur ouverte.

Suivi : [GitHub issue #10](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/10).
