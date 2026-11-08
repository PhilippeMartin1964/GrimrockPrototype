# FIREFX11 — Progressive Ground Fire Ignition

## Objectif

Lorsqu'une flasque de catalyseur embrase une flaque d'huile répartie
sur plusieurs cellules de grille, les flammes apparaissent depuis
la case d'impact, puis se propagent **sur les seules cases réellement
converties de Oil en Fire**. Chaque case développe ses flammes depuis
une petite surface d'émission centrale jusqu'à la surface finale
définie dans `DA_GroundFire`.

**C'est une animation de présentation uniquement.** Les réactions C6,
dégâts périodiques, coûts d'action, durée et sauvegarde des surfaces sont
inchangés et déterminés au lancement de l'action.

## Réglages dans DA_GroundFire → Fire → Ignition

| Champ | Valeur conseillée | Effet |
|---|---:|---|
| `PropagationDelayPerCell` | **0.15 seconde** | Départ de la case voisine après une case orthogonalement reliée |
| `IgnitionGrowthDuration` | **0.40 seconde** | Temps de déploiement du feu au sein d'une case |
| `InitialCoverage` | **0.10** | Largeur de départ : environ 20 cm sur une case de 200 cm |

La couverture finale continue d'utiliser **`GroundFireCoverage`** :
`0.9` signifie **180 × 180 cm d'émission par case** lorsque le
Niagara possède un `User.SpawnBox` de type Vector3f.

`FlameHeightCm=30`, `NiagaraReferenceHeightCm=150`, les taux
`FlameDensity=0.5`, `SmokeDensity=0.15`, `FireTint` et les
surcharges typées sont conservés.

## Algorithme

1. La réaction C6 consomme le catalyseur et convertit les surfaces
   avant le vol ; les visuels restent en attente de l'impact.
2. À l'impact, on ne sélectionne que les cellules de réaction
   **OilIgnition**, réellement `Fire` dans l'état runtime,
   **sans** composant de feu déjà présent.
3. `FGridFireIgnitionWave::BuildDelays` calcule un parcours BFS sur
   le graphe des quatre voisins orthogonaux. Chaque îlot isolé reçoit
   un départ à distance de Manhattan de l'impact, sans créer de feu
   dans les cases intermédiaires.
4. Les composants Niagara sont créés avec `AutoActivate=false` si la
   case est en attente. À l'heure prévue, ils démarrent.
5. Tant que le feu grandit, le runtime anime uniquement `SpawnBox`
   (dimensions XY) et `FlameRate` (densité). Sans paramètre
   `SpawnBox` compatible, l'emprise est interpolée via l'échelle XY
   du composant : il s'agit alors d'une proportion, non d'une largeur
   garantie en centimètres.
6. À la fin de la croissance, la liaison canonique complète réapplique
   les réglages du Data Asset et **toutes les surcharges typées**,
   de sorte que la flamme persistante retrouve exactement sa
   configuration permanente.

La luminosité C++ d'une nouvelle case suit la progression d'embrasement.
`NS_FFX_Ground_Fire` reste inchangé et n'est jamais copié dans Git.

## Cycle de vie et performances

- `ActiveFireNiagaraByCell` et `ActiveFireLightsByCell` associent les
  composants à leur cellule. Un rafraîchissement ne détruit plus les
  flammes des cellules inchangées : seules les cases retirées sont détruites.
- Une minuterie de 40 ms existe **uniquement** tant que des cellules
  sont en animation. Elle se termine quand toutes sont stabilisées.
- Le contrôle de modification de `DA_GroundFire` pendant le PIE
  reste **éditeur uniquement** (FIREFX10). Un changement du profil
  reconstruit exceptionnellement les composants.
- Un chargement de donjon, un feu préexistant, un feu issu d'autres
  actions ou une relecture de sauvegarde ne déclenche **pas** de vague
  d'embrasement : uniquement une réaction huile+catalyseur à l'impact.
- Nettoyage automatique lors de la suppression d'une surface Fire
  ou de la destruction du runtime ; aucune modification du système
  audio de feu (hors périmètre).

## Contrôles de non-régression

Le test `Grimrock.RPG.FIREFX11.ProgressiveOilIgnition`
vérifie la propagation orthogonale sur une croix de cinq cases,
le contournement d'un angle, les îlots déconnectés, le rapport
`InitialCoverage/GroundFireCoverage`, la fin de croissance et la
sanitisation des nouveaux paramètres.

Recette visuelle PIE sur `L_Dungeon` :
1. Jeter l'huile en croix puis le catalyseur au centre.
2. Vérifier : centre au moment du bris ; quatre voisins 150 ms après ;
   expansion de chaque zone en ~400 ms jusqu'à 180 × 180 cm.
3. Relancer une réaction sur des cases déjà en feu : ne pas redémarrer
   ni recréer les flammes existantes.
4. Vérifier que le `wouf` et le `NS_FFX_FireBurst` n'ont pas été
   déplacés, que l'huile n'est pas visible avant l'impact, et que
   le gameplay C6 reste inchangé.

Les tests automatiques Windows ne chargent pas le pack FAB commercial,
donc **la recette visuelle finale dans le PIE local est nécessaire**.
