# UI01.4.3e — Resolve & Execute Spell Hotbar Actions

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Date : **21 août 2026**

## Objectif

Faire d'un binding Spellbook présent dans la hotbar MON12 une vraie action de combat : résolution depuis le Spellbook autoritaire, ciblage MON18.4, transaction PA/mana MON18.3, effets MON18.5 puis présentation MON18.6.

## Résultat final

UI01.4.3e est clos. Le flux réel validé est :

```text
Spellbook
    -> drag/drop vers hotbar MON12
    -> clic ou touche 0-9
    -> catalogue d'actions
    -> résolution Spellbook
    -> ciblage MON18.4
    -> transaction PA/mana MON18.3
    -> effets MON18.5
    -> commit autoritaire personnage/monstre
    -> présentation MON18.6
```

Aucun deuxième moteur de coûts, ciblage, effets, hotbar ou présentation n'a été introduit.

## e.1 — Resolve — VALIDÉ

Le catalogue reconstruit chaque sort connu depuis :

```text
UGridPartySpellbookComponent
    -> FGridCharacterSpellbookState::KnownSpellIds
    -> FGridProductionSpellLibrary
    -> UGridSpellbookUILibrary::MakeSpellCombatActionDefinition()
```

Identité canonique :

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
SourceRuntimeId    = invalid
EquipmentSlot      = None
```

La palette générique MON12 n'affiche pas les entrées gérées par le Spellbook. Elles restent visibles dans le Livre de sorts et dans les slots explicitement configurés.

## e.2 — Execute — VALIDÉ

`UGridTurnManagerComponent::RequestCharacterCombatAction()` distingue maintenant un sort fourni par le Spellbook d'une ancienne action de classe dont `SourcePolicy` vaut également `Spell`.

Un sort Spellbook est exécuté par :

```text
Hotbar / clic / touche 0-9
    -> FGridSpellHotbarExecutionService
        -> FGridSpellCastPipelineService
            -> FGridSpellTargetingService
            -> FGridSpellCastTransactionService
        -> FGridSpellEffectResolver
    -> commit autoritaire TurnManager / Inventory / Monster
    -> FGridSpellPresentationService
    -> UGridSpellPresentationComponent
```

## Correction du verrou de catalogue

La première implémentation e.2 était correcte au niveau de l'exécuteur, mais restait inaccessible en PIE : `FGridCombatActionCatalog` classait encore les projections Spellbook comme anciennes actions de classe et renvoyait :

```text
EGridCombatActionAvailabilityReason::ExecutionNotImplemented
```

Le correctif descendant `56bce2cdd90064b1b548cd93649b9e1207ba0bdc` reconnaît explicitement une projection Spellbook exécutable par son identité canonique :

```text
SourcePolicy       = Spell
ActionId           = SpellId
SourceDefinitionId = SpellId
ResolutionProfile  = Effect
```

Les ciblages `Self`, `Ally` et `FirstAxialTarget` peuvent alors atteindre l'exécuteur MON18.

## Atomicité

`FGridSpellHotbarExecutionService` travaille sur des copies de :

- `FRPGDerivedStats` du lanceur ;
- `FGridPlayerCharacterTurnState` ;
- PV de la cible ;
- `FGridStatusEffectCollection` de la cible.

Les PA/mana ne sont écrits dans l'état autoritaire qu'après réussite de la résolution des effets. Un rejet de ciblage, de connaissance ou de définition de statut ne consomme donc rien.

## Ciblage depuis la hotbar

Les quatre sorts de production actuels sont pris en charge :

```text
Spell_ArcaneBolt   -> première cible hostile axiale suggérée
Spell_LesserHeal   -> allié
Spell_Haste        -> allié
Spell_CurePoison   -> allié
```

Pour UI01.4.3e, un sort `Ally` lancé directement depuis la hotbar cible par défaut le lanceur lui-même. Cette règle est volontaire et déterministe. Une future sélection par portrait pourra fournir un autre `CharacterId` sans modifier le pipeline MON18.

`Arcane Bolt` réutilise `SuggestedTargetId/SuggestedTargetCell` calculé par le catalogue. L'absence de cible vivante suggérée provoque `InvalidTarget` sans coût.

## Effets autoritaires

- personnage ciblé : commit de `CurrentHealth` et `StatusEffects` dans `FGridCharacterInventoryState` ;
- monstre ciblé : commit des statuts puis `SetCurrentHealth()` afin de conserver mort, loot, XP, persistance et libération d'occupation ;
- les changements de statut demandent au lifecycle MON16 de recalculer les modificateurs d'initiative ;
- les coûts écrits sont ceux du receipt MON18.3 ;
- le cooldown MON12 reste géré par `StartCombatActionCooldown()` ;
- AP à zéro termine normalement le tour actif ;
- la victoire conserve le mécanisme différé du TurnManager.

## Présentation

Après un cast accepté, le TurnManager construit le profil de présentation via `FGridProductionSpellLibrary::TryBuildPresentationProfile()` puis un plan MON18.6.

Le composant `UGridSpellPresentationComponent` est retrouvé sur le pawn ou créé comme composant runtime si nécessaire. Une présentation absente/incomplète n'annule jamais le gameplay déjà accepté.

Les icônes finales de sorts restent une finition graphique distincte.

## Status_Haste

Le code n'invente aucune définition de Haste. `UGridStatusEffectDefinitionAsset` reste data-driven. Si `Status_Haste` ne peut pas être résolu, Haste est rejeté atomiquement sans consommation réelle de PA ni de mana.

## Validation Automation

Filtre :

```text
Grimrock.UI.UI01.4.3e.2
```

Résultat UE5.5.4 : **6/6 Success**.

```text
ArcaneBoltExecution                Success
LesserHealExecution                Success
MissingStatusNoCostCommit          Success
SpellbookCatalogAvailability       Success
SpellbookCatalogExecutorGate       Success
UnknownSpellNoCostCommit           Success
```

Les deux tests `SpellbookCatalogAvailability` et `SpellbookCatalogExecutorGate` couvrent explicitement la régression `ExecutionNotImplemented` découverte en PIE.

## Validation PIE

Validation manuelle réussie sous UE5.5.4 avec `Grimrock.Spellbook.SeedProduction`.

### Lesser Heal

Observation réelle :

```text
[GridSpellAction] Accepted=true
Spell=Spell_LesserHeal
AP=2
Mana=4
Health=3->8
ManaState=18->14
Damage=0
Healing=5
```

### Arcane Bolt

Observation réelle :

```text
[GridSpellAction] Accepted=true
Spell=Spell_ArcaneBolt
AP=2
Mana=3
Damage=4
```

Le sort a été relancé sur plusieurs manches. Le coup létal sur un Gobelin lanceur a correctement déclenché le pipeline existant de mort, loot, XP, `MonsterDied` et libération de l'occupation.

### Mana insuffisant

Lorsque le personnage ne disposait plus que de 1 mana, `Arcane Bolt` a été refusé par le catalogue avec :

```text
EGridCombatActionAvailabilityReason::InsufficientMana
```

Aucun coût supplémentaire n'a été engagé.

## Conclusion

```text
UI01.4.3e.1 — Resolve Spell Hotbar Actions   VALIDÉ
UI01.4.3e.2 — Execute Spell Hotbar Actions   VALIDÉ Automation + PIE
UI01.4.3e   — Resolve & Execute               VALIDÉ ET CLOS
```

Le prochain travail fonctionnel de MON18 n'est plus l'UI d'exécution : il s'agit de `MON18.8 — Persistence / Migration`, afin de persister réellement `KnownSpellIds` et de restaurer le Spellbook après `Continue`.
