# ARM-CLEAN06.4X.51 — Préflight natif des quatre règles partagées, sans sauvegarde

**Parent :** `2690fc7dc335a552ffb0afd647604b5182d9176c` (X.50 vérifié SilentTask + LocalAuto).
**État :** préparation de la conversion binaire de quatre
`UGridWorldObjectDefinitionAsset` déjà équipés d'IDs canoniques.
**Aucun `.uasset`, `.umap`, SaveGame ou autre paquet réécrit.**

## Périmètre fermé

| Vrai package de règle | Ancienne propriété en attente | ItemId déjà canonique |
|---|---|---|
| `DA_Receptacle_TorchHolder` | `DefaultBehavior.Receptacle.AcceptedItems[0].ItemDefinition` | `Item_Torch` |
| `GridObjectArchetypeAsset/DA_Guardian` | Même propriété | `Gem_Blue` |
| `DA_Lock_CopperWall` | `DefaultBehavior.Lock.AcceptedKeyItems[0]` | `Key_Copper` |
| `DA_Lock_IronWall` | Même propriété | `Key_Iron` |

Ces quatre références sont parmi les **63 résiduelles X.48**
confirmées. Les 18 contenus initiaux de réceptacles, trois
overrides inactifs, 33 entrées de palette et cinq loots
de monstres sont exclus du périmètre. Le retrait de quatre
pointeurs ferait passer le solde attendu à **59**,
sous réserve d'une nouvelle vérification native après transaction.

## Nouveau test Editor : simulation sur copie transitoire

`Grimrock.RPG.ARM.CLEAN06_4X51.FourSharedRuleNativeDryRun`
charge le véritable `DA_ItemSystem` et les quatre vrais
assets ci-dessus, contrôle leurs identités et l'autorité
des nouveaux champs canoniques, puis appelle
`DuplicateObject(..., GetTransientPackage())`.

Sur **chaque copie transitoire uniquement** :

- réceptacles : retire un seul `ItemDefinition` de la règle,
  valide l'identité via le preview de règles, compare
  `bAcceptAnyItem`, capacité, présentation et physique
- serrures : vide les anciens `AcceptedKeyItems`, valide les
  `AcceptedKeyIds` existants via le preview de clés, compare
  l'état d'ouverture, consommation de clé et les messages
- vérifie que le **véritable package** reste non modifié,
  aucun ancien pointeur retiré du fichier source, aucun
  effet sur le catalogue canonique
- refuse toute divergence de règle/ID ou un état
  partiellement migré : **quatre pointeurs avant**, ou
  **zéro après**, jamais une valeur intermédiaire

La simulation est réexécutable avant et après migration.
Elle ne suffit **pas** à prouver une vraie réouverture de
package natif, les tests complets sur les nouveaux binaires,
ni le gameplay PIE des énigmes.

## Suite technique

1. Adapter les anciennes suites de manifeste et de schéma
   (`X.1661`, `TD0737`, `X.48`, etc.) à la double
   forme canonique, sans inventer de nouvelle clé autorisée.
2. Créer un commandlet UE Editor de conversion native
   **strictement borné aux quatre packages**, avec
   sauvegardes privées SHA-256 et sauvegarde/rollback
   atomique de la transaction avant Git.
3. Vérifier chaque package réouvert dans un autre processus
   UE et la suite `Grimrock` complète, puis pousser
   un commit binaire unique limité aux quatre fichiers.
4. Confirmer les deux statuts GitHub de ce SHA et recette PIE
   ciblée : torche/gemme acceptées, mauvaises offrandes refusées,
   clé cuivre/fer acceptée uniquement sur sa serrure,
   état de portes/consommation et Save/Continue inchangés.

Le script historique X.72/X.83 n'est **pas** réexécutable :
ses gardes exacts sont liés à des commits et schémas antérieurs.

**06.4 PARTIELLE ; 06.5 BLOQUÉE.**
