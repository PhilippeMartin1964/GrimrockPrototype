# MON17.6.1 — Goblin Encounter / Reward Integration Contract

Statut : **VALIDÉ ET CLOS sous UE5.5.4**

## Objectif

Prouver que `MON_GoblinThrower` réutilise sans branche spécifique les contrats déjà établis par MON13, MON8 et MON15 :

```text
MonsterSpawn / Encounter / Waves   MON13
MonsterDied / Loot / Occupancy     MON8
ExperienceReward / exactly-once    MON15
Save / Continue                    MON9 + MON13
```

MON17.6.1 ne modifie aucun code runtime de production.

## Audit du runtime existant

`UGridMonsterDefinitionAsset` porte déjà :

```text
ExperienceReward
LootTable
```

`UGridMonsterDeathComponent::CommitDeath()` pose `bDeathCommitted=true` avant les hooks de récompense, puis exécute :

```text
release occupancy
→ GenerateAndPlaceLoot()
→ AwardToActiveParty(ExperienceReward)
→ MonsterDied links / broadcast
→ NotifyMonsterEncounterDeath()
```

`RestoreCommittedDeathState()` restaure également :

```text
bDeathCommitted = true
bLootGenerated  = true
MonsterState    = Dead
```

Le contrat exactly-once nécessaire à MON17.6 existe donc déjà génériquement.

## Audit des assets de loot

Le Bestiaire Volume II définit pour le Gobelin lanceur :

```text
couteaux
pierres taillées
fiole vide
```

Lors de l'implémentation Automation initiale, le seul de ces concepts déjà versionné était :

```text
/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Stone
ItemDefinitionId = Stone
```

Les deux définitions manquantes ont ensuite été créées dans l'éditeur et sont maintenant versionnées avec le LootTable de production :

```text
DA_Item_GoblinKnife
DA_Item_EmptyVial
DA_MON_GoblinThrower
```

Le choix de fixture Automation reste volontaire :

- Automation utilise trois `UGridItemDefinitionAsset` synthétiques pour vérifier le contrat de loot indépendant ;
- le test de Save/Continue utilise le vrai `DA_Item_Stone`, afin que la définition puisse être résolue après reconstruction ;
- aucun `.uasset` n'est créé ou modifié par le test MON17.6.1 ;
- le PIE de production et MON17.7 chargent désormais les vrais assets versionnés.

Les IDs synthétiques `MON176_GoblinKnife`, `MON176_ShapedStone` et `MON176_EmptyVial` ne sont **pas** des IDs de production et ne doivent pas être copiés tels quels dans le DataAsset final.

## Suite automatisée

Filtre :

```text
Grimrock.Monsters.MON17.6.1
```

Quatre tests sont ajoutés.

### ProductionRewardContract

Vérifie les points déjà authorés en production :

```text
DA_MON_GoblinThrower.MonsterId          = MON_GoblinThrower
DA_MON_GoblinThrower.ExperienceReward   = 125
DA_Item_Stone.ItemDefinitionId          = Stone
```

Puis vérifie avec trois définitions synthétiques que les trois entrées ArtBook restent des probabilités indépendantes valides dans `FGridMonsterLootResolver`.

### EncounterWaveParticipation

Utilise le vrai `DA_MON_GoblinThrower` dans un encounter synthétique MON13 :

```text
Wave 0 : 2 Gobelins
Wave 1 : 1 Gobelin
```

Le test vérifie :

```text
StartMonsterEncounter
→ 2 Gobelins wave 0
→ première mort : wave 0 reste active
→ seconde mort : wave 1 spawn
→ dernière mort : EncounterCompleted
→ DefeatedSpawnIds = 3
→ XP total = 3 × 125 = 375
```

Aucun traitement `MonsterId == MON_GoblinThrower` n'est introduit.

### DeathRewardsExactlyOnce

Crée un Gobelin synthétique avec :

```text
ExperienceReward = 125
3 loots garantis
```

et vérifie dans un vrai runtime :

```text
première mort
→ XP +125
→ 3 objets placés
→ LogicalDeathEventCount = 1
→ occupation libérée
→ Victory

seconde MarkDead
→ XP inchangée
→ loot inchangé
→ événement logique inchangé
```

Le test capture aussi les trois objets dans `FGridLevelRuntimeState.Items`.

### PersistenceNoReplay

Crée un `MonsterSpawn` Gobelin persistant et lui attribue un loot garanti utilisant le vrai `DA_Item_Stone`.

Le test exécute :

```text
mort
→ XP +125
→ Stone placée
→ CaptureCurrentLevelRuntimeState
→ SaveGameToMemory
→ LoadGameFromMemory
→ DungeonRuntimeState restauré
→ RebuildLevel
→ ApplyCurrentLevelRuntimeState
→ Gobelin restauré Dead
→ bDeathCommitted restauré
→ nouvelle MarkDead
```

Résultat attendu et validé :

```text
XP reste 125
Items reste 1
aucun replay de récompense
```

## Validation UE5.5.4

Après correction de deux fixtures de test (`MonsterBehavior` manquant dans la fixture combat et `ApplyCurrentLevelRuntimeState()` manquant dans la simulation de Continue), la suite complète a été exécutée avec succès :

```text
Grimrock.Monsters.MON17.6.1.DeathRewardsExactlyOnce      Success
Grimrock.Monsters.MON17.6.1.EncounterWaveParticipation  Success
Grimrock.Monsters.MON17.6.1.PersistenceNoReplay          Success
Grimrock.Monsters.MON17.6.1.ProductionRewardContract    Success

MON17.6.1 = 4/4 Success
```

Le run valide notamment :

```text
Victory sur la dernière mort
3 loots indépendants, Failed=0
XP Gobelin = 125 exactement une fois
Encounter 2 vagues = 3 Gobelins = 375 XP
Save/Continue restaure le Gobelin Dead et Items=1
aucune duplication XP / loot après restore
```

## Porte de sortie MON17.6.1

```text
1. Compilation UE5.5.4                         VALIDÉE
2. Grimrock.Monsters.MON17.6.1                4/4 SUCCESS
3. Grimrock.Monsters.MON13.4                  4/4 SUCCESS
4. Grimrock.Monsters.MON8                     7/7 SUCCESS
5. Grimrock.RPG.MON15.2                       5/5 SUCCESS
6. Campagne ciblée de clôture                 20/20 SUCCESS
```

## Étape production après Automation — réalisée

Les deux définitions correspondant au Bestiaire ont été créées dans l'éditeur :

```text
couteau Gobelin
fiole vide
```

`DA_MON_GoblinThrower.LootTable` contient les trois entrées de production :

```text
couteau
Stone
fiole vide
```

La configuration déterministe `Chance=1.0`, `Quantity=1` reste la baseline de validation MON17.6. Les probabilités finales appartiennent à MON17.7.

## Hors périmètre

- création automatique de `.uasset` ;
- nouveau système de loot ;
- récompense d'encounter séparée des récompenses par monstre ;
- modification de `ExperienceReward=125` ;
- équilibrage des chances de drop ;
- refactor du SaveGame.
