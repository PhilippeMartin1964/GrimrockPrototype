# 10 — Mouse Interaction System

## 1. Objectif

Ce document décrit le système d’interaction souris du prototype Grimrock.

L’objectif est de remplacer l’ancienne action clavier globale `F` par des interactions directes à la souris : le joueur clique sur l’objet, ou sur la partie de l’objet, qu’il souhaite manipuler.

Le système doit rester compatible avec l’architecture orientée données du projet : les objets de niveau sont définis dans le `GridLevelAsset`, instanciés en runtime, puis activés via les composants runtime existants.

## 2. Règle de design principale

Le principe retenu est le suivant :

```text
Le PlayerController détecte ce qui se trouve sous la souris.
L’objet cliqué décide s’il peut interagir.
La logique de gameplay reste centralisée dans le runtime.
```

Concrètement :

- `AGrimrockPlayerController` effectue la détection souris ;
- les objets interactifs implémentent `IGridInteractableInterface` ;
- les objets exposent un curseur via `EGridInteractionCursor` ;
- les actions importantes continuent de passer par le runtime, notamment `TryInteractAtEdge`, afin de préserver les liens logiques et les événements.

La règle importante à ne pas casser :

```text
Une porte n’est jamais cliquable directement.
```

Une porte est actionnée uniquement par :

- bouton ;
- levier ;
- plaque de pression ;
- trigger ;
- lien logique ;
- script ou mécanisme futur.

## 3. Architecture technique

### 3.1 AGrimrockPlayerController

`AGrimrockPlayerController` est responsable de :

- afficher ou masquer le curseur système ;
- instancier le curseur custom UMG ;
- détecter l’objet sous la souris ;
- vérifier la distance d’interaction ;
- interroger `CanInteract` avant de promettre une interaction au joueur ;
- appeler `Interact` lors du clic gauche.

Le contrôleur ne doit pas contenir de logique spécifique du type :

```text
si bouton → ouvrir porte
si levier → basculer état
si torche → inventaire
```

Il ne connaît que l’interface générique.

### 3.2 IGridInteractableInterface

Les objets interactifs implémentent `IGridInteractableInterface`.

Méthodes principales :

```cpp
CanInteract(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent) const
Interact(APawn* InstigatorPawn, UPrimitiveComponent* HitComponent)
GetInteractionCursor(UPrimitiveComponent* HitComponent) const
GetInteractionText(UPrimitiveComponent* HitComponent) const
```

Chaque objet décide localement si le composant touché est valide.

Exemples :

- bouton : uniquement `MovingMeshComponent` ;
- levier : uniquement `MovingMeshComponent` ;
- item : uniquement `MeshComponent` ;
- réceptacle vide : support principal ;
- réceptacle plein : objet contenu uniquement ;
- porte : pas d’interface.

### 3.3 EGridInteractionCursor

`EGridInteractionCursor` décrit l’intention d’interaction :

```text
None
Default
Use
Push
Pull
Take
Read
Locked
Forbidden
```

Cette enum pilote le curseur custom `WBP_GridMouseCursor`.

### 3.4 GridInteractionUtils

`GridInteractionUtils` centralise la résolution des acteurs runtime utiles :

```cpp
GridInteractionUtils::ResolvePartyPawn(APawn* InstigatorPawn)
GridInteractionUtils::ResolveRuntimeActor(APawn* InstigatorPawn, const AActor* ContextActor)
```

Cela évite que chaque objet interactif duplique les mêmes fallbacks :

- `PartyPawn->LevelRuntimeActor` ;
- `ContextActor->GetOwner()` ;
- `TActorIterator<AGridLevelRuntimeActor>`.

### 3.5 WBP_GridMouseCursor

`WBP_GridMouseCursor` est le curseur UMG custom.

Il contient une image `CursorImage` et expose une fonction Blueprint :

```text
SetCursorState(Cursor : EGridInteractionCursor)
```

Le widget suit la position de la souris et change de texture selon l’état reçu.

Le widget doit être configuré en :

```text
Visibility = Hit Test Invisible
```

Il ne doit jamais intercepter les clics souris.

## 4. Détection souris

La détection utilise un `LineTraceMultiByChannel` sur `ECC_Visibility`.

Le multi-trace est volontaire : un simple premier hit ne suffit pas, car un mur, une porte, un sol, un ISM ou un support peut être touché avant le vrai composant interactif.

La logique est :

```text
1. Déprojeter la position souris en rayon monde.
2. Tracer en ECC_Visibility.
3. Parcourir les hits du plus proche au plus loin.
4. Trouver le premier acteur qui implémente UGridInteractableInterface.
5. Vérifier la distance.
6. Vérifier CanInteract.
7. Afficher le curseur ou exécuter Interact.
```

Le hover utilise `CanInteract` avant d’afficher un curseur interactif. Cela évite d’afficher une main alors que le clic serait refusé.

Hors portée :

```text
EGridInteractionCursor::Forbidden
```

## 5. Objets actuellement interactifs

### 5.1 AGridButtonActor

Le bouton est cliquable uniquement sur sa partie mobile :

```text
MovingMeshComponent
```

Curseur :

```text
Push
```

Le clic ne doit pas appeler directement `TriggerPress`.

Le clic doit passer par :

```cpp
RuntimeActor->TryInteractAtEdge(CellX, CellY, Edge, PartyPawn)
```

Cela permet au `UGridActivationComponent` de :

- déclencher l’animation du bouton ;
- exécuter les liens sortants ;
- envoyer l’événement `Activated`.

### 5.2 AGridLeverActor

Le levier est cliquable uniquement sur sa partie mobile :

```text
MovingMeshComponent
```

Curseur :

```text
Pull
```

Le clic ne doit pas appeler directement `ToggleLever` ou `SetLeverState`.

Le clic passe par :

```cpp
RuntimeActor->TryInteractAtEdge(CellX, CellY, Edge, PartyPawn)
```

Cela préserve :

- l’état ON/OFF ;
- les événements `Activated` / `Deactivated` ;
- les liens logiques.

### 5.3 AGridGenericObjectActor

Les objets génériques ne sont cliquables que s’ils possèdent un texte lisible :

```cpp
HasReadableText() == true
```

Curseur :

```text
Read
```

Le clic ne doit pas appeler directement `ShowReadableMessage`.

Il passe par :

```cpp
RuntimeActor->TryInteractAtEdge(CellX, CellY, Edge, PartyPawn)
```

Cela conserve la logique de `UGridActivationComponent::ActivateReadableObject`.

### 5.4 AGridItemActor

Les items placés au sol sont cliquables directement.

Curseur :

```text
Take
```

Le clic appelle :

```cpp
RuntimeActor->TryPickupItemActor(this, PartyPawn)
```

Cela permet de ramasser l’acteur réellement cliqué, et non seulement le premier item trouvé dans une cellule.

Le pickup :

- ajoute l’item à l’inventaire ;
- appelle `OnRemovedFromWorld` ;
- détruit l’acteur ;
- retire l’entrée runtime correspondante.

### 5.5 AGridReceptacleActor

Les réceptacles sont génériques : support de torche, alcôve, autel, bol d’offrande, niche, socle, etc.

Ils ne doivent pas connaître spécifiquement la torche.

#### Réceptacle vide

Si le joueur tient un item compatible :

```text
clic sur support = déposer l’objet tenu
```

Curseur :

```text
Use
```

Le clic passe par :

```cpp
RuntimeActor->TryInteractAtEdge(CellX, CellY, Edge, PartyPawn)
```

#### Réceptacle plein

Le support plein ne reprend pas l’objet.

```text
clic sur support = rien
```

Pour reprendre un objet contenu :

```text
clic sur l’objet contenu = reprendre
```

Curseur :

```text
Take
```

Cela respecte la règle de design : le joueur clique sur l’objet manipulé, pas sur une action abstraite.

### 5.6 AGridDoorActor

Les portes ne sont pas cliquables directement.

Une porte doit rester un résultat d’un mécanisme :

- bouton ;
- levier ;
- plaque ;
- trigger ;
- lien logique ;
- script.

## 6. Réceptacles et torches

Une torche placée sur un support est un `AGridItemActor` contenu dans un `AGridReceptacleActor`.

Problème rencontré et corrigé :

```text
Le clic touchait l’AGridItemActor de la torche.
L’item essayait de se ramasser comme un item au sol via TryPickupItemActor.
Mais la torche contenue n’est pas dans SpawnedItemEntries.
Le pickup échouait donc.
```

Correction :

```text
Si un AGridItemActor a pour owner un AGridReceptacleActor,
il délègue CanInteract, Interact, GetInteractionCursor et GetInteractionText au réceptacle owner.
```

Ainsi :

```text
Torche au sol       → pickup item au sol
Torche sur support  → délégation au réceptacle
```

Cette règle évite de coder un `TorchHolderActor` spécifique.

## 7. Curseur custom

Le curseur custom est géré par :

```text
WBP_GridMouseCursor
```

La fonction Blueprint exposée est :

```text
SetCursorState(Cursor : EGridInteractionCursor)
```

Correspondance des textures :

```text
None      → Cursor_None
Default   → Cursor_Default
Use       → Cursor_Use
Push      → Cursor_Push
Pull      → Cursor_Pull
Take      → Cursor_Take
Read      → Cursor_Read
Locked    → Cursor_Forbidden
Forbidden → Cursor_Forbidden
```

Le widget suit la souris dans son `Event Tick` via :

```text
Get Mouse Position on Viewport
Break Vector2D
X + 4
Y + 4
Set Render Translation sur CursorImage
```

Attention : `Set Brush From Texture` doit cibler `CursorImage`, pas `self`.

Le node correct est :

```text
Set Brush From Texture — Target is Image
```

et non :

```text
Set Brush From Texture — Target is Border
```

## 8. Ancien système F / UseAction

L’ancien système clavier existe encore dans `AGrimrockPartyPawn`.

`UseAction`, `HandleUse`, `TryUseFrontInteraction` et le buffer `Use` ne sont pas supprimés.

Mais l’action clavier est neutralisée par défaut via :

```cpp
bEnableLegacyKeyboardUseAction = false
```

Le binding de `UseAction` n’est actif que si ce booléen est explicitement activé.

Cela permet de conserver l’ancien système comme outil de debug, sans en faire le comportement joueur normal.

## 9. Règles à ne pas casser

Les règles suivantes doivent rester stables :

```text
Ne pas rendre les portes cliquables directement.
Ne pas réactiver F par défaut.
Ne pas appeler TriggerPress directement depuis le clic souris.
Ne pas appeler ToggleLever directement depuis le clic souris.
Ne pas appeler ShowReadableMessage directement depuis le clic souris.
Ne pas coder la torche en dur dans les réceptacles.
Ne pas créer de TorchHolderActor spécifique sauf décision future explicite.
Ne pas rendre toutes les décorations cliquables : seulement les lisibles.
Ne pas faire reprendre un objet en cliquant sur le support plein : cliquer sur l’objet contenu.
```

Les interactions de mécanismes doivent passer par :

```cpp
RuntimeActor->TryInteractAtEdge(CellX, CellY, Edge, PartyPawn)
```

Cela garantit que `UGridActivationComponent` reste l’autorité sur les liens et les événements.

## 10. Tests de non-régression

Après toute modification du système d’interaction, tester :

```text
Bouton : hover sur partie mobile → Push, clic → activation.
Levier : hover sur partie mobile → Pull, clic → bascule.
Objet lisible : hover → Read, clic → message.
Item au sol : hover → Take, clic → inventaire.
Torche sur support : hover torche → Take, clic → inventaire.
Réceptacle vide + item tenu compatible : hover support → Use, clic → dépôt.
Réceptacle vide sans item tenu : pas de curseur interactif.
Réceptacle plein + clic support : rien.
Réceptacle plein + clic item contenu : reprise.
Porte : aucun curseur interactif, aucun clic direct.
Hors portée : curseur Forbidden.
Touche F : inactive par défaut.
Curseur custom : texture correcte selon EGridInteractionCursor.
```

## 11. État actuel

Le système souris validé couvre actuellement :

```text
Boutons
Leviers
Objets lisibles
Items au sol
Réceptacles vides
Items contenus dans réceptacles
Torches sur supports
Curseur custom UMG
```

Les extensions futures possibles sont :

- amélioration graphique du curseur ;
- sons de hover / clic refusé ;
- highlight runtime des objets interactifs ;
- interaction avec inventaire complet ;
- script d’événements plus riche ;
- règles avancées pour objets d’énigme.
