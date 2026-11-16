# ARM-CLEAN06.4X.16.7.2B — Déclencheur sûr dans ValidateUE

Le grand agent UEValidationAgent.ps1 (199 866 octets)
a été restauré à l'identique sur le commit 44bc195f,
avec SilentTask et LocalAuto SUCCESS. Ne plus éditer ce
fichier via des lectures limitées : son blob original
doit demeurer inchangé.

Le petit script ValidateUE.ps1 contient désormais un
déclencheur ponctuel, à la fin d'une validation
réussie, uniquement si toutes ces conditions
sont réunies simultanément :
- checkout EXACT GrimrockPrototype_UEValidation ;
- Build effectué, pas de SkipBuild ni SkipAutomation ;
- filtre Automation exactement Grimrock ;
- dossier de rapport privé de l'agent Windows ;
- nom du commit égal au sujet autorisé de X.16.7.2B ;
- SHA Git valide et dossier de rapport préfixé par ce SHA.

Le helper ApplyARM_CLEAN06_4X72_ReceptacleRules.ps1
est analysé par le parseur PowerShell avant exécution,
et vérifie indépendamment le parent exact, la
liste blanche des trois fichiers source du commit,
les empreintes Git LFS des deux vrais .uasset,
les copies privées SHA-256 et l'identité des
deux références à convertir.

Après l'écriture, un deuxième processus UE doit
valider les deux packages, puis toute la suite
Grimrock doit réussir sur les binaires nouveaux.
Un nouveau commit enfant X.16.7.3, contenant
uniquement deux .uasset, est alors publié par
avance rapide et recevra sa propre validation
UE5.5.4 lors du cycle Windows suivant.

En cas d'échec avant publication, les originaux
sont restaurés avec contrôle SHA-256. Les
SaveGames, les niveaux, les clés et les Blueprints
restent hors de portée. Le contrôle PIE du
support de torche et du Gardien demeure nécessaire.

Ce déclencheur n'agit jamais lorsque le propriétaire
lance normalement ValidateUE dans son checkout
de développement. Ne pas relancer 06.4W/X.5.
