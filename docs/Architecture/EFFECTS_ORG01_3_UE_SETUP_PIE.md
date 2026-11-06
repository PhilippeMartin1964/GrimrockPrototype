# EFFECTS-ORG01.3 — Configuration UE 5.5.4 et vérifications PIE

Cette fiche est le **passage de relais à l'éditeur Unreal**, après
migration et validation effective des assets par l'agent Windows.

**Ne pas confondre :** la compilation/les tests UE5.5.4 \`-NullRHI\`
confirment les dépendances de packages, pas le rendu Niagara. Une
migration de torche ne peut être dite terminée qu'après un commit
d'assets réel (Unreal AssetTools), sa validation \`LocalAuto=success\`,
puis la suppression des chemins d'assets historiques dans le code/tests.

## 1. Organisation canonique des effets

- \`Effects/Alchimie/M_Potion_Spark\` : matériau des bulles remontantes
  de la potion de soin. La chaîne vérifiée est
  \`DA_Item_Potion_Healing_01 → NS_LiquideSparkle → M_Potion_Spark\`.
  L'ancien nom \`M_Teleporter_Spark_02\` est proscrit.
- \`Effects/Teleporter/\` : sept assets déjà regroupés, y compris
  \`NS_Teleporter_Blue\`, le matériau d'étincelles distinct,
  \`M_Teleporter_Volume\`, \`M_Teleporter_Wisp\`, les instances
  d'état actif/inactif et la texture de bruit.
- \`Effects/Fire/Torch/NS_Flame_8_Torch\` : **chemin cible** de la
  torche. Vérifier que la migration a réellement eu lieu : aucune
  copie ni redirector dans \`Art/FX/\` avant d'appliquer cette étape.
- \`Effects/Fire/Local/\` : profils locaux référencant le pack commercial.
  Répertoire exclu de Git, pas un nouvel emplacement à versionner.
- \`Effects/Alchimie/Catalyst/\` : profils et présentations du
  catalyseur, à créer/configurer dans l'éditeur.
- \`Art/Materials\` et \`Art/Textures\` : matériaux maîtres, instances
  et textures partagés. Ne pas les déplacer pour le simple aspect
  esthétique d'une arborescence.

Le pack \`Content/FireFX/\` reste à sa place, privé et exclu de Git.

## 2. Créer le profil local de feu au sol

Dans Content Browser :

1. Dans \`Content/GrimrockPrototype/Effects/Fire/\`, créer
   \`Local/\` s'il est absent.
2. Dans \`Local/\` : clic droit → Miscellaneous → Data Asset →
   classe \`GridSurfaceVfxProfileAsset\` → nom \`DA_GroundFire\`.
3. Sous \`Settings\`, sélectionner pour \`GroundFireSystem\` le Niagara
   \`/Game/FireFX/FX/NS_FFX_Ground_Fire\`.
4. Conserver d'abord les valeurs C++ par défaut :
   \`GroundFireScale=1.0\`, \`GroundFireCoverage=0.90\`,
   \`FlameHeightCm=75\`, \`FlameDensity=1.0\`,
   \`bEnableSmoke=true\`, \`SmokeDensity=0.5\`,
   \`MaxFireCells=48\`, \`MaxFireLights=8\`,
   \`FireLightIntensity=650\`, \`FireLightRadiusCells=1.2\`.
5. Les cinq champs de noms de \`Niagara User Parameters\`
   peuvent rester \`None\` ; ne renseigner \`User.*\` qu'après avoir
   vérifié le nom exact et le type dans l'asset Niagara local.

Les paramètres \`GroundFireCoverage\`, \`FlameHeightCm\`, etc. n'ont
un effet direct sur la simulation Niagara **que si les paramètres
User. correspondants sont correctement exposés et liés**. Les
valeurs numériques ne fabriquent pas de paramètres dans FireFX.

## 3. Configurer le catalyseur séparément

- Créer un \`GridCatalystVfxProfileAsset\`, par exemple
  \`Effects/Alchimie/Catalyst/DA_Catalyst_Default\`.
- \`ProjectileMesh\` : \`SM_Catalyst_01\` (dans \`Meshes/Items\`).
- \`RunePulseMaterial\` : \`M_SurfaceCatalyst_RunePulse\`.
- \`EmberMaterial\` : \`M_SurfaceCatalyst_Ember\`.
- \`SparkMesh\` : facultatif.
- Conserver pour le premier essai :
  \`FlightDurationSeconds=0.42\`,
  \`PulseDurationSeconds=0.95\`,
  \`WaveDelayPerCellSeconds=0.085\`,
  \`FlightArcHeightCm=82\`.

Ce profil peut être versionné si aucune référence à un asset commercial
local n'y est ajoutée. L'aspect du flacon dans le mesh ne doit pas être
confondu avec le projectile visuel Catalyst.

## 4. Assigner les profils au runtime

Ouvrir \`Content/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor\`,
puis **Class Defaults → Combat / VFX** :

- \`FireVfxProfile\` : \`DA_GroundFire\`.
- \`CatalystVfxProfile\` : \`DA_Catalyst_Default\`.

Compile, Save, puis lancer PIE dans \`L_Dungeon\`.

**Précaution Git cruciale :** \`BP_GridLevelRuntimeActor\` est
versionné. L'affectation d'un \`DA_GroundFire\` ignoré par Git
rendrait tout commit du Blueprint impropre au clonage (référence
sérialisée vers un asset absent sur les autres postes). Ce réglage doit
être traité comme **local pour l'essai** ; ne pas pousser le BP ainsi
modifié. Un système de distribution du profil privé reste une décision
séparée, à régler avant tout packaging destiné aux joueurs.

Aucun paramètre global d'effet ne doit être renseigné dans
\`DA_GridLevelAsset\` : le runtime référence directement ses deux profils.

## 5. Vérifications PIE à effectuer

| Scénario | Résultat attendu | À vérifier |
|---|---|---|
| Potion de soin | Bulles montantes dans \`NS_LiquideSparkle\` à partir de \`M_Potion_Spark\` | Débit, matériau, taille, disparition |
| Téléporteur | \`NS_Teleporter_Blue\` et matériaux propres au téléporteur | Wisp, Spark, volume, état actif/inactif |
| Torche | \`NS_Flame_8_Torch\` résolu depuis son chemin canonique | Allumage/extinction, position, lumière |
| Feu au sol | \`NS_FFX_Ground_Fire\` via \`DA_GroundFire\` | Taille sur une case 200 cm, intensité, fumée |
| Huile | Matériau d'huile existant et inflammation par catalyseur | Animation et durée sans changer les dégâts |
| Catalyseur | Flacon/onde/braises/éventuelles étincelles via profil | Arc de trajectoire, pulse, délais par case |
| Poison / autres réactions | VFX corrects, indépendants de la logique C6 | Apparition, retrait, pas de doublons |
| Changement de niveau | Aucune flamme ou particule résiduelle | Destruction propre des composants visuels |

Les règles de gameplay (réactions, dégâts, déplacements) doivent
rester identiques à celles d'avant la migration. Le feu visuel absent
à cause d'un profil non affecté n'implique **pas** que les dégâts
ont été désactivés.

## 6. Portes de sortie de EFFECTS-ORG01

Avant de déclarer le chantier **terminé** :

1. Déplacement réel et vérifié du Niagara torche via Unreal AssetTools.
2. Commit d'assets strict, validation complète UE5.5.4 du commit
   et contrôle de la référence sérialisée \`DA_Item_Torch\`.
3. Suppression du commandlet et des scripts de migration à usage unique,
   retrait de tout fallback vers \`Art/FX\`, tests du chemin final strict.
4. Réexécution de la compilation et des tests automatisés sur master.
5. Configuration des profils (locale pour FireFX) et contrôle PIE
   effectués dans l'éditeur avec le contenu commercial réellement installé.
6. Vérification de l'absence de matériaux/instances de feu ou
   d'alchimie involontairement dupliqués, en évitant de déplacer des
   textures et maîtres communs sans justification.

**Cette fiche ne signifie pas que les points 1–6 sont déjà réalisés.**
Elle distingue précisément les opérations automatisables de la
validation visuelle réservée à l'utilisateur.
