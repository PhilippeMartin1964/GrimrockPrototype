# ARMEMENT01.17.1.4 — Huile réellement visible au sol

**Date : 4 novembre 2026. Statut : code, tests et authoring automatique préparés pour GitHub ; validation UE 5.5.4 et rendu visuel réel à confirmer.**

## Objectif

Lorsqu'un Alchimiste/Transmutateur utilise sa flasque d'huile en combat, les cases de la zone d'effet doivent afficher une **nappe d'huile brun-noir, brillante, légèrement transparente et aux contours estompés**. L'animation doit suivre les données du combat sans créer de second système d'état.

Le rendu doit disparaître exactement lorsqu'une surface expire, est remplacée, se transforme ou réagit au feu. Une sauvegarde et un changement de niveau doivent restaurer correctement les nappes encore actives, sans instance fantôme.

## Source de vérité

La seule autorité reste `FGridLevelRuntimeState::Surfaces`, déjà sauvegardée par RPG03.6. Chaque surface possède notamment un `SurfaceType`, une durée en rounds et une provenance. La projection graphique n'est pas sauvegardée.

- `AGridLevelRuntimeActor::RefreshCombatOilSurfaceVisuals()` lit les surfaces du niveau courant et reconstruit les instances visuelles.
- `CombatOilSurfaceISM` est un seul `UInstancedStaticMeshComponent` sans collision, chevauchement, ombres ni Tick.
- `SurfaceOverlayMesh` reprend le plan Engine `/Engine/BasicShapes/Plane`, à une échelle adaptée à `LevelAsset->CellSize`.
- `OilSurfaceMaterial` est une référence souple vers l'instance native `MI_Surface_Oil`. **Aucune affectation manuelle dans le Blueprint n'est nécessaire.**
- `OilSurfaceHeightOffset` vaut **2 cm** au-dessus du sol, ajustable uniquement en cas de besoin visuel.

Le moteur vérifie que chaque case est praticable et possède un plancher apparent. Il ne peint pas de nappe au-dessus d'un puits ouvert. Les cases sont triées de manière déterministe par Y puis X.

## Réactivité

Le rendu est actualisé lors de :

1. l'application ou du remplacement d'une surface ;
2. la réaction canonique, notamment **Oil + Fire → Fire** ;
3. la conversion depuis ou vers le type `Oil` ;
4. l'expiration d'une ou plusieurs surfaces ;
5. la reconstruction du niveau ;
6. la restauration de l'état runtime après chargement.

Une surface `Water`, `Poison`, `Fire` ou une autre surface ne reçoit **jamais** le matériau de l'huile. Les effets sur les PA, la durée et les réactions ne sont pas recalculés par la présentation.

## Matériau Unreal natif

Destination versionnée :

```text
Content/GrimrockPrototype/Art/Materials/Surfaces/
  M_Surface_Oil.uasset
  MI_Surface_Oil.uasset
```

`M_Surface_Oil` utilise un matériau `Surface / Translucent / Default Lit / Surface ForwardShading` et un graphe simple créé par `UMaterialEditingLibrary` :

- `OilTint` : teinte brun-noir, pour assombrir la pierre tout en la laissant identifiable ;
- `OilRoughness` : 0,09, pour obtenir des reflets peu diffus ;
- `OilSpecular` : 0,8, pour une lecture sous l'éclairage des torches ;
- `OilOpacity` : 0,63, afin de conserver les détails du plancher ;
- `SphereMask` sur les UV du plan : nappe aux lisières estompées, sans rectangle opaque.

L'instance `MI_Surface_Oil` référence ce matériau maître. Les paramètres restent modifiables dans l'éditeur pour un affinage artistique ultérieur. Aucun Niagara ou effet `NS_LiquideSparkle` n'est ajouté à l'huile : ce dernier est réservé aux potions qui le nécessitent.

### Création sans manipulation manuelle

Le commandlet Editor `GridOilSurfaceMaterialAuthoring` matérialise les deux nouveaux assets à l'aide de l'API Unreal. Le script `Scripts/AuthorOilSurfaceMaterial.ps1` est déclenché automatiquement si **seul** le test `Grimrock.RPG.ARMEMENT01.17.1.4.ProductionMaterial` échoue parce que les assets sont absents.

Il vérifie l'intégrité du worktree, le SHA de `master`, l'origine GitHub, Git LFS et la liste stricte des deux fichiers autorisés. Si tout est conforme, il crée un commit de matériaux puis effectue un push non forcé. La validation automatique recommence sur le nouveau commit.

Une erreur de compilation ou une autre défaillance d'Automation **ne déclenche aucune publication d'asset**. Aucun autre matériau, mesh, Blueprint ni fichier utilisateur n'est modifié.

## Tests automatisés

Filtre : `Grimrock.RPG.ARMEMENT01.17.1.4`

| Test | Contrat |
|---|---|
| `OilCellProjection` | Placement sur les bonnes cases, transformation locale, exclusion des surfaces non huileuses et des coordonnées invalides. |
| `ReactionsAndConversions` | Réaction au feu, conversion en huile puis remplacement par de l'eau. |
| `Expiration` | Durée respectée et suppression de l'instance visuelle au round final. |
| `SaveLoadRebuild` | Sérialisation réelle SaveGame, reconstruction des nappes et préservation des surfaces tierces. |
| `ProductionMaterial` | Présence de `M_Surface_Oil` et `MI_Surface_Oil`, bon type de matériau, transparence et parent exact. |

La suite complète `Grimrock` est exécutée par `VALIDATION-AUTO01`. Le commit de source peut d'abord échouer sur **ProductionMaterial uniquement** ; dans ce cas la création automatique des deux assets lance une seconde validation. L'étape ne sera clôturée que lorsque le commit final sera **vert**.

## Acceptation visuelle — donjon réel

Le joueur commence en Alchimiste/Transmutateur au niveau 1, avec sa flasque existante :

- lancer la flasque sur une zone valide ; vérifier la consommation d'une flasque et de 2 PA ;
- constater des nappes bien visibles à l'intérieur de la zone d'effet, sans peinture sur des cases adjacentes non touchées ;
- examiner les reflets brun-noir sous les torches, la transparence de la pierre et les lisières des nappes ;
- vérifier l'absence de carrés flottants au-dessus des trous et des collisions avec le groupe ;
- avancer quatre rounds : la nappe disparaît exactement lorsque la surface RPG03.6 expire ;
- tester la réaction au feu, puis sauvegarder, quitter et reprendre : les surfaces actives reviennent ;
- tester le passage vers un autre niveau et le retour ;
- inspecter les sols irréguliers : si le décalage de 2 cm provoque des intersections, ajuster ce seul paramètre ou envisager ultérieurement un décalque.

**Limite actuelle :** les tests Automation de logique ne remplacent pas encore une capture PIE automatique avec RHI. Le rendu artistique est à accepter séparément. Aucun talent de niveau 2 n'est attribué et aucun texte anglais n'est ajouté.

## Jalons suivants

Après acceptation d'ARMEMENT01.17.1.4 : **ARMEMENT01.17.2** (catalyseur Mage/Tisseur niveau 1), **ARMEMENT01.17.3** (composants de transmutation, selon l'état réel du craft), puis **ARMEMENT01.18** (acceptation des 18 kits).


### Ajustements de compatibilité UE 5.5.4

Le premier cycle automatisé a confirmé trois détails de compilation propres à l'environnement du projet : les valeurs `MD_Surface` sont déclarées dans `MaterialDomain.h`, la définition complète de `FGridResolvedCombatModifiers` exige `GridCombatModifierResolver.h`, et `UMaterialEditingLibrary::RecompileMaterial` retourne `void` dans cette version de l'Engine. Les inclusions et appels ont été corrigés sans modifier la logique RPG03.6 ni les assets existants.


### Diagnostic automatique de la création des matériaux

Le premier passage après correction des erreurs C++ a réussi les quatre tests de logique, mais le test `ProductionMaterial` reste en échec tant que les deux assets natifs ne sont pas versionnés. Le journal de l'agent sera dorénavant assorti d'une **étape de matérialisation** strictement contrôlée : vérification initiale, rapport accepté, commandlet lancé, commandlet terminé, assets vérifiés, commit créé, puis publication réussie. Seul le nom de cette étape est ajouté à l'issue GitHub ; les messages Unreal et les chemins privés restent sur Windows. Cela permet de diagnostiquer automatiquement une erreur de création sans vous demander de copier de fichiers ou de journaux.


### Envoi Git LFS après création des matériaux

Le cycle du commit `0dbd7722` a prouvé que le commandlet avait généré les deux assets natifs, vérifié les fichiers puis créé un commit local. Le diagnostic public a indiqué **`COMMIT_CREATED`** : l'envoi de ce commit vers `master` n'a pas abouti. Les modifications du donjon et les tests de logique n'étaient pas en cause.

Le script effectue maintenant jusqu'à **trois tentatives de push du même commit**, toujours sans `--force`. En cas de nouvel échec, l'agent publie uniquement une catégorie technique autorisée : problème d'authentification, de quota LFS, de transfert LFS, de réseau, de divergence de référence ou catégorie inconnue. Il ne publie aucun extrait brut des journaux Git ni information personnelle. Si la publication reste impossible, les données et les objets Git du worktree ne sont pas supprimés automatiquement.
