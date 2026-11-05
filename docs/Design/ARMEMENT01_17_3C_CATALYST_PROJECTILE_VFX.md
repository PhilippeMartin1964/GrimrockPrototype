# ARMEMENT01.17.3C — Projectile et VFX multi-cellules

Date : 5 novembre 2026. Parent : c0d5aaa1529c4a0f82dae47ef84598cb00f18bc2.
Un seul commit, C++ + tests + docs ; aucun binaire .uasset ou .umap modifié.

## Précondition : réparation de 17.3B

Le diagnostic Windows du parent (UE5.5.4/LocalAuto=failure) mentionne
exactement trois erreurs de compilation : C2662 dans la minuterie
d'exploration (pointeur const), C2248 sur l'accès privé d'un test et
C2664 concernant la trace d'armure du poison. Le présent jalon les
corrige avant validation.

## Déroulement visuel

1. Un seul QuickItem est consommé et résolu par le système C6.
   `BeginCatalystSurfaceVisualBatch()` met en attente les seuls
   rafraîchissements des projections Fire/Water/Oil.
2. `PlaySurfaceCatalystArea()` crée un unique projectile à partir du
   mesh d'artiste `SM_Catalyst_01`, en conservant ses matériaux propres.
   L'arc est paramétrable, durée initiale 0,42 s ; animation par timer,
   pas de Tick permanent.
3. À l'impact, le projectile est détruit et un seul rafraîchissement
   des surfaces persistantes reconstruit l'état C6 final.
4. Chaque cellule ciblée (maximum neuf) joue sa propre variante :
   OilIgnition, PoisonBurst, IceMelt ou Fizzle pour les cases sèches,
   Water, Fire et non réactives. Une onde se décale de 0,085 s par
   case à partir du centre. Les pulses, étincelles et lumières coexistent.
5. L'ensemble des composants transitoires est détruit après l'effet.
   Les surfaces persistantes, leur durée et leurs dégâts ne dépendent
   jamais du VFX et ne sont pas rejoués par la minuterie visuelle.
6. Si le projectile/monde n'est pas disponible, impact immédiat et
   libération du verrou de rafraîchissement. Si un nouveau lancer arrive,
   les anciens composants temporaires sont nettoyés. Une annulation
   libère la projection différée.

## Assets

- Mesh : /Game/GrimrockPrototype/Meshes/Items/SM_Catalyst_01
- Matériau de la bouteille : assigné **par le mesh** (MI_Catalyst).
- Rune : M_SurfaceCatalyst_RunePulse
- Étincelles : M_SurfaceCatalyst_Ember
- Flammes persistantes : Niagara NS_Flame_8_Torch (système existant).
- Propriétés éditables de l'acteur : CatalystProjectileMesh,
  CatalystRunePulseMaterial, CatalystEmberMaterial, durée de l'arc,
  durée du pulse et délai de l'onde. Références soft définies
  dans le CDO natif pour l'inclusion en packaging.

## Limites explicites

- **Le gameplay C6 demeure validé lors de la confirmation du lancer** ;
  seul l'affichage des surfaces est différé jusqu'à l'impact. Ce choix
  conserve les coûts/PA et le tour atomiques, mais une capture du
  runtime peut observer la surface déjà transformée pendant le vol.
- Les pulses et petites sphères colorées sont la présentation native
  reproductible, pas encore une déflagration volumétrique artistique
  Niagara dédiée (nouvel asset à produire pour atteindre la planche).
- Aucun son, collision physique de bouteille, bris fracturé ni mesh
  de verre n'est produit par ce jalon.
- Le rendu final en RHI/PIE et en build packagé reste une recette
  distincte du test headless -NullRHI.

## Tests

- Grimrock.RPG.ARMEMENT01.17.3C.AreaFlightImpactAndCleanup :
  arc, impact unique, quatre pulses simultanés, état et délai,
  absence de fuite de composants.
- Grimrock.RPG.ARMEMENT01.17.3C.CancelPendingVisualBatch :
  annulation sécurisée des projections différées.
- Maintien des anciens tests
  Grimrock.RPG.ARMEMENT01.17.2C.CanonicalVfxClassification,
  TransientReactionLifecycle, IceMeltWaterProjection.
- Maintien de 17.3A / 17.3B / suite complète Grimrock.

## Contrat UI/localisation

Pas de WBP, texte visible supplémentaire, clé UI.csv ou ST_UI modifiée.
Pipeline UI.csv → ST_UI → WBP → Game/fr/en : sans changement.
Le texte d'événement dynamique existant dans 17.3B ne change pas.

## Livraison

Statut UE5.5.4/LocalAuto exigé sur le SHA exact.
Ne pas confondre build+Automation, recette GPU/PIE et packaging.
