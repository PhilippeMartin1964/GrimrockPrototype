# MON17.6 — Encounter / Loot / XP Integration — Clôture

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**
Date : **21 août 2026**

## Résultat

Le Gobelin lanceur réutilise les contrats génériques existants sans branche spécifique :

```text
Encounter / waves MON13
→ mort exactly-once MON8
→ loot data-driven
→ XP MON15
→ libération d'occupation / Victory
→ Save / Continue MON9 sans replay
```

## Validation Automation

La suite dédiée est **4/4 Success** :

```text
Grimrock.Monsters.MON17.6.1.DeathRewardsExactlyOnce
Grimrock.Monsters.MON17.6.1.EncounterWaveParticipation
Grimrock.Monsters.MON17.6.1.PersistenceNoReplay
Grimrock.Monsters.MON17.6.1.ProductionRewardContract
```

La campagne ciblée finale est **20/20 Success** :

```text
MON17.6.1    4/4
MON13.4      4/4
MON8         7/7
MON15.2      5/5
```

Elle confirme notamment l'encounter à deux vagues, `3 × 125 = 375 XP`, la mort exactly-once, le loot indépendant, la victoire et la persistance sans duplication.

## Validation PIE de production

Deux vrais `MON_GoblinThrower` de `Encounter_GoblinThrowers_01` ont chacun produit :

```text
GoblinKnife x1
Stone       x1
EmptyVial   x1
```

Chaque mort a donné `125 XP`, sans échec de placement. La seconde mort a libéré l'occupation restante et déclenché `Victory` avec `250 XP` cumulés.

## Assets de production

Les trois assets requis sont présents sur `origin/master` dans le commit `776ff9677cb304c025db2c01e8d5e13a89a81d78` :

```text
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_GoblinKnife.uasset
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_EmptyVial.uasset
Content/GrimrockPrototype/Monsters/GoblinThrower/Data/DA_MON_GoblinThrower.uasset
```

Leurs pointeurs Git LFS ont été vérifiés. La table de loot est volontairement restée déterministe pour la validation MON17.6 ; sa fréquence finale appartient à MON17.7.

## Décision

**MON17.6 — VALIDÉ ET CLOS.**

Prochaine étape autoritaire :

```text
MON17.7 — Balance / Closure
```
