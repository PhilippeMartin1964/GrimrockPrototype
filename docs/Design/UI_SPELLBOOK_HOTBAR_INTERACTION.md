# UI01.4.3d — Interaction Spellbook → Hotbar

Statut : **d.2 — DROP HOTBAR NATIF IMPLÉMENTÉ ; VALIDATION PIE EN ATTENTE**  
Date : **21 août 2026**

## Objectif

Permettre au joueur de glisser un sort connu depuis `WBP_GridSpellbookEntry` vers l'un des dix raccourcis MON12 sans créer de seconde hotbar ni dupliquer la connaissance des sorts.

Le flux cible est :

```text
WBP_GridSpellbookEntry
    ↓ drag gauche
UGridCombatHotbarDragDropOperation
    ↓ drop
UGridCombatHudActionWidget
    ↓
UGridCombatHudWidget::HandleHotbarDrop()
    ↓
UGridCombatHotbarDragDropOperation::CommitSpellbookDrop()
    ↓
UGridSpellbookUILibrary::AssignKnownSpellToHotbar()
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
5. `InitializeFromSpellbookEntry(CharacterIndex, Entry)` remplit le payload MON18.7a.

Le payload contient notamment :

```text
CharacterIndex
bFromSpellbook = true
Binding.ActionId = SpellId
Binding.SourcePolicy = Spell
Binding.SourceDefinitionId = SpellId
```

Le widget affiche un tooltip indiquant que le sort peut être glissé vers la barre d'actions.

## d.2 — Drop natif vers la hotbar MON12

`UGridCombatHudActionWidget::NativeOnDrop()` continue de déléguer à l'unique routeur du HUD :

```text
UGridCombatHudWidget::HandleHotbarDrop(TargetSlotIndex, DragOperation)
```

Le routeur traite maintenant explicitement `bFromSpellbook` avant les branches Action Palette et déplacement de raccourci.

Le drop est accepté uniquement si :

```text
HotbarOperation.bFromSpellbook == true
HotbarOperation.CharacterIndex == View.ActiveCharacterIndex
PartyPawn valide
InventoryComponent valide
CharacterIndex valide
CharacterId valide
UGridPartySpellbookComponent présent sur le Pawn
Spellbook du CharacterId présent
```

Le routeur ne reconstruit pas lui-même un binding. Il réutilise le contrat MON18.7a :

```text
CommitSpellbookDrop()
    ↓
AssignKnownSpellToHotbar()
```

Ainsi restent centralisées les règles existantes :

- rejet d'un sort inconnu ;
- rejet d'une définition invalide ;
- contrôle de l'index de slot ;
- identité `SpellId` stable ;
- un seul raccourci par sort ;
- réaffectation d'un sort déjà présent = move/swap MON12 ;
- remplacement/swap avec le contenu du slot cible selon les règles de la hotbar existante.

Aucun nouveau système de raccourcis n'est créé.

## Rafraîchissement

L'écriture dans la hotbar passe par `UGridPartyInventoryComponent`. Le mécanisme normal `OnPartyInventoryChanged` rafraîchit :

- le HUD/hotbar ;
- `UGridSpellbookWidget` ;
- `FGridSpellbookEntryView::bAssignedToHotbar` ;
- `AssignedHotbarSlotIndex` ;
- le texte `Raccourci N` de la ligne.

## Contrat UMG

Aucun nouveau widget, aucun nouveau `BindWidget` et aucun Event Graph Blueprint ne sont requis pour d.2.

Les contrats restent ceux de :

```text
docs/Design/UI_SPELLBOOK_WIDGET.md
docs/Design/UI_SPELLBOOK_HOTBAR_INTERACTION.md
```

## Validation PIE d.2

Après compilation UE5.5.4 :

1. lancer PIE ;
2. exécuter :

```text
Grimrock.Spellbook.SeedProduction
```

3. ouvrir `Menu → Sorts` ;
4. glisser `Arcane Bolt` vers le raccourci clavier **3** (slot interne 2) ;
5. vérifier que la ligne affiche :

```text
Raccourci 3
```

6. fermer puis rouvrir l'onglet `Sorts` : l'affectation doit toujours être visible ;
7. glisser `Arcane Bolt` vers le raccourci **5** : l'affectation doit se déplacer vers 5, sans duplication ;
8. glisser un second sort vers un slot déjà occupé afin de vérifier le comportement move/swap MON12 ;
9. clic droit sur le slot du sort : le raccourci doit être retiré mais le sort doit rester dans le livre ;
10. vérifier l'absence de `Blueprint Runtime Error`, crash ou mutation de `KnownSpellIds` ;
11. arrêter PIE : le seed de validation reste runtime-only.

## Point volontairement hors d.2

Le présent sous-jalon valide l'**affectation** d'un sort connu à la hotbar. La résolution/exécution complète de ces bindings par le catalogue de combat, leur présentation finale (icônes dédiées) et le lancement depuis la hotbar restent des responsabilités distinctes de la suite MON18.7b/UI01.4.3.
