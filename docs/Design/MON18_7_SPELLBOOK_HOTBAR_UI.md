# MON18.7 — Spellbook / Hotbar UI

Statut : **MON18.7a IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
Date : **21 août 2026**

## Découpage

```text
MON18.7a — Native Spellbook UI model + Hotbar bridge   EN VALIDATION
MON18.7b — WBP hookup + PIE validation                 À FAIRE APRÈS 18.7a
```

Ce découpage évite de modifier des `.uasset` avant que le contrat C++ soit validé.

## Objectif MON18.7a

Réutiliser le Spellbook MON18.2 et le hotbar MON12.8 sans créer de deuxième système de raccourcis.

`UGridSpellbookUILibrary` projette chaque `SpellId` connu vers `FGridSpellbookEntryView` :

- nom / description ;
- école ;
- coût mana / PA ;
- portée ;
- politique de ciblage ;
- exigence LOS ;
- état de résolution de la définition ;
- slot hotbar déjà assigné ;
- `FGridCombatActionDefinition` UI-ready.

Les sorts connus dont la définition n'est pas résolue restent visibles mais sont explicitement non assignables. Aucun `SpellId` n'est silencieusement supprimé de l'UI.

## Adaptateur vers le hotbar MON12

Un sort utilise exactement l'identité suivante :

```text
ActionId          = SpellId
SourcePolicy      = Spell
SourceDefinitionId= SpellId
```

Aucun état de sort n'est copié dans le hotbar. Les dix slots existants restent l'unique stockage de raccourcis.

`AssignKnownSpellToHotbar()` :

- vérifie le `CharacterId` ;
- refuse un sort inconnu ;
- refuse une définition de production absente/invalide ;
- réutilise `SetCharacterCombatHotbarBinding()` ;
- si le même sort existe déjà ailleurs, réutilise `MoveOrSwapCharacterCombatHotbarBinding()` afin d'éviter les doublons et de préserver le contenu du slot cible.

`UnassignSpellFromHotbar()` ne retire jamais le sort du Spellbook.

## Drag & Drop

`UGridCombatHotbarDragDropOperation` possède désormais le contrat :

```text
bFromSpellbook
InitializeFromSpellbookEntry(...)
CommitSpellbookDrop(...)
```

`CommitSpellbookDrop()` appelle le bridge natif et ne déplace/consomme aucun objet d'inventaire.

MON18.7b raccordera ce payload aux WBP existants et validera le comportement visuel/PIE.

## Action UI

Le projet n'ajoute pas une nouvelle valeur sérialisée à `EGridCombatActionType`. Le type visuel reste `Ability`, tandis que `EGridCombatActionSourcePolicy::Spell` demeure le discriminateur canonique d'un sort.

Le `FGridCombatActionDefinition` adapté expose les coûts et le ciblage pour le HUD. L'exécution autoritaire du cast reste portée par les services MON18.3–MON18.5 ; MON18.7a ne déplace aucune logique de gameplay dans l'UI.

## Tests Automation

Filtre :

```text
Automation RunTests Grimrock.Magic.MON18.7a
```

Tests attendus :

```text
ProductionEntriesReflectKnowledge
UnknownDefinitionRemainsVisible
SpellBindingContract
AssignKnownSpell
DuplicateAssignmentMovesOrSwaps
UnknownSpellNoMutation
DragDropPayload
```

Attendu : **7/7 Success**.

## Hors périmètre MON18.7a

- modification des WBP/.uasset ;
- icônes finales de sorts ;
- validation visuelle PIE ;
- persistance du Spellbook : MON18.8.
