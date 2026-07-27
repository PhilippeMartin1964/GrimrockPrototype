# MON8 — mort des monstres, butin et victoire automatique

## Périmètre

MON8 rend la mort logique indépendante de sa présentation. Dès que les points de vie atteignent zéro, le monstre cesse d’agir, libère sa cellule, évalue toutes ses entrées de butin, émet `MonsterDied` et permet au gestionnaire de tours de conclure immédiatement la rencontre. L’absence de Montage, son, VFX, lien ou butin ne bloque jamais le gameplay.

L’expérience reste stockée dans `ExperienceReward`, mais son attribution collective sera réalisée dans un jalon futur.

## Architecture de MonsterDeath

`AGridMonsterActor` possède nativement les composants hérités `MonsterCombat` et `MonsterDeath`. Il ne faut pas ajouter un second composant `MonsterDeath` dans `BP_MON_RatGiant`.

`UGridMonsterDeathComponent` résout paresseusement son monstre propriétaire et `AGridLevelRuntimeActor`. Il mémorise :

- `DeathCell` ;
- `bDeathCommitted` ;
- `bLootGenerated` ;
- l’état de présentation ;
- tous les objets réellement placés dans `GeneratedLoot` ;
- `PlacedLootCount`, nombre total de placements réussis ;
- `FailedLootCount`, nombre total de placements échoués.

`MarkDead()` fixe l’état vital et délègue la séquence irréversible au composant. La garde `bDeathCommitted` est posée avant tout appel externe. `bLootGenerated` est posée avant l’évaluation de la table : une seconde invocation ne libère pas deux fois la cellule, ne rejoue pas les jets, ne crée pas de nouveaux `RuntimeObjectId` et ne réémet ni liens, ni délégué, ni Montage.

## Ordre de la mort logique

L’ordre runtime est déterministe :

```text
CurrentHealth = 0
→ MonsterState = Dead
→ annulation de l’attaque
→ annulation du déplacement et des réservations
→ HandleOwnerDeath() et libération de l’occupation
→ collision désactivée
→ évaluation et placement de tous les butins réussis
→ liens EGridObjectEvent::MonsterDied
→ OnMonsterDied
→ présentation visuelle facultative
→ détection immédiate de Victory
```

La mort, `MonsterDied`, les liens, la victoire et la libération de cellule ne dépendent jamais du succès du butin. Le corps peut rester visible, mais sa `CollisionComponent` est en `NoCollision` et il n’occupe plus la grille.

## Présentation de mort

`DeathMontage` est facultatif et `DeathExpectedDuration` doit être fini et strictement positif. Si un Montage et un `AnimInstance` existent, le composant lance le Montage et un timer de sécurité réservé à la présentation. Le timer ne retarde ni les événements, ni le butin, ni la victoire.

`NotifyDeathPresentationComplete()` termine uniquement l’état de présentation. Il ne change jamais `MonsterState`, ne réactive jamais la collision et ne détruit pas le corps.

Dans `ABP_MON_RatGiant`, prévoir :

```text
Any State → Dead
Condition : bIsDead
```

## Table de butin à jets indépendants

`FGridMonsterLootResolver` est pur et indépendant des Actors et du monde. Chaque entrée valide de `LootTable` reçoit son propre jet dans `[0, 1[`. Si `DropRoll < DropChance`, l’entrée réussit et reçoit ensuite son propre jet de quantité entre `MinQuantity` et `MaxQuantity`. L’évaluation continue toujours avec l’entrée suivante.

- `DropChance = 0.0` signifie que l’objet ne tombe jamais ;
- `DropChance = 1.0` signifie que l’objet tombe toujours ;
- `DropChance = 0.4` signifie 40 % de chance, indépendamment des autres entrées ;
- plusieurs entrées peuvent réussir lors de la même mort ;
- l’échec d’une entrée n’empêche jamais les suivantes d’être évaluées ;
- la somme des chances n’a aucune limite globale.

Une somme de `1.00 + 0.40 + 0.80 = 2.20` est valide. Elle ne représente pas une probabilité globale : elle correspond à une espérance de `2,20` objets par évaluation si chaque quantité vaut un.

Exemple :

```text
Loot Table
├── Key_Iron
│   ├── Drop Chance = 1.00
│   ├── Min Quantity = 1
│   └── Max Quantity = 1
│
├── Item_RatTooth
│   ├── Drop Chance = 0.40
│   ├── Min Quantity = 1
│   └── Max Quantity = 1
│
└── Item_RatMeat
    ├── Drop Chance = 0.80
    ├── Min Quantity = 1
    └── Max Quantity = 2
```

Les résultats possibles incluent :

- clé seule ;
- clé + dent ;
- clé + viande ;
- clé + dent + viande.

`ItemDefinitionAsset`, ajouté à la fin de `FGridMonsterLootEntry`, est prioritaire pour résoudre l’identifiant. `ItemDefinitionId` reste le repli compatible avec les anciennes données. Si les deux sont renseignés, leurs identifiants doivent correspondre. Chaque entrée conserve sa validation individuelle : identifiant résolu, chance finie comprise entre 0 et 1, quantité minimale positive et maximum supérieur ou égal au minimum. Les doublons d’identifiant résolu restent interdits.

## Déterminisme par entrée

La graine MON8 de base combine `ResolvePersistenceId()`, `MonsterId` et `MON8LootSeedSalt`. Elle n’utilise jamais `CombatRandomStream`.

Le résolveur dérive ensuite un sous-seed pour chaque entrée à partir de la graine de base, de l’`ItemDefinitionId` résolu et d’un sel dédié. Chaque entrée possède donc son propre `FRandomStream`.

Conséquences :

- le même monstre et la même table reproduisent exactement les mêmes jets ;
- réordonner la table ne change pas le jet ni la quantité d’un identifiant ;
- ajouter une entrée ne change pas les résultats des entrées existantes ;
- une entrée invalide est ignorée sans consommer les jets des autres ;
- deux identités persistantes différentes peuvent produire des résultats différents ;
- restaurer un monstre mort ne rejoue aucun jet.

## Placement runtime

Chaque résultat réussi devient un `FGridItemInstance` indépendant avec un nouveau `RuntimeObjectId`, une quantité, un poids, un nom, l’état de lumière et le propriétaire `World`. Chaque objet reste un `AGridItemActor` ramassable par le système existant ; aucun Actor conteneur de butin n’est créé.

Le dépôt utilise la cellule de mort, `EGridEdge::None` et `PlacedLootCount` pour sélectionner un offset stable :

```text
(0, 0, 0)
(20, 0, 0)
(-20, 0, 0)
(0, 20, 0)
(0, -20, 0)
(20, 20, 0)
(-20, 20, 0)
(20, -20, 0)
(-20, -20, 0)
```

La hauteur standard de 12 unités est ajoutée par le dépôt monde. `TryDropItemInstanceAtCell()` alimente le système `SpawnedItemEntries`, rafraîchit les plaques de pression et laisse `CaptureCurrentLevelRuntimeState()` capturer chaque item dans un `FGridRuntimeItemState` distinct.

Si une définition ne peut pas être résolue ou si un placement échoue, `FailedLootCount` augmente et le composant poursuit avec les autres résultats. `GeneratedLoot` ne contient que les objets réellement placés.

## Événements et victoire immédiate

`OnMonsterDied(Monster, DeathCell)` représente la mort logique validée. Il est diffusé exactement une fois après le butin et les liens.

`EGridObjectEvent::MonsterDied` est ajouté après toutes les valeurs historiques. L’association nécessite que `SpawnObjectId` corresponde à l’`ObjectId` du `MonsterSpawn`. L’absence de lien n’empêche ni la mort, ni le butin, ni la libération de cellule.

Au démarrage d’une rencontre, le TurnManager lie chaque participant à `OnMonsterDied`. Le handler retire le mort de l’ordre ennemi, supprime ses actions, passe au monstre suivant si nécessaire et appelle immédiatement `FinishCombat(Victory)` si aucun participant vivant ne reste.

## Logs

Les catégories `LogGridMonsterDeath` et `LogGridMonsterLoot` produisent notamment :

```text
[GridMonsterLoot] Roll Monster=Rat Entry=0 Item=Key_Iron Chance=1.000 Roll=0.862 Dropped=true Quantity=1
[GridMonsterLoot] Roll Monster=Rat Entry=1 Item=Item_RatTooth Chance=0.400 Roll=0.317 Dropped=true Quantity=1
[GridMonsterLoot] Roll Monster=Rat Entry=2 Item=Item_RatMeat Chance=0.800 Roll=0.914 Dropped=false Quantity=0
[GridMonsterLoot] NoDrop Monster=Rat Entry=2 Item=Item_RatMeat Chance=0.800 Roll=0.914
[GridMonsterLoot] Placed Monster=Rat Item=Key_Iron Quantity=1 Cell=(X,Y) RuntimeId=...
[GridMonsterLoot] PlacementFailed Monster=Rat Item=... Cell=(X,Y) Reason=...
[GridMonsterLoot] Summary Monster=Rat Evaluated=3 Dropped=2 Placed=2 Failed=0
```

Il n’existe plus de log de probabilité totale ni de résultat global « aucun butin ». Aucun de ces logs n’est produit à chaque Tick.

## Tests Automation

Lancer la régression complète :

```text
Grimrock.Monsters.MON
```

Puis les deux tests de sauvegarde :

```text
Grimrock.CharacterCreation.CC5
```

Les tests MON8 couvrent notamment :

- table vide, chance nulle et drop garanti ;
- quantité bornée ;
- plusieurs réussites indépendantes ;
- échec d’une entrée suivi d’une réussite ;
- somme des chances égale à 2,20 ;
- entrée invalide ignorée ;
- déterminisme à graine identique ;
- stabilité par `ItemDefinitionId` après réordonnancement ou ajout ;
- graine différente ;
- placement réel de trois objets transient avec des `RuntimeObjectId` distincts ;
- garde contre un second `MarkDead()`.

MON9 vérifie également qu’un mort restauré conserve `bDeathCommitted=true` et `bLootGenerated=true`, sans nouveau jet, objet, événement ou lien.

## Réglages manuels UE5

Ne modifier aucun DataAsset automatiquement. Après compilation, le réglage prévu dans `DA_MON_RatGiant` est :

```text
Key_Iron
- Item Definition Asset = DA_Item_IronKey
- Item Definition Id = Key_Iron
- Drop Chance = 1.00
- Min Quantity = 1
- Max Quantity = 1

Item_RatTooth
- à ajouter lorsque DA_Item_RatTooth existera
- Drop Chance = 0.40
- Min Quantity = 1
- Max Quantity = 1

Item_RatMeat
- à ajouter lorsque DA_Item_RatMeat existera
- Drop Chance = 0.80
- Min Quantity = 1
- Max Quantity = 2
```

Tant que les DataAssets de dent et de viande n’existent pas, conserver uniquement la clé ou garder ces entrées de côté. Chaque définition réellement utilisée doit être valide et peut fournir un `WorldMesh`. La somme des `DropChance` ne rend plus `DA_MON_RatGiant` invalide.

## Checklist PIE MON8

1. Compiler avec Unreal Editor fermé.
2. Ouvrir `DA_MON_RatGiant`.
3. Configurer trois `ItemDefinitionAsset` réellement existants pour le test.
4. Régler temporairement leurs `DropChance` à `1.0`.
5. Lancer le PIE.
6. Tuer un Rat.
7. Vérifier trois logs `Dropped=true`.
8. Vérifier trois logs `Placed`.
9. Vérifier trois objets distincts sur la cellule.
10. Vérifier des `RuntimeObjectId` différents.
11. Ramasser les trois objets.
12. Vérifier leur présence dans l’inventaire.
13. Tuer une seconde fois le Rat via `DebugKillMonster`.
14. Vérifier qu’aucun objet supplémentaire n’apparaît.
15. Recommencer avec `1.00 / 0.40 / 0.80`.
16. Vérifier que la clé tombe toujours.
17. Vérifier que dent et viande varient indépendamment.
18. Sauvegarder avec plusieurs objets encore au sol.
19. Arrêter le PIE.
20. Recharger.
21. Vérifier que le Rat reste mort.
22. Vérifier que le nombre d’objets au sol est inchangé.
23. Vérifier qu’aucun nouveau `Roll` n’est produit au chargement.
24. Vérifier qu’aucun nouveau `MonsterDied` n’est émis.
