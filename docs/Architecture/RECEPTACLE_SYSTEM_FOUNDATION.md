# Architecture du système de réceptacles

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> L'ancien stockage \`FGridLevelObjectData / UGridLevelAsset::Objects\` est
> supprimé. Un réceptacle authored est aujourd'hui un
> \`FGridWorldObjectInstance\` résolu contre une
> \`UGridWorldObjectDefinitionAsset\`.

## 1. Définition / instance

La définition partage les règles par défaut dans
\`FGridObjectBehaviorParams::Receptacle\` :

\`\`\`text
bAcceptAnyItem
AcceptedItems[]
InitialContent[]
MaxContainedItems
VisualPlacementMode
bSimulatePhysicsWhenPlaced
PhysicalPlacementSurfaceOffset
PhysicalPlacementInitialRotationOffset
\`\`\`

Le placement \`FGridWorldObjectInstance\` porte :

- \`InstanceId\` ;
- \`WorldObjectDefinitionId\` ;
- cellule/surface/facing ;
- \`InstanceConfig.ReceptacleInitialContent\` ;
- éventuels \`InteractionOverrides\` sparse.

Les overrides ne copient pas tout le comportement : ils ne remplacent que les
familles explicitement authorées comme exception.

## 2. Runtime

\`AGridReceptacleActor\` porte la collection logique
\`ContainedItems\` de \`FGridContainedReceptacleItem\`.

Chaque entrée utilise :

- \`RuntimeObjectId\` ;
- \`ItemDefinitionId\` ;
- définition résolue/cache si disponible ;
- quantité ;
- Actor visuel optionnel.

La présence d'un Actor n'est jamais l'autorité de possession.

## 3. Permissions runtime

Le runtime distingue :

\`\`\`text
bCanRemoveItem
bCanInsertItems
\`\`\`

Ces permissions peuvent être modifiées par commande sans changer la définition
authored.

## 4. Acceptation

La validation d'insertion utilise la définition résolue et les éventuels
overrides d'instance :

- capacité ;
- accept-any ou liste \`AcceptedItems\` ;
- identité/type/tag selon règles de domaine pertinentes.

Une insertion refusée ne vide jamais la source.

## 5. Placement visuel

\`EGridReceptacleVisualPlacementMode\` :

\`\`\`text
AttachedSocket
PhysicalAtHit
\`\`\`

La représentation visuelle reste séparée de l'entrée logique contenue.

Le mode physique peut conserver simulation/gravité selon la configuration
authored.

## 6. Transferts

Les transferts inventaire/curseur/monde ↔ réceptacle passent par les services
transactionnels d'item.

Principe :

\`\`\`text
prévalider
    -> muter source/cible
    -> rollback si nécessaire
    -> publier les événements après succès
\`\`\`

## 7. Événements et commandes

Événements :

- ItemInserted ;
- ItemRemoved ;
- ItemChanged ;
- événements d'état pertinents.

Commandes :

- consommation d'un item / tous ;
- enable/disable removal ;
- enable/disable insertion.

Les links voient l'état final réussi, pas un état intermédiaire.

## 8. Interaction souris

Un item tenu peut être routé vers un Receptacle sous le curseur lorsque :

- la cible est accessible ;
- l'insertion est autorisée ;
- l'item est accepté ;
- la capacité le permet.

Le hover ne mute rien. Un refus ne devient pas un dépôt monde implicite.

Pour le retrait, l'item visuel/contenu touché et les règles du Receptacle
déterminent l'action.

## 9. Persistance

\`FGridRuntimeReceptacleState\` sauvegarde :

\`\`\`text
ObjectId
bCanRemoveItem
bCanInsertItems
ContainedItems[]
\`\`\`

Le restore recrée la représentation depuis le contenu logique et les
définitions courantes.

## 10. Initial content

Le contenu initial appartient à l'authoring (Definition puis override local
éventuel). Après démarrage/restauration, le RuntimeState prend l'autorité sur le
contenu vivant.

## 11. Invariants

1. Definition = règles partagées.
2. Instance = différences authored locales.
3. \`ContainedItems\` = contenu runtime logique.
4. Actor Item = présentation optionnelle.
5. Transfert atomique.
6. Événements de succès après commit.
7. Permissions runtime persistées.
8. Aucune seconde définition collectible propre au Receptacle.
