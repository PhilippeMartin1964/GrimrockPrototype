# RPG03.6 — Persistent Surfaces & Elemental Reactions (C6)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : **RPG03.5 validé 8/8**  
Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

RPG03.6 introduit l'autorité Surface demandée par RPG02.

Une surface est un **état de cellule du niveau/runtime**, jamais un Status Effect porté par un personnage ou un monstre.

Les types canoniques sont :

```text
Fire
Water
Ice
Poison
Oil
ElectrifiedWater
Smoke
PoisonCloud
```

## Autorité et persistance

Chaque `FGridLevelRuntimeState` possède désormais :

```text
TMap<FIntPoint, FGridCombatSurfaceState> Surfaces
```

avec `SaveGame`.

Une surface conserve :

- son type ;
- sa durée restante en rounds ;
- le `SourceCombatantId` ;
- le `SourceActionId` ;
- son DamageType périodique ;
- sa magnitude périodique ;
- ses éventuelles applications C1 après tick.

Ainsi les surfaces survivent aux rebuilds runtime et aux round-trips SaveGame comme les autres états du niveau.

## Authoring

`FGridCombatSurfaceEffectProfile` décrit :

- `SurfaceType` ;
- `DurationRounds`, 1..6 ;
- `PeriodicDamageType` ;
- `PeriodicDamagePerRound` ;
- `PeriodicStatusApplications`.

Aucune magnitude universelle n'est inventée.

RPG02 chiffre explicitement :

```text
Surface_PoisonCloud
3 rounds
2 Poison / round
```

mais ne donne pas une valeur générique pour Fire, Poison, Ice, etc. Ces valeurs restent donc authorables.

## Surface persistante du Mage

C2 gagne quatre champs génériques de surface :

```text
SurfaceDurationRoundsModifier
SurfacePeriodicDamagePercentModifier
SurfaceReactionDamagePercentModifier
SurfaceReactionAreaRadiusModifier
```

Les deux premiers permettent le Talent Surface persistante :

```text
durée créée par le Mage +2 rounds
cap = 6
dégâts périodiques +15 %
```

Le bonus est incorporé lors de la création de la surface. Une surface déjà créée conserve donc son snapshot et ne dépend pas ensuite de la présence du Talent chez l'auteur.

## Réactions canoniques

`FGridCombatSurfaceResolver` implémente uniquement la table demandée par RPG02 :

| Surface | Interaction | Résultat |
|---|---|---|
| Oil | Fire | Fire |
| Poison | Fire | Fire + explosive |
| Water | Ice | Ice |
| Water | Lightning | ElectrifiedWater |
| Ice | Fire | Water |
| PoisonCloud | Fire | Fire + explosive |
| Smoke | Wind | suppression |

Une réaction transforme l'état de la même cellule et conserve sa durée restante.

Elle n'invente pas une nouvelle magnitude périodique pour la surface de sortie. Si l'action veut authorer un Fire périodique particulier, son profil de surface doit le fournir.

## Réactions explosives

Le résultat de réaction expose :

```text
bExplosive
ExplosionDamageType
ExplosionDamagePercentModifier
ExplosionAreaRadiusModifier
```

Cela permet le contrat Réaction en chaîne :

```text
+25 % dégâts de réaction
AreaRadius +1
plafond final géré par l'exécuteur
non récursif via C4
```

RPG02 ne chiffre pas le **dommage de base** de l'explosion Poison/PoisonCloud + Fire. RPG03.6 ne fabrique donc aucune constante arbitraire.

Le futur authoring/exécuteur fournit la magnitude de base ; C4 contrôle `OncePerAction` et l'anti-récursion.

## Tick périodique

À chaque nouvelle frontière de round, avant le démarrage des activations du nouveau round :

1. snapshot des surfaces du niveau actif ;
2. dégâts périodiques sur les occupants ;
3. résistances et modificateurs C2 entrants ;
4. armure normale puis HP via `FGridCombatResolver::ResolveDirectDamage()` ;
5. application éventuelle du payload C1 ;
6. décrément de la durée ;
7. suppression des surfaces arrivées à zéro.

Le groupe partage une cellule : tous les personnages actifs vivants présents dans l'ancre de groupe subissent la surface.

Chaque monstre subit uniquement la surface de sa propre cellule.

## PoisonCloud et ArmorGate

Le contrat Corrosive Cloud est représentable sans second système :

```text
PeriodicDamageType      = Poison
PeriodicDamagePerRound  = 2
PeriodicStatus          = Status_Poison
Trigger                 = AfterResolution
ArmorGate               = MagicalArmorDepleted
```

Le même `FGridAttackResult` de dégâts directs est transmis à C1.

Exemple :

```text
MagicalArmor avant = 1
Poison surface      = 2

MagicalArmorDamage  = 1
HealthDamage        = 1
MagicalArmor après  = 0
→ ArmorGate ouvert
→ Status_Poison éligible
```

## API runtime

`AGridLevelRuntimeActor` fournit :

```text
ApplyCombatSurfaceAtCell()
InteractCombatSurfaceAtCell()
FindCombatSurfaceAtCell()
GetCurrentCombatSurfaceSnapshot()
AdvanceCombatSurfaceRound()
```

Le RuntimeActor reste l'autorité du niveau. Aucun WorldSubsystem Surface concurrent n'est créé.

## Interaction avec C4 et C8

C4 a déjà le trigger `SurfaceReaction`, le ledger `OncePerAction` et l'anti-récursion.

Les actions qui ciblent une cellule/zone — Bombes, Conversion élémentaire, Architecte du terrain, Catalyseur — restent volontairement non exécutées tant que C8 n'a pas fourni le ciblage/batch Effect générique.

RPG03.6 fournit donc l'autorité Surface et la résolution de réaction ; C8 branchera ces primitives sur les actions Cell/Area sans créer un exécuteur temporaire.

## Couverture RPG02

RPG03.6 fournit les primitives nécessaires pour :

- Fire Bomb → Surface_Fire ;
- Toxic Bomb → Surface_Poison ;
- Corrosive Cloud → Surface_PoisonCloud, 2 Poison/round ;
- Elemental Conversion ;
- Persistent Surface ;
- Terrain Architect ;
- Catalyst ;
- Chain Reaction ;
- Conduction via la lecture du type de surface de la cellule cible et les modificateurs C2.

## Automation ajoutée

Filtre :

```text
Grimrock.RPG.RPG03.6
```

Tests :

1. `ProfileValidation`
2. `StateModifierProjection`
3. `CanonicalReactionTable`
4. `ChainReactionModifiers`
5. `RuntimePersistence`
6. `DurationAdvance`
7. `RuntimeReactionMutation`
8. `PeriodicArmorGate`

La validation n'est considérée réussie qu'après exécution du harness UE5.5.4 et fourniture de la sortie utilisateur.
