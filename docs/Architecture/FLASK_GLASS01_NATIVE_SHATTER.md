# FLASK-GLASS01 — Bris de verre léger des flasques

## Résultat livré

Le point d'impact C++ déjà utilisé par l'huile et le catalyseur produit
désormais **un petit éclatement de verre visible même sans Niagara assigné**.
L'animation reprend le principe des huit gouttelettes d'huile : composants
visuels 3D, mouvement analytique en cm/s avec gravité, rotation, petit
rebond amorti au sol et disparition après la durée d'impact.
La trajectoire des bouteilles, leurs sons, la réaction de surface et
les dégâts RPG03.6 sont inchangés.

L'effet utilise **par défaut** la géométrie moteur
`/Engine/BasicShapes/Cube` comprimée en éclats fins (1,5–4,5 cm)
avec le matériau versionné `M_SurfaceCatalyst_Flask`.
Ce sont des **fragments fins rectangulaires de substitution**,
pas encore un mesh de triangulation artisanal. On peut remplacer
`GlassShardMesh` par un mesh de vrai éclat irrégulier fabriqué
dans Blender et versionné dans Grimrock, sans modifier le C++.

## Réglage dans BP_GridLevelRuntimeActor → Class Defaults → Combat|VFX|Flask|Impact

| Champ | Défaut |
|---|---|
| `FlaskGlassBreakVfx` | `None` → éclats natifs ; Niagara choisi → remplace les éclats natifs |
| `FlaskGlassBreakScale` | 1.0 |
| `FlaskGlassBreakMaxLifetimeSeconds` | 1.0 seconde (limite aussi la durée de l'effet natif) |
| `GlassShardMesh` | `/Engine/BasicShapes/Cube` |
| `GlassShardMaterial` | `M_SurfaceCatalyst_Flask` |
| `GlassShardCount` | 14 |
| `GlassShardLaunchSpeedCmPerSec` | 155 |

Ces réglages peuvent aussi être surchargés sur l'instance de
`BP_GridLevelRuntimeActor` placée dans `L_Dungeon`.

Les débris n'ont aucune collision, n'influencent pas le combat et ne
sont pas ramassables. Le rebond est simulé dans la présentation C++,
sans créer de corps physiques. Une seule minuterie existe pendant
l'éclatement (30 Hz), puis elle est supprimée ; le nettoyage du niveau
retire également tous les fragments restants. Plusieurs collisions
de flasque successives remplacent le précédent burst natif.

## Pourquoi pas un asset Niagara immédiatement ?

Un véritable `NS_FlaskGlassBreak.uasset` **n'est pas créé par
ce commit** : sa construction exige l'éditeur Niagara UE 5.5.4 et
le choix de ses émetteurs et Mesh Renderers. Inventer un fichier
`.uasset` via GitHub rendrait le niveau invalide.

Si un artiste crée ultérieurement
`/Game/GrimrockPrototype/Effects/Glass/NS_FlaskGlassBreak`
avec un burst de 12–16 meshes triangulaires, il suffit de
l'affecter à `FlaskGlassBreakVfx`. Le runtime utilise alors le
Niagara existant à la place des éclats natifs ; aucune duplication
de l'explosion n'a lieu.

## Vérifications PIE

1. `L_Dungeon` : lancer l'huile et observer 14 fragments vers le sol
   en même temps que le splash ; aucun lancement de fragments au départ.
2. Lancer le catalyseur : le même bris est déclenché à l'impact, puis
   les réactions de surfaces habituelles se produisent.
3. Tester plusieurs lancers successifs, quitter le PIE pendant l'effet,
   vérifier qu'aucune particule ni timer ne subsiste.
4. Vérifier que les fragments sont suffisamment visibles avec
   `M_SurfaceCatalyst_Flask`, et choisir un matériau de verre
   dédié si nécessaire.
5. Mettre `FlaskGlassBreakVfx` sur un Niagara valide :
   le runtime doit utiliser ce Niagara seul.

La compilation et les tests automatiques vérifient la trajectoire
numérique et le chargement des ressources, mais **ne démontrent pas
la qualité visuelle du matériau en PIE**.
