# 10 — Mouse Interaction System

## 1. Objectif

Ce document décrit le système d’interaction souris du prototype Grimrock.

L’objectif est de remplacer l’ancienne action clavier globale `F` par des interactions directes à la souris : le joueur clique sur l’objet, ou sur la partie de l’objet, qu’il souhaite manipuler.

Le système doit rester compatible avec l’architecture orientée données du projet : les objets de niveau sont définis dans le `GridLevelAsset`, instanciés en runtime, puis activés via les composants runtime existants.

Depuis les stabilisations MI1 à MI6, ce document décrit l’état final attendu du routage souris : les diagnostics existent, les priorités UI / monde sont explicites, le hover d’item tenu est stable et la verbosité des logs de hover est contrôlée par `LogGridMouse`.

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
- appeler `Interact` lors du clic gauche ;
- arbitrer la priorité entre UI modale, inventaire, item tenu au curseur et interaction monde ;
- router le clic gauche vers le chemin gameplay approprié sans exécuter lui-même la logique métier.

Le contrôleur ne doit pas contenir de logique spécifique du type :

```text
si bouton → ouvrir porte
si levier → basculer état
si torche → inventaire
```

Il ne connaît que l’interface générique.

La responsabilité importante du contrôleur est donc l’orchestration :

```text
Entrée souris
  -> filtrage UI / readable message
  -> résolution item tenu ou cible monde
  -> appel au runtime, à l'interface ou au service adapté
```

Il ne doit pas devenir le propriétaire des règles de compatibilité item, de serrure, de réceptacle ou de mécanisme.

### 3.1.1 ResolveLeftMouseInteraction

`ResolveLeftMouseInteraction()` centralise la décision du clic gauche.

Son rôle est de produire un seul routage clair pour le clic courant :

- fermer prioritairement un message lisible actif ;
- bloquer le clic monde lorsqu'une UI modale ou un menu d'action item est actif ;
- ignorer le clic monde lorsque l'inventaire est ouvert sans item tenu au curseur ;
- router un item tenu au curseur vers une serrure murale, un réceptacle, un dépôt monde ou un lancer ;
- exécuter l'interaction monde classique si aucun item n'est tenu ;
- tomber sur un fallback explicite `NoInteractable` lorsqu'aucune cible valide n'est trouvée.

Cette fonction sépare la résolution d'intention de l'exécution gameplay. Elle ne remplace pas `IGridInteractableInterface`, `TryInteractAtEdge`, les services de transfert ou les règles propres aux acteurs.

### 3.1.2 ResolveCursorItemHoverCursor

`ResolveCursorItemHoverCursor()` est la résolution de hover spécifique au cas où un item est tenu au curseur.

Elle évalue la cible sous la souris et retourne le curseur le plus honnête possible avant le clic :

- serrure murale accessible ;
- réceptacle compatible ;
- dépôt monde valide ;
- lancer possible pour un item throwable ;
- cible détectée mais action impossible ;
- aucun hit monde.

Le hover n'exécute aucune mutation. Il ne fait que traduire l'état courant en feedback visuel, avec des logs `Verbose` pour diagnostiquer les refus sans polluer les sessions normales.

### 3.1.3 SetGridInteractionCursor

`SetGridInteractionCursor()` est le point de sortie unique vers le curseur custom.

Il reçoit l'état `EGridInteractionCursor` résolu par le hover ou le routage courant, puis met à jour `WBP_GridMouseCursor`. Le reste du système ne doit pas manipuler directement la texture du curseur.

Cette centralisation permet de conserver une correspondance stable entre :

```text
état gameplay résolu
  -> EGridInteractionCursor
  -> WBP_GridMouseCursor.SetCursorState
  -> texture affichée
```

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

L'interface sert au monde interactable classique : bouton, levier, item au sol, objet lisible, item contenu, support ou autre acteur de niveau.

Elle ne doit pas absorber les responsabilités suivantes :

- décider si une UI modale bloque le clic ;
- fermer un menu d'inventaire ;
- choisir une action contextuelle d'item ;
- résoudre une cible face au groupe pour le menu clic droit ;
- exécuter des transferts d'inventaire hors du contexte de l'acteur touché.

Le contrat est volontairement local : l'acteur sous la souris dit s'il est interactif et comment il souhaite être présenté. Le runtime ou les services d'item restent responsables des mutations globales.

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

## 4.1 Priorité finale du clic gauche

Le clic gauche suit une priorité unique, de la couche la plus globale vers la cible monde :

1. Readable message actif : le clic ferme le message et ne continue pas vers le monde.
2. UI modale ou menu d'action item détecté : le clic monde est bloqué.
3. Inventaire ouvert sans item au curseur : le clic monde est ignoré.
4. Item tenu au curseur : le routage tente, dans l'ordre, serrure murale sous souris, réceptacle ou support sous souris, dépôt monde, lancer, puis échec explicite.
5. World interactable : le premier acteur valide sous la souris reçoit l'interaction via `IGridInteractableInterface`.
6. Fallback `NoInteractable` : aucun acteur ou chemin item valide n'a été trouvé.

Cette priorité empêche les effets de bord suivants :

- fermer un message lisible et activer en même temps un objet derrière lui ;
- cliquer dans le monde au travers d'un menu d'action item ;
- déposer ou lancer un item alors que le joueur voulait interagir avec une serrure ou un support ;
- promettre un hover interactif qui ne correspond pas au routage réel du clic.

### 4.1.1 Routage d'un item tenu au curseur

Quand un item est tenu au curseur, la cible sous la souris prime sur la cible face au groupe.

Ordre de routage :

```text
1. Wall lock sous souris
2. Réceptacle / support sous souris
3. Dépôt monde valide
4. Lancer, si l'item est throwable et que le contexte le permet
5. Échec explicite avec log de refus
```

La cible face au groupe reste importante pour le menu contextuel d'inventaire, mais elle ne doit pas détourner un clic souris explicitement posé sur une cible monde différente.

## 4.2 Priorité finale du hover avec item tenu

Lorsque le joueur tient un item au curseur, le hover ne passe pas par le même chemin que le hover monde classique. `ResolveCursorItemHoverCursor()` applique la priorité suivante :

1. Wall lock accessible : curseur d'utilisation ou de verrouillage selon l'état et la compatibilité.
2. Réceptacle compatible : curseur `Use`.
3. Dépôt monde valide : curseur de dépôt ou curseur par défaut d'action item selon le mapping courant.
4. Lancer possible avec item throwable : feedback de lancer si aucune cible prioritaire ne consomme l'action.
5. Cible détectée mais action impossible : curseur `Forbidden`.
6. Aucun hit monde : curseur neutre, sans interaction promise.

Le hover est une indication, pas une réservation d'action. Le clic refait la validation avant toute mutation.

## 4.3 Séparation hover, clic et exécution gameplay

Les trois niveaux doivent rester séparés :

| Niveau | Responsabilité | Mutation gameplay |
|---|---|---|
| Hover | Détecter et afficher un feedback honnête via `SetGridInteractionCursor()` | Non |
| Clic | Résoudre l'intention et choisir le chemin de routage via `ResolveLeftMouseInteraction()` | Non directement |
| Exécution gameplay | Appeler `Interact`, `TryInteractAtEdge`, les services de transfert, wall lock, drop ou throw | Oui, après validation |

Cette séparation rend le système prévisible : le hover explique ce qui semble possible, le clic choisit une intention unique, puis le runtime ou le service spécialisé applique les règles définitives.

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

## 10. Logs et diagnostics

La catégorie dédiée au routage souris est :

```text
LogGridMouse
```

Règle de verbosité :

- les logs de clic restent au niveau `Log`, car ils documentent une décision utilisateur ponctuelle ;
- les logs de hover restent au niveau `Verbose`, car ils peuvent être émis à chaque frame ;
- les refus explicites doivent indiquer le chemin de routage concerné lorsque c'est utile : UI bloquante, inventaire ouvert, wall lock, réceptacle, drop, throw ou `NoInteractable`.

Pour diagnostiquer le hover en PIE, réactiver temporairement la verbosité :

```text
Log LogGridMouse Verbose
```

ou au lancement :

```text
-LogCmds="LogGridMouse Verbose"
```

Les logs de hover ne doivent pas être remontés au niveau `Log` par défaut. Cela rendrait les sessions normales trop bruyantes et masquerait les décisions de clic réellement utiles.

## 11. Tests de non-régression

Après toute modification du système d’interaction, tester :

| Cas | Attendu |
|---|---|
| Readable message actif | Le clic gauche ferme le message et ne déclenche aucune interaction monde derrière lui. |
| Inventaire ouvert sans item curseur | Le clic monde est ignoré ; aucun pickup, dépôt, activation ou lancer ne se produit. |
| Menu action item ouvert | Le clic extérieur ferme le menu si le click catcher le demande ; le clic monde reste bloqué. |
| Item monde pickup | Hover `Take`, clic gauche ramasse l'acteur réel visé et met à jour l'inventaire. |
| Levier | Hover sur partie mobile `Pull`, clic gauche passe par `TryInteractAtEdge` et bascule l'état. |
| Bouton | Hover sur partie mobile `Push`, clic gauche passe par `TryInteractAtEdge` et déclenche l'activation. |
| Porte / chaîne logique | La porte n'est pas cliquable directement ; elle réagit seulement aux mécanismes et liens. |
| Torche tenue au curseur vers sol | Hover de dépôt valide, clic gauche dépose la torche si la cellule et la portée l'acceptent. |
| Torche tenue au curseur vers support | Le support compatible prime sur le dépôt monde ; clic gauche place la torche dans le réceptacle. |
| Torche tenue au curseur vers cible invalide | Hover `Forbidden` ou neutre selon le hit ; clic gauche échoue explicitement sans mutation. |
| Pierre tenue au curseur vers dépôt proche | Le dépôt monde valide est préféré au lancer si aucune cible prioritaire n'est touchée. |
| Pierre tenue au curseur hors portée pour lancer | Le lancer est refusé explicitement ; l'item reste au curseur. |
| Clé compatible tenue au curseur vers wall lock | La wall lock sous souris prime ; la clé est insérée seulement après validation de compatibilité. |
| Mauvaise clé ou item non clé tenu au curseur vers wall lock | La wall lock refuse l'action avec un feedback interdit ou un message de refus ; aucun dépôt implicite ne remplace l'échec. |

Ces tests résument les stabilisations MI1 à MI6 sous forme de comportement final attendu. Ils doivent être complétés par les tests historiques :

```text
Réceptacle vide sans item tenu : pas de curseur interactif.
Réceptacle plein + clic support : rien.
Réceptacle plein + clic item contenu : reprise.
Hors portée : curseur Forbidden.
Touche F : inactive par défaut.
Curseur custom : texture correcte selon EGridInteractionCursor.
```

## 12. État actuel

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
