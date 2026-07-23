# MON4 — Pathfinding de grille et perception logique

## 1. Objectif

MON4 ajoute la planification d'un chemin et la perception du groupe sans encore déplacer automatiquement le Rat géant.

Le jalon contient :

- `FGridMonsterPathfinder`, un BFS déterministe sur les quatre directions ;
- `FGridMonsterPerception`, pour la vue orthogonale et l'ouïe logique ;
- `UGridMonsterBehaviorComponent`, qui mémorise la dernière position connue et prépare un chemin de poursuite ;
- deux Automation Tests MON4 ;
- un affichage de debug du chemin.

MON4 ne contient pas encore :

- le gestionnaire de tours ;
- la dépense des points d'action ;
- l'exécution autonome du chemin ;
- l'attaque ;
- le comportement de repli `FastHarasser`.

Ces éléments commencent avec MON5, MON6 et MON7.

---

## 2. Règles du pathfinding

Le pathfinder utilise un parcours en largeur, ou BFS.

Ordre fixe des voisins :

```text
North
East
South
West
```

Le chemin :

- n'inclut jamais la cellule de départ ;
- respecte les cellules valides et praticables ;
- respecte les murs ;
- respecte les portes fermées ;
- accepte les portes ouvertes ;
- évite les monstres ;
- évite les destinations réservées ;
- évite la cellule du groupe ;
- reste déterministe lorsque plusieurs routes ont la même longueur.

Pour poursuivre le groupe, la destination n'est pas la cellule du groupe. Le composant recherche la plus proche des quatre cellules d'attaque accessibles autour de lui.

---

## 3. Règles de perception MON4

### Vue

La première version utilise une vue orthogonale simple :

- portée définie par `Sight Range Cells` ;
- le Rat et le groupe doivent partager la même ligne X ou Y ;
- chaque bord de cellule traversé doit être libre ;
- un mur ou une porte fermée bloque la vue ;
- une porte ouverte autorise la vue ;
- le Rat ne voit pas autour d'un angle.

Cette règle volontairement simple est adaptée aux couloirs d'un dungeon crawler. Une ligne de vue diagonale plus avancée pourra être ajoutée plus tard sans modifier le composant de comportement.

### Ouïe / odorat

L'ouïe utilise la distance de Manhattan :

```text
abs(RatX - PartyX) + abs(RatY - PartyY)
```

Elle :

- utilise `Hearing Range Cells` ;
- fonctionne autour d'un angle ;
- ne demande pas de ligne de vue ;
- ne traverse pas une distance supérieure à la portée configurée.

### Dernière position connue

Lorsque le Rat voit ou entend le groupe :

```text
Has Last Known Party Cell = true
Last Known Party Cell = cellule actuelle du groupe
```

Si le groupe quitte ensuite la perception, cette cellule reste mémorisée. Le Rat pourra plus tard la rejoindre avant de revenir à `Idle`.

---

## 4. Vérifier `DA_MON_RatGiant`

Ouvrir :

```text
Content/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant
```

Dans `Monster | Perception`, conserver :

```text
Sight Range Cells       = 5
Hearing Range Cells     = 3
Aggro Propagation Range = 3
Shares Aggro With Group = true
```

L'agression de groupe sera réellement raccordée dans MON7. MON4 utilise seulement les deux portées de perception.

---

## 5. Ajouter le composant au Blueprint

Ouvrir :

```text
Content/GrimrockPrototype/Monsters/RatGiant/Blueprints/BP_MON_RatGiant
```

Dans le panneau **Components** :

1. cliquer sur `Add` ;
2. rechercher `Grid Monster Behavior Component` ;
3. l'ajouter ;
4. le renommer `MonsterBehavior`.

Conserver :

```text
Auto Initialize                  = true
Refresh Perception On Begin Play = true
Draw Path After Query            = false
```

Le Blueprint doit maintenant contenir :

```text
SceneRoot
├── CollisionComponent
└── SkeletalMeshComponent

MonsterMovement
MonsterBehavior
```

Le composant trouve automatiquement :

- le `GridMonsterActor` propriétaire ;
- le `BP_GridLevelRuntimeActor` de la carte ;
- le `BP_GrimrockPartyPawn` ;
- le registre d'occupation MON3.

Chercher les erreurs sous :

```text
[GridMonsterBehavior]
```

---

## 6. Animation

Aucune modification artistique n'est demandée par MON4.

Conserver provisoirement :

```text
Idle → Local Space Ref Pose
Move → Local Space Ref Pose
Turn → Local Space Ref Pose
```

Le chapitre séparé consacré au pipeline artistique traitera plus tard :

- l'échelle Blender / FBX / UE5 ;
- les animations ;
- les textures ;
- les matériaux ;
- le Root Motion ;
- les clés d'échelle parasites.

---

## 7. Commandes temporaires de debug

Les touches `I`, `J`, `K` et `L` de MON3 peuvent rester en place.

Ajouter temporairement dans l'Event Graph de `BP_MON_RatGiant` :

```text
O Pressed
→ MonsterBehavior
→ Refresh Perception
```

```text
P Pressed
→ MonsterBehavior
→ Find Pursuit Path
```

```text
U Pressed
→ MonsterBehavior
→ Find Path To Last Known Party Cell
```

```text
Y Pressed
→ MonsterBehavior
→ Draw Debug Path
    Duration = 5.0
```

```text
H Pressed
→ MonsterBehavior
→ Log Debug State
```

Pour chaque fonction booléenne, connecter temporairement le `Return Value` à un `Print String` permet de voir immédiatement `true` ou `false`.

Ces touches seront supprimées lorsque MON5 appellera le composant au début du tour du monstre.

---

## 8. Scénarios manuels

### 8.1. Vue dans un couloir

Placer le Rat et le groupe sur la même ligne, à cinq cases ou moins, sans mur ni porte fermée.

Appuyer sur `O`.

Résultat attendu :

```text
Can See Party  = true
Can Hear Party = selon la distance
Monster State  = Alert
Last Known Party Cell = cellule du groupe
```

### 8.2. Mur ou porte fermée

Placer un mur ou fermer une porte entre le Rat et le groupe sur la même ligne.

Résultat attendu :

```text
Can See Party = false
```

Si la distance de Manhattan reste inférieure ou égale à 3 :

```text
Can Hear Party = true
```

Une porte ouverte doit rendre la vue possible au prochain `Refresh Perception`.

### 8.3. Angle

Placer le groupe autour d'un angle, à trois cases de Manhattan ou moins.

Résultat attendu :

```text
Can See Party  = false
Can Hear Party = true
```

### 8.4. Hors portée

Placer le groupe à plus de cinq cases de vue et plus de trois cases d'ouïe.

Résultat attendu :

```text
Can See Party  = false
Can Hear Party = false
```

La dernière cellule connue ne doit pas être effacée automatiquement.

### 8.5. Chemin de poursuite

Après `Refresh Perception`, appuyer sur `P`, puis `Y`.

Résultat attendu :

- une ligne cyan part de la cellule du Rat ;
- le chemin contourne les murs et portes fermées ;
- le dernier marqueur est sur une cellule adjacente au groupe ;
- la cellule du groupe n'est jamais incluse ;
- `Last Path` n'inclut pas la cellule de départ.

### 8.6. Occupation et réservation

Placer un second Rat sur le chemin le plus court ou réserver une destination avec MON3.

Le chemin doit contourner :

- la cellule occupée ;
- la cellule réservée.

S'il n'existe aucune autre route, `Find Pursuit Path` doit renvoyer `false`.

### 8.7. Dernière position connue

1. placer le groupe dans la perception ;
2. appuyer sur `O` ;
3. déplacer ensuite le groupe hors perception ;
4. appuyer à nouveau sur `O` ;
5. appuyer sur `U`, puis `Y`.

Le chemin doit conduire vers l'ancienne cellule mémorisée, tant qu'elle est accessible.

---

## 9. Lecture du log

`Log Debug State` produit une ligne de ce type :

```text
[GridMonsterBehavior] Monster=BP_MON_RatGiant_C_0 Initialized=true Cell=(4,4) See=true Hear=false LastKnown=true(4,8) PathFound=true Goal=(4,7) Steps=3 Visited=10
```

Interprétation :

- `Cell` : cellule logique du Rat ;
- `See` : vue actuelle ;
- `Hear` : ouïe actuelle ;
- `LastKnown` : mémoire de perception ;
- `Goal` : cellule d'attaque ou dernière cellule connue atteinte ;
- `Steps` : nombre de déplacements nécessaires ;
- `Visited` : nombre de cellules examinées par le BFS.

---

## 10. Tests automatisés

Ouvrir :

```text
Tools → Session Frontend → Automation
```

Rechercher :

```text
Grimrock.Monsters.MON4
```

Lancer :

```text
Grimrock.Monsters.MON4.Pathfinder
Grimrock.Monsters.MON4.Perception
```

Le premier test vérifie :

- chemin direct ;
- bord bloqué ;
- détour déterministe ;
- cellule occupée ;
- destination réservée ;
- destination bloquée ;
- choix stable entre plusieurs buts ;
- absence de chemin.

Le second vérifie :

- vue orthogonale ;
- blocage par mur ou porte fermée ;
- absence de vue autour d'un angle ;
- portée de vue ;
- ouïe autour d'un angle ;
- portée d'ouïe.

---

## 11. Critères de validation MON4

MON4 est validé lorsque :

1. `GrimrockPrototypeEditor` compile ;
2. `MonsterBehavior` est ajouté à `BP_MON_RatGiant` ;
3. l'initialisation trouve le RuntimeActor et le groupe ;
4. la vue détecte le groupe dans un couloir ;
5. un mur ou une porte fermée bloque la vue ;
6. l'ouïe détecte le groupe autour d'un angle dans sa portée ;
7. la dernière cellule connue est conservée après perte de perception ;
8. le BFS produit un chemin déterministe ;
9. le chemin respecte murs, portes, groupe, occupations et réservations ;
10. la destination de poursuite est une cellule adjacente au groupe ;
11. le chemin de debug est visible ;
12. les deux tests MON4 réussissent.
