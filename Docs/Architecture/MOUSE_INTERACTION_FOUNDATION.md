# Fondation des interactions souris

## 1. Portee

Ce document decrit le socle runtime existant pour les interactions directes a la souris. Il couvre la selection sous le curseur, la portee, les regles de grille, les curseurs, les messages lisibles et les interactions deja branchees. Il ne definit pas de nouveau systeme de gameplay.

## 2. Cartographie du code

| Domaine | Declaration | Implementation |
|---|---|---|
| Controleur souris | `Source/GrimrockPrototype/Public/Runtime/GrimrockPlayerController.h` | `Source/GrimrockPrototype/Private/Runtime/GrimrockPlayerController.cpp` |
| Pawn et fallback clavier | `Source/GrimrockPrototype/Public/Runtime/GrimrockPartyPawn.h` | `Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawn.cpp` |
| Contrat interactif | `Source/GrimrockPrototype/Public/Runtime/GridInteractableInterface.h` | fonctions virtuelles implementees par les acteurs |
| Resolution pawn/runtime | `Source/GrimrockPrototype/Public/Runtime/GridInteractionUtils.h` | `Source/GrimrockPrototype/Private/Runtime/GridInteractionUtils.cpp` |
| Regles de grille et texte lisible | `Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h` | `Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp` |
| Widget de lecture | `Source/GrimrockPrototype/Public/UI/ReadableMessageWidget.h` | `Source/GrimrockPrototype/Private/UI/ReadableMessageWidget.cpp` |

Les acteurs interactifs actuels sont `AGridButtonActor`, `AGridLeverActor`, `AGridGenericObjectActor`, `AGridItemActor`, `AGridReceptacleActor` et, uniquement par sa chaine optionnelle, `AGridDoorActor`.

## 3. Entree et selection

`AGrimrockPlayerController` lie directement `LeftMouseButton` a `HandleLeftMousePressed()`. Le controleur active le curseur et un mode `GameAndUI`; `PlayerTick()` recalcule le survol.

La selection suit un rayon deprojection souris sur `ECC_Visibility`. Seul le premier hit bloquant est considere. Un mur, une porte fermee ou tout autre composant bloquant `Visibility` interdit donc la selection d'un acteur situe derriere. La validite gameplay n'est pas recherchee en profondeur.

Le composant touche est conserve dans `FHitResult` puis transmis a :

- `CanInteract()` pour valider la cible et le sous-composant ;
- `GetInteractionCursor()` pour le retour de survol ;
- `InteractWithHit()` pour l'action.

La distance maximale est `AGrimrockPlayerController::MaxInteractionDistance`, initialisee a `300 cm`, mesuree entre la position du pawn et `HitResult::ImpactPoint`.

## 4. Priorite d'un clic gauche

L'ordre reel est :

1. fermer le message lisible actif, puis consommer le clic ;
2. si un item est porte par le curseur, tenter uniquement le receptacle directement touche ;
3. sinon, resoudre l'acteur interactif directement touche ;
4. verifier la distance ;
5. appeler `CanInteract()` sur le composant touche ;
6. executer une seule fois `InteractWithHit()` ;
7. ne rien faire si une etape echoue.

Il n'existe pas de fallback souris vers la cellule ou le bord en face. `AGrimrockPartyPawn::TryUseFrontInteraction()` reste un fallback clavier historique, desactive par defaut avec `bEnableLegacyKeyboardUseAction=false`.

## 5. Regles de grille

Les objets de bord ne sont pas valides par la distance seule. Boutons, leviers, receptacles et chaine de porte appellent `AGridLevelRuntimeActor::CanPartyInteractWithEdgeObject()`.

Une interaction de bord est autorisee uniquement si l'objet est :

- sur la cellule du groupe et sur le bord regarde ;
- ou dans la cellule avant et sur son bord oppose.

Les boutons, leviers et objets lisibles de bord routent ensuite l'action par `TryInteractAtEdge()`, afin de conserver l'activation et les liens runtime. Les items utilisent les regles propres de `CanPartyPickupItemEntry()` et du service de transfert.

## 6. Acteurs et composants cliquables

| Acteur | Composant accepte | Curseur | Action |
|---|---|---|---|
| `AGridButtonActor` | mesh mobile | `Push` | interaction de bord via le runtime |
| `AGridLeverActor` | mesh mobile | `Pull` | interaction de bord via le runtime |
| `AGridGenericObjectActor` lisible | mesh principal | `Read` | activation du texte via le runtime |
| `AGridItemActor` | mesh de l'item | `Take` | ramassage ou retrait du receptacle |
| `AGridReceptacleActor` | support ou mesh d'un item contenu | `Use` ou `Take` | depot ou retrait |
| `AGridDoorActor` | volume de la chaine optionnelle | `Take` actuellement | traction directe de la chaine |

La surface normale d'une porte n'est pas interactive. Les plaques de pression et triggers ne sont pas des cibles souris.

## 7. Depot d'item

Lorsqu'un item est attache au curseur, le controleur cherche un `AGridReceptacleActor` sur le premier hit `Visibility`. Le depot exige ensuite :

- une distance valide ;
- une position cellule/bord compatible avec le facing ;
- `CanAcceptItemInstance()` vrai ;
- un placement effectue avec le `FHitResult` direct.

Un clic ailleurs ne depose plus implicitement l'item dans le receptacle situe en face.

## 8. Curseur et interface

`EGridInteractionCursor` contient `None`, `Default`, `Use`, `Push`, `Pull`, `Take`, `Read`, `Locked`, `Forbidden`, `PlaceItem` et `CannotPlaceItem`.

Le widget configure dans `CustomCursorWidgetClass` doit fournir la fonction Blueprint `SetCursorState(EGridInteractionCursor)` et rester `HitTestInvisible`. Sans widget custom, le fallback systeme ne distingue actuellement que `Take` avec le curseur main.

Le survol affiche :

- `Forbidden` pour une cible interactive directe hors portee ;
- le curseur fourni par l'acteur si `CanInteract()` accepte ;
- `PlaceItem` ou `CannotPlaceItem` lorsqu'un item est porte ;
- `Default` dans les autres cas.

## 9. Messages lisibles

`AGridLevelRuntimeActor::ShowReadableMessage()` cree ou met a jour `UReadableMessageWidget`. L'auto-fermeture est optionnelle et desactivee par defaut. Le premier clic gauche suivant appelle `DismissReadableMessage()` et ne traverse pas vers une autre interaction.

Les actions de deplacement et rotation du pawn ferment aussi le message avant de poursuivre leur propre action. Le widget est ajoute au viewport avec un Z-order de `50`; le curseur custom utilise `9999`.

## 10. Invariants

- Un clic ne declenche au plus qu'une interaction.
- Le premier composant bloquant `ECC_Visibility` est autoritaire.
- Une cible hors portee n'est pas activee.
- Une interaction de bord respecte cellule, bord et orientation.
- Les composants non prevus par `CanInteract()` ne sont pas cliquables.
- Les liens et activations restent executes par le runtime lorsqu'un acteur le requiert.
- L'efficacite de l'occlusion depend des collisions `Visibility` des meshes et volumes utilises par les assets.

## 11. Validation manuelle

- Cliquer un bouton et un levier de face, puis depuis un autre bord.
- Verifier qu'un mur ou une porte fermee bloque une cible placee derriere.
- Ramasser un item dans la cellule courante et sur le bord oppose de la cellule avant.
- Deposer un item uniquement en cliquant directement un receptacle compatible.
- Verifier les curseurs hors portee et sur un sous-composant refuse.
- Lire un texte, puis confirmer que le clic suivant ferme seulement le message.
- Tester la chaine d'une porte depuis le bon bord et depuis un bord incorrect.

