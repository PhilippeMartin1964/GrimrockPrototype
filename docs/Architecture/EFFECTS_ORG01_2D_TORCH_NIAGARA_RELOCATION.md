# EFFECTS-ORG01.2D — Relocaliser le Niagara de la torche

**Objectif :** réunir les effets propriétaires à Grimrock dans `Effects/`,
sans déplacer les textures et matériaux généraux avant analyse séparée.

## Migration exacte

| Type | Ancien | Nouveau |
|---|---|---|
| Système Niagara | `/Game/GrimrockPrototype/Art/FX/NS_Flame_8_Torch` | `/Game/GrimrockPrototype/Effects/Fire/Torch/NS_Flame_8_Torch` |

Dépendance directe à resauvegarder :
`/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Torch`.
Dépendances du Niagara qui **restent dans Art** :
`MI_Distortion_1`, `MI_Flame_8`. Les maîtres et textures
sous `Art/Materials` et `Art/Textures` restent à leur place.

## Contrat de sécurité

- L'Asset Registry UE5.5.4 a confirmé via issue #50 :
  `DA_Item_Torch → NS_Flame_8_Torch → MI_Flame_8 + MI_Distortion_1`.
- La migration est effectuée **via IAssetTools::RenameAssets**
  dans le worktree Windows dédié après validation intégrale de la
  suite Grimrock, et non via git mv ou Explorateur Windows.
- L'outil exige un seul Niagara source, aucun paquet cible,
  un seul référent `DA_Item_Torch` et les deux matériaux attendus.
- Exactement trois changements .uasset attendus : suppression de
  l'ancien Niagara, création du nouveau Niagara et resauvegarde de
  `DA_Item_Torch`. Aucun autre fichier ne peut être poussé.
- L'outil corrige uniquement les redirectors propres à cette
  migration, sauvegarde les deux packages et refuse tout résidu à
  l'ancien emplacement.
- Aucun asset acheté FireFX, aucune carte `L_Dungeon`, aucun
  autre objet du donjon ne doit être modifié.
- **Infrastructure de validation figée :** le script
  `UEValidationAgent.ps1` est conservé octet pour octet au blob Git
  `b87446812fca8b82cf87fdb975f6196c7f6691bb` validé dans
  `b1b232a5` et confirmé dans `2345eb50`.
  **Il ne déclenche PAS le script de migration de torche** et ne doit pas
  être modifié pour intégrer une tâche de migration à usage unique.
- Le script autonome `Scripts/MigrateEffectsOrgTorch.ps1` est préparé
  mais nécessite un contexte Windows Unreal explicitement capable
  d'exécuter cette commande, **distinct de l'agent de validation**.
  Aucun dépôt GitHub, test `-NullRHI` ou statut vert ne constitue une
  preuve de renommage des .uasset.
- Le commit d'assets, lorsqu'il aura été effectivement créé par Unreal,
  portera le préfixe `EFFECTS-ORG01.2D-ASSET:` et devra ensuite passer
  toute la suite de validation Windows, indépendamment de l'outil de migration.
- Une fois le commit d'assets validé par l'agent UE5.5.4,
  les chemins de tests temporaires (ancien ou nouveau) sont
  remplacés par le chemin canonique uniquement. Le script,
  commandlet et hook ponctuels sont ensuite supprimés.

## Tests

- `Grimrock.Editor.EffectsOrg01.Inventory`
- `Grimrock.Editor.EffectsOrg01.DependencyContract`
- `Grimrock.Editor.EffectsOrg01.TorchMigrationPreflight` : appelle le
  **véritable commandlet** sans `-Apply` avant migration, vérifie l'absence
  de mutation; après migration, vérifie les nouvelles dépendances.
  Ce test de transition sera supprimé avec l'outillage provisoire.
- `Grimrock.Items.VFX01_3.*` et `Grimrock.Items.VFX01_4B.*`
- Compilation et suite complète `Grimrock` dans UE5.5.4.
- Vérification visuelle PIE de la torche allumée/éteinte
  encore nécessaire (un test -NullRHI ne démontre pas le rendu).

## Portée

Ce jalon déplace **uniquement** le système Niagara.
Les autres matériaux (en particulier ceux de l'huile, couplés
aux scripts de réparation et aux tests de production) feront
l'objet de migrations séparées. Les profils VFX FireFX
restent des assets privés locaux non versionnés.

## État vérifié du travail (6 novembre 2026)

- `master` au commit `2345eb50` : compilation UE5.5.4 + suite Grimrock **vertes**.
- Le Niagara se trouve encore sous `Art/FX`. Aucune migration binaire
  de la torche n'est attestée, contrairement au renommage déjà terminé
  de la potion.
- Aucun accès interactif à la station Windows ou canal de commande
  distinct n'est actuellement exposé au connecteur GitHub. Le job Windows
  existant valide uniquement les commits ; il ne constitue pas un exécuteur
  général de scripts. Il est interdit de contourner cette séparation en
  ajoutant du code de migration mutatif dans un test automatique de build.


## Exécuteur Windows indépendant — EFFECTS-ORG01.2D-RUNNER

Le script \`Scripts/RunEffectsOrgTorchStandalone.ps1\` contient l'exécution complète **hors agent de validation**. Son comportement par défaut est en lecture seule ; l'argument explicite \`-Apply\` est nécessaire pour agir.

L'exécution crée exclusivement un **worktree détaché distinct** sous
\`D:\Development\GrimrockPrototype_EffectsOrgTorch\`, sans modifier ni
le checkout \`D:\Development\GrimrockPrototype\` ni celui de l'agent
\`GrimrockPrototype_UEValidation\`.

Elle hydrate Git LFS, compile UE Editor et exécute
\`Grimrock.Editor.EffectsOrg01.TorchMigrationPreflight\` dans son propre
projet, vérifie qu'aucun fichier n'a été modifié, puis appelle le
commandlet natif Unreal avec \`-Apply\`. Le script de migration vérifie
un ensemble strict de **trois** changements, produit les sauvegardes
privées SHA-256 et pousse un commit fast-forward de la branche master.

L'agent Windows existant valide ensuite automatiquement ce nouveau
commit d'assets avec la **suite complète Grimrock**. Cet agent reste
intégralement inchangé.

**Contrat de lancement :** le nouveau HEAD Git doit être un jalon
\`EFFECTS-ORG01.2D-PREP:\`, \`EFFECTS-ORG01.2D-PREFLIGHT:\` ou
\`EFFECTS-ORG01.2D-RUNNER:\`; tout autre jalon est refusé. Un commit
contenant déjà la migration ne correspond pas à ces préfixes :
aucune réexécution intempestive n'est autorisée.

Commande destinée à un **exécuteur Windows explicitement autorisé**
(même utilisateur que la tâche Unreal, avec UE5.5.4 et Git LFS) :

    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "D:\Development\GrimrockPrototype\Scripts\RunEffectsOrgTorchStandalone.ps1" -Apply

Le script effectue son propre \`git fetch\` et ne nécessite pas de
\`git pull\` dans le checkout utilisateur. Il refuse un ancien worktree
indépendant existant et n'efface jamais automatiquement les sauvegardes.

**Important :** le connecteur GitHub disponible dans cette conversation
ne peut pas démarrer un processus Windows sur le PC. La validation
automatique GitHub/UE ne déclenche donc **pas** le script autonome. Il
faut un véritable exécuteur Windows indépendant pour effectuer la
migration réelle ; le simple commit du script n'a aucun effet sur les
assets. Il est interdit de contourner cette séparation en réinjectant
un hook de migration dans \`UEValidationAgent.ps1\`.

## Contrôle de syntaxe PowerShell dans la suite Grimrock

Le test \`Grimrock.Editor.EffectsOrg01.TorchMigrationScriptsSyntax\`
appelle \`Scripts/CheckEffectsOrgMigrationScripts.ps1\`, qui utilise
**uniquement le parser Windows PowerShell 5.1** pour contrôler les
deux scripts indépendants. Ni \`-Apply\`, ni \`git commit\`, ni
\`AssetTools::RenameAssets\` ne sont lancés par ce test.

La réussite des tests de syntaxe et du précontrôle ne vaut jamais
preuve de migration : il faut également constater le commit des
assets et son statut \`UE5.5.4/LocalAuto = success\`, puis supprimer
les outils provisoires et verrouiller les nouveaux chemins dans les
contrats de test.
