# MON1 — Création et validation de `DA_MON_RatGiant`

**Projet :** GrimrockPrototype  
**Moteur :** Unreal Engine 5.5.4  
**Jalon :** MON1 — Types, DataAsset et validation des données  
**Date :** 11 juillet 2026

---

## 1. État du jalon

La partie C++ de MON1 comprend désormais :

- `GridCombatTypes.h` ;
- `GridMonsterTypes.h` ;
- `GridMonsterDefinitionAsset.h/.cpp` ;
- les tests `Grimrock.Monsters.MON1.*`.

Le fichier `.uasset` doit être créé dans Unreal Editor, car il s’agit d’un asset binaire.

---

## 2. Compiler le projet

1. Fermer Unreal Editor s’il est ouvert.
2. Régénérer les fichiers du projet Visual Studio si les nouveaux fichiers n’apparaissent pas.
3. Compiler la cible `GrimrockPrototypeEditor` en configuration `Development Editor`.
4. Ouvrir le projet avec Unreal Engine 5.5.4.

Après compilation, la classe suivante doit être disponible dans le sélecteur de DataAsset :

```text
GridMonsterDefinitionAsset
```

---

## 3. Créer l’asset

Créer le dossier suivant s’il n’existe pas :

```text
Content/GrimrockPrototype/Monsters/RatGiant/Data
```

Dans le Content Browser :

1. clic droit ;
2. `Miscellaneous` ;
3. `Data Asset` ;
4. sélectionner `GridMonsterDefinitionAsset` ;
5. nommer l’asset :

```text
DA_MON_RatGiant
```

Chemin final recommandé :

```text
/Game/GrimrockPrototype/Monsters/RatGiant/Data/DA_MON_RatGiant
```

---

## 4. Valeurs de `DA_MON_RatGiant`

### Identity

| Propriété | Valeur |
|---|---|
| Monster Id | `MON_RatGiant` |
| Display Name | `Rat géant` |
| Description | `Vermine massive des profondeurs, rapide et agressive, utilisée comme premier ennemi de tutoriel.` |
| Category Id | `Vermin` |
| Danger Level | `1` |

### Visual

Les références visuelles peuvent rester vides pendant MON1.

| Propriété | Valeur MON1 |
|---|---|
| Icon | vide |
| Skeletal Mesh | vide |
| Animation Class | vide |
| Visual Scale | `(1, 1, 1)` |
| Visual Offset | `(0, 0, 0)` |

Elles seront renseignées pendant MON2.

### Stats

| Propriété | Valeur |
|---|---:|
| Max Health | 8 |
| Physical Armor | 0 |
| Magical Armor | 0 |
| Initiative | 12 |
| Accuracy | 2 |
| Evasion | 1 |
| Action Points Per Turn | 2 |

### Movement

| Propriété | Valeur |
|---|---|
| Grid Footprint | `(1, 1)` |
| Move Duration | `0.36` |
| Turn Duration | `0.12` |
| Blocks Movement | vrai |
| Can Open Doors | faux |
| Can Use Teleporters | faux |

### Perception

| Propriété | Valeur |
|---|---:|
| Sight Range Cells | 5 |
| Hearing Range Cells | 3 |
| Aggro Propagation Range | 3 |
| Shares Aggro With Group | vrai |

### AI

| Propriété | Valeur |
|---|---|
| Primary AI Profile | `DirectMelee` |
| Additional AI Profiles | `FastHarasser` |
| Preferred Min Distance | 1 |
| Preferred Max Distance | 1 |
| Retreat Chance | `0.40` |
| Low Health Threshold | `0.25` |

---

## 5. Attaque `Attack_Bite`

Ajouter une entrée dans `Attacks` :

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
| Attack Montage | vide pendant MON1 |
| Impact Notify Name | `Monster.AttackImpact` |
| Complete Notify Name | `Monster.ActionComplete` |
| Attack Sound | vide pendant MON1 |
| Impact VFX | vide pendant MON1 |

`2 à 5` représente provisoirement `1d4 + 1`.

Une attaque doit toujours coûter au moins un point d’action afin d’éviter une boucle d’actions gratuites.

---

## 6. Vulnérabilités

Ajouter deux entrées dans `Damage Modifiers`.

### Entrée 1 — Feu

| Propriété | Valeur |
|---|---|
| Damage Type | `Fire` |
| Physical Subtype | `None` |
| Damage Multiplier | `1.50` |

### Entrée 2 — Tranchant

| Propriété | Valeur |
|---|---|
| Damage Type | `Physical` |
| Physical Subtype | `Slashing` |
| Damage Multiplier | `1.25` |

Les dégâts physiques perforants ou contondants conservent un multiplicateur de `1.0`.

---

## 7. Récompenses et butin

### Experience Reward

```text
10
```

### Loot Table

Ajouter deux entrées :

| Item Definition Id | Chance | Min | Max |
|---|---:|---:|---:|
| `Item_RatMeat` | 0.30 | 1 | 1 |
| `Item_RatTooth` | 0.15 | 1 | 1 |

La probabilité restante, soit `0.55`, signifie qu’aucun objet n’est généré.

Les identifiants d’items pourront être reliés à leurs futurs `UGridItemDefinitionAsset` lorsque ces objets seront créés.

---

## 8. Vérifier la définition dans Blueprint

La classe expose les fonctions suivantes :

```text
Is Valid Definition
Validate Definition
Has AI Profile
Get Damage Multiplier
Get Attack Definition
```

Pour une vérification rapide :

1. appeler `Validate Definition` ;
2. vérifier que le résultat est vrai ;
3. vérifier que la chaîne d’erreur est vide.

---

## 9. Exécuter les Automation Tests

Dans Unreal Editor :

1. ouvrir `Tools > Test Automation` ou le panneau Automation du Session Frontend ;
2. rechercher :

```text
Grimrock.Monsters.MON1
```

3. exécuter :

```text
Grimrock.Monsters.MON1.DefinitionValidation
Grimrock.Monsters.MON1.InvalidData
```

Les tests vérifient notamment :

- l’identifiant de Primary Asset ;
- les profils `DirectMelee` et `FastHarasser` ;
- la résolution de `Attack_Bite` ;
- la vulnérabilité au feu ;
- la vulnérabilité au tranchant ;
- l’absence de modificateur perforant ;
- le rejet des attaques dupliquées ;
- le rejet d’une attaque gratuite ;
- le rejet d’une table de butin dépassant 100 % ;
- le rejet d’une définition vide.

---

## 10. Critères de validation MON1

MON1 est validé lorsque :

- le projet compile sous UE 5.5.4 ;
- `GridMonsterDefinitionAsset` apparaît dans le sélecteur de DataAsset ;
- `DA_MON_RatGiant` est créé au chemin recommandé ;
- toutes les valeurs ci-dessus sont renseignées ;
- `Validate Definition` retourne vrai ;
- les deux tests `Grimrock.Monsters.MON1` réussissent.

Une fois ces points confirmés, le projet peut passer à **MON2 — Actor animé du Rat géant**.
