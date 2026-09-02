# MON-DEATH-COLLISION01 — Obstacle-Aware Monster Death

Date : 02.09.2026

## Objectif

Empêcher une animation de mort qui recule ou se renverse de traverser visuellement un mur, une porte fermée ou un autre obstacle physique.

La logique de mort reste strictement inchangée :

```text
HP <= 0
→ CommitDeath immédiat
→ occupation libérée
→ collision gameplay OFF
→ loot / XP / MonsterDied
→ présentation de mort seulement
```

MON-DEATH-COLLISION01 agit exclusivement sur la présentation.

## Principe

Chaque `UGridMonsterDefinitionAsset` peut activer `Monster > Animation > Death Collision`.

Avant de jouer la présentation de mort, le runtime transforme `DeathFallLocalDirection` selon le Facing courant du monstre et effectue un sweep capsule dans cette direction.

Le probe accepte désormais explicitement les obstacles `QueryOnly`. C'est indispensable car les murs de grille et certaines géométries de salle peuvent bloquer les requêtes gameplay sans fournir directement une surface Chaos fiable pour un ragdoll.

### Aucun obstacle

```text
Probe libre
→ DeathMontage existant
→ comportement MON17.8 inchangé
```

### Obstacle physique détecté

```text
Probe bloqué
→ le DeathMontage n'est pas utilisé pour cette occurrence
→ création d'un garde physique invisible au plan d'impact
→ SkeletalMesh passe immédiatement en ragdoll
→ collision seulement avec WorldStatic / WorldDynamic / PhysicsBody
→ vitesse initiale vers la chute + composante descendante
→ le corps percute / glisse / se dévie sur l'obstacle
→ dissolution générique habituelle
```

Le basculement est volontairement immédiat lorsqu'un obstacle est pré-détecté : la priorité est de garantir qu'aucune animation cinématique ne puisse traverser le mur avant que la physique ne prenne le relais.

## Contrat de données

```text
bEnableObstacleAwareDeath       = false
DeathFallLocalDirection         = (-1, 0, 0)
DeathObstacleProbeDistance      = 120 cm
DeathObstacleProbeRadius        = 28 cm
DeathObstacleProbeHalfHeight    = 60 cm
DeathRagdollBackwardSpeed       = 140 cm/s
DeathRagdollDownwardSpeed       = 80 cm/s
DeathRagdollAngularSpeedDegrees = 90 deg/s
```

`DeathFallLocalDirection=(-1,0,0)` signifie « vers l'arrière du monstre ».

## Collision et grille

Le `CollisionComponent` gameplay reste désactivé dès `CommitDeath()`.

Le ragdoll utilise uniquement le `USkeletalMeshComponent` :

```text
Object Type = PhysicsBody
WorldStatic  = Block
WorldDynamic = Block
PhysicsBody  = Block
autres canaux = Ignore
```

Le cadavre physique ne reprend jamais une cellule de grille et ne bloque pas le combat. Le probe ignore les composants QueryOnly afin de ne pas prendre le collider gameplay d'un autre monstre pour un mur physique.

## Physics Asset

Le Skeletal Mesh doit posséder un Physics Asset valide. Sans Physics Asset, un obstacle détecté produit un warning `MissingPhysicsAsset` puis revient au `DeathMontage` existant, sans affecter la mort logique.

## Dissolution

Le ragdoll reste actif pendant la pose du cadavre et le début de la dissolution. À la fin du dissolve, le mesh est caché puis la simulation physique est arrêtée.

Une restauration vivante réattache le Skeletal Mesh au `SceneRoot` et réapplique les offsets/rotation/scale authored.

## Configuration UE5 recommandée

Pour une animation qui tombe vers l'arrière :

```text
Enable Obstacle Aware Death       = true
Death Fall Local Direction        = X -1 / Y 0 / Z 0
Death Obstacle Probe Distance     = 120
Death Obstacle Probe Radius       = 28
Death Obstacle Probe Half Height  = 60
Death Ragdoll Backward Speed      = 140
Death Ragdoll Downward Speed      = 80
Death Ragdoll Angular Speed       = 90
```

Adapter Radius/HalfHeight à la morphologie du monstre.

Dans le Skeletal Mesh, vérifier qu'un Physics Asset est assigné et que ses bodies couvrent au minimum bassin, torse, tête et membres principaux.

## Automation

Filtre :

```text
Grimrock.Monsters.MON_DEATH_COLLISION01
```

Les tests couvrent le contrat data-driven, l'API générique, le caractère opt-in, la direction de chute transformée par l'orientation et l'absence de Tick permanent supplémentaire.

Le rendu physique final doit également être validé en PIE contre un mur et une porte fermée.


## Correction 01 — QueryOnly / anti-traversée

Le premier playtest sur WereRat a montré qu'un mur pouvait être visible et logiquement bloquant tout en étant ignoré par le premier probe, car celui-ci exigeait `QueryAndPhysics` ou `PhysicsOnly`.

Le contrat corrigé est :

```text
NoCollision      → ignoré
QueryOnly        → obstacle valide + garde physique temporaire
QueryAndPhysics  → obstacle valide + garde physique temporaire
PhysicsOnly      → obstacle valide + garde physique temporaire
```

Le garde est un `UBoxComponent` transient, invisible, créé au plan d'impact et détruit avec la fin de la présentation/ragdoll ou la dissolution. Il n'appartient pas à la grille gameplay et ne modifie aucun asset de décor.
