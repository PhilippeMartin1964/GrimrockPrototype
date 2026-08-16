# MON1 — Création et validation de `DA_MON_RatGiant`

**Projet :** GrimrockPrototype  
**Moteur :** Unreal Engine 5.5.4  
**Jalon :** MON1 — Types, DataAsset et validation des données  
**Date initiale :** 11 juillet 2026  
**Note de production :** 16 août 2026

> Ce document conserve les valeurs historiques de MON1. Pour la production actuelle, la récompense XP du Rat Géant est supersédée par MON15.7 : `ExperienceReward = 500`.

---

## 1. État du jalon

MON1 a introduit :

- `GridCombatTypes.h` ;
- `GridMonsterTypes.h` ;
- `GridMonsterDefinitionAsset.h/.cpp` ;
- les tests `Grimrock.Monsters.MON1.*` ;
- le DataAsset `DA_MON_RatGiant`.

Chemin canonique :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant
```

---

## 2. Identité

| Propriété | Valeur |
|---|---|
| Monster Id | `MON_RatGiant` |
| Display Name | `Rat géant` |
| Category Id | `Vermin` |
| Danger Level | `1` |

---

## 3. Statistiques historiques de référence

| Propriété | Valeur |
|---|---:|
| Max Health | 8 |
| Physical Armor | 0 |
| Magical Armor | 0 |
| Initiative | 12 |
| Accuracy | 2 |
| Evasion | 1 |
| Action Points Per Turn | 2 |

Movement :

| Propriété | Valeur |
|---|---|
| Grid Footprint | `(1, 1)` |
| Move Duration | `0.36` |
| Turn Duration | `0.12` |
| Blocks Movement | vrai |
| Can Open Doors | faux |
| Can Use Teleporters | faux |

Perception :

| Propriété | Valeur |
|---|---:|
| Sight Range Cells | 5 |
| Hearing Range Cells | 3 |
| Aggro Propagation Range | 3 |
| Shares Aggro With Group | vrai |

AI :

| Propriété | Valeur |
|---|---|
| Primary AI Profile | `DirectMelee` |
| Additional AI Profiles | `FastHarasser` |
| Preferred Min Distance | 1 |
| Preferred Max Distance | 1 |
| Retreat Chance | `0.40` |
| Low Health Threshold | `0.25` |

---

## 4. Attaque `Attack_Bite`

| Propriété | Valeur |
|---|---|
| Attack Id | `Attack_Bite` |
| Display Name | `Morsure` |
| Damage Type | `Physical` |
| Physical Subtype | `Piercing` |
| Min Damage | 2 |
| Max Damage | 5 |
| Accuracy Bonus | 0 |
| Range Cells | 1 |
| Action Point Cost | 1 |

`2 à 5` représente historiquement `1d4 + 1`.

---

## 5. Vulnérabilités

Feu :

```text
Damage Type = Fire
Multiplier  = 1.50
```

Tranchant :

```text
Damage Type       = Physical
Physical Subtype  = Slashing
Multiplier        = 1.25
```

---

## 6. Récompenses et butin

### Experience Reward

Valeur historique MON1 :

```text
10
```

Valeur de production depuis MON15.7 :

```text
500
```

La valeur `500` est un **pool total de groupe** distribué par `FRPGExperienceRewardService` entre les personnages actifs éligibles.

Référence autoritaire actuelle :

```text
docs/Design/MON15_7_BALANCING.md
docs/Design/MON15_CLOSURE.md
```

### Loot Table historique

| Item Definition Id | Chance | Min | Max |
|---|---:|---:|---:|
| `Item_RatMeat` | 0.30 | 1 | 1 |
| `Item_RatTooth` | 0.15 | 1 | 1 |

Des entrées supplémentaires ont pu être ajoutées dans les assets de test/production ultérieurs. Le loot et l'XP restent indépendants.

---

## 7. Validation

La définition expose notamment :

```text
Is Valid Definition
Validate Definition
Has AI Profile
Get Damage Multiplier
Get Attack Definition
```

MON1 validait les règles structurelles de la définition. MON15.7 ajoute désormais une garde de production :

```text
Grimrock.RPG.MON15.7.ProductionRatAsset
```

qui charge directement `DA_MON_RatGiant` et exige :

```text
MonsterId = MON_RatGiant
ExperienceReward = 500
```

Cette garde a été validée sous UE5.5.4 le 16 août 2026.

---

## 8. Note historique

Les valeurs de MON1 restent utiles pour comprendre l'origine du premier monstre de test. Lorsqu'une valeur MON1 est en conflit avec un jalon ultérieur validé, le jalon le plus récent est autoritaire.

Pour l'XP du Rat Géant :

```text
MON1 / MON15.2 historique : 10 XP
MON15.5 / MON15.6 fixture PIE : 1000 XP
MON15.7 production : 500 XP
```
