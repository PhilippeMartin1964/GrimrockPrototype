# Architecture Runtime / Editor Split

## Objectif

Ce document prépare la séparation progressive du projet Unreal Engine 5.5.4 `GrimrockPrototype` en deux modules :

- un module runtime contenant le jeu, les données, la génération de niveau et les objets jouables ;
- un module editor contenant les outils d'édition Unreal, le mode d'édition de grille et l'interface Slate associée.

Le but est de rendre le projet plus propre pour un futur build standalone, sans casser l'édition actuelle du niveau 32x32.

## 1. Etat actuel

### Module existant

Le projet contient actuellement un seul module C++ principal :

```text
Source/GrimrockPrototype/
```

Ce module est déclare comme module `Runtime` dans `GrimrockPrototype.uproject`.

### Presence de GrimrockPrototypeEditor.Target.cs

Le fichier suivant existe :

```text
Source/GrimrockPrototypeEditor.Target.cs
```

Il permet de compiler une cible Editor, mais il ne correspond pas encore a un module C++ séparé `GrimrockPrototypeEditor`.

### Dépendances éditeur dans GrimrockPrototype.Build.cs

Le fichier suivant contient déjà une séparation conditionnelle partielle :

```text
Source/GrimrockPrototype/GrimrockPrototype.Build.cs
```

Dépendances runtime actuelles :

```text
Core
CoreUObject
Engine
InputCore
EnhancedInput
```

Dépendances editor ajoutées seulement quand `Target.bBuildEditor` est vrai :

```text
UnrealEd
EditorFramework
Slate
SlateCore
```

Cette approche limite les dépendances editor en build non-editor, mais le module principal contient encore du code editor.

### Code d'enregistrement du mode éditeur

Le fichier suivant enregistre actuellement le mode éditeur :

```text
Source/GrimrockPrototype/GrimrockPrototype.cpp
```

Il inclut `EditorModeRegistry.h` et `EditorTools/GridLevelEdMode.h` sous `#if WITH_EDITOR`, puis appelle :

```text
FEditorModeRegistry::Get().RegisterMode<FGridLevelEdMode>(...)
```

Cet enregistrement devrait à terme migrer dans le futur module `GrimrockPrototypeEditor`.

### Point incoherent actuel

`GrimrockPrototype.uproject` mentionne encore `UnrealEd` dans les `AdditionalDependencies` du module `GrimrockPrototype`, alors que ce module est declaré comme `Runtime`.

Ce point sera à nettoyer après création et validation du module editor.

## 2. Classification des fichiers C++

### Legende

- `Runtime` : doit rester dans le module runtime.
- `Editor` : devrait migrer vers un module editor.
- `Ambigu / à decider` : peut rester temporairement dans runtime, mais mérite une décision explicite.

| Fichier | Classification | Role probable | Remarques |
|---|---:|---|---|
| `Source/GrimrockPrototype/GrimrockPrototype.Build.cs` | Ambigu / à decider | Règles de build du module actuel | A nettoyer après création du module editor. |
| `Source/GrimrockPrototype/GrimrockPrototype.cpp` | Ambigu / à decider | Module principal | Contient l'enregistrement du EdMode, qui devrait migrer côté editor. |
| `Public/Core/GridTypes.h` | Runtime | Types de cellules, murs, objets, liens et directions | Coeur du format de niveau. |
| `Public/Core/GridDirectionUtils.h` | Runtime | Helpers de direction grille | Utilitaire runtime pur. |
| `Public/Core/GridLevelAsset.h` | Runtime | Asset de niveau 32x32, cellules, objets, liens | Doit rester accessible au runtime et à l'editor. |
| `Private/Core/GridLevelAsset.cpp` | Runtime | Implémentation de l'asset de niveau | Contient du code `WITH_EDITOR`, acceptable si limite a validation/maintenance editor-safe. |
| `Public/Core/GridObjectArchetypeAsset.h` | Runtime | Archetype data-driven d'objet | Utilise par runtime et editor ; appartient au modèle de données. |
| `Public/Core/GridObjectBehavior.h` | Runtime | Parametres de comportement d'objet | Donnees runtime/editor partagées. |
| `Public/Core/GridObjectPaletteAsset.h` | Runtime | Palette d'objets pour l'édition | Peut rester runtime car c'est un DataAsset partageable, meme si surtout utilise par l'editor. |
| `Public/Runtime/GridLevelRuntimeActor.h` | Runtime | Génération de géometrie, spawn runtime, interactions | Classe centrale du gameplay. |
| `Private/Runtime/GridLevelRuntimeActor.cpp` | Runtime | Implémentation du runtime de niveau | Contient des appels de preview editor ; a isoler progressivement. |
| `Public/Runtime/GrimrockPartyPawn.h` | Runtime | Pawn joueur case par case | Gameplay pur. |
| `Private/Runtime/GrimrockPartyPawn.cpp` | Runtime | Déplacement, interaction, camera, inventaire simple | Gameplay pur. |
| `Public/Runtime/GridRuntimeObjectActor.h` | Runtime | Base des objets runtime places sur la grille | Runtime pur. |
| `Private/Runtime/GridRuntimeObjectActor.cpp` | Runtime | Implémentation de base objet runtime | Runtime pur. |
| `Public/Runtime/GridMechanismActor.h` | Runtime | Base des mécanismes activables/animables | Runtime pur. |
| `Private/Runtime/GridMechanismActor.cpp` | Runtime | Implémentation commune des mecanismes | Runtime pur. |
| `Public/Runtime/GridDoorActor.h` | Runtime | Porte animable | Runtime pur. |
| `Private/Runtime/GridDoorActor.cpp` | Runtime | Animation/ouverture/fermeture de porte | Runtime pur. |
| `Public/Runtime/GridSecretDoorActor.h` | Runtime | Porte secrète specialisee | Runtime pur. |
| `Private/Runtime/GridSecretDoorActor.cpp` | Runtime | Comportement de porte secrète | Runtime pur. |
| `Public/Runtime/GridButtonActor.h` | Runtime | Bouton mural ou interactif | Runtime pur. |
| `Private/Runtime/GridButtonActor.cpp` | Runtime | Animation/action du bouton | Runtime pur. |
| `Public/Runtime/GridLeverActor.h` | Runtime | Levier | Runtime pur. |
| `Private/Runtime/GridLeverActor.cpp` | Runtime | Animation/action du levier | Runtime pur. |
| `Public/Runtime/GridPressurePlateActor.h` | Runtime | Plaque de pression | Runtime pur. |
| `Private/Runtime/GridPressurePlateActor.cpp` | Runtime | Activation/desactivation plaque | Runtime pur. |
| `Public/Runtime/GridTriggerActor.h` | Runtime | Trigger de cellule/evenement | Runtime pur. |
| `Private/Runtime/GridTriggerActor.cpp` | Runtime | Execution du trigger | Runtime pur. |
| `Public/Runtime/GridReceptacleActor.h` | Runtime | Receptacle d'objet, support de torche | Runtime pur. |
| `Private/Runtime/GridReceptacleActor.cpp` | Runtime | Insertion/retrait d'objet, lien avec inventaire | Runtime pur. |
| `Public/Runtime/GridActivationComponent.h` | Runtime | Indexation et execution des liens logiques | Runtime pur. |
| `Private/Runtime/GridActivationComponent.cpp` | Runtime | Activation, evenements, actions liees | Runtime pur. |
| `Public/Runtime/GridDoorSystemComponent.h` | Runtime | Indexation et controle des portes | Runtime pur. |
| `Private/Runtime/GridDoorSystemComponent.cpp` | Runtime | Etat ouvert/ferme, blocage de passage | Runtime pur. |
| `Public/EditorTools/GridLevelEditorActor.h` | Editor | Acteur d'édition du niveau | Devrait migrer vers `GrimrockPrototypeEditor`. |
| `Private/EditorTools/GridLevelEditorActor.cpp` | Editor | Peinture cellules/murs/objets, liens, selection | Utilise `WITH_EDITOR` et `GEditor`. |
| `Public/EditorTools/GridLevelEdMode.h` | Editor | Mode éditeur Unreal personnalise | Editor pur. |
| `Private/EditorTools/GridLevelEdMode.cpp` | Editor | Interaction viewport du mode éditeur | Editor pur. |
| `Public/EditorTools/GridLevelEdModeToolkit.h` | Editor | Toolkit Slate du mode éditeur | Editor pur. |
| `Private/EditorTools/GridLevelEdModeToolkit.cpp` | Editor | UI Slate du Grimrock Grid Editor | Editor pur. |
| `Public/Runtime/GridEditorPreviewComponent.h` | Ambigu / à decider | Preview d'objets en mode editor | Nom et role editor, mais depend de `GridLevelRuntimeActor`. |
| `Private/Runtime/GridEditorPreviewComponent.cpp` | Ambigu / à decider | Spawn/selection/hover des objets preview | Candidat a migrer ou a isoler derrière `WITH_EDITOR`. |
| `Public/Runtime/GridEditorPreviewObjectActor.h` | Ambigu / à decider | Acteur preview editor-only | Marque `bIsEditorOnlyActor` cote Implémentation. |
| `Private/Runtime/GridEditorPreviewObjectActor.cpp` | Ambigu / à decider | Mesh preview, hover, selection stencil | Candidat editor, mais attention aux références Blueprint. |

## 3. Proposition de structure cible

### Module runtime

Structure souhaitée :

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

Responsabilites :

- types de grille ;
- asset de niveau ;
- archetypes et comportements ;
- génération de géometrie runtime ;
- pawn joueur ;
- portes, boutons, leviers, plaques, triggers, receptacles ;
- composants d'activation et de portes ;
- logique jouable en standalone.

### Module editor

Structure souhaitée :

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
- logique de preview si elle reste strictement editor-only ;
- fonctions utilisant `GEditor`, `FEditorModeRegistry`, `FModeToolkit`, `Slate`, `UnrealEd`.

## 4. Dépendances attendues

### Module runtime `GrimrockPrototype`

Dépendances attendues :

```text
Core
CoreUObject
Engine
InputCore
EnhancedInput
```

Dépendances à éviter dans le runtime :

```text
UnrealEd
EditorFramework
Slate
SlateCore
LevelEditor
EditorStyle
UnrealEd-only APIs
```

### Module editor `GrimrockPrototypeEditor`

Dépendances attendues :

```text
Core
CoreUObject
Engine
UnrealEd
EditorFramework
Slate
SlateCore
InputCore
GrimrockPrototype
```

Dépendances possiblement necessaires selon les includes exacts du toolkit :

```text
LevelEditor
EditorStyle
Projects
ApplicationCore
ToolMenus
```

Ces dépendances optionnelles doivent être ajoutées seulement si la compilation les exige.

## 5. Plan de migration en petites etapes

### Etape 1 : documentation

Créer et maintenir ce document.

Objectif :

- clarifier la cible ;
- classer les fichiers ;
- éviter de déplacer du code sans vision stable.

Validation :

- aucun changement de comportement ;
- aucun changement C++.

### Etape 2 : création du module editor vide

Creer :

```text
Source/GrimrockPrototypeEditor/
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.Build.cs
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.cpp
```

Mettre à jour `GrimrockPrototype.uproject` pour déclarer le module editor.

Validation :

- le projet ouvre encore dans Unreal Editor ;
- la compilation Editor passe ;
- aucun fichier editor n'a encore été deplacé.

### Etape 3 : déplacement du EdMode et Toolkit

Déplacer :

```text
GridLevelEdMode.h/.cpp
GridLevelEdModeToolkit.h/.cpp
```

vers :

```text
Source/GrimrockPrototypeEditor/Public/EditorTools/
Source/GrimrockPrototypeEditor/Private/EditorTools/
```

Déplacer aussi l'enregistrement du mode éditeur depuis `GrimrockPrototype.cpp` vers `GrimrockPrototypeEditor.cpp`.

Validation :

- le mode `Grimrock Grid Editor` apparait encore dans l'éditeur ;
- le toolkit s'affiche ;
- aucune erreur d'include.

### Etape 4 : déplacement de GridLevelEditorActor

Déplacer :

```text
GridLevelEditorActor.h/.cpp
```

vers le module editor.

Validation :

- `BP_GridLevelEditorActor` reste valide ;
- l'acteur peut toujours modifier `UGridLevelAsset` ;
- la sélection, la peinture et les liens fonctionnent encore.

### Etape 5 : traitement des preview components

Decider le destin de :

```text
GridEditorPreviewComponent.h/.cpp
GridEditorPreviewObjectActor.h/.cpp
```

Options :

1. les migrer vers le module editor ;
2. les garder dans runtime mais les renommer pour clarifier leur role ;
3. les garder temporairement dans runtime avec garde-fous `WITH_EDITOR` plus stricts.

Recommandation initiale :

- commencer par les garder en place ;
- retirer d'abord les dépendances editor du module principal ;
- les migrer ensuite si les références Blueprint sont bien comprises.

Validation :

- `BP_GridEditorPreviewObjectActor` reste valide ;
- la preview d'objets fonctionne dans `L_GrimrockEditor` ;
- aucun acteur preview n'apparait en build runtime.

### Etape 6 : nettoyage du Build.cs runtime

Nettoyer `GrimrockPrototype.Build.cs`.

Objectif final :

- retirer `UnrealEd`, `EditorFramework`, `Slate`, `SlateCore` du module runtime ;
- garder seulement les dépendances runtime ;
- retirer `UnrealEd` des `AdditionalDependencies` runtime dans `.uproject`.

Validation :

- compilation runtime ;
- compilation editor ;
- pas de reference editor dans le module runtime.

### Etape 7 : validation en Editor

Valider dans Unreal Editor :

- ouverture du projet ;
- chargement des maps `L_GrimrockEditor` et `L_GrimrockRuntime` ;
- presence du mode `Grimrock Grid Editor` ;
- édition d'une cellule ;
- peinture d'un mur ;
- placement d'un objet ;
- création d'un lien ;
- rebuild preview.

### Etape 8 : validation en build standalone

Valider un build non-editor :

- aucune dépendance a `UnrealEd` ;
- aucun include editor dans runtime ;
- lancement standalone ;
- chargement du niveau runtime ;
- déplacement du joueur ;
- interactions de base : porte, bouton, levier, plaque, trigger, receptacle.

## 6. Risques Unreal

### References Blueprint cassees

Déplacer une `UCLASS` vers un autre module change son chemin script Unreal.

Risque :

- Blueprints derives invalides ;
- variables de type classe perdues ;
- references d'assets cassees.

Mitigation :

- déplacer peu de classes a la fois ;
- ouvrir l'éditeur après chaque etape ;
- sauvegarder les assets concernés ;
- prevoir eventuellement des redirecteurs `CoreRedirects`.

### Includes incorrects

Le split de module peut casser des includes qui fonctionnaient par proximité.

Risque :

- erreurs de compilation ;
- includes publics trop larges ;
- dépendances implicites revelées.

Mitigation :

- preferer forward declarations dans les headers ;
- inclure les headers complets dans les `.cpp` ;
- garder les API runtime publiques minimales.

### Dépendances circulaires

Le module editor dependra du runtime. Le runtime ne doit jamais dependre du module editor.

Risque :

- `GrimrockPrototype` inclut un header de `GrimrockPrototypeEditor` ;
- reference directe du runtime vers `GridLevelEdMode` ou `GridLevelEditorActor`.

Mitigation :

- supprimer l'enregistrement EdMode du module runtime ;
- garder les types de donnees partages dans `Core` runtime ;
- faire appeler le runtime par l'editor, jamais l'inverse.

### UCLASS deplacees

Les classes Unreal exposees aux Blueprints sont sensibles au module d'origine.

Risque :

- `/Script/GrimrockPrototype.GridLevelEditorActor` devient `/Script/GrimrockPrototypeEditor.GridLevelEditorActor` ;
- les assets existants doivent etre re-sauvegardes ou rediriges.

Mitigation :

- commencer par les classes non référencées par assets si possible ;
- identifier les Blueprints derivés avant déplacer ;
- utiliser `CoreRedirects` si necessaire.

### Module non charge dans l'éditeur

Le nouveau module editor devra etre declare et charge correctement.

Risque :

- le mode éditeur n'apparait plus ;
- le toolkit ne s'initialise pas ;
- classes editor indisponibles dans les assets.

Mitigation :

- déclarer le module editor dans `.uproject` avec `Type: Editor` ;
- utiliser une phase de chargement appropriee ;
- verifier l'enregistrement du `FGridLevelEdMode`.

### Assets pointant vers des classes deplacees

Les assets suivants sont particulierement sensibles :

```text
Content/GrimrockPrototype/Blueprints/Editor/BP_GridLevelEditorActor.uasset
Content/GrimrockPrototype/Blueprints/Editor/BP_GridEditorPreviewObjectActor.uasset
Content/GrimrockPrototype/Maps/L_GrimrockEditor.umap
```

Risque :

- classes parent introuvables ;
- references nulles ;
- composants perdus.

Mitigation :

- tester ces assets immédiatement après chaque etape ;
- éviter de déplacer les preview classes avant d'avoir stabilisé le module editor ;
- garder des commits petits et faciles a revert.

### Differenciation preview editor / runtime gameplay

`GridEditorPreviewComponent` et `GridEditorPreviewObjectActor` sont actuellement dans `Runtime`, mais leur rôle est editor.

Risque :

- confusion de responsabilités ;
- code editor-only embarque dans le runtime ;
- preview visible ou référencee dans des builds non-editor.

Mitigation :

- traiter ces classes dans une étape séparée ;
- verifier les références Blueprint ;
- envisager une abstraction runtime minimale pour le placement d'objets, appelée par un composant editor-only.

## Conclusion

La séparation la plus sûre consiste a commencer par créer un module editor vide, puis à migrer d'abord le `GridLevelEdMode` et son toolkit. Le déplacement de `GridLevelEditorActor` et des classes de preview doit venir ensuite, car ces classes sont plus susceptibles d'être référencées par des Blueprints et des maps.

La règle directrice est simple :

- le module runtime contient le jeu et les donnees jouables ;
- le module editor dépend du runtime pour modifier ces données ;
- le runtime ne depend jamais du module editor.
