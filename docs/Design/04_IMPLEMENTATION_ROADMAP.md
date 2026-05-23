# GrimrockPrototype — Feuille de route d’implémentation

Statut : roadmap historique initiale. Plusieurs phases sont déjà réalisées ou remplacées par les décisions du `99_DECISIONS_LOG.md`.

## Objectif

Ce document propose une feuille de route progressive pour implémenter le nouveau système d’objets, d’événements, de commandes et de liens sans casser le projet existant.

---

## Règle générale

Ne pas faire un refactor massif.

Chaque étape doit :

1. modifier peu de fichiers ;
2. compiler ;
3. être testable dans UE5 ;
4. produire un commit Git clair ;
5. mettre à jour les documents `Docs/Design`.

---

## Phase 0 — Sauvegarde et branche Git

Créer une branche dédiée :

```text
feature/grid-object-system
```

Ou, mieux, plusieurs branches courtes :

```text
feature/grid-events-commands
feature/grid-object-links
feature/archetype-object-fields
feature/receptacle-archetypes
feature/runtime-event-dispatcher
feature/editor-link-inspector
```

---

## Phase 1 — Documentation

Créer les fichiers :

```text
Docs/Design/00_PROJECT_OVERVIEW.md
Docs/Design/01_GRID_OBJECT_SYSTEM.md
Docs/Design/02_OBJECT_ARCHETYPES.md
Docs/Design/03_EVENT_COMMAND_LINKS.md
Docs/Design/04_IMPLEMENTATION_ROADMAP.md
Docs/Design/05_CODEX_TASKS.md
Docs/Design/99_DECISIONS_LOG.md
```

Objectif : stabiliser la cible avant de modifier le code.

---

## Phase 2 — Types C++ sans runtime

Fichiers probables :

```text
Source/GrimrockPrototype/Public/Core/GridTypes.h
```

Ajouter ou compléter :

```text
EGridObjectEvent
EGridObjectCommand
EGridObjectInitialState
FGridObjectLink
```

Contraintes :

- ne pas supprimer `Spawn` ;
- ne pas supprimer `Readable` ;
- ne pas renommer `WallInscription` ;
- ne pas modifier le runtime ;
- ne pas modifier les Blueprints ;
- compiler.

Commit recommandé :

```text
Add grid object events commands and link types
```

---

## Phase 3 — Archétypes

Fichiers probables :

```text
Source/GrimrockPrototype/Public/Core/GridObjectArchetypeAsset.h
Source/GrimrockPrototype/Private/Core/GridObjectArchetypeAsset.cpp
```

ou selon l’existant :

```text
Source/GrimrockPrototype/Public/Core/UGridObjectArchetypeAsset.h
Source/GrimrockPrototype/Private/Core/UGridObjectArchetypeAsset.cpp
```

Ajouter ou clarifier :

```text
Category
ActorClass
InitialState
EmittedEvents
AcceptedCommands
InteractionType
RequiredItemTag
bConsumeInsertedItem
bAllowItemRemoval
bDisplayInsertedItem
bOneShot
bEnabledByDefault
```

Contraintes :

- préserver la compatibilité des DataAssets existants ;
- ne pas casser les archétypes déjà créés ;
- éviter les renames destructifs ;
- compiler.

Commit recommandé :

```text
Expose events commands and runtime class on object archetypes
```

---

## Phase 4 — DataAssets concrets

Créer ou corriger les archétypes :

```text
Button_Normal
Button_Secret
Button_Wall
Lever_Standard
PressurePlate_Stone
Trigger_Floor
Rune_Magic
Timer_Default

Receptacle_Alcove
Receptacle_TorchHolder
Receptacle_Altar
Receptacle_OfferingBowl
Receptacle_CoinSlot
Lock_Keyhole

Door_Stone
Door_Secret
Trapdoor_Stone
Teleporter_Rune

Item_Key
Item_Coin
Item_Torch

Readable_WallInscription
Spawn_Player
Spawn_Monster
Spawn_Item
```

Objectif :

- les objets sont distincts dans la palette ;
- les classes runtime peuvent être partagées ;
- les paramètres d’archétype pilotent le comportement.

Commit recommandé :

```text
Add concrete object archetypes for mechanisms receptacles and passages
```

---

## Phase 5 — Runtime dispatcher

Créer ou consolider :

```text
UGridActivationComponent
```

Fonctions minimales :

```cpp
void EmitEvent(FName SourceObjectId, EGridObjectEvent Event);

void ExecuteCommand(FName TargetObjectId, EGridObjectCommand Command);
```

Le composant doit :

1. indexer les objets runtime par `ObjectId` ;
2. indexer les liens ;
3. recevoir un événement ;
4. trouver les liens correspondants ;
5. appliquer la commande sur la cible ;
6. gérer les délais ;
7. gérer `bOneShot`.

Contraintes :

- ne pas connecter directement bouton -> porte ;
- ne pas mettre la logique des énigmes dans les acteurs ;
- logs de debug utiles.

Commit recommandé :

```text
Add runtime event command dispatcher
```

---

## Phase 6 — Adaptation des acteurs runtime

Adapter progressivement :

```text
AGridButtonActor
AGridLeverActor
AGridPressurePlateActor
AGridDoorActor
AGridSecretDoorActor
AGridReceptacleActor
```

Ordre recommandé :

1. `AGridButtonActor` émet `OnActivate`.
2. `AGridDoorActor` accepte `Open`, `Close`, `ToggleOpen`, `Lock`, `Unlock`.
3. `AGridLeverActor` émet `OnActivate`, `OnDeactivate`, `OnToggle`.
4. `AGridPressurePlateActor` émet `OnActivate`, `OnDeactivate`.
5. `AGridReceptacleActor` émet `OnInsertItem`, `OnRemoveItem`.
6. `AGridSecretDoorActor` accepte `Open`, `Close`, `ToggleOpen`.

Commit recommandé :

```text
Connect existing runtime actors to event command dispatcher
```

---

## Phase 7 — Éditeur

Adapter l’inspecteur d’objet pour afficher :

```text
ObjectId
ArchetypeId
Category
ActorClass
InitialState
EmittedEvents
AcceptedCommands
Outgoing Links
Incoming Links
```

Ajouter une interface de création de lien :

```text
Source Event
Target Object
Target Command
Delay
One Shot
```

Contraintes :

- garder l’éditeur simple ;
- ne pas surcharger l’UI ;
- afficher seulement les Events/Commands pertinents si possible ;
- garder les données éditables dans le DataAsset de niveau.

Commit recommandé :

```text
Add object link editing to grid editor inspector
```

---

## Phase 8 — Carte de test

Créer une map ou un niveau de test :

```text
L_ObjectSystemTest
```

Scénarios minimaux :

```text
Button_Normal -> Door_Stone.ToggleOpen
Button_Secret -> Door_Secret.Open
Lever -> Teleporter.Toggle
PressurePlate -> Door_Stone.Open/Close
TorchHolder.OnInsertItem -> Door_Stone.Open
CoinSlot.OnInsertItem -> Door_Secret.Open
Timer.OnTimer -> Door_Stone.Close
WallInscription.OnUse -> ShowText
```

Objectif :

- tester le modèle complet ;
- valider les logs ;
- valider l’éditeur ;
- valider le runtime.

Commit recommandé :

```text
Add object system test level
```

---

## Phase 9 — Nettoyage

Après validation :

- supprimer les anciens liens spécifiques devenus inutiles ;
- retirer les helpers bouton/porte trop spécialisés ;
- nettoyer les logs obsolètes ;
- documenter les conventions d’archétypes ;
- vérifier les Blueprints.

Commit recommandé :

```text
Clean legacy object activation code
```

---

## Critères de réussite

Le chantier est réussi si :

- les objets concrets restent visibles dans la palette ;
- les comportements communs sont factorisés ;
- le runtime ne dépend plus de cas particuliers bouton -> porte ;
- les liens sont lisibles ;
- les DataAssets restent compréhensibles ;
- le projet compile ;
- une carte de test démontre les cas principaux.

