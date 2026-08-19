# MON17.3.1 — Gobelin lanceur — Exécution d'attaque à distance

Statut : **VALIDÉ / CLOS**

Base initiale : `9c391965ce3f25129a640098cbb5475398d2f177` (`MON 17.2; add basic GoblinThrower assets.`)

Implémentation : `467f2eb2c4efc2480ef65c48fb8decd0fcb412ba` (`Start MON17.3 ranged monster attack execution`)

Correctif Unity Build : `f97583470b077f12193cd00c88a76f0b1d1c2446` (`Fix MON17.3 unity build helper collision`)

## Objectif

Rendre une attaque monstre `RangedAttack` réellement exécutable lorsque le monstre est **déjà sur une case de tir valide**.

Cas de production visé :

```text
MON_GoblinThrower
Attack_ThrowKnife
Delivery = Projectile
MinRangeCells = 2
RangeCells = 6
bRequiresLineOfSight = true
ActionPointCost = 2
```

MON17.3.1 ne cherche aucune meilleure case de tir. Le maintien de distance, le recul et le kiting restent strictement MON17.4.

## Réutilisation de l'existant

Aucun second système de combat n'est introduit.

Le jalon réutilise :

- `EGridCombatActionType::RangedAttack`, déjà présent ;
- `FGridMonsterAttackDefinition` et son contrat portée / Delivery / LOS / PA ;
- `UGridMonsterCombatComponent::GetPreferredAttackForRange()` ;
- `UGridMonsterCombatComponent::ResolveAndApplyPartyAttack()` ;
- `UGridMonsterCombatComponent::StartAttackPresentation()` ;
- le séquençage impact / fin / timeout du TurnManager ;
- `FGridMonsterPerception::HasStraightLineOfSight()` pour la LOS de grille ;
- le budget PA existant.

## Planner stationnaire

Nouveau helper pur :

```text
FGridMonsterRangedAttackPlanner
```

Responsabilités :

1. vérifier que la cible est sur le même axe cardinal ;
2. vérifier `MinRangeCells <= distance <= RangeCells` ;
3. vérifier le budget PA ;
4. respecter le résultat de LOS lorsque l'attaque l'exige ;
5. ajouter zéro, une ou deux rotations gratuites pour faire face au groupe ;
6. ajouter exactement une action `RangedAttack` payante.

Il ne génère **jamais** d'action `Move`.

## Intégration TurnManager

Le cycle d'attaque monstre historique est généralisé pour accepter :

```text
MeleeAttack
RangedAttack
```

Le nom privé historique `StartActiveMeleeAttack()` est conservé temporairement pour éviter un renommage sans valeur fonctionnelle ; son implémentation exécute maintenant les deux types.

Avant une attaque à distance, le runtime revalide :

- l'AttackId et sa définition ;
- la cellule courante du groupe ;
- la distance réelle ;
- le caractère ranged de l'attaque ;
- l'alignement axial ;
- le Facing du monstre ;
- la LOS au travers des arêtes de grille si `bRequiresLineOfSight=true`.

La résolution Hit/Miss/dégâts reste celle déjà utilisée par les attaques monstres de contact.

## RangedKeeper pendant MON17.3.1

Si un `RangedKeeper` ne peut pas tirer depuis sa case actuelle, il produit volontairement `Wait`.

Exemples :

```text
Distance 3 + axe + LOS + 3 PA
    -> Turn éventuel
    -> RangedAttack coût 2

Distance 1
    -> Wait

Distance valide mais mur/porte fermée dans la LOS
    -> Wait
```

Il ne poursuit pas le groupe jusqu'au contact. La recherche d'une case à distance favorable sera introduite uniquement en MON17.4.

## Validation UE5.5.4

Compilation `Development_Editor x64` : **validée** après correction de la collision de helper révélée par le Unity Build d'Unreal.

Filtre exécuté :

```text
Grimrock.Monsters.MON17.3.1
```

Résultat local fourni le 19 août 2026 : **3/3 Success**.

```text
LineOfSight              Success
MeleeRegressionPlanner   Success
StationaryRangedPlanner  Success
```

Couverture validée :

- distance 2 acceptée ;
- distance 3 avec rotation puis tir ;
- distance 1 rejetée ;
- distance 7 rejetée ;
- cible diagonale rejetée ;
- AP=1 pour coût 2 rejeté ;
- LOS requise mais absente rejetée ;
- couloir axial ouvert accepté ;
- arête bloquée casse la LOS ;
- planner Rat Géant conserve `MeleeAttack` / `Attack_Bite`.

## Hors périmètre

- mesh/Actor de projectile : MON17.3.2 ;
- synchronisation visuelle du couteau : MON17.3.2/17.3.3 ;
- montage de lancer final : MON17.3.3 ;
- cooldown runtime monstre : MON17.3.4 ;
- choix de case, recul, maintien de distance, kiting : MON17.4.
