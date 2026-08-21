# UI01.4.3e — Resolve & Execute Spell Hotbar Actions

Statut : **e.1 VALIDÉ ; e.2 IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
Date : **21 août 2026**

## Objectif

Faire d'un binding Spellbook présent dans la hotbar MON12 une vraie action de combat : résolution depuis le Spellbook autoritaire, ciblage MON18.4, transaction PA/mana MON18.3, effets MON18.5 puis présentation MON18.6.

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

Les tests `SpellCatalogProjection` et `SpellBindingMatchesProjectedAction` ont été validés sous UE5.5.4. La palette générique MON12 n'affiche pas ces entrées Spellbook ; elles restent dans le Livre de sorts et dans les slots explicitement configurés.

## e.2 — Execute — contrat runtime

`UGridTurnManagerComponent::RequestCharacterCombatAction()` distingue maintenant un sort réellement fourni par le Spellbook d'une ancienne action de classe dont `SourcePolicy` vaut également `Spell`.

Un sort Spellbook est exécuté par :

```text
Hotbar / clic / touche 1-0
    -> FGridSpellHotbarExecutionService
        -> FGridSpellCastPipelineService
            -> FGridSpellTargetingService
            -> FGridSpellCastTransactionService
        -> FGridSpellEffectResolver
    -> commit autoritaire TurnManager / Inventory / Monster
    -> FGridSpellPresentationService
    -> UGridSpellPresentationComponent
```

Aucun deuxième moteur de coûts, ciblage, effets ou présentation n'est introduit.

## Atomicité

`FGridSpellHotbarExecutionService` travaille sur des copies de :

- `FRPGDerivedStats` du lanceur ;
- `FGridPlayerCharacterTurnState` ;
- PV de la cible ;
- `FGridStatusEffectCollection` de la cible.

Les PA/mana validés par MON18.3 ne sont donc écrits dans l'état autoritaire qu'après réussite de la résolution des effets MON18.5. Si `Haste` ne peut pas résoudre `Status_Haste`, par exemple, le cast est rejeté sans consommation réelle de PA ni de mana.

## Ciblage depuis la hotbar

Les quatre sorts de production actuels sont pris en charge :

```text
Spell_ArcaneBolt   -> première cible hostile axiale suggérée
Spell_LesserHeal   -> allié
Spell_Haste        -> allié
Spell_CurePoison   -> allié
```

Pour UI01.4.3e.2, un sort `Ally` lancé directement depuis la hotbar cible par défaut **le lanceur lui-même**. Cette règle est volontaire et déterministe : elle permet d'utiliser immédiatement les sorts alliés sans inventer un deuxième sélecteur d'allié. Une future sélection par portrait pourra fournir un autre `CharacterId` sans modifier le pipeline MON18.

`Arcane Bolt` réutilise la cible `SuggestedTargetId/SuggestedTargetCell` déjà calculée par le catalogue avec la traversée axiale du niveau. L'absence de cible vivante suggérée provoque `InvalidTarget` sans coût.

## Effets autoritaires

- personnage ciblé : commit de `CurrentHealth` et `StatusEffects` dans `FGridCharacterInventoryState` ;
- monstre ciblé : commit de `StatusEffects`, puis `SetCurrentHealth()` afin de conserver le pipeline de mort/persistance existant ;
- les changements de statut demandent ensuite au lifecycle MON16 de recalculer les modificateurs d'initiative ;
- les coûts écrits sont ceux du receipt MON18.3 ;
- le cooldown MON12 reste géré par `StartCombatActionCooldown()` ;
- AP à zéro termine normalement le tour actif ;
- la mort du dernier monstre conserve le mécanisme de victoire différée déjà utilisé pendant la résolution d'une attaque joueur.

## Présentation

Après un cast accepté, le TurnManager tente de construire le profil de présentation via `FGridProductionSpellLibrary::TryBuildPresentationProfile()` puis un plan MON18.6.

Le composant `UGridSpellPresentationComponent` est retrouvé sur le pawn ou créé comme composant runtime si nécessaire. Une présentation absente/incomplète n'annule jamais le gameplay déjà accepté, conformément au contrat MON18.6.

L'absence actuelle d'icônes de sorts dans la hotbar reste une finition graphique distincte.

## Status_Haste

Le code n'invente aucune définition de Haste. `UGridStatusEffectDefinitionAsset` reste data-driven. UI01.4.3e.2 résout une définition `Status_Haste` chargée et valide ; si aucune définition n'est disponible, Haste est rejeté atomiquement et le log indique le rejet d'effet.

## Tests Automation e.2

Filtre :

```text
Grimrock.UI.UI01.4.3e.2
```

Tests :

```text
ArcaneBoltExecution
LesserHealExecution
MissingStatusNoCostCommit
UnknownSpellNoCostCommit
```

Ils vérifient notamment :

- dégâts + PA + mana pour Arcane Bolt ;
- soin + PA + mana pour Lesser Heal ;
- absence de coût réel lorsque Haste manque sa définition MON16 ;
- absence de coût pour un sort non connu.

## Validation PIE attendue

1. `Grimrock.Spellbook.SeedProduction`.
2. Affecter `Arcane Bolt` à un slot de hotbar.
3. Entrer en combat avec une cible axiale à 1–5 cases.
4. Au tour du personnage, cliquer le slot ou utiliser sa touche 1–0.
5. Vérifier : cast accepté, -2 PA, -3 mana, -4 PV sur la cible.
6. Affecter `Lesser Heal`, blesser le personnage, puis lancer le sort : -2 PA, -4 mana, +5 PV plafonné au maximum.
7. Vérifier qu'un rejet de ciblage, de connaissance ou de définition de statut ne consomme rien.

Après validation compilation + Automation + PIE, UI01.4.3e pourra être déclaré clos et la documentation MON18.7/roadmap sera remise à jour lors de la clôture.
