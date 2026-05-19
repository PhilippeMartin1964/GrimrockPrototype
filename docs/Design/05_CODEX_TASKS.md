# GrimrockPrototype — Tâches Codex

## Objectif

Ce document contient des prompts prêts à donner à Codex.

Principe : Codex doit recevoir des tâches courtes, fermées, avec fichiers autorisés et contraintes explicites.

Ne pas demander à Codex de refactoriser tout le système d’un coup.

---

## Règles d’utilisation de Codex

Pour chaque tâche :

1. créer une branche Git dédiée ;
2. donner le contexte ;
3. limiter les fichiers autorisés ;
4. demander un diff minimal ;
5. compiler localement ;
6. tester dans UE5 ;
7. committer ;
8. mettre à jour `99_DECISIONS_LOG.md`.

---

## Tâche Codex 01 — Ajouter Events et Commands

### Branche recommandée

```text
feature/grid-events-commands
```

### Prompt

```text
Contexte :
Projet GrimrockPrototype UE5.5.4 C++.
Architecture validée dans Docs/Design/01_GRID_OBJECT_SYSTEM.md et Docs/Design/03_EVENT_COMMAND_LINKS.md.

Objectif :
Ajouter les enums EGridObjectEvent et EGridObjectCommand.

Contraintes :
- Ne pas supprimer les catégories existantes.
- Ne pas supprimer Spawn.
- Ne pas supprimer Readable.
- Ne pas renommer WallInscription.
- Ne pas modifier le runtime.
- Ne pas modifier les Blueprints.
- Ne pas faire de refactor massif.
- Préserver la compilation Unreal.

Fichiers autorisés :
- Source/GrimrockPrototype/Public/Core/GridTypes.h
- éventuellement fichiers strictement nécessaires si GridTypes.h dépend d’autres types

Livrable attendu :
- Diff minimal.
- Explication courte.
- Points à vérifier dans UE5.
```

---

## Tâche Codex 02 — Ajouter FGridObjectLink

### Branche recommandée

```text
feature/grid-object-links
```

### Prompt

```text
Contexte :
Projet GrimrockPrototype UE5.5.4 C++.
Les enums EGridObjectEvent et EGridObjectCommand existent ou doivent être utilisés selon Docs/Design/03_EVENT_COMMAND_LINKS.md.

Objectif :
Ajouter une structure FGridObjectLink permettant de représenter :
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand + Delay + bOneShot.

Contraintes :
- Ne pas brancher encore le runtime.
- Ne pas modifier les Blueprints.
- Ne pas supprimer l’ancien système de liens si existant.
- Ne pas faire de migration automatique.
- Préserver la compilation Unreal.

Fichiers autorisés :
- Source/GrimrockPrototype/Public/Core/GridTypes.h
- éventuellement un nouveau fichier Core/GridObjectLink.h si cela paraît plus propre, mais préférer GridTypes.h si le projet est encore simple

Livrable attendu :
- Diff minimal.
- Explication courte.
- Points à vérifier dans UE5.
```

---

## Tâche Codex 03 — Exposer Events / Commands dans les archétypes

### Branche recommandée

```text
feature/archetype-events-commands
```

### Prompt

```text
Contexte :
Projet GrimrockPrototype UE5.5.4 C++.
Les objets concrets restent distincts dans les DataAssets, mais peuvent partager des classes runtime.
Voir Docs/Design/02_OBJECT_ARCHETYPES.md.

Objectif :
Adapter UGridObjectArchetypeAsset pour exposer :
- Category si pas déjà proprement exposé ;
- ActorClass si pas déjà présent ;
- InitialState ;
- EmittedEvents ;
- AcceptedCommands.

Contraintes :
- Préserver la compatibilité avec les DataAssets existants.
- Ne pas renommer les champs existants sans nécessité.
- Ne pas casser les Blueprints.
- Ne pas modifier le runtime.
- Ne pas supprimer les catégories existantes.
- Ne pas renommer WallInscription.

Fichiers autorisés :
- Source/GrimrockPrototype/Public/Core/GridObjectArchetypeAsset.h
- Source/GrimrockPrototype/Private/Core/GridObjectArchetypeAsset.cpp
- ou les noms réels équivalents dans le projet
- Source/GrimrockPrototype/Public/Core/GridTypes.h si nécessaire

Livrable attendu :
- Diff minimal.
- Explication des champs ajoutés.
- Points à vérifier dans les DataAssets UE5.
```

---

## Tâche Codex 04 — Préparer le dispatcher runtime

### Branche recommandée

```text
feature/runtime-event-dispatcher
```

### Prompt

```text
Contexte :
Projet GrimrockPrototype UE5.5.4 C++.
Le modèle validé est :
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand.
Voir Docs/Design/03_EVENT_COMMAND_LINKS.md.

Objectif :
Créer ou consolider un composant UGridActivationComponent capable de :
- indexer les objets runtime par ObjectId ;
- recevoir EmitEvent(SourceObjectId, Event) ;
- chercher les FGridObjectLink correspondants ;
- appeler ExecuteCommand(TargetObjectId, Command) ;
- gérer Delay ;
- gérer bOneShot ;
- produire des logs utiles.

Contraintes :
- Ne pas modifier tous les acteurs en même temps.
- Ne pas faire de refactor massif.
- Ne pas supprimer l’ancien système si cela risque de casser le jeu.
- Prévoir une intégration progressive.
- Les objets ne doivent pas se connaître directement.
- Préserver la compilation Unreal.

Fichiers autorisés :
- Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h
- Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp
- Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h si nécessaire
- Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp si nécessaire
- Source/GrimrockPrototype/Public/Core/GridTypes.h si nécessaire

Livrable attendu :
- Diff minimal.
- Explication du flux runtime.
- Points à vérifier en PIE.
```

---

## Tâche Codex 05 — Brancher Button et Door

### Branche recommandée

```text
feature/button-door-event-command
```

### Prompt

```text
Contexte :
Le dispatcher Event -> Command existe.
Objectif : faire un premier test minimal :
Button.OnActivate -> Door.ToggleOpen.

Objectif :
Adapter AGridButtonActor pour émettre OnActivate.
Adapter AGridDoorActor pour accepter ToggleOpen, Open, Close.

Contraintes :
- Ne brancher que Button et Door.
- Ne pas modifier Lever, PressurePlate, Receptacle pour l’instant.
- Ne pas supprimer l’ancien comportement si cela casse les tests existants ; le désactiver seulement si sûr.
- Ajouter des logs utiles.
- Préserver la compilation Unreal.

Fichiers autorisés :
- AGridButtonActor .h/.cpp
- AGridDoorActor .h/.cpp
- UGridActivationComponent .h/.cpp
- AGridLevelRuntimeActor .h/.cpp si nécessaire

Livrable attendu :
- Diff minimal.
- Explication courte.
- Test PIE recommandé.
```

---

## Tâche Codex 06 — Brancher Lever et PressurePlate

### Branche recommandée

```text
feature/lever-pressureplate-events
```

### Prompt

```text
Contexte :
Button et Door fonctionnent avec Event -> Command.

Objectif :
Adapter :
- AGridLeverActor : OnActivate, OnDeactivate, OnToggle
- AGridPressurePlateActor : OnActivate, OnDeactivate, OnToggle

Contraintes :
- Ne pas modifier Receptacle, Timer, Teleporter.
- Préserver le comportement existant autant que possible.
- Ajouter des logs utiles.
- Préserver la compilation Unreal.

Fichiers autorisés :
- AGridLeverActor .h/.cpp
- AGridPressurePlateActor .h/.cpp
- UGridActivationComponent .h/.cpp si nécessaire

Livrable attendu :
- Diff minimal.
- Test PIE recommandé.
```

---

## Tâche Codex 07 — Créer / consolider AGridReceptacleActor

### Branche recommandée

```text
feature/receptacle-base
```

### Prompt

```text
Contexte :
Les réceptacles concrets validés sont :
- Alcove
- TorchHolder
- Altar
- OfferingBowl
- CoinSlot
- Lock/Keyhole

Ils peuvent partager AGridReceptacleActor.
Voir Docs/Design/02_OBJECT_ARCHETYPES.md.

Objectif :
Créer ou consolider AGridReceptacleActor pour gérer :
- ajout d’item ;
- retrait d’item ;
- filtre RequiredItemId / RequiredItemTag ;
- bConsumeInsertedItem ;
- bAllowItemRemoval ;
- bDisplayInsertedItem ;
- émission OnInsertItem ;
- émission OnRemoveItem.

Contraintes :
- Ne pas implémenter encore tout l’inventaire si absent.
- Prévoir des stubs propres si nécessaire.
- Ne pas coder la torche en dur dans le réceptacle.
- Le support de torche doit rester un réceptacle paramétré.
- Préserver la compilation Unreal.

Fichiers autorisés :
- AGridReceptacleActor .h/.cpp
- GridTypes.h si nécessaire
- GridObjectArchetypeAsset si nécessaire

Livrable attendu :
- Diff minimal.
- Explication des paramètres.
- Tests recommandés.
```

---

## Tâche Codex 08 — Éditeur : afficher Events / Commands / ActorClass

### Branche recommandée

```text
feature/editor-object-inspector-events
```

### Prompt

```text
Contexte :
Les archétypes exposent Category, ActorClass, InitialState, EmittedEvents et AcceptedCommands.

Objectif :
Adapter l’inspecteur d’objet de l’éditeur pour afficher :
- ObjectId
- ArchetypeId
- Category
- ActorClass
- InitialState
- EmittedEvents
- AcceptedCommands
- Outgoing Links
- Incoming Links

Contraintes :
- Ne pas refaire toute l’UI.
- Ne pas casser les outils Paint Cell / Paint Wall / Paint Object / Erase.
- Garder l’affichage lisible.
- Préserver la compilation Unreal.

Fichiers autorisés :
- fichiers EditorTools liés à GridLevelEditorActor / Toolkit / Inspector
- fichiers Core nécessaires en lecture

Livrable attendu :
- Diff minimal.
- Capture ou description des vérifications UE5.
```

---

## Tâche Codex 09 — Éditeur : création de liens

### Branche recommandée

```text
feature/editor-object-link-creation
```

### Prompt

```text
Contexte :
Le système FGridObjectLink existe.
L’inspecteur affiche les objets et leurs Events/Commands.

Objectif :
Ajouter une interface simple pour créer un lien :
Source Event -> Target Object -> Target Command -> Delay -> One Shot.

Contraintes :
- Ne pas créer un éditeur graphique complexe.
- Une UI simple dans l’inspecteur suffit.
- Les liens doivent être stockés dans le DataAsset de niveau ou la structure existante appropriée.
- Ne pas casser les liens existants s’il y en a.
- Préserver la compilation Unreal.

Fichiers autorisés :
- EditorTools concernés
- GridLevelAsset / GridTypes si nécessaire

Livrable attendu :
- Diff minimal.
- Explication de l’usage côté éditeur.
- Tests recommandés.
```

---

## Tâche Codex 10 — Nettoyage final

### Branche recommandée

```text
feature/cleanup-legacy-activation
```

### Prompt

```text
Contexte :
Le système Event -> Command est fonctionnel pour Button, Lever, PressurePlate, Door, SecretDoor et Receptacle.

Objectif :
Identifier et supprimer uniquement le code legacy devenu inutile :
- liens directs bouton -> porte ;
- helpers trop spécifiques ;
- duplications entre Door et SecretDoor ;
- logs obsolètes.

Contraintes :
- Ne rien supprimer sans justification.
- Ne pas casser les tests existants.
- Faire un rapport des suppressions proposées avant gros changement.
- Préserver la compilation Unreal.

Fichiers autorisés :
- à déterminer après analyse

Livrable attendu :
- Liste des éléments legacy trouvés.
- Diff minimal.
- Justification de chaque suppression.
```

