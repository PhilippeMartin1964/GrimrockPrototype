# MON14.3 — Validation Checklist

## 1. Compilation

Compiler `GrimrockPrototypeEditor Win64 Development` sous UE 5.5.4.

Attendu : aucune erreur C++/UHT/Unity Build.

## 2. Tests automatisés ciblés

Exécuter :

```text
Grimrock.Monsters.MON14.1
Grimrock.Monsters.MON14.2
Grimrock.Monsters.MON14.3
```

Puis la régression monstres :

```text
Grimrock.Monsters.MON
```

Attendu : aucun échec.

## 3. PIE — Loop

Configurer un MonsterSpawn `Idle` avec :

```text
PatrolMode = Loop
Waypoints = A, B, C
```

Vérifier :

- le garde rejoint le waypoint approprié ;
- il se déplace case par case ;
- A -> B -> C -> A ;
- aucune traversée de mur/porte fermée/monstre ;
- une case occupée provoque un retry, pas une superposition.

## 4. PIE — PingPong

Configurer :

```text
PatrolMode = PingPong
Waypoints = A, B, C
```

Attendu :

```text
A -> B -> C -> B -> A -> B ...
```

## 5. Orientation et attente

Sur un waypoint :

- définir un `Facing` cardinal différent de l'orientation d'arrivée ;
- définir `WaitSeconds > 0`.

Attendu : arrivée -> rotation -> attente -> départ.

Avec `Facing=None`, aucune orientation forcée.

## 6. Dormant

MonsterSpawn :

```text
InitialMonsterState = Dormant
PatrolMode = Loop
```

Sans perception : le monstre reste immobile.

S'il voit le groupe : réveil/Alert puis engagement automatique.

S'il entend seulement le groupe : réveil/Alert puis investigation, sans combat
automatique immédiat.

## 7. Investigation

Placer le groupe hors du rayon visuel mais dans le rayon auditif.

Attendu :

- `bCanHearParty=true` ;
- dernière cellule connue mise à jour ;
- le garde abandonne sa patrouille ;
- il se dirige vers la dernière position exploitable ;
- arrivé sur place, il observe les quatre directions ;
- s'il ne retrouve rien, il repasse Idle et rejoint sa route.

## 8. Détection pendant patrouille

Faire entrer le groupe dans la ligne de mire d'un garde en mouvement ou en
attente.

Attendu :

- interruption de la patrouille ;
- aucune position intermédiaire conservée ;
- engagement MON14.1 ;
- initiative/combat normaux.

## 9. Aggro de groupe

Un garde A voit le groupe et partage l'aggro avec un garde B qui patrouille.

Attendu : B est admis au combat par MON7 et sa locomotion d'exploration est
suspendue avant son tour de combat.

## 10. Après victoire

S'il existe un autre garde vivant hors de la rencontre précédente :

- sa patrouille peut reprendre ;
- une vision ultérieure peut déclencher une nouvelle rencontre depuis l'état
  terminal Victory précédent.

## 11. Continue / transition

Sauvegarder pendant qu'un garde est Idle/Alert/Pursuing puis Continue.

Attendu :

- cellule/facing/LastKnown restaurés par MON9 ;
- la route est relue depuis le LevelAsset ;
- le curseur de patrouille est reconstruit ;
- aucune position entre deux cellules n'est sauvegardée ;
- aucune patrouille ne s'exécute pendant une transition de donjon.
