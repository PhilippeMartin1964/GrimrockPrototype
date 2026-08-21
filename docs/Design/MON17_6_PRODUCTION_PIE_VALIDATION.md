# MON17.6 — Production PIE Validation

Statut : **VALIDÉ ET CLOS SOUS UE5.5.4**

## Contexte

Après validation Automation de `Grimrock.Monsters.MON17.6.1` en **4/4 Success**, un PIE de production a été exécuté avec deux vrais `MON_GoblinThrower` placés dans `Encounter_GoblinThrowers_01`.

Le `LootTable` local de `DA_MON_GoblinThrower` avait été configuré avec trois entrées déterministes pour la validation :

```text
GoblinKnife  Chance=1.0 Quantity=1
Stone        Chance=1.0 Quantity=1
EmptyVial    Chance=1.0 Quantity=1
```

## Résultat PIE

Le premier Gobelin meurt avec :

```text
GoblinKnife  Dropped=true Placed=true
Stone        Dropped=true Placed=true
EmptyVial    Dropped=true Placed=true
Summary      Evaluated=3 Dropped=3 Placed=3 Failed=0
XP           Requested=125 Applied=125 Previous=0 New=125
RemainingLiving=1
```

Le second Gobelin meurt avec :

```text
GoblinKnife  Dropped=true Placed=true
Stone        Dropped=true Placed=true
EmptyVial    Dropped=true Placed=true
Summary      Evaluated=3 Dropped=3 Placed=3 Failed=0
XP           Requested=125 Applied=125 Previous=125 New=250
RemainingLiving=0
Victory=true
```

Le combat se termine ensuite explicitement en :

```text
Phase=Victory Type=Victory Message="Victoire."
```

## Contrats validés

Ce PIE confirme sur les assets réellement joués :

```text
MonsterSpawn / automatic engagement     OK
Encounter group à deux Gobelins         OK
RangedKeeper / ThrowKnife                OK
LootTable de production                  OK
3 objets distincts déposés               OK
FailedLootCount                          0
ExperienceReward                         125 par Gobelin
XP cumulée après deux morts              250
MonsterDied exactly once                 OK
OccupancyReleased                        true
Victory après la dernière mort           OK
```

## État Git final

Le runtime contient les deux nouvelles définitions :

```text
DA_Item_GoblinKnife
DA_Item_EmptyVial
```

ainsi que le `LootTable` modifié de :

```text
DA_MON_GoblinThrower
```

Les trois assets sont présents sur `origin/master` dans :

```text
776ff9677cb304c025db2c01e8d5e13a89a81d78
Add Goblin Thrower production loot Settings Goblin
```

Chemins versionnés :

```text
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_GoblinKnife.uasset
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_EmptyVial.uasset
Content/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.uasset
```

Pointeurs Git LFS vérifiés :

```text
DA_Item_GoblinKnife     sha256:d42195ba9df41db624931752e1744b7a5be5a5713cce23fa84a25c563ad26426
DA_Item_EmptyVial      sha256:a3c4e33bca82222efc33e2209e61cbdcb5573fd2f8e6e54cedbad39dd8ee9070
DA_MON_GoblinThrower   sha256:95d8325f38ac3f59f8917decd95ae4c000843d115ea9cd2c950d3f105852baba
```

La campagne ciblée de clôture est **20/20 Success**. Les probabilités `1.0` restent une configuration de validation MON17.6 ; leur équilibrage final appartient à MON17.7.
