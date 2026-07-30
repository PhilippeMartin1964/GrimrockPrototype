# MON11.3 — Armes et équipement offensif

## Périmètre

MON11.3 remplace le profil d’attaque provisoire de MON11.2 par un profil
offensif défini sur les DataAssets d’items. La commande reste synchrone,
atomique et résolue par `FGridCombatResolver`.

Le jalon ne crée ni double attaque, munition, projectile, trajectoire physique,
coût de ressource, cooldown, animation, son, VFX ou widget de dégâts.

## Modèle de données

`UGridItemDefinitionAsset` expose :

- `bProvidesAttack`, qui annonce qu’une définition fournit une attaque ;
- `OffensiveProfile`, éditable lorsque cette annonce est active ;
- `HasValidOffensiveProfile()` ;
- `CanProvideAttackFromSlot()`.

Une définition sans profil offensif reste valide. Si `bProvidesAttack` est
vrai, le profil doit être valide et la définition doit déclarer au moins
`MainHand` ou `OffHand` dans `CompatibleEquipmentSlots`.

Le type d’item n’est pas une contrainte. Une arme, une torche, un bouclier, un
focaliseur ou un objet magique peut donc fournir une attaque si sa définition
et son emplacement sont compatibles.

## Profil offensif

`FGridOffensiveEquipmentProfile` contient :

| Champ | Rôle |
| --- | --- |
| `AttackId` | identité stable de l’attaque |
| `AttackDefinition` | type, sous-type, dégâts minimum et maximum, précision |
| `FlatDamageBonus` | bonus fixe ajouté aux dégâts |
| `DamageScalingAttribute` | caractéristique finale utilisée |
| `RangeCells` | portée axiale de 1 à 32 cellules |

Un profil valide exige un `AttackId`, une définition d’attaque valide, un
`MaxDamage` strictement positif et une portée comprise entre 1 et 32. Une
attaque non physique doit conserver `PhysicalSubtype=None`.

## Scaling

`EGridAttackScalingAttribute` accepte `None`, `Strength`, `Dexterity`,
`Constitution`, `Intelligence`, `Wisdom` et `Charisma`.

Le bonus de dégâts source est :

```text
FlatDamageBonus
+ GetAttributeModifier(caractéristique finale sélectionnée)
```

Avec `None`, aucun modificateur de caractéristique n’est ajouté. Le calcul
emploie exclusivement
`URPGCharacterRulesLibrary::GetAttributeModifier()`.

## Sélection MainHand, OffHand, Unarmed

Le TurnManager examine uniquement les deux mains, dans cet ordre :

1. une attaque offensive valide en `MainHand` gagne ;
2. une main principale valide mais non offensive laisse examiner `OffHand` ;
3. une attaque offensive valide en `OffHand` est alors utilisée ;
4. sans profil offensif dans les mains, `Attack_Unarmed` sert de repli.

Deux attaques équipées ne sont ni combinées ni résolues deux fois. Un profil
placé dans `Head`, `Ring`, `Belt` ou tout autre emplacement est ignoré.

Le repli conserve `Physical/Bludgeoning`, 1–3 dégâts, aucun bonus plat,
scaling `Strength` et portée 1.

## Portée et ciblage

La recherche part de la cellule du groupe et avance cellule par cellule dans
`PartyFacing`. Chaque étape utilise `TryGetNeighborCell()` puis `CanMove()`.
Un mur ou une porte fermée interrompt la ligne avec `PassageBlocked`.

Le premier monstre rencontré arrête la recherche. Une attaque ne traverse
jamais ce monstre pour viser le suivant. Sa cellule exacte est conservée dans
la requête.

- première cible dans `RangeCells` : validations MON11.1 puis résolution ;
- première cible dans la cellule immédiatement au-delà : `TargetOutOfRange` ;
- aucune cible sur la ligne examinée : `NoMonsterInFront` ;
- première cellule indisponible : `TargetCellUnavailable`.

`Attack_Unarmed` reste limité à la cellule adjacente.

## Refus liés à l’équipement

`EquippedItemDefinitionUnavailable` indique qu’une main contient une instance
valide dont la définition n’est pas disponible dans le registre runtime.

`InvalidOffensiveEquipment` indique qu’une définition annonce une attaque mais
que son profil, sa portée ou sa compatibilité avec la main concernée est
invalide.

Ces refus surviennent avant tout tirage, consommation d’action, journal
`AttackHit`/`AttackMiss`, delegate `Requested`/`Resolved` ou mutation du
monstre.

## Statistiques source et cible

La source reçoit la précision depuis
`CharacterSummary.DerivedStats.Accuracy`. Les caractéristiques du résumé sont
les valeurs finales après bonus généraux d’équipement.

La cible conserve le mapping MON11.2 : évasion de la définition, PV et armures
de l’Actor, résistance provisoire à zéro. Le multiplicateur vient du type réel
de l’attaque :

```cpp
MonsterDefinition->GetDamageMultiplier(
    AttackDefinition.DamageType,
    AttackDefinition.PhysicalSubtype)
```

Les formules de hit, critique, dégâts, multiplicateurs et absorption restent
dans `FGridCombatResolver`, qui n’est pas modifié.

## Requête et journal

`FGridPlayerAttackRequest` et `FGridCombatLogEntry` conservent :

- l’`AttackId` réellement utilisé ;
- `OffensiveItemDefinitionId` ;
- `OffensiveEquipmentSlot`.

Pour `Attack_Unarmed`, l’item et le slot valent `None`. Pour une attaque
équipée, l’identifiant réel de définition et la main réelle sont conservés.
NumPad 7 affiche aussi la portée, la cellule cible, le type et le sous-type,
les jets, la défense, le résultat, les dégâts bruts et appliqués, les PV et
`TargetDefeated`.

## Shuriken

`DA_Weapon_Shuriken` est configuré avec :

| Donnée | Valeur |
| --- | --- |
| `ItemType` | `Weapon` |
| `bProvidesAttack` | `true` |
| `AttackId` | `Attack_Shuriken` |
| dégâts | `Physical/Piercing`, 1–4 |
| bonus de précision | 0 |
| bonus plat | 0 |
| scaling | `Dexterity` |
| portée | 3 |
| compatibilité requise | `MainHand` |

Ses autres propriétés, références, meshes, icônes, tags et paramètres de
lancer sont préservés. L’item n’est ni consommé ni retiré après l’attaque.

## Persistance

`FGridItemInstance` n’est pas étendu par le profil offensif. La sauvegarde
conserve `ItemDefinitionId`, puis
`RehydrateOwnedItemDefinitions()` reconstruit le registre runtime. Le profil
est retrouvé sur le DataAsset par cet identifiant.

## Tests automatisés

Le filtre `Grimrock.Monsters.MON11` conserve les huit scénarios MON11.1 et
MON11.2 et ajoute :

- `OffensiveProfileValidation` ;
- `EquippedWeaponMapping` ;
- `HandPriorityAndFallback` ;
- `RangedWeaponTargeting` ;
- `ElementalOffensiveEquipment`.

Les nouveaux tests utilisent uniquement des définitions, personnages, grilles
et monstres transitoires. Ils ne chargent aucun asset `Content/`.

## Procédure PIE

1. Placer ou récupérer le shuriken existant sans modifier la carte.
2. L’équiper en `MainHand`.
3. Placer le Rat géant à deux ou trois cellules dans l’axe du groupe.
4. Démarrer le combat et attendre `PlayerPhase`.
5. Appuyer sur NumPad 7.
6. Vérifier `Attack_Shuriken`, l’identifiant d’item, `MainHand`, portée 3,
   `Dexterity` et `Physical/Piercing` 1–4.
7. Vérifier l’absorption par l’armure physique puis les PV.
8. Interposer un mur ou une porte fermée et vérifier `PassageBlocked` sans
   tirage.
9. Retirer le shuriken, se placer adjacent et vérifier `Attack_Unarmed`.
10. Réessayer avec le même personnage et vérifier
    `AttackerAlreadyActed`.

## Limites différées

MON11.4 fournit désormais la présentation de l’objet tenu, l’audio et les VFX
orientés données, les impacts distincts hit/miss, le feedback utilisateur des
résultats et le feedback des refus.

Restent différés le dual wield, les armes à deux mains, le choix manuel
d’attaque, les munitions, la durabilité, les compétences et prérequis, les
ressources, cooldowns, zones et projectiles.
