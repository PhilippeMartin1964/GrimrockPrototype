# MON15.2 — Clôture

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**.

MON15.2 raccorde `UGridMonsterDefinitionAsset::ExperienceReward` au champ persistant existant `FGridCharacterInventoryState::Experience`, avec partage déterministe entre les personnages actifs éligibles, plafond MON15.1 et garantie exactly-once via `UGridMonsterDeathComponent::bDeathCommitted`.

## 1. Validation Automation Tests

Les cinq tests dédiés MON15.2 ont été exécutés sous Unreal Engine 5.5.4 et retournent `Success` :

```text
Grimrock.RPG.MON15.2.ActivePartyDistribution
Grimrock.RPG.MON15.2.ProgressionBoundaries
Grimrock.RPG.MON15.2.MonsterDeathExactlyOnce
Grimrock.RPG.MON15.2.LootIndependence
Grimrock.RPG.MON15.2.PersistenceState
```

Les logs confirment notamment :

```text
Reward=10 Eligible=4 Applied=10 Unapplied=0
Character=0 Requested=3 Applied=3 Previous=0 New=3
Character=1 Requested=3 Applied=3 Previous=0 New=3
Character=2 Requested=2 Applied=2 Previous=0 New=2
Character=3 Requested=2 Applied=2 Previous=0 New=2
```

Le franchissement de seuil reste volontairement différé jusqu'à MON15.3 :

```text
Character=0 Requested=2 Applied=2 Previous=999 New=1001 LevelStored=1
Character=1 Requested=1 Applied=1 Previous=189999 New=190000 LevelStored=19
```

`MonsterDeathExactlyOnce` confirme qu'une mort déjà committée ne rejoue pas la récompense XP.

`LootIndependence` confirme qu'un échec de placement du loot ne bloque pas l'XP :

```text
PlacementFailed ... Reason=MissingRuntimeActor
Character=0 Requested=7 Applied=7 Previous=0 New=7
Reward=7 Eligible=1 Applied=7 Unapplied=0
```

`PersistenceState` confirme que `Experience` reste sauvegardée dans le champ existant et que `CurrentSaveVersion` reste inchangé.

## 2. Régressions validées

Les régressions exécutées restent vertes :

```text
Grimrock.RPG.MON15.1.*
Grimrock.Monsters.MON8.* ciblés et suites exécutées
Grimrock.Monsters.MON9.*
Grimrock.CharacterCreation.CC2.*
```

Les tests MON8 confirment notamment que le pipeline de mort, le loot, `MonsterDied` et la victoire restent fonctionnels.

Les tests MON9 confirment notamment les round-trips de monstre mort et les sauvegardes disque, sans replay du commit logique de mort.

## 3. Validation PIE — Rat Géant réel

La validation PIE a été effectuée avec `BP_MON_RatGiant` / `DA_MON_RatGiant` de production.

La valeur réelle observée est :

```text
ExperienceReward = 10
```

Trois Rats Géants distincts ont été tués. Les identités persistantes sont différentes et chaque mort ajoute exactement 10 XP au personnage actif :

```text
Character=0 Requested=10 Applied=10 Previous=0 New=10 LevelStored=1
Monster=BP_MON_RatGiant_C_4 PersistenceId=E4DC825C490F3B73EA579EB7AC3D2AEA Reward=10 Applied=10

Character=0 Requested=10 Applied=10 Previous=10 New=20 LevelStored=1
Monster=BP_MON_RatGiant_C_5 PersistenceId=F73199084F46EDCC7D64ED9C42588D57 Reward=10 Applied=10

Character=0 Requested=10 Applied=10 Previous=20 New=30 LevelStored=1
Monster=BP_MON_RatGiant_C_6 PersistenceId=AAF0E03145A2838D0B4CCB98FC04126C Reward=10 Applied=10
```

Le cumul observé est donc :

```text
0 XP -> 10 XP -> 20 XP -> 30 XP
```

`LevelStored` reste à `1`, conformément au périmètre MON15.2.

## 4. Exactly-once et persistance

La validation du gain unique repose sur deux niveaux complémentaires :

- le PIE réel confirme une seule récompense par mort de Rat Géant observée ;
- le test automatisé `MonsterDeathExactlyOnce` couvre explicitement le second `MarkDead()` et un état de mort restauré ;
- les tests MON9 couvrent la restauration et le round-trip disque des monstres morts ;
- `PersistenceState` couvre la persistance du champ `Experience` existant.

Aucun registre supplémentaire de récompenses n'est nécessaire dans le SaveGame.

## 5. Contrats clôturés

MON15.2 est considéré clos avec les contrats suivants :

- `ExperienceReward` reste data-driven dans `UGridMonsterDefinitionAsset` ;
- seul `Experience` est modifié ;
- `Level` n'est pas modifié par MON15.2 ;
- les statistiques dérivées ne sont pas recalculées ;
- la récompense est partagée uniquement entre `ActiveCharacters` éligibles ;
- les personnages au plafond sont exclus du partage ;
- le reste de division est attribué selon l'ordre stable des personnages actifs ;
- le loot et l'XP restent indépendants ;
- une mort déjà committée ne rejoue pas l'XP ;
- aucun `.uasset`, `.umap` ou WBP n'est requis par MON15.2 ;
- `CurrentSaveVersion` reste `3`.

## 6. Suite

Le prochain sous-jalon est **MON15.3 — Level-up transaction**.

MON15.3 devra traiter le décalage volontaire laissé par MON15.2 entre `Experience` et `Level`, notamment lorsqu'un seuil d'XP est franchi. Il devra définir la transaction de montée de niveau et le recalcul des statistiques sans réintroduire de logique XP dans MonsterDeath.
