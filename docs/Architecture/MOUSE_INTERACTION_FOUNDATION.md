# Fondation des interactions souris

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> Cette fondation reflète les stabilisations MI1–MI6 et le routage réellement
> présent dans \`AGrimrockPlayerController\`.

## 1. Principe

\`\`\`text
PlayerController
    -> détecte / arbitre l'intention

Actor / service de domaine
    -> décide et exécute la règle métier
\`\`\`

Le contrôleur ne code pas « bouton ouvre porte » ou « torche entre dans
réceptacle ». Il orchestre des interfaces et services existants.

## 2. Résolution centrale du clic

\`ResolveLeftMouseInteraction()\` construit une intention unique pour le clic
courant.

Intentions actuelles couvrant notamment :

\`\`\`text
DismissReadableMessage
IgnoreInventoryUiWithoutCursorItem
IgnoreModalUi
CursorItemNoWorldHit
CursorItemCannotPlace
CursorItemWallLock
CursorItemReceptacle
CursorItemWorldDrop
CursorItemThrow
WorldInteractable
WorldInteractableOutOfRange
WorldInteractableInvalidPawnOrComponent
WorldInteractableCanInteractRejected
FallbackNoInteractable
\`\`\`

Une résolution ne mute pas le gameplay ; le handler exécute ensuite le chemin
correspondant.

## 3. Priorité

Ordre fonctionnel :

1. fermer un message lisible actif ;
2. respecter UI modale / menu item ;
3. lorsque Inventory est ouvert sans item curseur, ne pas cliquer le monde ;
4. si un item est tenu : wall lock, receptacle, dépôt monde ou lancer selon la
   cible et les règles ;
5. sinon interaction monde classique ;
6. aucun hit/action valide : fallback silencieux.

Un refus ne doit pas être transformé en action différente involontaire. Exemple :
une mauvaise clé sur WallLock ne devient pas un dépôt au sol.

## 4. Hover avec item au curseur

\`ResolveCursorItemHoverCursor()\` ne fait qu'évaluer l'affordance :

- wall lock compatible/incompatible ;
- receptacle compatible/incompatible ;
- dépôt monde ;
- lancer ;
- interdit/neutre.

Aucune mutation n'est autorisée pendant le hover.

\`SetGridInteractionCursor()\` est le point central de sortie vers le curseur
custom.

## 5. Interaction monde

Les acteurs interactifs implémentent \`IGridInteractableInterface\` :

\`\`\`text
CanInteract(...)
Interact(...)
InteractWithHit(...)
GetInteractionCursor(...)
GetInteractionText(...)
\`\`\`

Le composant touché peut faire partie du contrat : bouton/levier, item,
réceptacle, chaîne de porte, readable, etc.

La porte elle-même ne devient pas un bouton générique : ses mécanismes, liens
ou chaîne éventuelle conservent leurs règles propres.

## 6. Trace et distance

Le premier hit bloquant pertinent de visibilité possède le clic. La distance
d'interaction est contrôlée avant l'exécution d'une interaction monde.

Le système ne recherche pas une cible « derrière » un composant bloquant pour
contourner les collisions authored.

## 7. Item tenu

Le controller délègue :

- compatibilité WallLock au système de lock ;
- acceptation Receptacle aux règles/transfer service ;
- dépôt monde aux services d'item ;
- lancer au pipeline de targeting/projectile.

Ownership reste atomique : un échec laisse l'item à sa source.

## 8. UI et silence des refus

L'exploration normale utilise le curseur comme feedback principal. Un clic qui
ne produit aucune mutation peut rester silencieux ; les raisons diagnostiques
restent dans \`LogGridMouse\`/logs appropriés.

Les modes explicites de combat/throw targeting gardent leurs feedbacks propres.

## 9. Readables

Le message lisible actif a priorité sur le clic monde. Sa fermeture consomme le
clic et n'active rien derrière.

Mouvement/rotation peuvent aussi fermer le message selon le contrat du Pawn.

## 10. Invariants

1. Un clic = au plus une action.
2. Hover = zéro mutation.
3. UI modale prime sur monde.
4. Refus métier ne déclenche pas un fallback dangereux.
5. Ownership/transferts restent atomiques.
6. \`IGridInteractableInterface\` reste local à l'acteur touché.
7. Le controller orchestre, il ne devient pas l'autorité de lock/receptacle/item.
