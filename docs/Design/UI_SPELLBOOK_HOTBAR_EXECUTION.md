# UI01.4.3e — Resolve & Execute Spell Hotbar Actions

Statut : **e.1 — RÉSOLUTION CATALOGUE EN COURS ; EXÉCUTION À VALIDER ENSUITE**  
Date : **21 août 2026**

## Objectif

Faire d’un binding Spellbook déjà présent dans la hotbar MON12 une vraie action de combat résolue à partir du Spellbook autoritaire, avant de raccorder l’exécution au pipeline MON18.3–MON18.6.

## Découpage

UI01.4.3e est volontairement séparé en deux validations techniques :

- **e.1 — Resolve** : projeter les sorts connus dans le catalogue de combat ;
- **e.2 — Execute** : lancer le sort depuis le raccourci en réutilisant ciblage, transaction, effets et présentation MON18.

Ce découpage évite de modifier simultanément la source d’action et la mutation gameplay sans compilation intermédiaire.

## e.1 — source autoritaire

La source reste :

```text
UGridPartySpellbookComponent
    -> FGridCharacterSpellbookState
    -> KnownSpellIds[]
```

Pour chaque `KnownSpellId`, le TurnManager doit :

1. résoudre la définition dans `FGridProductionSpellLibrary` ;
2. ignorer une définition absente ou invalide ;
3. construire `FGridCombatActionDefinition` via `UGridSpellbookUILibrary::MakeSpellCombatActionDefinition()` ;
4. ajouter une contribution avec :

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
SourceRuntimeId    = invalid
EquipmentSlot      = None
```

Aucun sort inconnu du personnage ne doit être ajouté au catalogue.

## Effet attendu sur la hotbar

Après e.1, un binding Spellbook n’est plus une identité orpheline : `FGridCombatHudViewModelBuilder::BuildHotbarActions()` peut retrouver la contribution correspondante et fournir le nom, la description et les coûts réels.

L’absence d’icône reste normale tant que les quatre définitions MON18.5 ne possèdent pas d’asset d’icône.

## e.2 — contrat d’exécution prévu

L’exécution devra réutiliser exclusivement :

```text
FGridSpellTargetingService / FGridSpellCastPipelineService
FGridSpellCastTransactionService
FGridSpellEffectResolver
UGridStatusEffectLifecycleSubsystem
FGridSpellPresentationService
UGridSpellPresentationComponent
```

Aucun second moteur de coûts, ciblage, effets ou hotbar ne doit être introduit.

Les quatre sorts de production concernés sont :

```text
Spell_ArcaneBolt
Spell_LesserHeal
Spell_Haste
Spell_CurePoison
```

`Haste` ne pourra appliquer son statut que si la définition MON16 `Status_Haste` est réellement résolue ; l’absence de définition doit provoquer un rejet sans consommation de PA/mana.

## Validation e.1

Après compilation UE5.5.4 :

1. lancer PIE ;
2. exécuter `Grimrock.Spellbook.SeedProduction` ;
3. assigner `Arcane Bolt` à un raccourci ;
4. déclencher un combat et sélectionner le personnage concerné ;
5. vérifier que le raccourci n’est plus présenté comme une action inconnue/non résolue ;
6. vérifier que nom/coût/description correspondent à la définition MON18.5 ;
7. ne pas considérer encore le clic/touche comme validé avant e.2.
