# ARM-CLEAN06.4X.1 — Préflight des lecteurs runtime et éditeur

**Statut : inventaire C++ en lecture seule ; aucun basculement de comportement réalisé ici.**
Version cible Unreal Engine 5.5.4. Autorité obligatoire :
`docs/Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`.
Base vérifiée : **ARM-CLEAN06.4W**,
`a56f4b5c22a7d883b9e9daad432e876676f2ce82`,
avec `SilentTask`, `LocalAuto` et `ARM-CLEAN06.4W` **SUCCESS** sur le SHA
final, un commit depuis `5b7131fe55d7cd770e9ae2bcc10901db7d7dfd51`.

## Résultats concrets déjà matérialisés

Le vrai `DA_ItemSystem` contient 34 définitions. Le commandlet W a écrit
puis rouvert les `CanonicalItemId` de **95 références physiques**
dans quatre niveaux et une palette : **44 `LooseItemInstances`,
0 `ItemSpawns` réels, 18 `ReceptacleInitialContent` d'instance et
33 `DefaultItemDefinition` directs de palette**. Les définitions
de monde inspectées avaient **zéro contenu initial par défaut**.
Ce sont des nombres d'occurrences, non des classes d'items distinctes.

**W n'a pas modifié les anciennes propriétés pointeurs, le `ItemLevel`
authoré, les .sav, .umap ou WBP.** On ne peut donc pas affirmer que les
lecteurs de production utilisent déjà la nouvelle identité.

## Matrice des lecteurs restant à basculer

| Domaine | Source C++ actuelle | Risque et action obligatoire |
|---|---|---|
| Placement physique au démarrage | `Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp`, `AddPlacedItemActor` | Utilise `ObjectData.ItemDefinition` pour l'identité, le niveau N0 hérité, les lumières et l'acteur. Le nouveau chemin doit exiger `CanonicalItemId`, `ResolveItem` et le niveau résolu, sans rebaptiser l'instance. |
| Spawns typés | même fichier, `ItemSpawns` dans la construction du niveau | Vérifie encore `Spawn.ItemDefinition` et le copie dans `Generated`. Aucun spawn réel dans le périmètre W, mais **le type de données et les tests doivent être migrés**, pas supprimés. |
| Contenu initial des réceptacles | `Source/GrimrockPrototype/Private/Runtime/GridReceptacleActor.cpp`, `InitializeRuntimeWorldObject` | Ignore l'entrée lorsque `ConfigItem.ItemDefinition` est nul et dérive `ItemDefinitionId` du pointeur. Les 18 `CanonicalItemId` d'instance doivent devenir autorité, tout en conservant quantité et niveau réel. |
| Règles d'acceptation des réceptacles | même fichier et `GridLevelEditorActor_Validation.inl` | `AcceptedItems` continue de s'appuyer sur l'ancienne définition. Ces règles sont **hors des 95 contenus initiaux comptés par W** ; les inventorier et décider d'une conversion séparée avant suppression. |
| Palette directe | `Source/GrimrockPrototype/Private/Core/GridObjectPaletteAsset.cpp` ; `Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorToolPalettePanel.cpp` | Validation, libellés, icônes, recherche et catégories consultent `DefaultItemDefinition` ; la projection canonique doit conserver les 33 entrées et l'ergonomie de l'éditeur. |
| Placement depuis la palette | `Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActorParts/EditingObjectsLinks/GridLevelEditorActor_EditingObjectsLinks_04.inl` | Le placement copie encore l'ancienne définition. Le nouveau contenu doit porter `CanonicalItemId` et la valeur de niveau explicite, sans réintroduire une seconde autorité. |
| Validation et prévisualisation éditeur | `GridLevelEditorActor_Validation.inl` et `Source/GrimrockPrototype/Private/Runtime/GridEditorLooseItemPreviewReadModel.h` | Vérifie encore les pointeurs et dérive le preview via `Definition->ItemDefinitionId`. Exiger l'ID persistant, le registre unique, l'exacte parité des niveaux, meshes et orientations. |
| Résolution historique et sauvegardes | `GridLevelRuntimeActor.cpp::ResolveRuntimeItemDefinition`, `GridLevelRuntimeActorPersistence.cpp` et `GridLevelRuntimeActorWorldItems.cpp` | Plusieurs chemins de reprise et transfert consultent encore la définition historique. Ne pas effacer ces pointeurs avant la migration SaveGame dédiée sur copies. |
| Acteur d'item visuel | `Source/GrimrockPrototype/Private/Runtime/GridItemActor.cpp` | `InitializeFromItemDefinitionId` remet les configs de lumière et VFX à zéro, contrairement à `InitializeFromItemDefinition`. `ApplyCanonicalWorldVisual` ne projette que le mesh/transform. La bascule doit restituer les données authorées canoniques. |

## Portes de sécurité avant un cutover

1. **Niveaux N0 authorés.** En 06.4H, les 95 occurrences
   de placement étaient au niveau `ItemLevel=0`. Pour un ARM réel, un
   N0 d'authoring était une demande d'héritage de
   `UGridItemDefinitionAsset::DefaultWeaponItemLevel`. La source est
   encore lisible, mais **ne pas transformer N0 en N1 par défaut**.
   Matérialiser son niveau effectif N1–N20 à partir de la source
   historique dans un passage Editor clos, puis rouvrir et valider
   les cinq .uasset ; les non-ARM N0 restent N0.
   Le contrat `FGridWorldItemLevelReadModel::ResolveAuthoredPlacement`
   utilise encore cette source pour l'héritage ARM ; elle ne peut être
   supprimée avant cette étape.
2. **Identité sans double autorité.** Les lecteurs de production
   doivent demander `UGridItemSubsystem::ResolveItem(ItemId, ItemLevel)`
   après validation stricte des ID et classes. Une définition
   historique ne peut pas corriger un `CanonicalItemId` invalide.
   Pas de fallback sur nom d'asset, icône, mesh ou `ItemDefinition`.
3. **Parité non-ARM visuelle/physique.** Les véritables
   `UGridNonArmamentDefinitionAsset` possèdent `LightEmitter`,
   `VisualEffects`, `bEnableWorldSparkle`, `ReadText`,
   `bReadable`, `WorldPhysicsInitialTiltDegrees` et les règles
   de masse/jet. `FGridResolvedItem` n'expose pas actuellement
   tous ces champs. Prévoir une projection transiente canonique
   dédiée ou son extension contrôlée, et des tests de lumière,
   lecture, Niagara, ramassage et Chaos. Ne pas confondre item et
   icône d'action.
4. **Éditeur.** Les 33 entrées de palette directes, la recherche
   et les valeurs modifiables d'un objet sélectionné doivent toujours
   fonctionner. Les 18 contenus initiaux et les règles d'acceptation
   doivent préserver la logique des énigmes. Les objets de monde ne
   sont pas des collectibles par défaut.
5. **SaveGames.** `FGridRuntimeItemState.ProjectileDefinitionAsset`
   et `FGridPendingWorldItemState.ItemDefinitionAsset` subsistent
   dans le schéma historique, ainsi que des chemins de restauration
   d'acteurs et réceptacles. **Aucun fichier .sav personnel n'a été
   ouvert par W.** La migration de sauvegardes se fait séparément,
   sur copies immuables, avec preuve des GUID, identités, niveaux
   et quantités avant de supprimer les champs.
6. **Validation.** Pour chaque étape fonctionnelle : un seul
   commit sur `master`, suite `Grimrock` complète, tests ciblés,
   audit strict des fichiers changés et preuve sur le SHA final.
   Les tests `-NullRHI` ne prouvent pas PIE/WBP, Chaos dynamique,
   Save/Continue ou Cook/Shipping.

## Ordre de réalisation prudent

- **06.4X.2 :** établir en Unreal une preuve read-only des niveaux
  ARM hérités et des règles d'acceptation hors inventaire W,
  avec manifestes privés et comparaison stricte.
- **06.4X.3 :** matérialiser puis rouvrir les niveaux effectifs des
  placements ARM avant extinction des pointeurs.
- **06.4X.4 :** basculer par sous-système les lecteurs runtime
  puis éditeur vers l'ID canonique et vérifier les effets, le
  ramassage, les réceptacles et les interactions.
- **Ensuite seulement :** SaveGames sur copies, suppression
  des anciennes propriétés après conversion, validations PIE,
  Chaos, Save/Continue, Cook et Shipping.

Ce préflight n'autorise ni une mutation de DataAsset supplémentaire
ni une suppression de champ. La base 06.4W est validée ; la bascule
de lecteurs, elle, n'est **pas encore réalisée**.
