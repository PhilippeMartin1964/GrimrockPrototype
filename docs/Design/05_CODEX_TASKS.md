# GrimrockPrototype — Stratégie d'utilisation de Codex et tâches historiques

**Document actif pour la stratégie ; exemples de tâches ci-dessous historiques.**
La présente stratégie complète `AGENTS.md` et `docs/PROJECT_WORKFLOW_RULES.md`.
En cas de contradiction avec un ancien prompt ou une ancienne « branche recommandée », **les règles actuelles prévalent : `master` exclusivement, un seul commit final maximum par étape, aucun commit intermédiaire**.

## 1. Pourquoi réserver Codex

Le projet utilise Unreal Engine 5.5.4 sous Windows, à l'emplacement local
`D:\Development\GrimrockPrototype` (moteur : `D:\UE_5.5`).
Une session Codex peut consommer rapidement le quota d'utilisation inclus dans
l'abonnement, surtout avec des audits ouverts, des parcours de dépôt étendus et
des compilations/tests répétés. **Ne pas présumer qu'un nombre fixe de messages
ou de crédits sera disponible** : consulter les limites affichées par le
produit au moment de l'utilisation.

Codex n'est **plus l'outil obligatoire pour chaque jalon**. Il sert surtout
quand un agent disposant de l'ordinateur local doit itérer de façon autonome
sur une difficulté qui requiert l'exécution réelle du projet.

## 2. Répartition recommandée

| Travail | Responsable / environnement |
| --- | --- |
| Cahier des charges, conception, revue de règles et documentation | ChatGPT dans le projet |
| Lecture du dépôt, audit statique, architecture, correctif ciblé et patch | ChatGPT + GitHub |
| Validation avant publication : compilation UBT, Automation, PIE et assets binaires | Installation UE locale, avec l'utilisateur |
| Commit final atomique / push / vérification de `origin/master` | Git local après validation ; assistance GitHub selon les outils disponibles |
| Blocage multi-fichiers complexe, diagnostic dépendant d'UHT, du runtime ou de Windows, reproduction autonome | Codex local, exceptionnellement |

Depuis une conversation ChatGPT sans accès à `D:\UE_5.5`, **ne jamais
annoncer une compilation UE, une recette PIE ni une matérialisation de
DataAsset comme effectuée**. Préparer un patch, le faire valider localement,
puis publier. Ne pas utiliser GitHub pour pousser plusieurs corrections
spéculatives successives.

Le choix du modèle Codex est proportionnel à la difficulté : utiliser un
modèle/effort plus économique pour une tâche fermée ; réserver les modèles à
fort raisonnement aux enquêtes complexes. Les noms, disponibilités et quotas
des modèles évoluent : les vérifier dans l'interface plutôt que figer ici
une recommandation commerciale.

## 3. Quand déclencher Codex

**Utiliser ChatGPT + validation locale** pour :
- les changements documentaires, les réglages de DataAssets et de Material
  Instances, les ajustements C++ localisés et les tests ciblés ;
- les tâches où un diff relisible peut être appliqué puis compilé en une fois.

**Utiliser Codex** pour :
- un défaut qui persiste après une première investigation raisonnable ;
- un problème où le comportement runtime, le code généré ou les dépendances
  de plusieurs modules doivent être vérifiés sur la machine ;
- une investigation dont la conclusion exige plusieurs compilations ou tests
  locaux que l'utilisateur ne devrait pas orchestrer à répétition.

**Exemple vérifié — TRAP01.5** : les méthodes C++ directes de l'interaction
réussissaient mais `IGridInteractableInterface::Execute_*` échouait. La cause
était un monde transitoire de test dont les acteurs n'étaient pas correctement
initialisés : `ProcessEvent` n'exécutait pas l'événement comme en jeu.
La correction a porté sur le fixture du test, pas sur les règles du piège.
**Toujours auditer la validité du test et son monde UE avant de conclure
à un défaut de gameplay ou de réflexion Unreal.**

## 4. Contrat d'une tâche Codex économique et vérifiable

1. Relever `git status --short`, `git branch --show-current` et le HEAD de
   `origin/master` ; préserver tous les changements préexistants.
2. Délimiter **un seul objectif**, des chemins/fichiers précis et les
   comportements interdits ; éviter « nettoie le dépôt » ou « corrige tout ».
3. Exiger une hypothèse et une preuve de la cause racine avant de refactoriser.
   Lire les journaux antérieurs pour éviter les mêmes compilations inutiles.
4. Imposer un diff minimal, des tests de régression ciblés, et le respect de
   `AGENTS.md`, des DataAssets, de la localisation et des WBP.
5. Compiler une seule fois après stabilisation du patch ; exécuter ensuite
   plusieurs filtres Automation avec `-SkipBuild` lorsque possible.
6. **Aucun commit tant que compilation/tests requis ne sont pas réussis.**
   Après succès : un commit atomique final sur `master`, push et preuve du SHA ;
   si PIE reste nécessaire, l'indiquer explicitement comme non validé.
7. Exiger un bilan court : cause, fichiers, tests réellement exécutés,
   rapports, SHA et recette manuelle restante. Ne pas relancer l'agent
   pour de simples reformulations du compte rendu.

Commandes de base depuis le poste de développement :

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git branch --show-current
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP01.2"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01.4"
git diff --check
```

Les filtres ci-dessus sont des **exemples**, pas une matrice universelle :
choisir ceux qui correspondent au jalon en cours. Ne jamais écraser les
changements utilisateur, réinitialiser `master` ou modifier des fichiers
`.uasset` hors des outils Unreal.

## 5. Anciens prompts conservés pour mémoire

Les exemples qui suivent reflètent une ancienne organisation du dépôt :
leurs noms de classes, chemins `Docs/` et propositions de branches
`feature/*` **ne sont pas des consignes applicables aujourd'hui**.
Les adapter à l'architecture réelle avant utilisation ; ne jamais créer
de branche pour les exécuter.

---

## Tâche Codex 01 — Ajouter Events et Commands

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

### Branche historique — ne pas créer

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

