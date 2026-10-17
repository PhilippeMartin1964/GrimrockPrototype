# UI-RPG-TALENT-CODE-AUDIT02.3 — Runtime Talent progression cache ownership

Date : 17 octobre 2026  
Statut : **IMPLÉMENTÉ — EN ATTENTE DE VALIDATION LOCALE**

## Problème constaté

La progression durable est correcte :

```text
FGridCharacterInventoryState::SelectedClassProgressionChoiceIds
```

En revanche, la projection dérivée utilisait un cache process-global :

```text
TMap<FGuid, FRuntimeClassProgressionState> RuntimeStates
```

Le `CharacterId` seul ne constitue pas une identité runtime suffisante. Deux
`UGridPartyInventoryComponent` provenant de mondes PIE distincts, de tests
concurrents ou d'un chargement/rechargement peuvent porter le même
`CharacterId`. Le dernier writer remplaçait alors la projection du premier.

Un second défaut provenait de `UGrimrockPartySaveGame::Serialize()` : la simple
désérialisation d'un slot reconstruisait des entrées « détachées » avec
`InventoryComponent == nullptr`. L'inspection des slots depuis le menu pouvait
donc modifier le cache global avant toute restauration dans un groupe vivant,
et `ResetRuntimeState(Component)` ne pouvait pas supprimer ces entrées.

## Contrat retenu

Le cache dérivé est maintenant organisé ainsi :

```text
RuntimeStates
    UGridPartyInventoryComponent vivant
        CharacterId
            SatisfiedRequirements
```

Invariants :

- aucun `ChoiceId` durable n'est déplacé ou dupliqué dans le cache ;
- deux composants portant le même `CharacterId` restent isolés ;
- `ResetRuntimeState(Component)` ne touche qu'au composant demandé ;
- les clés de composants détruits sont purgées opportunistement ;
- `RebuildRuntimeProjection(Component)` remplace atomiquement uniquement la
  projection de ce composant ;
- `AppendRuntimeSatisfiedRequirements(Component, CharacterId, ...)` refuse un
  personnage absent ou ambigu dans ce composant ;
- la désérialisation d'un SaveGame ne peuple aucun cache runtime ;
- le chemin combat rafraîchit la projection depuis le personnage vivant avant
  d'évaluer les requirements.

## Pureté du catalogue de combat

`FGridCombatActionCatalog::Build()` redevient un service pur : il ne consulte
plus de registry statique par `CharacterId`.

`UGridTurnManagerComponent::GetAvailableCombatActions()` possède déjà le
`UGridPartyInventoryComponent` et le personnage autoritaire. C'est donc ce
chemin qui rafraîchit la projection Talent et ajoute ses requirements au
`FGridCombatActionCatalogContext` avant l'appel au catalogue.

## Persistance

Le SaveGame ne change pas de schéma et aucun `.uasset` n'est modifié.

Après désérialisation :

```text
SelectedClassProgressionChoiceIds -> présents sur le personnage
RuntimeStates                     -> inchangé
```

Après rattachement à un composant vivant :

```text
Refresh/RebuildRuntimeProjection(Component)
    -> SatisfiedRequirements reconstructibles
```

## Régressions

Le filtre `Grimrock.UI.RPG.CODEAUDIT02.RuntimeCacheOwnership` couvre :

1. deux composants distincts avec le même `CharacterId` ;
2. reset ciblé d'un seul composant ;
3. réutilisation du même `CharacterId` avec un état durable différent ;
4. désérialisation SaveGame sans pollution d'un cache live homonyme ;
5. reconstruction explicite après rattachement du SaveGame à un composant vivant.

Les suites historiques MON15.5, MON20.7, TD07.3.2 et TD07.3.3.5 restent les
régressions fonctionnelles de persistance/progression.
