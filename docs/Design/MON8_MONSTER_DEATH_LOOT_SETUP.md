# MON8 — mort des monstres, butin et victoire automatique

## Périmètre

MON8 rend la mort logique indépendante de sa présentation. Dès que les points de vie atteignent zéro, le monstre cesse d’agir, libère sa cellule, produit au plus un butin, émet `MonsterDied` et permet au gestionnaire de tours de conclure immédiatement la rencontre. L’absence de Montage, son, VFX, lien ou butin ne bloque jamais le gameplay.

L’expérience reste stockée dans `ExperienceReward`, mais son attribution collective sera réalisée dans un jalon futur.

## Architecture de MonsterDeath

`AGridMonsterActor` possède nativement les composants hérités `MonsterCombat` et `MonsterDeath`. Il ne faut pas ajouter un second composant `MonsterDeath` dans `BP_MON_RatGiant`.

`UGridMonsterDeathComponent` résout paresseusement son monstre propriétaire et `AGridLevelRuntimeActor`. Il mémorise :

- `DeathCell` ;
- `bDeathCommitted` ;
- `bLootGenerated` ;
- l’état de présentation ;
- les objets réellement placés dans `GeneratedLoot` ;
- les compteurs de placement réussi ou échoué.

`MarkDead()` fixe l’état vital et délègue la séquence irréversible au composant. La garde `bDeathCommitted` est posée avant tout appel externe : une seconde invocation ne libère pas deux fois la cellule, ne rejoue pas le butin, les liens, le délégué ou le Montage.

## Ordre de la mort logique

L’ordre runtime est déterministe :

```text
CurrentHealth = 0
→ MonsterState = Dead
→ annulation de l’attaque
→ annulation du déplacement et des réservations
→ HandleOwnerDeath() et libération de l’occupation
→ collision désactivée
→ génération et placement du butin
→ liens EGridObjectEvent::MonsterDied
→ OnMonsterDied
→ présentation visuelle facultative
→ détection immédiate de Victory
```

Le corps peut rester visible, mais sa `CollisionComponent` est en `NoCollision` et il n’occupe plus la grille. `UGridMonsterMovementComponent::HandleOwnerDeath()` reste l’unique voie de libération : aucun second registre d’occupation n’est créé.

## Présentation de mort

`DeathMontage` est facultatif et `DeathExpectedDuration` doit être fini et strictement positif. Si un Montage et un `AnimInstance` existent, le composant lance le Montage et un timer de sécurité réservé à la présentation. Le timer ne retarde ni les événements, ni le butin, ni la victoire.

`NotifyDeathPresentationComplete()` termine uniquement l’état de présentation. Il ne change jamais `MonsterState`, ne réactive jamais la collision et ne détruit pas le corps. Aucun ragdoll n’est ajouté par MON8.

Dans `ABP_MON_RatGiant`, prévoir :

```text
Any State → Dead
Condition : bIsDead
```

L’état `Dead` peut temporairement utiliser `Local Space Ref Pose`, ou une animation de mort existante.

## Table de butin pondérée

`FGridMonsterLootResolver` est pur et indépendant des Actors. Il effectue un seul jet de sélection et parcourt le tableau dans son ordre :

```text
Entrée A : 0,40  → tranche [0,00 ; 0,40[
Entrée B : 0,25  → tranche [0,40 ; 0,65[
Reste             → aucun butin
```

Les entrées ne reçoivent pas de jets indépendants. Une seule entrée au maximum est retenue. Le jet de quantité `RandRange(MinQuantity, MaxQuantity)` n’est consommé que si une entrée est sélectionnée.

`ItemDefinitionAsset`, ajouté à la fin de `FGridMonsterLootEntry`, est prioritaire pour résoudre l’identifiant. `ItemDefinitionId` reste le repli compatible avec les anciennes données. Si les deux sont renseignés, leurs identifiants doivent correspondre. Les doublons sont validés sur l’identifiant résolu et la somme des chances ne peut pas dépasser `1,0`.

La graine du butin combine `SpawnObjectId`, `MonsterId` et le sel constant `MON8LootSeedSalt`. Ce `FRandomStream` est indépendant du `CombatRandomStream`.

## Placement runtime

Le résultat devient un `FGridItemInstance` avec un nouveau `RuntimeObjectId`, une quantité, un poids, un nom, l’état de lumière et le propriétaire `World`. Le drop utilise la cellule de mort, `EGridEdge::None` et les offsets stables suivants :

```text
(0, 0, 0)
(20, 0, 0)
(-20, 0, 0)
(0, 20, 0)
(0, -20, 0)
```

La hauteur standard de 12 unités est ajoutée par le dépôt monde. `TryDropItemInstanceAtCell()` conserve son ancienne signature et possède une surcharge C++ acceptant directement le `UGridItemDefinitionAsset` de la table. Le même `SpawnedItemEntries` est alimenté, les plaques de pression sont rafraîchies et `CaptureCurrentLevelRuntimeState()` reste compatible.

Si aucune définition n’est résolue ou si le placement échoue, le compteur d’échec augmente, mais la mort et les événements continuent.

## Événements

`OnMonsterDied(Monster, DeathCell)` représente la mort logique validée. Il est diffusé exactement une fois après le butin et les liens.

`EGridObjectEvent::MonsterDied` est ajouté après toutes les valeurs historiques. Un `MonsterSpawn` peut désormais être source de ce seul événement dans le panneau de liens. À la mort :

```cpp
RuntimeActor->ExecuteLinksFromRuntimeObject(
    SpawnObjectId,
    EGridObjectEvent::MonsterDied);
```

L’association nécessite que `SpawnObjectId` corresponde à l’`ObjectId` du `MonsterSpawn`. Le projet ne possède pas encore le pipeline natif complet qui génère tous les monstres depuis ces placements. Pour un `BP_MON_RatGiant` directement placé sans identifiant correspondant, la mort, le butin, l’occupation et `OnMonsterDied` fonctionnent ; seul le lien de LevelAsset peut ne pas être trouvé. Aucune association ambiguë par cellule n’est tentée.

## Victoire immédiate

Au démarrage d’une rencontre, le TurnManager lie chaque participant à `OnMonsterDied` sans doublon. Le handler :

- retire le mort de l’ordre ennemi ;
- supprime ses actions futures ;
- annule son action active ;
- passe au monstre suivant si nécessaire ;
- appelle immédiatement `FinishCombat(Victory)` si aucun participant vivant ne reste.

Les bindings sont retirés dans `AbortCombat`, `FinishCombat` et `EndPlay`. Pendant `PlayerPhase`, la mort d’un monstre laisse la phase continuer si un autre ennemi est vivant.

## Logs

Les catégories `LogGridMonsterDeath` et `LogGridMonsterLoot` produisent notamment :

```text
[GridMonsterDeath] Commit Monster=... Cell=(X,Y) SpawnObjectId=... OccupancyReleased=true
[GridMonsterDeath] Links Monster=... SourceId=... Event=MonsterDied Executed=true
[GridMonsterDeath] Broadcast Monster=... DeathCell=(X,Y)
[GridMonsterLoot] Roll Monster=... Roll=0.312 Selected=... Quantity=1
[GridMonsterLoot] NoDrop Monster=... Roll=0.812 TotalChance=0.650
[GridMonsterLoot] Placed Monster=... Item=... Quantity=1 Cell=(X,Y) RuntimeId=...
[GridMonsterLoot] PlacementFailed Monster=... Item=... Cell=(X,Y) Reason=...
[GridTurnManager] MonsterDied Monster=... RemainingLiving=0 Victory=true
```

Ces logs ne sont jamais produits à chaque Tick.

## Tests Automation

Lancer :

```text
Grimrock.Monsters.MON8.LootResolver
Grimrock.Monsters.MON8.DeathExactlyOnce
Grimrock.Monsters.MON8.OccupancyRelease
Grimrock.Monsters.MON8.VictoryOnLastDeath
Grimrock.Monsters.MON8.MonsterDiedEvent
```

Puis les régressions :

```text
Grimrock.Monsters.MON3
Grimrock.Monsters.MON4
Grimrock.Monsters.MON5
Grimrock.Monsters.MON6
Grimrock.Monsters.MON7
```

Les tests MON8 couvrent les tranches cumulatives, le reliquat sans drop, les quantités, la graine, la garde de mort, l’occupation MON3, la victoire et la stabilité numérique de l’enum.

## Réglages manuels UE5

Dans `DA_MON_RatGiant` :

```text
Death Montage            = None temporairement
Death Expected Duration  = 1.0

Loot Table / Entry 0
Item Definition Asset    = un ItemDefinition existant et valide
Item Definition Id       = le même identifiant
Drop Chance              = 1.0 pour le premier test
Min Quantity             = 1
Max Quantity             = 1
```

L’ItemDefinition doit posséder un `WorldMesh`. Après validation, remettre une probabilité de jeu, par exemple `Drop Chance = 0.40`. Aucun DataAsset n’est modifié automatiquement par MON8.

Dans le Grid Editor, un `MonsterSpawn` correctement associé peut émettre `Monster Died` vers une porte avec la commande `Open`.

## Checklist PIE MON8

1. Fermer Unreal Editor et compiler `GrimrockPrototypeEditor` en Development Win64.
2. Ouvrir `BP_MON_RatGiant`.
3. Vérifier les composants C++ hérités `MonsterCombat` et `MonsterDeath`.
4. Vérifier `MonsterMovement` et `MonsterBehavior`, sans ajouter un second `MonsterDeath`.
5. Ouvrir `DA_MON_RatGiant`.
6. Configurer une entrée `LootTable` avec `Drop Chance = 1.0`.
7. Choisir un `ItemDefinitionAsset` valide possédant un `WorldMesh`.
8. Lancer le PIE.
9. Démarrer le combat par perception.
10. Tuer un Rat avec `DebugKillMonster` ou la voie de dégâts disponible.
11. Vérifier `CurrentHealth = 0`.
12. Vérifier `MonsterState = Dead`.
13. Vérifier que la collision est désactivée.
14. Vérifier que la cellule est immédiatement libérée.
15. Vérifier qu’un autre Rat peut traverser ou réserver cette cellule.
16. Vérifier que le corps reste visible.
17. Vérifier qu’un seul objet de butin apparaît sur la cellule de mort.
18. Vérifier que cet objet est ramassable.
19. Appeler une seconde fois `DebugKillMonster` et vérifier l’absence de second butin ou événement.
20. Avec un lien `Monster Died → Open`, vérifier que la porte s’ouvre une seule fois et que les identifiants correspondent.
21. Tuer le dernier Rat et vérifier la transition immédiate vers `Victory`.
22. Restaurer `Drop Chance` à la valeur de jeu retenue, par exemple `0.40`.
