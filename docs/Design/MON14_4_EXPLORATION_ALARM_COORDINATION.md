# MON14.4 — Exploration Alarm & Reinforcement Coordination

## Statut

Implémentation C++ prête pour compilation et validation UE 5.5.4.

MON14.4 complète MON14.1–MON14.3 en permettant à un monstre qui perçoit le
groupe d'alerter localement des alliés déjà présents dans le niveau. Le système
ne crée aucun nouveau modèle de groupe : il réutilise exactement le contrat
MON7 déjà utilisé pour la propagation d'aggro au démarrage du combat.

## Objectifs

- un garde qui entend le groupe peut alerter des alliés proches ;
- un allié dormant peut être réveillé par cette alarme ;
- l'allié reçoit la dernière cellule connue du groupe et passe en investigation ;
- l'alarme respecte `EncounterGroupId`, `MonsterId` et `AggroPropagationRange` ;
- `bSharesAggroWithGroup=false` désactive toute propagation depuis la source ;
- l'alarme seule ne peut jamais démarrer le combat ;
- la vision réelle reste l'autorité d'engagement automatique MON14.1 ;
- aucun Tick IA permanent n'est ajouté.

## Réutilisation du contrat MON7

Les paramètres existent déjà dans `UGridMonsterDefinitionAsset` :

```cpp
bool bSharesAggroWithGroup;
int32 AggroPropagationRange;
```

et sur chaque Actor runtime :

```cpp
FName EncounterGroupId;
```

La sélection des alliés réutilise :

```cpp
FGridFastHarasserPlanner::SelectAggroTargets(...)
```

Les règles restent donc identiques à MON7 :

1. source avec identité persistante valide ;
2. `bSharesAggroWithGroup=true` ;
3. `EncounterGroupId` non vide ;
4. même `MonsterId` ;
5. même `EncounterGroupId` ;
6. cible vivante et activée ;
7. distance Manhattan <= `AggroPropagationRange` ;
8. ordre déterministe par `SpawnObjectId`.

MON14.4 ne crée volontairement pas de `AlarmGroupId` séparé. Si le projet a
plus tard besoin de groupes d'alarme différents des groupes de combat, cela
devra être une décision de données explicite et non un comportement caché.

## Déclenchement

`UGridMonsterBehaviorComponent::RefreshPerception()` reste l'autorité de
perception MON4.

Lorsqu'une perception réelle existe :

```text
RefreshPerception
    -> vue ou ouïe valide
    -> mémorise PartyCell
    -> HandleExplorationAlert(Source, PartyCell)
```

Le sous-système de patrouille vérifie ensuite les règles MON7 et redirige les
alliés éligibles.

Cette notification a lieu au moment où la perception logique est réellement
rafraîchie. Elle ne dépend pas du rendu, de la caméra ou d'une collision visuelle
3D.

## Effet sur un allié alerté

Une cible éligible :

1. annule son timer de patrouille éventuel ;
2. annule proprement son interpolation d'exploration éventuelle ;
3. reçoit `bHasLastKnownPartyCell=true` ;
4. reçoit la cellule connue de la source ;
5. passe en `EGridMonsterState::Alert` ;
6. passe en activité `Investigating` ;
7. planifie une étape MON14.3 au prochain court timer one-shot.

Le chemin d'investigation existant prend ensuite le relais :

```text
Alert
  -> Investigating
  -> path vers LastKnownPartyCell
  -> Search
  -> vision => combat
  -> rien => retour Idle / patrouille
```

## Anti-reset

Un garde source peut rafraîchir plusieurs fois la même perception pendant son
déplacement. MON14.4 ne doit pas annuler et recommencer sans cesse le mouvement
d'un allié déjà en investigation.

Une cible déjà `Investigating` ou `Searching` vers exactement la même
`LastKnownPartyCell` n'est donc pas redirigée une deuxième fois.

Si la cellule connue change, l'alarme est considérée comme nouvelle : le
mouvement courant peut être annulé et l'allié est redirigé vers la nouvelle
position.

## Autorité de combat

MON14.4 ne contient aucun appel à :

```cpp
StartCombat()
StartCombatFromPerception()
```

Une alarme auditive peut uniquement provoquer :

```text
Dormant / Idle
    -> Alert
    -> Investigating
```

Le combat automatique reste exclusivement déclenché lorsque MON14.1 obtient
une source visuelle valide.

Si la source voit déjà le groupe, MON7 sélectionne également les alliés lors de
la construction de la rencontre. MON14.4 et MON7 utilisent les mêmes règles,
ce qui évite deux définitions concurrentes de la notion de groupe.

## Portée et non-propagation globale

MON14.4 effectue une propagation locale depuis la source de perception.

Il n'existe pas de flood-fill d'alarme dans tout le donjon. Un allié alerté ne
réémet pas automatiquement l'alarme simplement parce qu'il a été alerté. Il ne
peut devenir source à son tour que s'il obtient lui-même une perception réelle
par MON4.

Cela permet des configurations comme :

```text
Garde A ---- 2 cases ---- Garde B ---- 2 cases ---- Garde C
```

avec `AggroPropagationRange=2` : A alerte B ; C n'est pas automatiquement
alerté par simple chaînage. Si B entend ou voit réellement le groupe ensuite,
il peut alors devenir une nouvelle source légitime.

## Fichiers

Modifiés :

```text
Source/GrimrockPrototype/Public/Runtime/Monsters/GridMonsterPatrolSubsystem.h
Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterBehaviorComponent.cpp
```

Ajoutés :

```text
Source/GrimrockPrototype/Private/Runtime/Monsters/GridMonsterPatrolSubsystemAlarm.cpp
Source/GrimrockPrototype/Private/Tests/GridMonsterMON14_4Tests.cpp
docs/Design/MON14_4_EXPLORATION_ALARM_COORDINATION.md
docs/Design/MON14_4_VALIDATION_CHECKLIST.md
```

## Tests automatisés

```text
Grimrock.Monsters.MON14.4.HearingAlarmPropagation
Grimrock.Monsters.MON14.4.AlarmFiltering
Grimrock.Monsters.MON14.4.SharingDisabled
```

Ils vérifient :

- réveil d'un allié `Dormant` ;
- copie correcte de la dernière cellule connue ;
- activité `Investigating` ;
- absence de combat sur alarme seule ;
- anti-reset sur alarme répétée identique ;
- exclusion hors portée ;
- exclusion autre `EncounterGroupId` ;
- exclusion autre `MonsterId` ;
- désactivation complète lorsque `bSharesAggroWithGroup=false`.

## Hors périmètre

MON14.4 ne couvre pas encore :

- animation ou cri visuel/sonore spécifique d'alarme ;
- délai de réaction configurable ;
- groupes d'alarme différents des groupes MON7 ;
- propagation à des types de monstres différents ;
- ouverture automatique de portes par un garde alerté ;
- renforts qui n'existent pas encore dans le monde (vagues futures MON13.4).

Ces extensions doivent rester optionnelles et pilotées par les données.
