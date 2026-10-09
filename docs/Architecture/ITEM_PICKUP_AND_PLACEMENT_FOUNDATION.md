# Fondation du ramassage et du placement des items

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> Les collectibles utilisent directement \`UGridItemDefinitionAsset\`; aucun
> World Object Definition miroir n'est requis.

## 1. Définitions et instances

\`UGridItemDefinitionAsset\` porte les données partagées d'un item :
identité, présentation, poids, stack, équipement, stats, actions, lumière,
sparkle et mesh monde.

\`FGridItemInstance\` porte l'état vivant :

- \`RuntimeObjectId\` ;
- \`ItemDefinitionId\` ;
- quantité ;
- ownership ;
- état lumière ;
- données runtime nécessaires.

Un item dans le niveau authored utilise \`FGridLooseItemInstance\` avec une
référence directe \`ItemDefinition\`.

## 2. Ownership

Un item possède un seul owner logique à la fois :

\`\`\`text
World
Receptacle
CharacterInventory
EquipmentSlot
Cursor
HeldBySelectedCharacter
Removed
\`\`\`

Les Actors monde/held ne remplacent jamais cette autorité logique.

## 3. Party inventory

\`UGridPartyInventoryComponent\` possède
\`FGridPartyInventoryState\` et les états d'équipement des personnages.

L'implémentation est répartie entre noyau, transferts curseur, équipement,
monde, hotbar et diagnostics tout en gardant une autorité unique.

## 4. Transfer service

\`UGridItemTransferService\` centralise les transactions entre :

- inventory ;
- equipment ;
- cursor ;
- world ;
- receptacle ;
- autre personnage.

Contrat :

\`\`\`text
validate source + target
    -> commit atomique
    -> rollback en cas d'échec
    -> notification
\`\`\`

Aucune duplication/perte d'instance n'est acceptable.

## 5. Mouse interaction

Avec un item au curseur, \`AGrimrockPlayerController\` résout notamment :

- WallLock ;
- Receptacle ;
- world drop ;
- throw.

Le controller orchestre ; les règles métier restent dans les systèmes
correspondants.

Un refus explicite de cible ne doit pas être transformé en dépôt involontaire.

## 6. World Item

\`AGridItemActor\` est la représentation générique d'un pickup monde.

Il peut prendre en charge :

- collision/physics ;
- lumière data-driven ;
- sparkle ;
- interaction/pickup ;
- contenu readable.

La pose physique finale peut être gérée par Chaos lorsque la définition l'exige.

## 7. Throw

Le lancer utilise un vrai projectile/runtime Actor avec sweep collision.
Le targeting détermine l'intention ; la collision physique reste autoritaire sur
l'impact et la reconversion en World Item récupérable.

Les bindings historiques synthétiques supprimés ne constituent pas une voie de
compatibilité à maintenir.

## 8. Equipment / Held visual

L'équipement reste dans l'état personnage. Le visuel tenu est reconstruit depuis
l'item équipé du personnage sélectionné.

Une lumière portée peut déléguer son PointLight au
\`UGridPartyIlluminationComponent\` tout en gardant le Niagara physique de
l'item.

## 9. Persistence

Les items monde sont capturés dans le DungeonRuntimeState.
Les items de groupe/équipement/hotbar vivent dans PartyInventoryState.

Les pointeurs/Actors transients sont réhydratés depuis les identités/définitions
courantes.

## 10. Invariants

1. Une seule \`UGridItemDefinitionAsset\` par collectible.
2. Un seul owner logique par instance.
3. \`RuntimeObjectId\` stable pour l'instance.
4. Transferts atomiques.
5. Actor = représentation, pas ownership.
6. Refus = aucune mutation partielle.
7. Les systèmes de Receptacle/Lock ne dupliquent pas la définition Item.
