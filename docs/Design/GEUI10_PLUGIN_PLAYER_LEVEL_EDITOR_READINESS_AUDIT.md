# GEUI10 — Audit de préparation Plugin / Player Level Editor

**Date :** 28 août 2026  
**Statut :** audit d’architecture terminé  
**Impact d’implémentation :** documentation uniquement — aucun refactor C++, aucune création de plugin, aucune modification d’asset/map

## 1. Objectif

GEUI10 clôt la refonte des espaces de travail du Grid Editor en répondant à une question d’architecture à long terme :

> Dans quelle mesure le Grimrock Grid Editor actuel pourra-t-il, à terme, prendre en charge des donjons créés par les joueurs dans un jeu packagé, et quelles responsabilités devront être séparées avant que cela devienne sûr et maintenable ?

Ce jalon est volontairement un **audit**, et non un jalon d’implémentation.

L’objectif immédiat du projet reste le workflow d’authoring dans l’Unreal Editor. Un éditeur packagé destiné aux joueurs est une capacité future.

GEUI10 ne :

- déplace pas de classes entre modules ;
- ne crée pas de plugin ;
- ne crée pas d’UI d’éditeur runtime ;
- n’introduit pas de nouveau format de sauvegarde/package ;
- ne refactor pas `AGridLevelEditorActor` ;
- ne modifie pas les DataAssets existants ;
- ne modifie pas le gameplay runtime ;
- ne modifie pas de `.uasset` ou `.umap` ;
- n’ouvre pas MON21.4.

## 2. Conclusion exécutive

Le projet est bien positionné pour de futurs donjons créés par les joueurs, car la décision d’architecture la plus importante a déjà été prise correctement :

~~~text
topologie du niveau et données gameplay
            ↓
UGridLevelAsset / UGridDungeonAsset
            ↓
AGridLevelRuntimeActor
~~~

La map Unreal n’est pas la source de vérité du donjon.

L’architecture actuelle sépare déjà :

~~~text
Données/gameplay Runtime
    GrimrockPrototype
    GrimrockLua

Authoring Unreal Editor
    GrimrockPrototypeEditor
~~~

Cela signifie qu’un futur éditeur joueur ne nécessite **pas** de réécrire le runtime du donjon.

En revanche, le Grid Editor Unreal actuel ne peut pas simplement être livré tel quel.

Le futur éditeur packagé nécessite une troisième couche conceptuelle :

~~~text
Developer Unreal Editor
        ↓
GrimrockPrototypeEditor
        ↓
        ┌───────────────────────────┐
        │ futur Authoring Core      │
        │ mutation/validation data  │
        └───────────────────────────┘
                   ↓
       GrimrockPrototype Runtime
                   ↑
        ┌───────────────────────────┐
        │ future Player Editor UI   │
        │ packaged runtime UI/input │
        └───────────────────────────┘
~~~

La recommandation est donc :

> **Ne pas convertir GrimrockPrototypeEditor en module runtime.**

Lorsque l’authoring joueur deviendra un jalon actif, il faudra plutôt extraire uniquement les règles d’authoring orientées données vers une petite couche compatible runtime et construire autour d’elle une UI packagée dédiée.

## 3. Topologie actuelle des modules

Le `.uproject` actuel déclare :

~~~text
GrimrockLua              Runtime
GrimrockPrototype        Runtime
GrimrockPrototypeEditor  Editor
~~~

La cible Game inclut :

~~~text
GrimrockPrototype
~~~

La cible Editor inclut :

~~~text
GrimrockPrototype
GrimrockPrototypeEditor
~~~

Cette direction est correcte :

~~~text
GrimrockPrototypeEditor
          ↓
GrimrockPrototype
          ↓
GrimrockLua
~~~

Le runtime ne dépend pas de `GrimrockPrototypeEditor`.

### 3.1 Dépendances runtime actuelles

Le `GrimrockPrototype.Build.cs` courant contient :

~~~text
Public:
Core
CoreUObject
Engine
InputCore
EnhancedInput
UMG
Niagara
GrimrockLua

Private:
Slate
SlateCore
~~~

Cet état est plus récent que l’instantané historique de `docs/Architecture_Runtime_Editor_Split.md`, antérieur au travail actuel sur l’UI runtime.

La présence de `Slate/SlateCore` dans le runtime n’est pas en soi un blocage pour un éditeur joueur : ces dépendances sont utilisées par du code UI de jeu packagé et sont des dépendances runtime valides.

La direction interdite importante reste :

~~~text
GrimrockPrototype -> UnrealEd
GrimrockPrototype -> GrimrockPrototypeEditor
~~~

Aucune dépendance de module de ce type n’est actuellement déclarée.

### 3.2 Dépendances éditeur actuelles

`GrimrockPrototypeEditor` possède les dépendances editor-only attendues :

~~~text
UnrealEd
EditorFramework
PropertyEditor
ToolMenus
Slate
SlateCore
ApplicationCore
AssetRegistry
~~~

Ce module est donc correctement exclu d’une cible Game packagée.

## 4. Matrice de préparation

| Domaine | Préparation actuelle pour un futur éditeur joueur | Évaluation |
|---|---:|---|
| Modèle de données grille/cellule | Élevée | Les données runtime sont déjà indépendantes de l’Unreal Editor. |
| Modèle d’objet placé | Élevée | `FGridLevelObjectData` est runtime et data-driven. |
| Liens Event -> Command | Élevée | Le modèle persistant des liens est runtime. |
| Modèle de donjon multi-niveaux | Élevée | `UGridDungeonAsset` fournit déjà une identité de niveau stable et une position logique. |
| Reconstruction runtime | Élevée | `AGridLevelRuntimeActor` consomme directement les données de niveau. |
| Archétypes d’objets | Élevée | Les DataAssets runtime définissent déjà le placement, le rendu et le comportement runtime. |
| Métadonnées de palette | Moyenne/Élevée | DataAsset runtime ; utile pour l’authoring, mais l’exposition joueur devra être limitée à une liste autorisée. |
| Sandbox Lua runtime | Élevée | Module runtime dédié avec limites strictes et API contrôlée par l’hôte. |
| Primitives de mutation de niveau | Moyenne | Certaines sont déjà runtime ; une grande partie de l’authoring haut niveau reste dans l’acteur/services éditeur. |
| Validation | Moyenne | De nombreuses règles pures existent, mais la validation éditeur complète appartient encore au module éditeur. |
| Persistance de l’authoring runtime | Faible | Aucun sérialiseur de document/package de donjon joueur n’existe actuellement. |
| Undo/redo runtime | Faible | Le workflow courant repose sur les transactions/sémantiques d’authoring de l’Unreal Editor. |
| Picking runtime/caméra éditeur | Faible | L’interaction actuelle utilise EdMode / l’infrastructure du viewport éditeur. |
| UI d’authoring runtime | Faible | L’espace de travail Slate courant est editor-only par conception. |
| Sécurité du contenu joueur | Moyenne | Le sandbox Lua est solide, mais l’allowlist des assets/archétypes/packages n’est pas encore une frontière complète pour le contenu joueur. |
| Packaging en plugin | Non requis actuellement | Un plugin est une option d’organisation, pas un prérequis. |

## 5. Éléments déjà réutilisables sans extraction d’architecture

### 5.1 Types de grille Core

Les éléments suivants sont déjà des types runtime et constituent de solides fondations pour un futur authoring :

~~~text
EGridCellType
EGridWallType
EGridEdge
FGridLevelCellData
FGridLevelObjectData
FGridObjectLink
EGridObjectEvent
EGridObjectCommand
EGridObjectCondition
FGridObjectBehaviorParams
~~~

Ils contiennent le véritable langage du donjon plutôt qu’un état UI de l’Unreal Editor.

C’est le constat positif le plus important de GEUI10.

### 5.2 UGridLevelAsset

`UGridLevelAsset` contient déjà :

- largeur / hauteur / taille de cellule ;
- topologie des cellules ;
- murs ;
- plafonds et occupation ;
- départ du joueur ;
- objets placés ;
- liens Event -> Command ;
- définitions de quêtes ;
- définitions de variables persistantes de niveau ;
- scripts source Lua.

Il expose également des mutations/helpers compatibles runtime tels que :

~~~text
EnsureCellCount
ClearLevel
AddObject
RemoveObjectById
RemoveLinksForObject
EnsureObjectIds
~~~

Les appels `Modify()` et `MarkPackageDirty()` présents dans certaines de ces méthodes sont protégés par :

~~~cpp
#if WITH_EDITOR
~~~

Ces mutations de base sont donc déjà appelables dans un build runtime sans introduire `UnrealEd`.

Précision importante :

> La mutabilité runtime n’est **pas équivalente à la persistance packagée**.

Un joueur peut modifier un UObject en mémoire, mais un `UDataAsset` cooké n’est pas un format de fichier joueur approprié et inscriptible.

Cette lacune de persistance est le principal blocage futur.

### 5.3 UGridDungeonAsset

`UGridDungeonAsset` fournit déjà :

~~~text
DungeonName
Author
Version
DefaultLevelId
Levels[]
    LevelId
    DisplayName
    LevelAsset
    LogicalPosition
    bEnabled
~~~

Ces concepts sont directement adaptés à des packages de donjons créés par les joueurs.

La représentation par références UObject est orientée assets développeur, mais le modèle d’identité est sain.

### 5.4 Archétypes d’objets

`UGridObjectArchetypeAsset` est déjà runtime et contient les données centrales nécessaires à la fois à l’authoring et à l’exécution :

- type gameplay ;
- catégorie ;
- type de placement ;
- policy de partage cellule/ancre ;
- remplacement de mur ;
- blocage du déplacement ;
- paramètres interaction/readable/lumière ;
- meshes/matériaux ;
- classe d’acteur runtime ;
- transformations de placement ;
- comportement par défaut.

Cela donne à un futur éditeur joueur un modèle de contenu contrôlé très solide :

> Les joueurs doivent choisir un `ArchetypeId` autorisé ; ils ne doivent pas créer des classes Unreal arbitraires.

### 5.5 Palette d’objets

`UGridObjectPaletteAsset` est également runtime.

Ses entrées fournissent déjà :

~~~text
EntryId
DisplayNameOverride
CategoryOverride
Icon
DefaultArchetype
DefaultMonsterDefinition
DefaultStoryCompanionDefinition
~~~

La palette développeur actuelle peut donc, conceptuellement, devenir la source d’une future palette destinée aux joueurs.

L’éditeur joueur devra exposer un sous-ensemble contrôlé plutôt que permettre de parcourir librement tous les assets.

### 5.6 AGridLevelRuntimeActor

L’acteur runtime reconstruit et exécute déjà le niveau à partir de `UGridLevelAsset`.

Ses responsabilités comprennent :

- génération du sol / des murs / du plafond ;
- spawn des objets runtime ;
- transformations de placement des objets ;
- portes et interaction ;
- pickup/drop d’items ;
- spawn/état runtime des monstres ;
- encounters ;
- triggers ;
- links ;
- transitions ;
- état runtime multi-niveaux ;
- pont Lua via les systèmes d’activation/runtime.

Cela signifie que le futur éditeur joueur pourra utiliser **le même renderer/consommateur gameplay runtime** pour une preview en direct.

Aucun second renderer de donjon ne doit être créé.

### 5.7 GrimrockLua

`GrimrockLua` est un module Runtime dédié ne dépendant que de :

~~~text
Core
CoreUObject
~~~

Son `FGridLuaVm` fournit déjà des limites strictes non contrôlables par les données du niveau :

~~~text
HardMaxScriptCount = 64
HardMaxSourceBytesPerScript = 256 KiB
HardMaxTotalSourceBytes = 1 MiB
MemoryLimitBytes
InstructionBudgetPerCall
~~~

L’API hôte est basée sur des callbacks et, volontairement, ne connaît rien de :

~~~text
UWorld
Actors
gameplay enums
filesystem
network
OS APIs
~~~

C’est une fondation très solide pour de futurs mécanismes scriptés créés par les joueurs.

## 6. Implémentation strictement Unreal-Editor-only

Les éléments suivants doivent rester editor-only.

### 6.1 FGridLevelEdMode

Dépend de l’infrastructure du viewport/outils de l’Unreal Editor :

~~~text
FEdMode
FEditorViewportClient
FViewport
GEditor
FEditorModeRegistry
~~~

Un éditeur packagé devra implémenter son propre chemin caméra/input/picking.

### 6.2 FGridLevelEdModeToolkit

Utilise l’infrastructure Toolkit éditeur et les onglets Nomad globaux.

Il ne doit pas être migré vers le runtime.

L’UX des espaces de travail GEUI01–09 peut inspirer l’éditeur joueur, mais l’implémentation du Toolkit Slate elle-même n’est pas la couche réutilisable.

### 6.3 Onglets Nomad des espaces de travail

Les éléments suivants relèvent de la présentation de l’éditeur développeur :

~~~text
Dungeon Levels
PlayTest & Validation
Tools & Palette
Selected Object
Grimrock Lua Scripts
~~~

Ils dépendent de :

~~~text
FGlobalTabmanager
SDockTab
SWindow
editor mode lifetime
Window menu integration
~~~

Un éditeur joueur a besoin à la place d’un modèle d’espace de travail/layout intégré au jeu.

### 6.4 AGridLevelEditorActor

`AGridLevelEditorActor` est maintenant correctement situé dans `GrimrockPrototypeEditor`.

Il combine plusieurs responsabilités propres à l’éditeur développeur :

- sélection courante ;
- sélection survolée ;
- peinture dans le viewport ;
- état UI de placement des objets ;
- état d’authoring des liens ;
- coordination de la preview ;
- diagnostics éditeur ;
- préparation PIE ;
- mutations conscientes des transactions ;
- focus de sélection ;
- interaction d’édition des routes de patrouille.

Il ne doit **pas** être déplacé en bloc vers le module runtime.

Les futures extractions devront en sortir les fonctions orientées données une capacité à la fois.

### 6.5 Workflow PIE

~~~text
GridPIEPlaytestRequest
PreBeginPIE
BeginPIE
Debug Prepare PIE
Auto Prepare PIE
~~~

sont des workflows propres à l’éditeur développeur.

Un éditeur joueur packagé devra plutôt proposer directement :

~~~text
Edit Mode
   ↓
Playtest Mode
   ↓
return to Edit Mode
~~~

en utilisant le monde runtime, et non une duplication PIE.

## 7. Candidats futurs à forte valeur pour extraction

Ces éléments appartiennent aujourd’hui à l’éditeur mais contiennent de la logique dont un workflow d’authoring packagé aura un jour besoin.

Ils ne doivent **pas** être extraits pendant GEUI10.

### 7.1 Policy des liens

État actuel :

~~~text
GridEditorLinkPolicy
~~~

Elle détermine :

- quels objets émettent des événements ;
- quelles cibles reçoivent des commandes ;
- les événements source pris en charge ;
- les commandes cible prises en charge ;
- les conditions prises en charge ;
- la classification du support runtime des commandes ;
- l’identité exacte des liens.

L’essentiel de cette logique est une policy de données pure et ne dépend pas fondamentalement de l’Unreal Editor.

Cible future :

~~~text
GridAuthoringLinkPolicy
~~~

dans un module/core d’authoring compatible runtime.

### 7.2 Service de mutation des liens

État actuel :

~~~text
GridEditorLinkService
~~~

Ses fonctions de bas niveau sont déjà presque pures :

~~~text
NormalizeLink
IsConditionConfigurationValid
IsLinkSupported
ContainsExactLink
AddExactLink
RemoveExactLink
~~~

Les overloads qui acceptent `AGridLevelEditorActor` sont des ponts éditeur.

Une future extraction devra séparer :

~~~text
opérations pures sur le modèle de liens
de
pont transaction/sélection éditeur
~~~

### 7.3 Service d’authoring Lua

État actuel :

~~~text
GridEditorLuaService
~~~

Ses responsabilités mélangées comprennent :

- analyse de script ;
- découverte des callbacks ;
- validation de script ;
- identité de script ;
- mutation du source de script ;
- synchronisation des déclarations persistantes ;
- mutation LogicId de l’objet sélectionné ;
- validation complète du niveau côté éditeur.

La VM elle-même est déjà runtime.

Un futur éditeur joueur aura besoin d’une façade d’authoring sûre pour le runtime couvrant :

~~~text
AnalyzeLevel
ValidateScriptDefinitions
GetCallbacksForScript
Add/Rename/Remove script
SetScriptEnabled
SetScriptSource
~~~

Les opérations liées à `AGridLevelEditorActor` devront rester du code adaptateur.

### 7.4 Validation du niveau

La présentation complète de validation utilise actuellement :

~~~text
FGridLevelValidationMessage
EGridLevelValidationSeverity
~~~

déclarés dans le header de l’acteur éditeur.

Pour l’authoring joueur, un contrat de validation neutre devra à terme sortir du module éditeur, par exemple :

~~~text
EGridLevelValidationSeverity
FGridLevelValidationIssue
FGridLevelValidationResult
GridLevelValidationService
~~~

Le panneau de l’Unreal Editor et le futur éditeur joueur consommeront alors le même résultat.

Cette extraction a une forte valeur car les packages joueur invalides doivent être rejetés avant play/export.

### 7.5 Mutations d’édition du niveau

Le futur core d’authoring réutilisable aura besoin d’opérations explicites telles que :

~~~text
PaintCell
PaintWall
EraseWall
PlaceObject
RemoveObject
MoveObject
SetObjectProperty
AddLink
RemoveLink
SetStartCell
AddDungeonLevel
RemoveDungeonLevel
RenameDungeonLevel
~~~

Aujourd’hui, une grande partie de ces workflows est incarnée dans `AGridLevelEditorActor`.

Il ne faut pas exposer l’acteur lui-même comme future API.

Il faudra plutôt extraire une couche commandes/services uniquement lorsque l’édition joueur deviendra un jalon actif.

## 8. Blocage principal : format de persistance des donjons joueur

La source de vérité actuelle est un `UDataAsset`.

C’est idéal pour du contenu cooké créé par les développeurs.

Ce n’est pas suffisant comme format de fichier pour des niveaux créés par les joueurs dans un build packagé.

### 8.1 Ce qu’il ne faut pas faire

Ne pas concevoir l’éditeur joueur autour de l’écriture de fichiers `.uasset` cookés modifiés.

Ne pas faire dépendre le contenu joueur des APIs de création d’assets de l’Unreal Editor.

Ne pas sérialiser des graphes UObject arbitraires provenant de contenu joueur non fiable.

### 8.2 Modèle futur recommandé

Introduire un document de donjon joueur versionné et contrôlé par le moteur, conceptuellement :

~~~text
FGridPlayerDungeonDocument
    SchemaVersion
    DungeonId
    DungeonName
    Author
    GameContentVersion
    DefaultLevelId
    Levels[]

FGridPlayerLevelDocument
    LevelId
    DisplayName
    LogicalPosition
    Width
    Height
    CellSize
    Cells[]
    Objects[]
    Links[]
    LevelVariables[]
    LuaScripts[]
~~~

Ce document devra utiliser autant que possible des identifiants stables plutôt que des références UObject arbitraires.

Exemples :

~~~text
ArchetypeId
ItemDefinitionId
MonsterDefinitionId
ReadableContentId
QuestId
StoryCompanionId
~~~

### 8.3 Matérialisation runtime

Flux recommandé :

~~~text
package joueur
     ↓ désérialisation + validation du schéma
FGridPlayerDungeonDocument
     ↓ résolution des IDs de contenu développeur autorisés
UGridDungeonAsset / UGridLevelAsset transients
     ↓
AGridLevelRuntimeActor
~~~

Une représentation UObject transient permet au runtime existant de continuer à consommer son modèle courant.

### 8.4 Choix de sérialisation

L’audit n’impose ni JSON, ni binaire, ni SaveGame.

Les propriétés requises sont plus importantes que le conteneur :

- version de schéma explicite ;
- propriété déterministe des champs ;
- limites de taille/bornes ;
- échec sûr pour les versions inconnues ;
- IDs stables ;
- aucun chargement arbitraire de classes ;
- tests aller-retour ;
- format lisible par l’humain facultatif si le partage/modding en bénéficie.

## 9. Résolution des assets et cooking

Le contenu créé par les joueurs ne peut référencer que du contenu présent dans le build packagé, sauf si un système distinct et fiable de distribution de contenu est créé ultérieurement.

Un éditeur joueur a donc besoin d’un catalogue d’authoring contrôlé.

Règle recommandée :

~~~text
document joueur
    stocke ArchetypeId
        ↓
catalogue d’authoring runtime
        ↓
UGridObjectArchetypeAsset cooké connu
~~~

Le même principe s’applique à :

- items ;
- monstres ;
- readables ;
- quêtes ;
- story companions ;
- icônes ;
- meshes/matériaux référencés indirectement par les archétypes.

### 9.1 Sécurité de RuntimeActorClass

`UGridObjectArchetypeAsset` contient :

~~~text
RuntimeActorClass
ItemActorClass
~~~

C’est acceptable car les archétypes sont du contenu cooké contrôlé par les développeurs.

Les packages joueur ne doivent **pas** spécifier des chemins de classes arbitraires.

Ils doivent uniquement spécifier un ID d’archétype/définition autorisé.

## 10. Frontière de validation du contenu joueur

Un futur donjon joueur doit réussir la validation avant :

~~~text
Save
Export
Publish
Playtest
Play
~~~

Au minimum, la validation doit couvrir :

### Limites structurelles

- dimensions de grille autorisées ;
- nombre exact de cellules ;
- coordonnées valides ;
- limites du nombre d’objets ;
- limites du nombre de liens ;
- ObjectIds uniques ;
- LogicIds uniques lorsque requis ;
- LevelIds de donjon valides ;
- transitions valides.

### Placement

- type de placement ;
- murs requis pour les objets muraux ;
- conflits d’occupation ;
- partage d’ancre ;
- validité de la cellule de départ ;
- validité des MonsterSpawns ;
- validité des waypoints de patrouille.

### Références

- ArchetypeIds connus ;
- IDs de définitions connus ;
- aucun contenu interdit ;
- aucun chemin UObject/classe arbitraire.

### Connecteurs

- source valide ;
- cible valide ou commande targetless valide ;
- événement pris en charge ;
- commande prise en charge ;
- payload de condition valide ;
- callback/script Lua valide ;
- identifiants de quête valides.

### Lua

- nombre de scripts ;
- tailles des sources ;
- validation syntaxe/chargement ;
- existence des callbacks ;
- cohérence des déclarations persistantes ;
- limite mémoire VM ;
- budget d’instructions.

## 11. Évaluation de la sécurité Lua

L’architecture Lua actuelle est particulièrement bien positionnée pour l’authoring joueur.

Propriétés positives déjà présentes :

- module Lua dédié ;
- aucun header Lua ne traverse la frontière du module ;
- aucune autorité directe monde/acteur dans la VM ;
- API `grid` contrôlée par l’hôte ;
- quota mémoire ;
- budget d’instructions ;
- limites sur le nombre/la taille des sources ;
- environnements de scripts isolés ;
- rechargement atomique de la VM ;
- exécution des callbacks via des fonctions hôte contrôlées.

Règle future pour l’éditeur joueur :

> Ne jamais assouplir ces limites en fonction de données créées dans le donjon.

Le package ne doit pas pouvoir augmenter lui-même :

~~~text
memory quota
instruction budget
script count
source size
host API privileges
~~~

## 12. Stratégie d’UI de l’éditeur runtime

L’UI de l’éditeur actuelle doit être considérée comme un **prototype/référence UX**, et non comme du code à livrer.

Concepts UX réutilisables de GEUI01–09 :

- espace de travail Dungeon Levels ;
- overview map ;
- Tools & Palette ;
- recherche/catégories/favorites/recent ;
- Selected Object Properties/Connectors ;
- recherche/filtrage/focus de validation ;
- espace de travail des scripts Lua ;
- état PlayTest clair.

Technologies d’implémentation possibles dans le build packagé :

~~~text
UMG
runtime Slate
ou une combinaison des deux
~~~

Le projet utilise déjà UMG et Slate runtime ailleurs.

Recommandation :

> Préférer l’architecture UI existante du jeu, sauf si un prototype ultérieur démontre qu’un éditeur de style desktop en Slate runtime apporte un avantage matériel.

Ne pas ajouter `UnrealEd`, `EditorFramework`, `ToolMenus` ou le code de docking éditeur à la cible Game.

## 13. Picking runtime et caméra d’édition

Le Grid Editor actuel repose sur :

~~~text
FEditorViewportClient
FSceneView deprojection
FEdMode mouse handling
Unreal Editor camera/navigation
~~~

Un éditeur joueur nécessitera un contrôleur d’authoring runtime explicite.

Responsabilités conceptuelles :

~~~text
AGridAuthoringPawn / Controller
    camera pan/orbit/free-look
    grid raycast
    cell hover
    edge resolution
    object hover
    selection
    paint gesture
    erase gesture
~~~

Les mathématiques existantes de coordonnées/placement devront être réutilisées autant que possible.

Le EdMode lui-même ne doit pas être réutilisé.

## 14. Stratégie de preview

Deux classes actuelles restent dans le module runtime :

~~~text
UGridEditorPreviewComponent
AGridEditorPreviewObjectActor
~~~

Leur nom vient historiquement de la preview de l’éditeur développeur, mais leur implémentation compile en runtime.

Elles restent ambiguës architecturalement.

Recommandation GEUI10 :

- ne pas les déplacer maintenant ;
- ne pas faire dépendre d’elles le modèle fondamental du futur éditeur joueur ;
- construire d’abord le véritable donjon de référence et continuer à stabiliser le rendu runtime ;
- décider plus tard si leur comportement de sélection/highlight devient une capacité générique de preview d’authoring ou reste du code de compatibilité réservé au développeur.

Pour un éditeur packagé, la preview live devra toujours être centrée sur :

~~~text
AGridLevelRuntimeActor
~~~

plus une présentation runtime de la sélection/highlight.

## 15. Undo / redo

L’Unreal Editor bénéficie des sémantiques de transactions UObject/éditeur via les appels d’authoring utilisant `Modify()`.

Un éditeur packagé ne peut pas reposer sur l’infrastructure de transactions de l’Unreal Editor.

Le futur core d’authoring devra utiliser des commandes explicites, conceptuellement :

~~~text
FGridAuthoringCommand
    Execute()
    Undo()
~~~

Exemples :

~~~text
PaintCellCommand
PaintWallCommand
PlaceObjectCommand
DeleteObjectCommand
ChangePropertyCommand
AddLinkCommand
RemoveLinkCommand
~~~

Cette couche de commandes est une raison supplémentaire de ne pas exposer directement `AGridLevelEditorActor` à un éditeur packagé.

## 16. État dirty et autosave

Un éditeur joueur a besoin d’un état documentaire explicite :

~~~text
Clean
Modified
Saving
Save failed
Validation failed
~~~

Cela doit être distinct de :

~~~text
UPackage::MarkPackageDirty()
~~~

Responsabilités futures recommandées :

- compteur de révision dirty ;
- autosave ;
- fichier de récupération ;
- Save As ;
- métadonnées de package ;
- validation avant export ;
- confirmation lors d’une fermeture destructive.

## 17. Implications multi-niveaux

Le modèle de donjon actuel est déjà avantageux.

Un package joueur doit préserver :

~~~text
LevelId
DisplayName
LogicalPosition
DefaultLevelId
~~~

Les objets de transition peuvent continuer à référencer des IDs de niveau stables.

Le futur éditeur joueur devra donc éditer un document de donjon contenant plusieurs niveaux plutôt que d’écrire des fichiers de maps indépendantes sans rapport.

Cela correspond directement au design data-driven actuel du projet.

## 18. Décision concernant un plugin

### 18.1 Avons-nous besoin d’un plugin maintenant ?

Non.

Créer un plugin aujourd’hui déplacerait surtout des fichiers sans résoudre un besoin de l’éditeur joueur.

La séparation actuelle des modules suffit pour poursuivre le développement du jeu.

### 18.2 Quand un plugin pourrait-il devenir utile ?

Un plugin devient justifié si l’une de ces conditions devient vraie :

- le système d’authoring est réutilisé dans un autre projet ;
- les modules d’authoring joueur nécessitent un package versionné indépendamment ;
- le Grid Editor développeur et l’éditeur joueur runtime partagent un core d’authoring mature ;
- les frontières de distribution/mod SDK bénéficient d’un packaging plugin.

### 18.3 Forme future possible du plugin/des modules

Uniquement lorsque cela sera justifié :

~~~text
GrimrockAuthoring
├── GrimrockAuthoringCore        Runtime
│     document schema
│     mutation commands
│     validation
│     catalog resolution
│
├── GrimrockAuthoringRuntimeUI   Runtime
│     packaged player editor
│
└── GrimrockAuthoringEditor      Editor
      UE EdMode adapters
      developer-specific Slate
      asset integration
~~~

Il s’agit d’une direction future, pas d’un refactor demandé.

## 19. Ordre d’extraction recommandé lorsque l’authoring joueur deviendra actif

Ne pas commencer par l’UI.

Séquence recommandée :

### PLE01 — Player Dungeon Document

Créer le document de donjon/niveau versionné et sérialisable au runtime.

Critère d’acceptation :

~~~text
document -> serialize -> deserialize -> identical document
~~~

### PLE02 — Catalogue et résolution sûre

Mapper les IDs visibles par le joueur vers des assets cookés autorisés.

Critère d’acceptation :

~~~text
unknown/forbidden IDs are rejected without arbitrary asset loading
~~~

### PLE03 — Runtime Validation Core

Extraire les problèmes de validation neutres et les règles de validation pures.

Critère d’acceptation :

~~~text
same invalid level produces equivalent diagnostics
in Unreal developer editor and packaged authoring tests
~~~

### PLE04 — Commandes de mutation d’authoring

Créer des commandes d’édition pures avec undo/redo.

Critère d’acceptation :

~~~text
execute -> undo -> original document
execute -> undo -> redo -> edited document
~~~

### PLE05 — Runtime Live Preview

Matérialiser un niveau transient et reconstruire `AGridLevelRuntimeActor`.

Critère d’acceptation :

~~~text
player document edits appear in runtime preview
without map or asset editing APIs
~~~

### PLE06 — Player Editor UI

Implémenter les workflows géométrie/objet/lien/propriété.

### PLE07 — Authoring Lua

Exposer l’édition/validation sûre des scripts en utilisant le sandbox existant.

### PLE08 — Import / Export / Sharing

Ajouter métadonnées de package, contrôles de compatibilité et gestion sûre des fichiers.

## 20. Registre des risques

| Risque | Sévérité | Mitigation actuelle / action future |
|---|---:|---|
| Essayer de livrer `GrimrockPrototypeEditor` | Élevée | Explicitement interdit par cet audit. |
| Écrire des DataAssets cookés comme fichiers joueur | Élevée | Introduire à la place un document joueur versionné. |
| Références UObject/classe arbitraires provenant d’un package joueur | Élevée | ID stable + catalogue allowlisté uniquement. |
| Validation obsolète liée à l’acteur éditeur | Moyenne | Future extraction d’un service de validation neutre. |
| Duplication des règles d’authoring développeur/joueur | Élevée | Extraire un core d’authoring pur avant l’UI joueur. |
| Éditeur runtime construit avant le contrat de sérialisation | Élevée | PLE01 doit précéder l’UI. |
| Abus de ressources Lua | Moyenne | Conserver les limites strictes existantes de la VM, non configurables par l’auteur. |
| Données de niveau invalides/hostiles trop volumineuses | Élevée | Limites structurelles strictes pendant désérialisation/validation. |
| Indisponibilité d’assets cookés | Moyenne | Policy explicite de catalogue/cooking requise. |
| Ambiguïté des classes Preview | Faible/Moyenne | Laisser inchangé jusqu’à l’apparition d’un besoin réel. |
| Refactor plugin prématuré | Moyenne | Aucun travail plugin avant un besoin réel de réutilisation/distribution. |

## 21. Résumé de la classification architecturale

### À conserver dans GrimrockPrototype Runtime

~~~text
GridTypes
GridObjectBehavior
GridLevelAsset
GridDungeonAsset
GridObjectArchetypeAsset
GridObjectPaletteAsset
definition assets
GridLevelRuntimeActor
runtime object actors/components
runtime state/persistence
gameplay interaction
~~~

### À conserver dans GrimrockLua Runtime

~~~text
FGridLuaVm
script types
sandbox/resource limits
host API bridge
~~~

### À conserver dans GrimrockPrototypeEditor

~~~text
FGridLevelEdMode
FGridLevelEdModeToolkit
SGridEditorWorkspaceTab
all current Grid Editor Slate workspaces
AGridLevelEditorActor
PIE preparation
Window/NomadTab integration
GEditor/editor viewport integration
~~~

### Candidats à une extraction future

~~~text
Link policy
pure link mutation
neutral validation model/service
Lua authoring analysis/mutation facade
level/dungeon mutation commands
safe authoring catalog
player dungeon document serialization
~~~

## 22. Recommandation immédiate après GEUI10

**Arrêter ici le refactor d’architecture du Grid Editor.**

GEUI01–10 ont maintenant produit :

- des espaces de travail dockables et focalisés ;
- un dashboard principal compact ;
- une palette utilisable ;
- un espace de travail Selected Object ;
- un espace de travail PlayTest & Validation ;
- un espace de travail Lua ;
- la gestion du cycle de vie du mode éditeur ;
- la restauration des fenêtres ;
- un comportement de rafraîchissement/état ciblé ;
- un chemin documenté vers un futur authoring joueur.

Le travail de plus forte valeur n’est désormais plus un nouveau refactor abstrait de l’éditeur.

Il s’agit de :

> **Construire un vrai donjon de référence avec l’éditeur.**

Périmètre recommandé du donjon de référence :

~~~text
3 levels
multiple transitions
doors
secret doors
buttons
levers
pressure plates
receptacles
items / keys / locks
teleporters
triggers
monster spawns
patrols / encounters
readables
Lua mechanisms
quest hooks
validation
playtest
~~~

Utiliser intensivement l’éditeur sur du contenu réel révélera les lacunes futures d’UX et de modèle de données beaucoup plus sûrement que poursuivre un travail spéculatif sur l’interface.

## 23. Registre des décisions

GEUI10 enregistre les décisions suivantes :

1. `UGridLevelAsset` / `UGridDungeonAsset` restent l’autorité du contenu développeur.
2. Le runtime reste indépendant de `GrimrockPrototypeEditor`.
3. `AGridLevelEditorActor` reste editor-only.
4. L’UI Slate/EdMode actuelle n’est pas l’implémentation d’un futur éditeur packagé.
5. Un éditeur joueur utilisera un core d’authoring compatible runtime, et non le module Unreal Editor.
6. Les niveaux joueur nécessitent un contrat séparé de document/package versionné et sérialisable.
7. Les packages joueur référencent le contenu contrôlé via des IDs stables.
8. Le sandbox Lua existant est la fondation du scripting créé par les joueurs et ses limites strictes doivent rester non contrôlables par l’auteur.
9. Aucun plugin n’est créé pendant GEUI10.
10. Aucun nouveau refactor d’architecture du Grid Editor n’est recommandé avant la construction d’un véritable donjon de référence.

## 24. Clôture GEUI01–GEUI10

La roadmap GEUI peut maintenant être considérée comme architecturalement terminée :

~~~text
GEUI01  Fondation de l’espace de travail dockable
GEUI02  Dungeon Levels
GEUI03  Espace de travail PlayTest & Validation
GEUI04  Espace de travail Tools & Palette
GEUI05  Espace de travail Selected Object
GEUI06  Toolkit principal allégé + cycle de vie des fenêtres
GEUI07  UX de la palette
GEUI08  UX de validation
GEUI09  Nettoyage du rafraîchissement / état
GEUI10  Audit de préparation éditeur joueur / plugin
~~~

Aucun jalon GEUI supplémentaire n’est requis avant le retour à l’authoring du contenu réel du donjon.
