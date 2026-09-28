# MON21.6.1 — Map Architecture Contract

Date : **28 septembre 2026**  
Statut : **VALIDÉ — CONTRAT FIGÉ — AUCUNE IMPLÉMENTATION RUNTIME/UMG DANS CE TICKET**  
Baseline auditée : `7750ede9220eb94341ff02c618d9e7fa4c8050be`

---

## 1. Objet du ticket

MON21.6.1 fixe l’architecture de la carte du donjon avant toute implémentation.

Le besoin produit est :

- ouvrir la carte via la commande `M` déjà existante ;
- révéler progressivement le donjon autour du groupe ;
- conserver un brouillard de guerre permanent sur les zones inconnues ;
- représenter clairement cellules, murs et portes ;
- rendre un passage secret strictement identique à un mur tant qu’il n’est pas découvert ;
- afficher un étage composé d’une ou plusieurs grilles 32×32 adjacentes ;
- permettre de naviguer verticalement entre les étages du donjon ;
- produire un rendu de carte dessiné à la main sur parchemin ;
- conserver une architecture simple, data-driven, sans seconde autorité de niveau.

MON21.6.1 ne modifie aucun `.cpp`, `.h`, `.uasset`, `.umap` ni schéma SaveGame.

---

## 2. Fondations existantes réutilisées

Le système Map doit se brancher sur les autorités déjà présentes :

```text
UGridDungeonAsset
    -> DungeonName
    -> Levels[]
       -> LevelId
       -> DisplayName
       -> UGridLevelAsset*
       -> LogicalPosition (X/Y/Z)
       -> bEnabled

UGridLevelAsset
    -> Width / Height
    -> Cells[]
    -> WorldObjectInstances[]
    -> géométrie et placements

FGridDungeonRuntimeState
    -> LevelStates[LevelId]

FGridLevelRuntimeState
    -> état vivant/persistant d’un LevelId

AGrimrockPartyPawn / SaveGame
    -> CurrentDungeonLevelId
    -> PartyCellX / PartyCellY
    -> PartyFacing

WBP_GridMap
    -> surface Map existante

UGrimrockMenuWidget
    -> shell de navigation existant
```

Décision : **aucun `MapActor`, aucun second `MapAsset`, aucune copie persistante de la grille n’est autorisé**.

---

## 3. Terminologie canonique

### 3.1 Dalle

Un `UGridLevelAsset` est une **dalle logique 32×32**.

Le contrat MON21.6 repose sur la taille canonique du projet :

```text
Width  = 32
Height = 32
```

Une taille différente n’est pas prise en charge par la composition multi-dalles de MON21.6.

### 3.2 Étage

Un **étage de carte** est l’ensemble des `FGridDungeonLevelEntry` activées dont :

```text
LogicalPosition.Z == SelectedFloorZ
```

Plusieurs LevelAssets peuvent donc appartenir au même étage.

### 3.3 Position logique d’une dalle

`LogicalPosition` est interprété pour la carte comme suit :

```text
X = position horizontale Est/Ouest de la dalle
Y = position horizontale Nord/Sud de la dalle
Z = étage vertical
```

Il ne s’agit pas d’une transform Unreal.

Deux entrées activées ne doivent pas occuper le même triplet `(X,Y,Z)`. Une collision logique est une erreur de validation.

---

## 4. Espace cartographique global

Pour une cellule locale `(LocalX, LocalY)` d’une dalle placée en `(TileX, TileY, Z)` :

```text
MapX = TileX * 32 + LocalX
MapY = TileY * 32 + LocalY
```

Les coordonnées négatives sont valides.

Les limites techniques entre deux LevelAssets ne sont jamais dessinées. Le joueur voit un seul plan continu.

Orientation canonique :

```text
haut de la carte  = North = Y+
droite            = East  = X+
bas               = South = Y-
gauche            = West  = X-
```

Le rendu écran est responsable de l’inversion éventuelle de l’axe Y propre aux coordonnées UI.

---

## 5. Autorités et séparation des responsabilités

### 5.1 Géométrie statique

`UGridLevelAsset` reste l’unique autorité pour :

- cellule praticable ou vide ;
- murs N/E/S/O ;
- plafonds si une information de carte future en dépend ;
- placement des portes et passages secrets ;
- autres placements statiques utiles à la cartographie.

La carte ne reconstruit jamais la géométrie depuis les Actors 3D.

### 5.2 État vivant

`FGridDungeonRuntimeState` / `FGridLevelRuntimeState` restent l’autorité de l’état vivant du donjon.

L’exploration durable sera ajoutée dans cette famille d’état runtime, par `LevelId`, lors des tickets suivants.

### 5.3 Position du groupe

La position et l’orientation du groupe sont dérivées de l’autorité runtime existante. Elles ne sont jamais dupliquées dans l’état Map.

### 5.4 UI

`WBP_GridMap` reste la surface existante. Il sera branché sur une projection native read-only ; il ne possédera ni la géométrie, ni l’exploration autoritaire.

---

## 6. Contrat d’exploration / fog-of-war

### 6.1 État durable minimal

Chaque cellule possède sémantiquement deux états :

```text
Unknown
Explored
```

`Explored` est durable : une cellule découverte ne redevient jamais inconnue pendant la partie.

Le choix exact de la représentation C++ (`TArray<uint8>`, structure compacte, etc.) est reporté à MON21.6.2. Le contrat impose uniquement une connaissance persistante par cellule.

### 6.2 Rayon de révélation

Valeur de design par défaut :

```text
RevealRadiusCells = 1.25
```

Depuis le centre de la cellule du groupe, ce rayon sélectionne naturellement :

- la cellule courante ;
- les quatre cellules cardinales à distance 1 ;
- aucune diagonale, dont la distance est environ 1.414.

Le rayon n’est pas une autorisation de voir au travers des obstacles.

### 6.3 Feather visuel

Le brouillard peut visuellement s’estomper sur environ `0.25` cellule au-delà de la zone explorée.

Ce feather est **strictement graphique** :

- il ne marque aucune cellule supplémentaire comme explorée ;
- il ne transmet aucune géométrie cachée à la présentation ;
- il ne révèle aucun secret.

---

## 7. Révélation topologique

Le reveal est filtré par les frontières de cellules.

Contrat :

```text
cellule courante              -> explorée
voisine par passage libre     -> explorée
voisine derrière porte ouverte-> explorée
voisine derrière mur          -> non explorée
voisine derrière porte fermée -> non explorée
voisine derrière secret caché -> non explorée
```

La frontière observable depuis une cellule explorée peut elle-même être dessinée : le joueur peut donc découvrir un mur ou une porte sans découvrir la cellule située derrière.

MON21.6 ne déduit pas automatiquement une continuité de gameplay entre deux LevelAssets uniquement parce que leurs `LogicalPosition.X/Y` sont adjacentes. Les dalles sont co-projetées sur la carte ; leur exploration reste attachée à leur `LevelId` et aux transitions/observations réellement produites par le gameplay.

---

## 8. Passages et portes secrètes

### 8.1 Avant découverte

Une porte ou un passage secret non découvert est projeté comme un **mur plein normal**.

Interdictions :

- symbole spécial ;
- pointillé ;
- variation de couleur ;
- différence d’épaisseur ;
- métadonnée exposée au Blueprint permettant de reconnaître le secret.

### 8.2 Après découverte

La connaissance du secret devient durable et indépendante de son état ouvert/fermé.

Principe :

```text
secret caché + fermé  -> mur normal
secret découvert      -> passage/porte secrète connue
secret découvert puis refermé -> reste connu
```

Le futur état de découverte utilisera l’identité stable `ObjectId`/`FGuid` de l’instance concernée.

Le déclencheur exact de découverte est implémenté dans MON21.6.4 ; au minimum, l’exposition effective du passage doit pouvoir enregistrer cette connaissance.

---

## 9. Read model de carte

La Map est une projection filtrée, jamais un miroir complet du niveau.

Pipeline cible :

```text
UGridDungeonAsset / UGridLevelAsset
        +
FGridDungeonRuntimeState
        +
Party LevelId / Cell / Facing
        |
        v
Map Read Model
        |
        v
WBP_GridMap / rendu natif
```

Le read model ne fournit à l’UI que :

- les cellules autorisées par l’exploration ;
- les frontières connues ;
- les portes connues ;
- les secrets déjà découverts ;
- les symboles explicitement autorisés ;
- la position/facing du groupe lorsque l’étage affiché est l’étage courant.

Conséquence de sécurité UX : **la géométrie inconnue n’est pas envoyée au WBP puis cachée par un masque**.

---

## 10. Navigation multi-étages

À l’ouverture de la carte :

```text
SelectedFloorZ = LogicalPosition.Z du CurrentDungeonLevelId
```

Les boutons `Level Up` et `Level Down` appartiennent à la surface Map.

Ils parcourent les valeurs `Z` distinctes des entrées activées du `UGridDungeonAsset`, triées verticalement. Ils ne supposent pas que tous les entiers intermédiaires existent.

Exemple :

```text
Z = +2
Z =  0
Z = -3
```

`Level Down` depuis `+2` conduit à `0`, puis à `-3`.

Le bouton correspondant est désactivé lorsqu’aucun étage activé n’existe dans cette direction.

Lorsque l’utilisateur consulte un autre étage :

- l’exploration de cet étage est affichée ;
- le marqueur du groupe n’est pas affiché ;
- une commande `Recentrer` doit pouvoir revenir à l’étage et à la position courante du groupe.

---

## 11. Contrat de présentation

L’identité visuelle visée est une carte de dungeon crawler dessinée à la main.

### 11.1 Fond

- papier parchemin ;
- grain discret ;
- contraste suffisant pour rester lisible en jeu.

### 11.2 Géométrie

- cellule explorée : léger lavis/hachurage ;
- mur : trait sombre et lisible ;
- porte : interruption/symbole clairement différent d’un mur ;
- secret caché : exactement le même rendu qu’un mur.

### 11.3 Effet manuscrit

Les irrégularités de traits doivent être déterministes à partir des coordonnées/identités de la primitive.

Interdiction d’un jitter aléatoire par frame.

### 11.4 Brouillard

Le fog peut utiliser une bordure irrégulière et un feather, mais ne doit jamais être la seule barrière empêchant l’UI d’accéder à la géométrie cachée.

---

## 12. Surface UI et interaction

La touche `M` et le routage existant sont conservés.

`WBP_GridMap` est réutilisé ; aucun second menu Map n’est créé.

Interactions prévues pour les tickets UI :

```text
M              -> ouvrir/fermer la Map
Level Up       -> étage supérieur disponible
Level Down     -> étage inférieur disponible
Recentrer      -> étage + position du groupe
molette        -> zoom
clic-glisser   -> pan
```

Le choix technique exact du widget natif de dessin est différé à MON21.6.8, mais il devra éviter une architecture de type « un UWidget par cellule ».

---

## 13. Persistance

MON21.6.1 ne change pas `CurrentSaveVersion`.

Quand l’état durable d’exploration sera effectivement ajouté :

- il sera persistant par `LevelId` ;
- les secrets découverts seront persistants par identité stable ;
- aucun read model ou cache de rendu ne sera sauvegardé ;
- le prototype conservera sa politique exact-match ;
- l’incrément de `CurrentSaveVersion` sera réalisé uniquement dans le ticket qui introduit réellement ces données durables.

---

## 14. Performance et cycle de vie

Le contrat interdit :

- un Actor Map runtime permanent ;
- un Tick Map permanent ;
- un scan global des Actors du monde ;
- une duplication de `Cells[]` comme nouvelle autorité ;
- 1024 widgets UMG par dalle ;
- une reconstruction complète de la carte à chaque frame.

Les rafraîchissements doivent être événementiels : déplacement réussi du groupe, transition de niveau, découverte, mutation pertinente ou ouverture/navigation de la Map.

---

## 15. Hors périmètre de MON21.6.1

Ce ticket ne décide pas encore :

- le type C++ exact du stockage des cellules explorées ;
- la classe native exacte du canvas de dessin ;
- les textures finales de parchemin ;
- les icônes finales ;
- les annotations libres du joueur ;
- les marqueurs de quêtes ;
- l’affichage éventuel de monstres ;
- la mini-map temps réel.

Ces sujets ne doivent pas provoquer la création d’une nouvelle autorité.

---

## 16. Invariants d’acceptation pour les tickets suivants

Les implémentations MON21.6 doivent préserver les invariants suivants :

1. la cellule du groupe est révélée ;
2. à rayon 1.25, les quatre cardinales sont candidates mais pas les diagonales ;
3. un mur bloque la révélation de la cellule derrière ;
4. une porte fermée est connue depuis le côté exploré mais bloque la cellule derrière ;
5. une porte ouverte peut laisser la révélation atteindre la cellule voisine ;
6. un secret caché est strictement rendu comme un mur normal ;
7. un secret découvert reste connu après fermeture ;
8. plusieurs LevelAssets du même `Z` sont co-projetés sans couture technique visible ;
9. les coordonnées globales utilisent `LogicalPosition.X/Y` et le stride 32 ;
10. Level Up/Down parcourent les `Z` disponibles sans supposer `Z±1` ;
11. le groupe n’apparaît que sur son étage courant ;
12. Save/Load devra restituer exactement exploration et secrets découverts ;
13. aucune géométrie inconnue n’est exposée au WBP ;
14. aucun Actor/Tick Map permanent n’est introduit.

---

## 17. Découpage MON21.6 retenu

```text
MON21.6.1  Map Architecture Contract                         VALIDÉ
MON21.6.2  Exploration State                               À FAIRE
MON21.6.3  Topology-Aware Reveal                           À FAIRE
MON21.6.4  Secret Discovery                                À FAIRE
MON21.6.5  Exploration Persistence / Save Schema           À FAIRE
MON21.6.6  Map Read Model                                  À FAIRE
MON21.6.7  Multi-Tile / Floor Projection                   À FAIRE
MON21.6.8  Existing WBP + Native Map Rendering             À FAIRE
MON21.6.9  Floor Navigation                                À FAIRE
MON21.6.10 Zoom / Pan / Recenter                            À FAIRE
MON21.6.11 Hand-Drawn Parchment Artistic Pass              À FAIRE
MON21.6.12 Map Symbols                                     À FAIRE
MON21.6.13 Automation / Regression / Closure               À FAIRE
```

Le premier jalon jouable complet de la Map est visé à MON21.6.8 ; MON21.6.9–12 ajoutent la navigation et la finition UX/artistique.

---

## 18. Conclusion

MON21.6 repose sur les autorités déjà présentes :

```text
UGridDungeonAsset
    -> composition logique multi-dalles / multi-étages

UGridLevelAsset
    -> géométrie statique 32×32

FGridDungeonRuntimeState
    -> état vivant + future exploration

Party LevelId / Cell / Facing
    -> position courante

Map Read Model
    -> projection filtrée

WBP_GridMap
    -> présentation uniquement
```

Décision finale : **pas de seconde grille, pas de MapActor, pas de Tick permanent, pas de fuite de géométrie cachée vers l’UI.**

Prochain ticket : **MON21.6.2 — Exploration State**.
