# Architecture des liens, événements et commandes

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**

## 1. Principe

`UGridLevelAsset::Links` stocke les `FGridObjectLink` authored.
`UGridActivationComponent` est le dispatcher runtime Event → Command.

```text
Source Object + Event
    -> FGridObjectLink
        -> condition optionnelle
        -> target / payload
        -> EGridObjectCommand
```

Les Actors émettent des événements ; le dispatcher évalue et route ; la cible ou
le service de domaine applique la mutation.

## 2. Identités

`FGridObjectLink` référence les placements par GUID :

```text
SourceObjectId
TargetObjectId
```

`LogicId` est un alias d'authoring résolu vers une identité stable lorsqu'un
script l'utilise.

Les commandes Quest portent en plus :

```text
QuestId
QuestObjectiveId
```

Lua porte :

```text
LuaScriptId
LuaCallbackName
```

## 3. Événements courants

`EGridObjectEvent` couvre notamment :

- Activated / Deactivated ;
- ItemInserted / ItemRemoved / ItemChanged ;
- Used ;
- Entered / Exited ;
- Opened / Closed ;
- Enabled / Disabled ;
- MonsterDied / Spawned / Despawned / Teleported ;
- EncounterWaveStarted / EncounterCompleted ;
- Sabotaged.

Une enum déclarée ne suffit pas : un événement n'est fonctionnel que s'il
possède un émetteur réel.

## 4. Commandes courantes

`EGridObjectCommand` couvre notamment :

- Toggle / Open / Close ;
- Activate / Deactivate ;
- Enable / Disable ;
- Lock / Unlock ;
- Spawn / Despawn / Teleport ;
- ShowMessage ;
- consommation et permissions de Receptacle ;
- StartEncounter ;
- LogicExecute / LogicReset ;
- LuaCallback ;
- OfferRecruitment / OpenCustomRecruit ;
- QuestStart / QuestCompleteObjective / QuestComplete / QuestFail.

Les commandes de domaine restent implémentées par leur runtime/service
spécialisé ; `UGridActivationComponent` ne devient pas une seconde
implémentation métier.

## 5. Conditions

Le contrat direct `EGridObjectCondition` reste volontairement centré sur le
contenu d'un réceptacle :

- vide / non vide ;
- contient une définition ;
- contient un tag ;
- contient un type ;
- quantité minimale ;
- poids minimal ;
- inversion.

Les variables Bool/Int, comparaisons plus générales et états complexes
appartiennent au système Logic/Lua plutôt qu'à une explosion de cette enum.

## 6. Logic

```text
Event
    -> LogicExecute
        -> GridLogicRuntime
        -> nouvel Event
        -> dispatcher
```

Logic fournit les primitives génériques (Relay, Set/Toggle, Add/Subtract,
Compare, Latch, Reset...) et revient toujours dans le même bus.

## 7. Lua

```text
Event
    -> LuaCallback
        -> FGridLuaVm
        -> grid.command(...)
        -> dispatcher canonique
```

Lua n'est pas une voie parallèle de mutation.

## 8. Quest

MON21.3 ajoute les commandes Quest au bus existant.

`UGridActivationComponent` délègue au `UGridQuestSubsystem`; l'état Quest
reste unique dans ce subsystem et n'est pas encore persisté.

## 9. Receptacles

Les événements ItemInserted/Removed/Changed viennent du Receptacle runtime.
Les commandes de permission de retrait/insertion et de consommation ciblent le
même acteur/state.

Une transaction annulée ne doit pas émettre un succès.

## 10. Editor

Le Grid Editor permet de choisir source, event, target et command. Les services
d'authoring/validation travaillent sur le `LevelAsset`, jamais sur une copie
Slate.

Les scripts Lua et identités Quest possèdent leurs panneaux/contrats
spécialisés.

## 11. Invariants

1. `UGridLevelAsset::Links` = autorité authored.
2. `UGridActivationComponent` = dispatcher runtime unique.
3. Un événement métier est émis une seule fois par action.
4. Une condition invalide échoue avant la mutation.
5. Logic/Lua/Quest réutilisent le bus existant.
6. Les Actors/services spécialisés restent autorités des mutations de domaine.
7. Les links ne stockent pas un état runtime propre à sauvegarder.
