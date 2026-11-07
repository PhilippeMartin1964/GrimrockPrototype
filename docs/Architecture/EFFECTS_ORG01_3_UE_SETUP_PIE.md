# EFFECTS-ORG01.3 — Configuration UE 5.5.4 et vérifications PIE

> **Actualisation FIREFX05 — 7 novembre 2026.** `DA_GroundFire` et
> `BP_GridLevelRuntimeActor` sont versionnés dans Git, le Blueprint
> référence directement le profil. Seul `Content/F_FreeFlameFx_Pack/`
> est exclu en tant que pack tiers. La configuration locale par
> `Saved/Config/GrimrockLocalVfx.ini` a été supprimée.
> Voir `FIREFX05_VERSIONED_ASSETS.md` pour la règle canonique.

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
- `Effects/Fire/Local/DA_GroundFire` : **profil Grimrock versionné** ; le nom historique `Local` ne signifie plus « ignoré ».
- \`Effects/Alchimie/Catalyst/\` : profils et présentations du
  catalyseur, à créer/configurer dans l'éditeur.
- \`Art/Materials\` et \`Art/Textures\` : matériaux maîtres, instances
  et textures partagés. Ne pas les déplacer pour le simple aspect
  esthétique d'une arborescence.

Le pack tiers est installé dans `Content/F_FreeFlameFx_Pack/`, qui est exclu de Git.

## 2. Profil de feu au sol déjà créé et versionné

Dans le Content Browser, ouvrir
`Content/GrimrockPrototype/Effects/Fire/Local/DA_GroundFire`.
Cet asset **existe et est versionné** depuis le commit `a0bfc58e`.
Il doit conserver sa référence `GroundFireSystem` vers le Niagara réellement
installé dans `Content/F_FreeFlameFx_Pack/`.

Le pack tiers n'est pas versionné ; le Data Asset Grimrock l'est.
Le lien est une référence souple : aucun Niagara propriétaire n'est copié
dans le dépôt.

Les valeurs `GroundFireScale`, `GroundFireCoverage`, `FlameHeightCm`,
`FlameDensity`, `SmokeDensity` et `FireTint` sont des réglages du profil.
Elles ne modifient les particules Niagara que si les vrais `User Parameters`
du système exposent des noms et types compatibles avec les champs configurés.
Vérifier cela directement dans l'éditeur avant d'activer les liaisons.

## 3. Configurer le catalyseur séparément

- Créer un \`GridCatalystVfxProfileAsset\`, par exemple
  \`Effects/Alchimie/Catalyst/DA_Catalyst_Default\`.
- \`ProjectileMesh\` : \`SM_Catalyst_01\` (dans \`Meshes/Items\`).
- \`RunePulseMaterial\` : \`M_SurfaceCatalyst_RunePulse\`.
- \`EmberMaterial\` : \`M_SurfaceCatalyst_Ember\`.
- \`SparkMesh\` : facultatif.
- Conserver pour le premier essai :
  `LaunchSpeedCmPerSec=1300`, `PulseDurationSeconds=0.95`,
  `WaveDelayPerCellSeconds=0.085`. Le vol est balistique, sa durée est calculée.

Ce profil Grimrock doit être versionné, même si une référence souple pointe vers le pack tiers. L'aspect du flacon dans le mesh ne doit pas être
confondu avec le projectile visuel Catalyst.

## 4. Assigner les profils au runtime

Ouvrir \`Content/GrimrockPrototype/Blueprints/Runtime/BP_GridLevelRuntimeActor\`,
puis **Class Defaults → Combat / VFX** :

- \`FireVfxProfile\` : \`DA_GroundFire\`.
- \`CatalystVfxProfile\` : \`DA_Catalyst_Default\`.

Compile, Save, puis lancer PIE dans \`L_Dungeon\`.

**Politique Git actuelle :** les Class Defaults du Blueprint runtime,
`DA_GroundFire` et tous les assets Grimrock sont versionnés.
Seul le pack tiers `Content/F_FreeFlameFx_Pack/` est exclu.
Le mécanisme de substitution par `GrimrockLocalVfx.ini` a été supprimé
dans FIREFX05.

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
5. Configuration des profils versionnés et contrôle PIE
   effectués dans l'éditeur avec le contenu commercial réellement installé.
6. Vérification de l'absence de matériaux/instances de feu ou
   d'alchimie involontairement dupliqués, en évitant de déplacer des
   textures et maîtres communs sans justification.

**Cette fiche ne signifie pas que les points 1–6 sont déjà réalisés.**
Elle distingue précisément les opérations automatisables de la
validation visuelle réservée à l'utilisateur.
