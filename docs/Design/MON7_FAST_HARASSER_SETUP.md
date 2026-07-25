# MON7 — FastHarasser, repli tactique et agression de groupe

## Périmètre

MON7 étend le combat MON6 sans modifier la politique `DirectMelee`. Le Rat géant peut désormais :

- mordre normalement au contact ;
- tenter un repli après la morsure s’il lui reste au moins un PA ;
- sélectionner une destination sûre avec un score déterministe ;
- passer par l’état `Repositioning` pendant la rotation et le déplacement ;
- alerter les Rats vivants et actifs de son groupe de rencontre.

Le raisonnement reste déclenché au début du combat ou du tour. Aucun Tick d’intelligence artificielle et aucun `UAIPerceptionComponent` ne sont ajoutés.

## DirectMelee et FastHarasser

`DirectMelee` conserve exactement la planification MON6 :

- adjacent : rotation éventuelle puis `MeleeAttack` ;
- à deux cases avec deux PA : `Move`, puis `MeleeAttack` ;
- à trois cases avec deux PA : deux `Move` ;
- aucun repli.

`FastHarasser` est un profil additionnel. La planification commence par le résultat `DirectMelee`, puis ajoute éventuellement après la morsure :

```text
Turn — 0 PA — bIsRepositioningAction = true
Move — 1 PA — bIsRepositioningAction = true
```

Le déplacement ne commence qu’après l’impact et la fin complète de l’action de morsure. Le TurnManager continue donc à exécuter une seule action animée à la fois.

## Choix de la case de repli

Le runtime construit les candidats dans l’ordre stable suivant :

```text
North
East
South
West
```

Une destination est rejetée lorsqu’elle est hors grille, non marchable, séparée par un mur ou une porte fermée, occupée, réservée, non réservable, ou égale à la cellule du groupe. Les contrôles utilisent `AGridLevelRuntimeActor::CanMove` et `UGridMonsterOccupancySubsystem` ; aucun second registre d’occupation n’est créé.

Pour chaque destination valide :

```text
DistanceDelta =
    DistanceCandidateToParty - DistanceCurrentToParty

Score =
    DistanceDelta * 100
    + ExitCount * 10
    + (bCanContinuePursuit ? 5 : 0)
    - (bIsCulDeSac ? 60 : 0)
```

`ExitCount` compte les sorties praticables depuis la candidate sans compter la cellule du groupe. Une sortie rapprochant le Rat du groupe indique qu’il pourra reprendre la poursuite. Une case ayant au plus une sortie pertinente est un cul-de-sac.

La distance domine le score, puis le nombre de sorties. Une égalité est résolue strictement par `North`, `East`, `South`, `West`.

## RetreatChance

Le repli n’est évalué que si :

1. le DataAsset possède `FastHarasser` via `HasAIProfile()` ;
2. une `MeleeAttack` est planifiée ;
3. au moins un PA reste après la morsure ;
4. une destination valide existe.

Le jet utilise exclusivement le `FRandomStream` du combat :

```cpp
Roll = CombatRandomStream.FRand();
bShouldRetreat =
    Roll < FMath::Clamp(RetreatChance, 0.0f, 1.0f);
```

Ainsi, `0.0` interdit toujours le repli et `1.0` l’autorise toujours lorsqu’une case existe. Aucun jet n’est consommé pour un profil non concerné, un manque de PA ou une absence de case.

## État Repositioning

La rotation et le déplacement marqués placent le Rat dans `Repositioning`. Après un déplacement réussi, il revient à `Pursuing`.

Si une rotation ou un déplacement de repli échoue :

- toute action de mouvement et réservation éventuelle est annulée ;
- la file de repli est vidée ;
- le tour se termine sans blocage ;
- le Rat vivant revient à `Pursuing`.

L’état `Dead` n’est jamais remplacé.

## EncounterGroupId et propagation

`EncounterGroupId` est stocké à la fin de `FGridLevelObjectData` et vaut `None` pour les anciens niveaux. Il est aussi éditable sur chaque instance de `AGridMonsterActor`, ce qui couvre les `BP_MON_RatGiant` directement placés.

Lorsqu’un Actor possède le `SpawnObjectId` d’un placement `MonsterSpawn`, le runtime lui applique le `EncounterGroupId` du `LevelAsset`. L’API `InitializeMonster` accepte également ce groupe.

Une propagation exige :

- `bSharesAggroWithGroup = true` sur la définition de la source ;
- un `EncounterGroupId` non vide ;
- le même `MonsterId` et le même `EncounterGroupId` ;
- une distance de Manhattan inférieure ou égale à `AggroPropagationRange` ;
- un Rat vivant et activé.

MON7 effectue volontairement une seule vague, non transitive. Les cibles sont dédupliquées et triées par `SpawnObjectId`. Un Rat alerté rejoint les participants avec l’état `Alert` ; le comportement existant pourra le passer à `Pursuing` au début de son tour.

## Réglages manuels UE5

Ne pas modifier automatiquement les assets. Dans `DA_MON_RatGiant`, régler :

```text
Primary AI Profile        = DirectMelee
Additional AI Profiles   contient FastHarasser
Retreat Chance            = 0.40
Aggro Propagation Range   = 5, ou une valeur adaptée au niveau
Shares Aggro With Group   = true
```

Pour chaque `BP_MON_RatGiant` directement placé, renseigner par exemple :

```text
Encounter Group Id = RatRoom_A
Monster Enabled     = true
```

Utiliser `RatRoom_B` pour un Rat témoin qui ne doit pas rejoindre le groupe A.

## Logs

Décision avec une case valide :

```text
[GridFastHarasser] Decision Monster=... Chance=0.40 Roll=...
    CandidateCount=... Retreat=true Cell=(X,Y) Score=...
```

Refus avant le jet :

```text
[GridFastHarasser] NoRetreat Monster=... Reason=NoActionPoints
[GridFastHarasser] NoRetreat Monster=... Reason=NoValidCell
```

Propagation :

```text
[GridMonsterAggro] Source=... Group=RatRoom_A Propagated=... Distance=...
```

Ces messages sont produits pendant la préparation du combat ou du tour, jamais à chaque Tick.

## Tests Automation

Dans `Tools > Session Frontend > Automation`, lancer :

```text
Grimrock.Monsters.MON7.RetreatChance
Grimrock.Monsters.MON7.RetreatScoring
Grimrock.Monsters.MON7.ActionPlanning
Grimrock.Monsters.MON7.GroupAggro
```

Puis relancer les régressions :

```text
Grimrock.Monsters.MON4
Grimrock.Monsters.MON5
Grimrock.Monsters.MON6
```

Les tests MON7 couvrent les chances 0 et 1, la graine déterministe, la formule de score, les culs-de-sac, l’ordre cardinal, la marque d’action, la conservation de `DirectMelee` et le filtrage de groupe.

## Checklist PIE MON7

1. Fermer Unreal Editor et compiler `GrimrockPrototypeEditor` en Development Win64.
2. Rouvrir le projet et vérifier que les Blueprints natifs se chargent sans erreur.
3. Régler `DA_MON_RatGiant` avec `DirectMelee` et le profil additionnel `FastHarasser`.
4. Régler `Retreat Chance = 0.40`.
5. Régler `Aggro Propagation Range = 5` et activer `Shares Aggro With Group`.
6. Placer deux ou trois `BP_MON_RatGiant` avec `Encounter Group Id = RatRoom_A`.
7. Placer un Rat témoin avec `Encounter Group Id = RatRoom_B`.
8. Vérifier que les Rats A sont dans la portée de Manhattan et que le Rat B peut rester hors groupe.
9. Démarrer le combat par perception, sans utiliser le démarrage forcé avec tous les monstres.
10. Vérifier les logs `[GridMonsterAggro]`.
11. Vérifier que seuls les Rats A vivants, activés et dans la portée rejoignent `CombatMonsters`.
12. Vérifier que les Rats propagés commencent dans l’état `Alert`.
13. Placer un FastHarasser adjacent au groupe, avec deux PA et au moins deux cases praticables autour de lui.
14. Terminer la phase joueur et vérifier `MeleeAttack` avant toute action de repli.
15. Vérifier l’impact unique de la morsure, puis l’état `Repositioning`, la rotation éventuelle et le `Move`.
16. Régler temporairement `Retreat Chance = 1.0` pour imposer un repli, puis `0.0` pour confirmer son absence.
17. Bloquer les destinations avec un mur, une porte fermée, un autre Rat ou une réservation et vérifier `Reason=NoValidCell` sans tour bloqué.
18. Restaurer `Retreat Chance = 0.40`, relancer plusieurs combats et confirmer que `DirectMelee`, les dégâts MON6, la défaite et les raccourcis NumPad 1 à 6 restent fonctionnels.
