# Carte textuelle des domaines

La [synthèse du projet](../PROJECT_SYNTHESIS.md) fixe l’état des systèmes. La [carte Mermaid](GRIMROCK_PROJECT_MAP_MERMAID.md) montre leurs relations.

| Domaine | Autorité et frontière |
| --- | --- |
| Donjon et grille | `UGridLevelAsset` porte la géométrie et les placements. `AGridLevelRuntimeActor` orchestre la matérialisation, les interactions et l’état mutable du niveau. |
| Grid Editor | `GrimrockPrototypeEditor` utilise des outils et projections spécialisés. Les objets placés ont des définitions réutilisables et des instances à identité stable. |
| Mouvement et interaction | Pawn et contrôleur délèguent à la grille, aux politiques de ciblage et aux services de transfert. Portes, serrures, réceptacles et pièges gardent leurs conditions propres. |
| Objets physiques | `DA_ItemSystem` et `UGridItemSubsystem` résolvent `ItemId` et niveau. L’instance ne porte que son identité et son état mutable. [Cardinalités et propriété](../DataModel.md) |
| Groupe et inventaire | `UGridPartyInventoryComponent` possède l’inventaire, le curseur et l’équipement du groupe. Les transferts valident source et destination par identités stables. |
| Combat et monstres | `UGridTurnManagerComponent` possède l’orchestration du tour, l’initiative, les réservations et refus. Les politiques d’action, ciblage, dégâts, occupation et AI restent distinctes. |
| Événements, Logic et Lua | L’activation route Event → Command. Logic et Lua prolongent le même contrat. Le module `GrimrockLua` fournit l’intégration de Lua54. |
| RPG, magie et quêtes | Les services RPG et définitions spécialisées portent la progression. La résolution de sort précède sa transaction. `UGridQuestSubsystem` possède l’état de campagne et ses APIs de snapshot. |
| Map et UI | Les vues de carte dérivent de la géométrie et de l’exploration de niveau. C++ projette l’état, le Designer dispose et présente les widgets. [Architecture UI](../../Design/UI_ARCHITECTURE_CURRENT.md) |
| Persistance | `UGrimrockPartySaveGame` v27 valide les états durables. Les projections temporaires et autorités statiques sont reconstruites. |
| Validation | [Chaîne Windows](../VALIDATION_AUTO01_WINDOWS_UE_AGENT.md), [recettes](../../Tests/TEST_DUNGEON_PASS_CHECKLIST.md), [consolidation](../../Technical/Consolidation.md). |

La [carte détaillée DOC-ARCH01 figée](https://github.com/PhilippeMartin1964/GrimrockPrototype/blob/2941d211ded3ffca873065d93eb05a9a2c51c7b3/docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md) conserve les inventaires et décisions d’avant consolidation.
