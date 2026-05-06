# Architecture Runtime / Editor Split

## Objectif

Ce document suit la séparation progressive du projet Unreal Engine 5.5.4 `GrimrockPrototype` en deux modules :

- `GrimrockPrototype` : module runtime contenant les données, le gameplay, la génération de niveau et les objets jouables ;
- `GrimrockPrototypeEditor` : module editor contenant les outils Unreal Editor, le mode d'édition de grille et l'interface Slate associée.

Il sert à la fois d'historique de décision, d'état des lieux et de checklist restante pour continuer la migration sans casser les assets existants.

## 1. État actuel

### Modules Unreal

Le projet contient maintenant deux modules C++ :

```text
Source/GrimrockPrototype/
Source/GrimrockPrototypeEditor/
```

`GrimrockPrototype` reste déclaré comme module `Runtime`.

`GrimrockPrototypeEditor` existe maintenant comme module `Editor` et est chargé avec :

```text
LoadingPhase = PreDefault
```

Ce chargement précoce est volontaire afin que les classes et l'enregistrement du mode editor soient disponibles au bon moment dans Unreal Editor.

### Cible Editor

Le fichier suivant existe :

```text
Source/GrimrockPrototypeEditor.Target.cs
```

Il référence maintenant le module editor afin de compiler l'éditeur avec `GrimrockPrototypeEditor`.

### Module runtime GrimrockPrototype

Le runtime ne contient plus l'enregistrement du mode éditeur.

`Source/GrimrockPrototype/GrimrockPrototype.cpp` est redevenu minimal :

```text
IMPLEMENT_PRIMARY_GAME_MODULE(...)
```

Le runtime ne dépend plus explicitement de :

```text
UnrealEd
EditorFramework
Slate
SlateCore
```

Les dépendances attendues du module runtime sont :

```text
Core
CoreUObject
Engine
InputCore
EnhancedInput
```

### Module editor GrimrockPrototypeEditor

Le module editor dépend du runtime :

```text
GrimrockPrototypeEditor -> GrimrockPrototype
```

Cette direction est correcte. Le runtime ne doit jamais dépendre du module editor.

Le module editor contient maintenant :

```text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdMode.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdMode.cpp
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdModeToolkit.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEditorActor.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActor.cpp
```

`GrimrockPrototypeEditor.cpp` est responsable de l'enregistrement et du désenregistrement de `FGridLevelEdMode`.

### CoreRedirect actif

`AGridLevelEditorActor` a changé de module, donc son chemin script Unreal est passé de :

```text
/Script/GrimrockPrototype.GridLevelEditorActor
```

vers :

```text
/Script/GrimrockPrototypeEditor.GridLevelEditorActor
```

Un `CoreRedirect` existe dans `Config/DefaultEngine.ini` pour préserver les références existantes :

```ini
+ClassRedirects=(OldName="/Script/GrimrockPrototype.GridLevelEditorActor",NewName="/Script/GrimrockPrototypeEditor.GridLevelEditorActor")
```

Ce redirect doit être conservé tant que les assets concernés n'ont pas été ouverts, validés et resauvegardés avec la nouvelle classe.

### Classes preview restantes

Les classes suivantes restent volontairement dans le runtime pour l'instant :

```text
Source/GrimrockPrototype/Public/Runtime/GridEditorPreviewComponent.h
Source/GrimrockPrototype/Private/Runtime/GridEditorPreviewComponent.cpp
Source/GrimrockPrototype/Public/Runtime/GridEditorPreviewObjectActor.h
Source/GrimrockPrototype/Private/Runtime/GridEditorPreviewObjectActor.cpp
```

Elles sont encore référencées par `AGridLevelRuntimeActor` et potentiellement par des Blueprints ou maps editor. Leur migration doit rester une étape séparée.

## 2. Historique de migration

### Étape 1 : documentation

Statut : terminé.

Ce document a été créé pour cadrer la séparation `Runtime / Editor`.

### Étape 2 : création du module editor vide

Statut : terminé.

Créé :

```text
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.Build.cs
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp
```

Le module `GrimrockPrototypeEditor` est déclaré dans `GrimrockPrototype.uproject` avec `Type = Editor`.

### Étape 3 : migration du EdMode et du Toolkit

Statut : terminé.

Déplacé vers `GrimrockPrototypeEditor` :

```text
GridLevelEdMode.h/.cpp
GridLevelEdModeToolkit.h/.cpp
```

L'enregistrement du mode éditeur a été retiré du runtime et déplacé dans le module editor.

### Étape 4 : migration de GridLevelEditorActor

Statut : terminé.

Déplacé vers `GrimrockPrototypeEditor` :

```text
GridLevelEditorActor.h/.cpp
```

Un `CoreRedirect` a été ajouté pour préserver les références `/Script/GrimrockPrototype.GridLevelEditorActor`.

### Étape 5 : nettoyage des dépendances editor du runtime

Statut : terminé.

Nettoyé :

- retrait de `UnrealEd`, `EditorFramework`, `Slate`, `SlateCore` du module runtime ;
- retrait de `UnrealEd` des `AdditionalDependencies` du module runtime dans `.uproject`.

### Étape 6 : traitement des classes preview

Statut : restant.

Les classes de preview sont encore dans le runtime par décision prudente. Elles doivent être traitées plus tard, après validation des références Blueprint et map.

## 3. Classification actuelle des fichiers C++

### Légende

- `Runtime` : doit rester dans le module runtime.
- `Editor` : appartient maintenant ou devrait appartenir au module editor.
- `Ambigu / à décider` : reste temporairement dans runtime, mais demande une décision explicite.

| Fichier | Classification | Rôle | État |
|---|---:|---|---|
| `Source/GrimrockPrototype/GrimrockPrototype.Build.cs` | Runtime | Règles de build runtime | Nettoyé, sans dépendances editor explicites. |
| `Source/GrimrockPrototype/GrimrockPrototype.cpp` | Runtime | Module principal runtime | Minimal, sans enregistrement editor. |
| `Public/Core/GridTypes.h` | Runtime | Types de cellules, murs, objets, liens et directions | Stable. |
| `Public/Core/GridDirectionUtils.h` | Runtime | Helpers de direction grille | Stable. |
| `Public/Core/GridLevelAsset.h` | Runtime | Asset de niveau, cellules, objets, liens | Données partagées runtime/editor. |
| `Private/Core/GridLevelAsset.cpp` | Runtime | Implémentation de l'asset de niveau | Contient `WITH_EDITOR` pour `Modify` / `MarkPackageDirty`, acceptable. |
| `Public/Core/GridObjectArchetypeAsset.h` | Runtime | Archétypes data-driven d'objets | Partagé runtime/editor. |
| `Public/Core/GridObjectBehavior.h` | Runtime | Paramètres de comportement d'objet | Partagé runtime/editor. |
| `Public/Core/GridObjectPaletteAsset.h` | Runtime | Palette d'objets pour l'édition | Peut rester runtime comme DataAsset partagé. |
| `Public/Runtime/GridLevelRuntimeActor.h` | Runtime | Génération niveau, gameplay, interactions | Contient encore des références preview. |
| `Private/Runtime/GridLevelRuntimeActor.cpp` | Runtime | Implémentation du niveau jouable | Contient encore la logique preview editor-world. |
| `Public/Runtime/GrimrockPartyPawn.h` | Runtime | Pawn joueur case par case | Gameplay pur. |
| `Private/Runtime/GrimrockPartyPawn.cpp` | Runtime | Déplacement, interaction, caméra, inventaire simple | Gameplay pur. |
| `Public/Runtime/GridRuntimeObjectActor.h` | Runtime | Base des objets runtime placés sur la grille | Gameplay pur. |
| `Private/Runtime/GridRuntimeObjectActor.cpp` | Runtime | Implémentation objet runtime | Gameplay pur. |
| `Public/Runtime/GridMechanismActor.h` | Runtime | Base des mécanismes animables | Gameplay pur. |
| `Private/Runtime/GridMechanismActor.cpp` | Runtime | Implémentation commune des mécanismes | Gameplay pur. |
| `Public/Runtime/GridDoorActor.h` | Runtime | Porte animable | Gameplay pur. |
| `Private/Runtime/GridDoorActor.cpp` | Runtime | Animation ouverture/fermeture | Gameplay pur. |
| `Public/Runtime/GridSecretDoorActor.h` | Runtime | Porte secrète spécialisée | Gameplay pur. |
| `Private/Runtime/GridSecretDoorActor.cpp` | Runtime | Implémentation porte secrète | Gameplay pur. |
| `Public/Runtime/GridButtonActor.h` | Runtime | Bouton mural/interactif | Gameplay pur. |
| `Private/Runtime/GridButtonActor.cpp` | Runtime | Animation/action bouton | Gameplay pur. |
| `Public/Runtime/GridLeverActor.h` | Runtime | Levier | Gameplay pur. |
| `Private/Runtime/GridLeverActor.cpp` | Runtime | Animation/action levier | Gameplay pur. |
| `Public/Runtime/GridPressurePlateActor.h` | Runtime | Plaque de pression | Gameplay pur. |
| `Private/Runtime/GridPressurePlateActor.cpp` | Runtime | Activation/désactivation plaque | Gameplay pur. |
| `Public/Runtime/GridTriggerActor.h` | Runtime | Trigger de cellule | Gameplay pur. |
| `Private/Runtime/GridTriggerActor.cpp` | Runtime | Trigger caché runtime | Gameplay pur. |
| `Public/Runtime/GridReceptacleActor.h` | Runtime | Réceptacle d'objet | Gameplay pur. |
| `Private/Runtime/GridReceptacleActor.cpp` | Runtime | Insertion/retrait d'objet | Gameplay pur. |
| `Public/Runtime/GridActivationComponent.h` | Runtime | Indexation et exécution des liens | Gameplay pur. |
| `Private/Runtime/GridActivationComponent.cpp` | Runtime | Activation objets, plaques, triggers, réceptacles | Gameplay pur. |
| `Public/Runtime/GridDoorSystemComponent.h` | Runtime | Contrôle centralisé des portes | Gameplay pur. |
| `Private/Runtime/GridDoorSystemComponent.cpp` | Runtime | État portes et blocage de passage | Gameplay pur. |
| `Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEditorActor.h` | Editor | Acteur d'édition du niveau | Migré. |
| `Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActor.cpp` | Editor | Peinture, sélection, liens, édition asset | Migré. |
| `Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdMode.h` | Editor | Mode éditeur Unreal personnalisé | Migré. |
| `Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdMode.cpp` | Editor | Interaction viewport du mode éditeur | Migré. |
| `Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEdModeToolkit.h` | Editor | Toolkit Slate du mode éditeur | Migré. |
| `Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp` | Editor | UI Slate du Grimrock Grid Editor | Migré. |
| `Public/Runtime/GridEditorPreviewComponent.h` | Ambigu / à décider | Preview d'objets dans l'éditeur | Reste volontairement runtime pour l'instant. |
| `Private/Runtime/GridEditorPreviewComponent.cpp` | Ambigu / à décider | Spawn/hover/sélection preview | Reste volontairement runtime pour l'instant. |
| `Public/Runtime/GridEditorPreviewObjectActor.h` | Ambigu / à décider | Acteur preview editor-only | Reste volontairement runtime pour l'instant. |
| `Private/Runtime/GridEditorPreviewObjectActor.cpp` | Ambigu / à décider | Mesh preview, stencil hover/sélection | Reste volontairement runtime pour l'instant. |

## 4. Structure cible

### Runtime

```text
Source/GrimrockPrototype/
├── GrimrockPrototype.Build.cs
├── GrimrockPrototype.cpp
├── Public/
│   ├── Core/
│   └── Runtime/
└── Private/
    ├── Core/
    └── Runtime/
```

Responsabilités :

- format de grille ;
- `UGridLevelAsset` ;
- archétypes et comportements ;
- génération de géométrie runtime ;
- pawn joueur ;
- objets jouables ;
- systèmes d'activation et de portes ;
- logique standalone.

### Editor

```text
Source/GrimrockPrototypeEditor/
├── GrimrockPrototypeEditor.Build.cs
├── GrimrockPrototypeEditor.cpp
├── Public/
│   └── EditorTools/
└── Private/
    └── EditorTools/
```

Responsabilités :

- enregistrement du `GridLevelEdMode` ;
- mode éditeur Unreal ;
- toolkit Slate ;
- acteur d'édition ;
- logique utilisant `GEditor`, `FEditorModeRegistry`, `FModeToolkit`, `Slate`, `UnrealEd`.

## 5. Dépendances attendues

### Module runtime `GrimrockPrototype`

Dépendances attendues :

```text
Core
CoreUObject
Engine
InputCore
EnhancedInput
```

Dépendances interdites ou à éviter dans le runtime :

```text
UnrealEd
EditorFramework
Slate
SlateCore
LevelEditor
EditorStyle
ToolMenus
```

### Module editor `GrimrockPrototypeEditor`

Dépendances attendues :

```text
Core
CoreUObject
Engine
InputCore
UnrealEd
EditorFramework
Slate
SlateCore
GrimrockPrototype
```

Dépendances additionnelles possibles selon l'évolution du toolkit :

```text
LevelEditor
EditorStyle
Projects
ApplicationCore
ToolMenus
```

Elles doivent rester dans `GrimrockPrototypeEditor`, pas dans `GrimrockPrototype`.

## 6. Checklist restante

### Validation immédiate

- Compiler `GrimrockPrototypeEditor`.
- Ouvrir le projet dans Unreal Editor.
- Vérifier que le mode `Grimrock Grid Editor` apparaît toujours.
- Ouvrir `L_GrimrockEditor`.
- Vérifier que `BP_GridLevelEditorActor` garde sa classe parent.
- Vérifier que le `CoreRedirect` ne produit pas d'erreur au chargement.
- Ouvrir `L_GrimrockRuntime`.
- Vérifier le déplacement, `Use`, portes, boutons, leviers, plaques, triggers et réceptacles.
- Lancer un build standalone/development pour confirmer l'absence de dépendance runtime à `UnrealEd`.

### Traitement des preview classes

Classes concernées :

```text
GridEditorPreviewComponent
GridEditorPreviewObjectActor
```

Options possibles :

1. les migrer vers `GrimrockPrototypeEditor` ;
2. les garder dans runtime mais les renommer pour clarifier leur rôle ;
3. les garder temporairement dans runtime avec garde-fous `WITH_EDITOR` plus stricts ;
4. extraire une interface minimale runtime et déplacer seulement le comportement editor.

Recommandation actuelle :

- ne pas les déplacer encore ;
- inventorier d'abord les références Blueprint et map ;
- vérifier `BP_GridEditorPreviewObjectActor` et `L_GrimrockEditor` ;
- déplacer dans un commit séparé avec `CoreRedirects` si nécessaire.

### Nettoyage futur

- Vérifier si le `CoreRedirect` de `GridLevelEditorActor` peut rester permanent ou être retiré après resauvegarde des assets.
- Réduire les références preview dans `AGridLevelRuntimeActor`.
- Séparer clairement la génération jouable de la prévisualisation editor.
- Continuer à garder les commits petits et validables dans Unreal Editor.

## 7. Risques Unreal

### Références Blueprint cassées

Déplacer une `UCLASS` vers un autre module change son chemin script Unreal.

Risque :

- Blueprints dérivés invalides ;
- variables de classe perdues ;
- références d'assets cassées.

Mitigation :

- déplacer peu de classes à la fois ;
- utiliser `CoreRedirects` ;
- ouvrir et resauvegarder les assets concernés ;
- tester les maps immédiatement après chaque déplacement.

### Includes incorrects

La séparation de modules révèle les includes implicites.

Mitigation :

- préférer les forward declarations dans les headers ;
- inclure les headers complets dans les `.cpp` ;
- garder l'API publique runtime minimale.

### Dépendances circulaires

Le module editor dépend du runtime. Le runtime ne doit jamais dépendre du module editor.

Règle :

```text
GrimrockPrototypeEditor -> GrimrockPrototype
GrimrockPrototype -> GrimrockPrototypeEditor interdit
```

### Module editor non chargé

Si `GrimrockPrototypeEditor` n'est pas chargé assez tôt, le mode éditeur peut ne pas apparaître.

Mitigation actuelle :

- module déclaré `Type = Editor` ;
- `LoadingPhase = PreDefault`.

### Classes preview

`GridEditorPreviewComponent` et `GridEditorPreviewObjectActor` restent le principal point ambigu.

Risque :

- responsabilités editor dans le runtime ;
- confusion entre preview et gameplay ;
- références Blueprint fragiles en cas de déplacement.

Mitigation :

- migration séparée ;
- analyse des assets avant déplacement ;
- `CoreRedirects` si les classes changent de module.

## Conclusion

La séparation `Runtime / Editor` a nettement avancé :

- le module editor existe ;
- le EdMode et son Toolkit sont migrés ;
- `AGridLevelEditorActor` est migré ;
- l'enregistrement editor n'est plus dans le runtime ;
- les dépendances editor explicites ont été retirées du runtime ;
- `GrimrockPrototypeEditor` est chargé en `PreDefault` ;
- un `CoreRedirect` protège `GridLevelEditorActor`.

La prochaine décision d'architecture concerne les classes preview. Elles restent volontairement dans le runtime pour l'instant, afin de préserver les références Blueprint et map pendant que le gameplay runtime continue de se stabiliser.
