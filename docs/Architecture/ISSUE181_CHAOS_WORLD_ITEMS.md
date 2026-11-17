# Issue #181 — objets placés, Chaos et relocalisation locale explicite

Audit Windows en lecture seule sur `master`, parent
`169b2c397673d10a32ef65d66aeabb7ebdd3eb25`, avant correction.
[Issue et décision du propriétaire](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/181).

## Preuve de la cause

Une session **Simulate In Editor, monde PIE, rendu D3D12 et vrai solveur
Chaos** a chargé les placements de `DA_GridLevel_00`, les définitions de
la palette et la géométrie de `L_Dungeon`. La reproduction utilisait une
carte temporaire non sauvegardée, un GameInstance de base et un groupe
sans slot de sauvegarde. Aucun asset ni fichier `.sav` n'a été écrit.

Les huit GUID N1 sont présents dans `SpawnedItemEntries`, acteurs valides,
`Edge=None`, registre configuré et résolution canonique valide. Leurs
cellules enregistrées sont toutes `(28,24)`. Exemple après six secondes
de physique naturelle, sans téléportation des armes :

| Arme | GUID | Cellule du pivot | Cellule du centre du mesh |
|---|---|---|---|
| Dagger | `3A88B1C74EE191D96EB7AE8B4B86F601` | 28,24 | 28,24 |
| LongSword | `04B0792343EB8804BFA3F8BC417FE42D` | 28,24 | 27,24 |
| WarPick | `D6038B634EFEF5A71424D881B0EC1D3D` | 28,24 | 28,24 |
| GreatAxe | `6D9744564FDCB7EC69B465961254F723` | 28,24 | 27,24 |
| Stiletto | `9589F33845144C130516A2BF5FE8E730` | 28,24 | 28,24 |
| MainGauche | `884DC0134637541DE3579493E12D2FDA` | 28,24 | 28,24 |
| Mace | `57A644734B7570F21808CF9CC813FF92` | 28,24 | 28,24 |
| ShortSpear | `560393614DF54B568FBD3592C659186E` | 28,24 | 28,24 |

Les poses exactes varient entre exécutions Chaos. La distinction reste
mesurable : le pivot de ces meshes est proche du manche, pas du centre
de leur forme couchée. Une hache avait son centre à X=5542, dans la
fosse, avec son pivot dans la cellule voisine. `Transferred=0` a été
reproduit à l'ouverture avec les huit pivots toujours dans la cellule 28.

Au HEAD initial, `GridLevelRuntimeActorWorldItems.cpp` :

- `DropWorldItemsThroughOpenPitAtCell`, lignes 654–672 : le filtre de
  surface/validité passe, mais **cellule enregistrée et cellule du pivot
  valent toutes deux (28,24)**. Le `continue` écarte les armes avant la
  validation d'identité de la ligne 692 et le routage de la ligne 720.
- `CanPartyPickupItemEntry`, lignes 94–123 : la portée utilise déjà le
  mesh, mais **sa position est celle du pivot**. Dans une autre pose SIE,
  un rayon complexe atteint la lance à 174 cm horizontalement, 227 cm
  en 3D, tandis que le pivot est à 305 cm. Le raycast est valide et dans
  les 300 cm du contrôleur ; `CanInteract` refuse ensuite la portée de
  main de 200 cm. Ce n'est pas une collision restée à la racine.
- `RefreshWorldItemPhysicsCells` et `GetWorldItemWeightAtCell` utilisent
  aussi le pivot, d'où une même incohérence pour la cellule et le poids.

Le contrôleur conserve son premier obstacle Visibility et son trace
complexe (`GrimrockPlayerController.cpp`, lignes 1706–1757). Le contrat
`CanInteract` exige toujours le véritable `MeshComponent` touché.

## Blocage distinct : destination volontaire dans le même niveau

La fosse `5E12044B43CCA332F0CE18B83DD87038` et le piège
`4EF9461E42AD96FCEA2C04B48DC0CBB7` sont correctement appariés en `(27,24)`.
La destination `Into_The_Dark (27,23)` est **volontaire**, selon l'issue.

Au parent, `GridLevelRuntimeActorTrapDamage.cpp`, lignes 315–322, exige
un niveau différent et inférieur. `TryRouteWorldItemThroughOpenPit`,
ligne 399, refuse également le niveau courant. En revanche, le lien
`Activated -> Open` appelle `SetPitOpen` sans cette validation de chute.
Son succès prouve l'ouverture, pas le transfert.

Un journal utilisateur plus récent contient effectivement un candidat
Mace suivi de `Reason=InvalidTargetLevel`. Ce deuxième zéro est un refus
de routage, distinct du zéro par absence de sélection reproduit ci-dessus.

## Contrat du correctif

- La **cellule de jeu** d'un objet physique utilise le centre des bounds
  de son mesh. Fosse, réconciliation de cellule et poids consultent ce
  même point. Le transform physique sauvegardé reste le vrai transform
  du mesh. Les arêtes authorées restent distinctes.
- Le **ramassage** mesure la distance horizontale jusqu'au point le
  plus proche de la collision du mesh. Un monde sans corps de requête
  utilise ses bounds. Le raycast souris reste responsable de l'occlusion.
- Une simple intersection du bout d'une arme avec une cellule voisine
  ne distribue pas son poids entre plusieurs plaques. Ce patch ne crée
  ni comptabilité des contacts Chaos, ni balayage par Tick des armes.
- `bEnableSameLevelPitRelocation`, désactivé par défaut sur le runtime,
  autorise explicitement le test local. **Pour la recette locale, activer
  “Enable Same Level Pit Relocation” sur l'instance runtime en PIE.**
  Aucun `.umap` n'est nécessaire pour cette activation de session.
- Ce mode exige une destination explicite, praticable, différente de la
  source et sans fosse ouverte. Il déplace l'acteur existant avec le même
  GUID, mesh, niveau et état ; aucun `PendingInboundItems`, destruction,
  reconstruction du niveau ou changement de schéma SaveGame.
- Un nouveau dépôt sur la fosse ouverte est matérialisé directement à
  destination. La garde locale empêche une réentrée pendant les notifications
  de plaques. Le groupe réutilise la relocalisation locale gardée existante,
  sans fausse animation de chute inter-niveaux ni voyage/reconstruction.
- Les fosses ordinaires conservent leur destination inférieure et leur
  file inter-niveaux. Une cible égale à la source n'est pas implicitement
  transformée en téléporteur.

## Régressions et limites

`Grimrock.Pit.Issue181` utilise des copies en mémoire des huit placements
réels via `BuildInitialRuntimeState -> RebuildRuntimeObjects ->
AddPlacedItemActor`, les vrais meshes et corps Chaos. La compilation
asynchrone des assets doit être terminée avant les requêtes physiques.
Une pose couchée fixe reproduit la séparation forme/pivot de façon
déterministe ; elle ne prétend pas reproduire chaque trajectoire Chaos.

- `RealPlacedMeshPickup` : trace complexe sur le mesh final, portée,
  ramassage effectif et GUID dans l'inventaire.
- `RealPlacedMeshSelection` : hache N1 et épée N20 placées dans la cellule
  voisine, sélection de leur géométrie couchée, file inter-niveaux et
  arrivée avec les mêmes GUID/niveaux.
- `ExplicitLocalDisarmAndMemoryRestore` : vrais piège, compétence et outils,
  échec du désamorçage, deux acteurs déplacés en `(27,23)`, garde de
  destination, nouveau dépôt, groupe, absence de file/rebuild/doublon,
  SaveGame en mémoire et restauration des poses N1/N20.

Le passage rouge contrôlé remet temporairement les deux lecteurs spatiaux
du parent, avec les corps physiques correctement préparés : les deux tests
échouent sur sélection/ramassage/GUID, sans échec de préparation ni de trace.
Le correctif fait passer ces tests. La suite Grimrock complète a ensuite
réussi localement : 1948 succès, 16 succès avec avertissements, zéro échec.
Les statuts Windows doivent aussi être suivis sur le SHA publié exact.

Après correction, une nouvelle session SIE D3D12, avec roulement naturel,
perception et interaction natives du piège, a produit un échec de
désamorçage (`Triggered=true`), `Transferred=2` pour LongSword/GreatAxe,
huit GUID physiques uniques, puis le ramassage effectif d'un objet arrivé,
présent une seule fois dans l'inventaire avec son N1. La génération runtime
est restée à 1. Les journaux bruts sont conservés localement.

Il s'agit de requêtes et interactions automatisées en SIE ; aucune recette
de clic souris manuelle ni appréciation artistique n'est revendiquée.
Les tests NullRHI restent une preuve distincte de cette session avec rendu.
Le parcours UI.csv -> ST_UI -> WBP -> Game/fr/en n'est pas modifié : aucun
nouveau texte joueur ni authoring UMG, message de relocation existant réutilisé.
