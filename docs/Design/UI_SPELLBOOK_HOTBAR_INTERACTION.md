# UI01.4.3d — Interaction Spellbook → Hotbar

Statut : **d.1 — SOURCE DE DRAG NATIVE IMPLÉMENTÉE ; DROP HOTBAR EN ATTENTE**  
Date : **21 août 2026**

## Objectif

Permettre au joueur de glisser un sort connu depuis `WBP_GridSpellbookEntry` vers l'un des dix raccourcis MON12 sans créer de seconde hotbar ni dupliquer la connaissance des sorts.

Le flux cible reste :

```text
WBP_GridSpellbookEntry
    ↓ drag gauche
UGridCombatHotbarDragDropOperation
    ↓ drop
UGridCombatHudActionWidget
    ↓
UGridPartyInventoryComponent / MON12 hotbar
```

## d.1 — Source de drag native

`WBP_GridSpellbookEntry` garde comme parent :

```text
GridSpellbookEntryWidget
```

Aucun Event Graph Blueprint n'est requis.

`UGridSpellbookEntryWidget` intercepte le bouton gauche dans `NativeOnPreviewMouseButtonDown()` uniquement si :

```text
Entry.bCanAssignToHotbar == true
Entry.SpellId != NAME_None
```

Le drag est déclenché avec `DetectDragIfPressed`.

Dans `NativeOnDragDetected()` :

1. le Pawn propriétaire est résolu par `GetOwningPlayerPawn()` ;
2. le `UGridPartyInventoryComponent` existant est utilisé ;
3. `GetSelectedCharacterIndex()` fournit le personnage autoritaire ;
4. un `UGridCombatHotbarDragDropOperation` est créé ;
5. `InitializeFromSpellbookEntry(CharacterIndex, Entry)` remplit le payload existant MON18.7a.

Le payload contient notamment :

```text
CharacterIndex
bFromSpellbook = true
Binding.ActionId = SpellId
Binding.SourcePolicy = Spell
Binding.SourceDefinitionId = SpellId
```

Le widget affiche aussi un tooltip :

```text
Glissez ce sort vers un raccourci de la barre d'actions.
```

ou, si la définition ne peut pas être assignée :

```text
Ce sort ne peut pas être assigné à la barre d'actions.
```

## Contrat UMG

Aucun nouveau widget ni nom `BindWidget` n'est ajouté pour d.1.

La hiérarchie documentée dans `docs/Design/UI_SPELLBOOK_WIDGET.md` reste inchangée.

## Validation d.1

Après compilation :

1. lancer PIE ;
2. exécuter `Grimrock.Spellbook.SeedProduction` ;
3. ouvrir `Menu → Sorts` ;
4. maintenir le bouton gauche sur une ligne de sort et déplacer la souris ;
5. vérifier qu'aucune erreur Blueprint Runtime ni crash ne se produit.

À ce stade, le drop sur un slot n'est **pas encore considéré comme validé** : le HUD MON12 sait déjà déplacer les raccourcis, recevoir des objets d'inventaire et des actions de palette, mais son chemin `HandleHotbarDrop()` doit encore traiter explicitement `bFromSpellbook`.

## d.2 — suite prévue

Le sous-jalon suivant ajoutera le traitement du payload Spellbook côté `UGridCombatHudActionWidget` / `UGridCombatHudWidget::HandleHotbarDrop()` en réutilisant `UGridCombatHotbarDragDropOperation::CommitSpellbookDrop()` et `UGridSpellbookUILibrary::AssignKnownSpellToHotbar()`.

Validation finale attendue :

```text
Drag Arcane Bolt → slot 3
    → hotbar slot 3 contient Spell_ArcaneBolt
    → la ligne Spellbook affiche Raccourci 3
    → nouveau drag vers un autre slot = move/swap MON12
    → clic droit sur le slot = retrait du raccourci uniquement
    → le sort reste connu
```
