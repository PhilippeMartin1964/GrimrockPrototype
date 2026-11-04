# ARMEMENT01.17.1.4 — Refonte du rendu de l’huile au sol

**4 novembre 2026 — Refonte de la présentation, à valider sous UE 5.5.4.**

## Pourquoi l’ancienne réalisation était incorrecte

La capture utilisateur montrait de larges rectangles blancs, flottant au-dessus du sol, avec les joints des cellules nettement apparents. L’implémentation ajoutait un plan `Plane` au moyen de `CombatOilSurfaceISM`, puis avait tenté successivement un décalage de 2 cm et un décalage calculé depuis l’épaisseur du mesh. Or **le plancher logique est construit à Z = 0** par `AGridLevelRuntimeActor::AddFloor()`. Une surface plane ne peut jamais épouser les reliefs et joints du sol.

Les anciens tests `-NullRHI` vérifiaient seulement l’existence d’instances, sans vérifier l’image réellement produite. Le correctif d’option « Used with Instanced Static Meshes » ne répondait pas à ce défaut de conception.

## Nouvelle architecture : projection sur les dalles réelles

- **Aucun mesh ajouté à la flaque.** Suppression de `CombatOilSurfaceISM`, `SurfaceOverlayMesh`, `OilSurfaceHeightOffset` et du calcul `FloorTopZ`.
- Les sols restent des instances `FloorISM`, positionnées à **Z = 0**, avec leur matériau de pierre intact ; `SetReceivesDecals(true)` autorise les projections.
- `UDecalComponent` projette le matériau `MI_Surface_Oil` directement sur les surfaces 3D du sol : la coloration et la rugosité suivent les reliefs, sans plan flottant ni décalage vertical.
- Le volume de projection traverse Z = 0 ; **sa profondeur n’est pas une hauteur de flaque**. La position locale des décalques reste exactement sur le plan de référence Z = 0.
- Les cases d’huile voisines sont regroupées en rectangles ne contenant que des cases d’huile. Une même projection peut couvrir plusieurs cases, évitant les contours artificiels à chaque joint.
- La silhouette est irrégulière et douce : un `SphereMask` aux contours perturbés par un bruit procédural. Teinte brun-noir et **rugosité très faible** pour foncer localement les pierres et conserver des reflets.
- Le matériau maître `M_Surface_Oil` passe au domaine **Deferred Decal**, avec mélange translucide et sorties BaseColor/Roughness/Opacity. `MI_Surface_Oil` conserve son parent et son identité de DataAsset.
- Les `UDecalComponent` sont créés et supprimés uniquement aux changements d’état ; ils n’ont pas de Tick. Ils ne sont pas sauvegardés.

L’autorité de combat **RPG03.6**, les consommables, les durées, les réactions et les sauvegardes ne sont pas modifiés. La présentation est reconstruite lors de l’application, de la réaction, de l’expiration, du chargement et du changement de niveau.

## Conversion native du matériau — sans opération manuelle

Le commandlet Unreal `GridOilSurfaceMaterialAuthoring -ConvertToDecal` reconstruit **uniquement le graphe de `M_Surface_Oil.uasset`**, le convertit en Deferred Decal et le sauvegarde. `MI_Surface_Oil.uasset` et tous les autres assets sont conservés.

L’agent Windows déclenche cette conversion seulement si le test `Grimrock.RPG.ARMEMENT01.17.1.4.ProductionMaterial` est **le seul test échoué**, et si le worktree dédié est propre. Il refuse tout autre fichier modifié, vérifie le dépôt et le commit, crée un commit Git LFS limité à `M_Surface_Oil.uasset` et pousse sans forcer `master`. Le commit d’asset sera ensuite validé par la suite complète.

L’ancien script de réparation `RepairOilInstancedMaterialUsage.ps1` et le contrôle `InstancedMaterialUsage` sont supprimés, puisqu’aucun mesh instancié n’est désormais utilisé pour l’huile.

## Tests et limites

- `OilCellProjection` : projection, regroupement des cases voisines, matériau et ancrage exact à Z = 0.
- `NoFloatingOverlay` : absence de plan et profondeur de projection autour du niveau de référence.
- `ReactionsAndConversions` : feu, conversion et remplacement par eau.
- `Expiration` : la flaque reste pendant la durée prévue puis disparaît.
- `SaveLoadRebuild` : sérialisation de l’état et reconstruction.
- `ProductionMaterial` : matériau natif Deferred Decal, liaisons du graphe et instance de production.
- Intégration combat/hotbar : utilisation réelle de la flasque jusqu’à l’existence d’une projection.

**L’acceptation visuelle exige une scène rendue avec GPU/RHI**. Les tests `-NullRHI` ne certifient ni les couleurs ni les reflets ni la qualité des contours. Après validation technique, il faudra obtenir une capture du vrai donjon sous les torches et examiner l’absence de surfaces blanches, de flottement et de joints de cellules.


## ARMEMENT01.17.1.5 — Splash et événement de flasque (4 novembre 2026)

Une flasque d'huile effectivement consommée déclenche désormais une **projection animée temporaire**, indépendante du décalque durable déjà validé. Un décalque éphémère centré sur la cellule visée grandit rapidement et s'estompe sur **0,42 seconde**, en utilisant le matériau d'huile existant avec un paramètre dynamique `OilOpacity`. Le volume de projection reste à **Z = 0**, suit les reliefs du sol, n'ajoute aucun mesh, n'a ni collision ni effet mécanique, et est détruit à la fin. La mise à jour utilise un timer temporaire de 30 Hz, **pas de Tick permanent**. Aucune création ou affectation d'asset Niagara n'est nécessaire pour cette première animation de type *splash*.

Le flux d'événements publie **une seule entrée** dans la catégorie **Combat**, code stable `Combat.Surface.OilSpread` : « Une flasque d’huile se brise : l’huile se répand sur N cases. » La publication a lieu **après validation et exécution de la vraie action `Action_Alchemist_OilSlick`**, jamais pendant le survol ou le choix de cellule, et n'est pas répétée pour chaque case. L'animation n'est pas réémise lors d'un chargement ou d'une reconstruction des flaques.

### Durée : anomalie hors combat, décision mécanique séparée

Le profil actuel reste **4 rounds**, coût **2 PA + 1 flasque**, portée **4 cases** et rayon de **1 case** autour de la cellule choisie. Le décompte se produit uniquement pendant la résolution des rounds de combat. **Hors combat, aucune expiration temporelle n'est actuellement prévue** pour ces surfaces. Ce décalage n'est pas corrigé silencieusement dans le jalon d'animation : la définition d'un équivalent de quatre rounds en exploration (actions de grille ou secondes de jeu) et la sauvegarde de ce nouveau temps doivent être tranchées comme une règle de gameplay distincte.

### Vérifications

Le test d'intégration `Grimrock.RPG.ARMEMENT01.17.1.4.QuickItemToWorldProjection` contrôle maintenant, en plus du ciblage et de la consommation : l'unicité du déclenchement du splash, l'entrée unique dans le flux Combat, son contenu, la destruction du décalque transitoire et la conservation de la flaque durable. La compilation UE 5.5.4 et la suite Automation doivent confirmer ces nouvelles assertions avant validation technique définitive.


### Validation du splash — 4 novembre 2026

Le commit `b40d5a3c` a obtenu **`UE5.5.4/LocalAuto = success`** (compilation et suite complète `Grimrock`). L'animation transitoire de 0,42 seconde et l'entrée unique du flux d'événements sont donc validées sur leurs contrats C++ et Automation. Une vérification visuelle avec rendu GPU reste séparée.

Le contrat final confirme qu'**aucune animation et aucun événement ne sont émis pendant le simple ciblage** : le splash ne démarre qu'après l'utilisation réellement réussie de la flasque. Le flux de la catégorie `Combat` emploie un message français distinct au singulier et au pluriel. Les quatre rounds de combat et la portée de quatre cases sont inchangés ; **l'écoulement du temps hors combat reste à définir**, sans inventer un nombre de secondes par round.


## ARMEMENT01.17.1.6 — Durée d'huile hors combat

**Règle provisoire et configurable :** 1 round = 6 secondes de jeu en exploration. Une flasque de quatre rounds reste donc 24 secondes si aucun combat ne commence. `OilExplorationSecondsPerRound` est une propriété de l'acteur runtime ; RPG02 ne fixe pas ce rapport, qui doit être validé avant publication.

- Une seule autorité : `FGridLevelRuntimeState::Surfaces` ; le champ SaveGame `RemainingOilSeconds` conserve les fractions de durée. Les anciennes sauvegardes à zéro reprennent `RemainingRounds × seconds/round`.
- En combat, seule la fin du round décrémente le temps de gameplay de l'huile ; en exploration, un timer de 0,25 seconde actif seulement si de l'huile existe débite le temps de jeu. Aucune double décrémentation.
- Les surfaces autres que l'huile et l'animation splash ARMEMENT01.17.1.5 restent inchangées. L'expiration supprime aussi le décalque de sol.
- L'horloge s'appuie sur `GetTimeSeconds()` (pauses exclues). Les niveaux inactifs ne consomment pas de durée.
- Éviter de changer le ratio en cours de sauvegarde : il fait partie des règles d'une partie.

Tests supplémentaires : `Grimrock.RPG.ARMEMENT01.17.1.6.ExplorationTime`,
`Grimrock.RPG.ARMEMENT01.17.1.6.SaveLoad`,
`Grimrock.RPG.ARMEMENT01.17.1.6.LegacySave`.

**À valider :** build Windows UE5.5.4, toute la suite `Grimrock`, contrôle PIE/GPU du splash et de l'expiration en sortie de combat. Aucun test visuel GPU n'a été réalisé ici.


## ARMEMENT01.17.1.5C — Splash tridimensionnel perceptible

**Pourquoi :** le décalque précédent (0,42 s) utilisait exactement la même
apparence que la flaque persistante. L'effet était imperceptible en PIE malgré
la validation technique du déclenchement et du flux d'événements.

Nouvelle présentation (uniquement C++, sans nouveau `.uasset` nécessaire) :

- **Impact central** : décalque de `MI_Surface_Oil`, teinte légèrement
  réchauffée, expansion accélérée et durée par défaut de **0,78 s**.
- **Huit éclaboussures de sol** : décalques secondaires mobiles, projetés sur
  la pierre, s'écartant de l'impact et disparaissant progressivement.
- **Huit gouttes réellement 3D** : sphères Engine de faible taille avec
  le matériau **déjà présent** `M_Flask_Oil.uasset`, décrivant une trajectoire
  en arc au-dessus du sol puis rétrécissant. Pas de Niagara ni d'asset généré
  artificiellement ; pas de collision, pas d'ombre, pas d'effet mécanique.
- **Durée configurable** dans `OilSplashDurationSeconds` sur le RuntimeActor.
  Actualisation temporaire à 30 Hz, aucune simulation permanente.
- **Nettoyage** lors de la fin, du nouvel impact, d'un rebuild ou d'un EndPlay.
  Conservation des surfaces RPG03.6, du décompte d'exploration et des
  messages `Combat.Surface.OilSpread` existants.
- Si la sphère Engine ou le matériau de flasque sont absents, un warning
  technique est journalisé ; les décalques de sol restent disponibles.

Tests `Grimrock.RPG.ARMEMENT01.17.1.5C.VisibleBurst` et
`Grimrock.RPG.ARMEMENT01.17.1.5C.Lifecycle` : composants 3D et projetés
présents, mouvement mesuré, une animation par utilisation, relancement
sans accumulation, nettoyage et flaque durable intacte.

**Critère PIE/GPU :** en vue subjective, voir des gouttelettes sombres
jaillir au-dessus de la pierre, avec des taches radiales qui s'étalent.
La validation Windows via `UE5.5.4/LocalAuto` confirme la compilation et
les assertions, pas la perception visuelle réelle : la recette PIE reste
requise pour l'acceptation artistique.
