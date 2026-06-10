# Fondation des interactions souris

## 1. Portée

Ce document décrit le socle runtime existant pour les interactions directes à la souris. Il couvre la sélection sous le curseur, la portée, les règles de grille, les curseurs, les messages lisibles et les interactions déjà raccordées. Il ne définit pas de nouveau système de gameplay.

## 2. Cartographie du code

| Domaine | Déclaration | Implémentation |
|---|---|---|
| Contrôleur souris | `Source/GrimrockPrototype/Public/Runtime/GrimrockPlayerController.h` | `Source/GrimrockPrototype/Private/Runtime/GrimrockPlayerController.cpp` |
| Pawn et solution de repli au clavier | `Source/GrimrockPrototype/Public/Runtime/GrimrockPartyPawn.h` | `Source/GrimrockPrototype/Private/Runtime/GrimrockPartyPawn.cpp` |
| Contrat interactif | `Source/GrimrockPrototype/Public/Runtime/GridInteractableInterface.h` | fonctions virtuelles implémentées par les acteurs |
| Résolution du pawn et du runtime | `Source/GrimrockPrototype/Public/Runtime/GridInteractionUtils.h` | `Source/GrimrockPrototype/Private/Runtime/GridInteractionUtils.cpp` |
| Règles de grille et texte lisible | `Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h` | `Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp` |
| Widget de lecture | `Source/GrimrockPrototype/Public/UI/ReadableMessageWidget.h` | `Source/GrimrockPrototype/Private/UI/ReadableMessageWidget.cpp` |

Les acteurs interactifs actuels sont `AGridButtonActor`, `AGridLeverActor`, `AGridGenericObjectActor`, `AGridItemActor`, `AGridReceptacleActor` et, uniquement par sa chaîne optionnelle, `AGridDoorActor`.

## 3. Entrée et sélection

`AGrimrockPlayerController` lie directement `LeftMouseButton` à `HandleLeftMousePressed()`. Le contrôleur active le curseur et un mode `GameAndUI` ; `PlayerTick()` recalcule le survol.

La sélection suit un rayon obtenu par déprojection de la position de la souris sur `ECC_Visibility`. Seul le premier impact bloquant est considéré. Un mur, une porte fermée ou tout autre composant bloquant `Visibility` interdit donc la sélection d'un acteur situé derrière. Le système ne recherche pas en profondeur une autre cible valide pour le gameplay.

Le composant touché est conservé dans `FHitResult`, puis transmis à :

- `CanInteract()` pour valider la cible et le sous-composant ;
- `GetInteractionCursor()` pour le retour de survol ;
- `InteractWithHit()` pour l'action.

La distance maximale est `AGrimrockPlayerController::MaxInteractionDistance`, initialisée à `300 cm` et mesurée entre la position du pawn et `HitResult::ImpactPoint`.

## 4. Priorité d'un clic gauche

L'ordre réel est :

1. fermer le message lisible actif, puis consommer le clic ;
2. si un item est porté par le curseur, tenter uniquement le réceptacle directement touché ;
3. sinon, résoudre l'acteur interactif directement touché ;
4. vérifier la distance ;
5. appeler `CanInteract()` sur le composant touché ;
6. exécuter une seule fois `InteractWithHit()` ;
7. ne rien faire si une étape échoue.

Il n'existe pas de solution de repli à la souris vers la cellule ou le bord situé en face. `AGrimrockPartyPawn::TryUseFrontInteraction()` reste une solution de repli historique au clavier, désactivée par défaut avec `bEnableLegacyKeyboardUseAction=false`.

## 5. Règles de grille

Les objets placés sur un bord ne sont pas validés par la seule distance. Les boutons, leviers, réceptacles et chaînes de porte appellent `AGridLevelRuntimeActor::CanPartyInteractWithEdgeObject()`.

Une interaction de bord est autorisée uniquement si l'objet se trouve :

- sur la cellule du groupe et sur le bord regardé ;
- ou dans la cellule située devant le groupe et sur son bord opposé.

Les boutons, leviers et objets lisibles placés sur un bord font ensuite transiter l'action par `TryInteractAtEdge()`, afin de conserver l'activation et les liens runtime. Les items utilisent les règles propres à `CanPartyPickupItemEntry()` et au service de transfert.

## 6. Acteurs et composants cliquables

| Acteur | Composant accepté | Curseur | Action |
|---|---|---|---|
| `AGridButtonActor` | mesh mobile | `Push` | interaction de bord via le runtime |
| `AGridLeverActor` | mesh mobile | `Pull` | interaction de bord via le runtime |
| `AGridGenericObjectActor` lisible | mesh principal | `Read` | activation du texte via le runtime |
| `AGridItemActor` | mesh de l'item | `Take` | ramassage ou retrait du réceptacle |
| `AGridReceptacleActor` | support ou mesh d'un item contenu | `Use` ou `Take` | dépôt ou retrait |
| `AGridDoorActor` | volume de la chaîne optionnelle | `Take` actuellement | traction directe de la chaîne |

La surface normale d'une porte n'est pas interactive. Les plaques de pression et les triggers ne sont pas des cibles pour la souris.

## 7. Dépôt d'un item

Lorsqu'un item est attaché au curseur, le contrôleur recherche un `AGridReceptacleActor` sur le premier impact `Visibility`. Le dépôt exige ensuite :

- une distance valide ;
- une position de cellule et de bord compatible avec l'orientation du groupe ;
- `CanAcceptItemInstance()` vrai ;
- un placement effectué avec le `FHitResult` direct.

Un clic ailleurs ne dépose plus implicitement l'item dans le réceptacle situé en face.

## 8. Curseur et interface

`EGridInteractionCursor` contient `None`, `Default`, `Use`, `Push`, `Pull`, `Take`, `Read`, `Locked`, `Forbidden`, `PlaceItem` et `CannotPlaceItem`.

Le widget configuré dans `CustomCursorWidgetClass` doit fournir la fonction Blueprint `SetCursorState(EGridInteractionCursor)` et rester `HitTestInvisible`. Sans widget personnalisé, le curseur système de repli ne distingue actuellement que l'état `Take`, représenté par une main.

Le survol affiche :

- `Forbidden` pour une cible interactive directe hors de portée ;
- le curseur fourni par l'acteur si `CanInteract()` accepte ;
- `PlaceItem` ou `CannotPlaceItem` lorsqu'un item est porté ;
- `Default` dans les autres cas.

## 9. Messages lisibles

`AGridLevelRuntimeActor::ShowReadableMessage()` crée ou met à jour `UReadableMessageWidget`. La fermeture automatique est optionnelle et désactivée par défaut. Le premier clic gauche suivant appelle `DismissReadableMessage()` et ne déclenche aucune autre interaction.

Les actions de déplacement et de rotation du pawn ferment également le message avant de poursuivre leur propre action. Le widget est ajouté au viewport avec un ordre Z de `50` ; le curseur personnalisé utilise `9999`.

## 10. Invariants

- Un clic déclenche au plus une interaction.
- Le premier composant bloquant `ECC_Visibility` est autoritaire.
- Une cible hors de portée n'est pas activée.
- Une interaction de bord respecte cellule, bord et orientation.
- Les composants non prévus par `CanInteract()` ne sont pas cliquables.
- Les liens et activations restent exécutés par le runtime lorsqu'un acteur le requiert.
- L'efficacité de l'occlusion dépend des collisions `Visibility` des meshes et volumes utilisés par les assets.

## 11. Validation manuelle

- Cliquer un bouton et un levier de face, puis depuis un autre bord.
- Vérifier qu'un mur ou une porte fermée bloque une cible placée derrière.
- Ramasser un item dans la cellule courante et sur le bord opposé de la cellule située devant.
- Déposer un item uniquement en cliquant directement sur un réceptacle compatible.
- Vérifier les curseurs hors de portée et sur un sous-composant refusé.
- Lire un texte, puis confirmer que le clic suivant ferme seulement le message.
- Tester la chaîne d'une porte depuis le bon bord et depuis un bord incorrect.
