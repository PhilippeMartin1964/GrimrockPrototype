# MON17.1 — Gobelin Archer — Definition / Assets / Spawn Contract

Statut : **implémenté, validation UE5.5.4 requise**  
Référence de départ : `af87daf6cf40684ada8775fdff585d173400939e`

## 1. Objectif

MON17.1 ouvre la seconde famille de monstres avec le **Gobelin Archer** et doit démontrer que le pipeline MON13–MON16 est data-driven. Ce sous-jalon ne livre ni l'AnimBP final, ni l'exécution d'une attaque à distance, ni le planner complet `RangedKeeper`.

Le contrat cible est :

```text
GridMonsterDefinitionAsset
        ↓
GridObjectPalette / MonsterSpawn
        ↓
MonsterActorClass
        ↓
FGridRuntimeMonsterState / MonsterPlacementState
```

Aucune branche `MonsterId == GoblinArcher` n'est autorisée.

## 2. Audit du code existant

### Déjà générique et réutilisé

- `UGridMonsterDefinitionAsset` possède une identité `MonsterId`, un `MonsterActorClass`, les statistiques, PA, perception, récompenses, loot, attaques et profils IA.
- `EGridMonsterAIProfile` contient déjà `RangedKeeper`.
- `PreferredMinDistance` / `PreferredMaxDistance` existent déjà et décrivent la bande de distance tactique d'un monstre sans dépendre de son identité.
- `FGridLevelObjectData` représente tout monstre par `MonsterDefinitionAsset` + `MonsterDefinitionId`; `ObjectId` reste le SpawnId stable.
- `FGridObjectPaletteEntry::DefaultMonsterDefinition` permet à une entrée de palette `MonsterSpawn` de choisir n'importe quelle définition.
- `UGridMonsterDefinitionAsset::MonsterActorClass` choisit la classe runtime, avec `AGridMonsterActor` comme défaut natif.
- MON13 persiste `MonsterDefinitionId`, cellule, facing, état, HP/armures, encounter, perception et présence dans `FGridRuntimeMonsterState` / `FGridRuntimeMonsterPlacementState`.
- MON14 perception, dormance, patrouille, investigation et alarmes travaillent sur `AGridMonsterActor` / sa définition, pas sur l'identité Rat Géant.
- MON15 XP et loot sont déjà portés par `UGridMonsterDefinitionAsset`.
- MON16 stocke les Status Effects dans l'état runtime monstre sans introduire de type Rat spécifique.

### Limites constatées avant MON17.1

`FGridMonsterAttackDefinition` possédait : dégâts, précision, portée maximale (`RangeCells`), coût PA, animation, audio et VFX. Il ne décrivait pas explicitement :

- la portée minimale ;
- le mode contact / projectile / instantané ;
- l'exigence de ligne de vue ;
- un cooldown générique ;
- une priorité d'attaque ;
- les paramètres minimaux d'un projectile visuel.

Le système joueur possède `AGridThrownItemActor`, mais celui-ci est lié au pipeline d'objets/inventaire lancé par le groupe. MON17.1 ne le transforme donc pas artificiellement en projectile monstre.

Autre dépendance historique identifiée : `UGridMonsterCombatComponent::GetPreferredMeleeAttack()` privilégiait explicitement `Attack_Bite`. Ce choix était Rat-spécifique.

## 3. Contrat ajouté par MON17.1

`FGridMonsterAttackDefinition` reçoit le minimum générique suivant :

```text
MinRangeCells
RangeCells                  // maximum, nom conservé pour compatibilité
Delivery                    // Contact / Projectile / Instant
bRequiresLineOfSight
ActionPointCost             // déjà existant
CooldownTurns
Priority
ProjectileVisualMesh
ProjectileVisualScale
ProjectileRotationOffset
ProjectileTravelDuration
```

Helpers :

```text
SupportsDistance(DistanceCells)
IsRangedAttack()
```

Validation :

- `MinRangeCells >= 1` ;
- `RangeCells >= MinRangeCells` ;
- `CooldownTurns >= 0` ;
- paramètres visuels projectile finis et positifs.

`GetPreferredMeleeAttack()` n'utilise plus `Attack_Bite`. Il délègue à `GetPreferredAttackForRange(1)` qui sélectionne la plus forte `Priority` parmi les attaques valides à cette distance. En cas d'égalité, l'ordre authored est conservé.

## 4. Contrat du Gobelin Archer pour les assets UE

La DataAsset à créer dans l'éditeur après validation C++ devra partir du profil suivant :

```text
MonsterId             = MON_GoblinArcher
DisplayName           = Gobelin Archer
PrimaryAIProfile      = RangedKeeper
PreferredMinDistance  = 3
PreferredMaxDistance  = 5
MonsterActorClass     = AGridMonsterActor ou BP dérivé générique
```

Première attaque contractuelle :

```text
AttackId               = Attack_Shortbow
Delivery               = Projectile
MinRangeCells           = 2
RangeCells              = 6
bRequiresLineOfSight    = true
ActionPointCost         = 2
CooldownTurns           = 0
Priority                = 100
```

Ces valeurs sont un **fixture de contrat**, pas l'équilibrage MON17.7.

## 5. Ce qui reste volontairement hors MON17.1

- mesh/skeleton/AnimBP définitifs : MON17.2 ;
- résolution/exécution de l'attaque projectile, trajectoire visuelle et impact : MON17.3 ;
- consommation effective de `CooldownTurns` : MON17.3 ;
- planner `RangedKeeper`, orientation et repositionnement : MON17.4 ;
- intégration PIE complète patrol/perception/alarm : MON17.5 ;
- encounter/loot/XP de production : MON17.6 ;
- équilibrage : MON17.7.

La donnée de cooldown est définie maintenant afin que l'asset d'attaque soit stable ; son état runtime ne doit pas être ajouté avant que MON17.3 en ait besoin.

## 6. Tests ajoutés

Filtre :

```text
Grimrock.Monsters.MON17.1
```

Cas couverts :

- contrat attaque à distance générique et validations de portée/cooldown ;
- définition Gobelin Archer avec identité `GridMonster:MON_GoblinArcher` et profil `RangedKeeper` ;
- palette `MonsterSpawn` avec seconde définition ;
- synchronisation du `MonsterDefinitionId` ;
- représentation de l'identité Gobelin Archer dans `FGridRuntimeMonsterState` et `FGridRuntimeMonsterPlacementState`.

La campagne de non-régression MON13–MON16 reste à exécuter sous UE5.5.4 après compilation.

## 7. Intervention UE5 après compilation

Aucun `.uasset` / `.umap` n'est modifié par MON17.1.

Après compilation et tests C++ :

1. créer `DA_MON_GoblinArcher` comme `GridMonsterDefinitionAsset` ;
2. renseigner l'identité, `RangedKeeper`, la bande 3–5 et `Attack_Shortbow` selon le contrat ci-dessus ;
3. pour MON17.1 seulement, laisser les assets finaux de présentation non définis ou utiliser temporairement des placeholders contrôlés ;
4. créer/dupliquer l'archetype de `MonsterSpawn` seulement si nécessaire pour l'affichage de palette ;
5. ajouter une entrée de palette `MON_GoblinArcher` avec `DefaultMonsterDefinition = DA_MON_GoblinArcher` ;
6. placer un Gobelin Archer dans une map de test et vérifier que le spawn produit bien un `AGridMonsterActor` portant la bonne définition et que Save/Continue conserve son `MonsterDefinitionId`.

Cette validation manuelle ferme le volet asset du contrat avant MON17.2.