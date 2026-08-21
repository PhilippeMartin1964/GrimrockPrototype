# MON17.6 — Production PIE Validation

Statut : **RUNTIME VALIDÉ SOUS UE5.5.4 — clôture Git en attente des assets binaires de loot**

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

## État Git à la validation

Le runtime local contient les deux nouvelles définitions :

```text
DA_Item_GoblinKnife
DA_Item_EmptyVial
```

ainsi que le `LootTable` modifié de :

```text
DA_MON_GoblinThrower
```

Au moment de cette validation, ces modifications binaires Unreal ne sont **pas encore présentes sur `origin/master`**. Le connecteur GitHub ne peut pas lire ni téléverser les `.uasset` présents uniquement dans le dossier local du poste de développement.

MON17.6 ne doit donc être marqué `CLOS` dans la roadmap qu'après versionnement de :

```text
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_GoblinKnife.uasset
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_EmptyVial.uasset
Content/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.uasset
```

Les probabilités `1.0` sont une configuration de validation MON17.6. L'équilibrage final des chances de drop appartient à MON17.7.
