# ARM-CLEAN06.4X.19 — préflight de sortie 06.4 et dépendances legacy

**Date :** 17 novembre 2026. **Base audité :** `9e8331c122e319aff26a2934293744f3e73b6108` (`master`).
**Demande :** « Go ARM-CLEAN06.4X.17 », renommée **X.19** car X.17 désigne déjà la régression de la torche équipable et X.18 la torche lumineuse au sol.
**Statut : PRÉFLIGHT STATIQUE CONFIRMÉ ; AUDIT NATIF DES ASSETS À TERMINER.** **ARM-CLEAN06.4 et 06.5 ne sont pas déclarées terminées.**
**Exécution UE Windows demandée :** [issue #182](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/182).

## 1. Périmètre réellement vérifié

- Lecture de `AGENTS.md`, du contrat `ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`, du `ARM_CLEAN06_MIGRATION_PLAN.md`, de l'historique `ARM_CLEAN06_CURRENT_HANDOFF.md` et des sources C++ de production ciblées via GitHub sur le SHA ci-dessus.
- Inventaire des chemins versionnés dans les arbres Git `Content/`, `Source/` et `Scripts/`. Les présences et noms sont vérifiés ; **les valeurs internes des .uasset ne sont pas inspectables de façon fiable via ce connecteur**.
- Pas d'Unreal Editor/AssetRegistry exécuté sur le poste Windows pendant cette étape distante, pas de parcours PIE supplémentaire, pas de modification du code de gameplay, des binaires ni des sauvegardes.
- Ce rapport n'est **pas** une preuve de « zéro lecteur legacy » à l'échelle de chaque ligne du dépôt. L'issue #182 impose la recherche statique exhaustive et le contrôle natif des packages.

## 2. Inventaire des packages : preuves et limites

| Ensemble | Résultat | Niveau de preuve |
|---|---|---|
| `Content/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_*.uasset` | **24 fichiers** conservés | Arbre Git, chemins exacts |
| `Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_*.uasset` | **4 fichiers** | Arbre Git, chemins exacts |
| `.../Core/DataAssets/DA_ObjectPalette_Default.uasset` | Présent | Arbre Git |
| `.../Core/DataAssets/CanonicalStage/DA_ItemSystem.uasset` | Présent | Arbre Git |
| `.../Core/DataAssets/CanonicalStage/` | **39 assets** au total (catalogues et définitions inclus) | Arbre Git ; pas un compte de lignes du registre |
| `DA_Receptacle_TorchHolder`, `DA_Guardian`, `DA_Lock_CopperWall`, `DA_Lock_IronWall` | Quatre packages versionnés | Arbre Git |
| Définitions enregistrées : **34 (22 ARM, 12 non-ARM)** | Comptage des jalons historiques X.7/X.1 | **À reconfirmer** dans l'asset UE actuel |
| Occurrences matérialisées par X.4W : **95** | 44 loose + 0 spawns réels + 18 contenus initiaux de réceptacle + 33 palette | **Historique**, pas preuve que les pointeurs ont été supprimés |

Les anciens `DA_Weapon_Shuriken.uasset`, `DA_Weapon_ThrowingKnife.uasset` et `DA_Weapon_ThrowingKnife_01.uasset` figurent parmi les 24. **Ne pas déduire d'une différence entre 24 fichiers historiques et 22 ARM canoniques que deux assets précis seraient orphelins :** faire une correspondance native `package → ItemId → ArchetypeId` et relever les éventuels alias/doublons.

## 3. Matrice des dépendances C++ confirmées

**Légende :** P0 = bloque le cutover/suppression ; P1 = à qualifier avant suppression ; P2 = outillage/test historique. Un chemin de compatibilité réellement exécuté n'est pas prouvé par la seule présence d'un symbole.

| Priorité | Source actuelle (lignes approx. au SHA de base) | Preuve de lecture et risque | Porte de sortie |
|---|---|---|---|
| **P0** | `GridLevelRuntimeActor.cpp:2133–2260` — `AddPlacedItemActor` | Loose Item ARM canonique : exige encore `ObjectData.ItemDefinition`, refuse un ARM sans ancien pointeur ; appelle `InitializeFromItemDefinition` puis `ApplyCanonicalWorldVisual`. Le non-ARM canonique utilise déjà `InitializeFromCanonicalItem`. | Bascule ARM **après** parité mesh/lumière/Niagara/masse/pose/GUID/N1–N20 sur assets réels. |
| **P0** | `GridLevelRuntimeActorPersistence.cpp:690–736` | Pour restaurer le contenu de réceptacle, `bCanonicalNonArm` prend le chemin unifié, les ARM reprennent `ResolveRuntimeItemDefinition`, `InitializeFromItemDefinition` ou ID puis présentation partielle. | Restaurer ARM canonique complet avec tests SaveGame en mémoire et copies v27. |
| **P0** | `GridReceptacleActor.cpp:1116–1275` — `AddContainedItem` | L'initialisation canonique intégrale reste réservée aux items initiaux non-ARM ; les autres configurations conservent les chemins `ItemDefinition`, métadonnées et visuels historiques. | Auditer ARM/objets insérés/retirés, niveaux et états lumineux, sans toucher aux règles d'énigmes déjà validées. |
| **P0** | `GridLevelEditorActor_EditingObjectsLinks_07.inl:59–170`, `SGridEditorObjectInspectorPanel.cpp:1313–1349`, `GridLevelEditorActor_Validation.inl:448–484` | Le Selected Object écrit `ItemDefinition` et l'inspecteur affiche `ItemDefinitionAsset` ; la validation exige encore ce pointeur. Retirer le champ maintenant casserait l'authoring et/ou créerait des erreurs factices. | UI d'édition pilotée par `CanonicalItemId` + niveau, validation réelle et resauvegarde UE contrôlée. |
| **P0** | `GridObjectPaletteAsset.h:38–47`, `GridObjectPaletteAsset.cpp:40–60` | La palette conserve et valide `DefaultItemDefinition` (avec `CanonicalItemId` matérialisé). | Contrat palette canonique validé sur ses 33 entrées **actuelles** avant suppression du pointeur. |
| **P0** | `GridDungeonRuntimeState.h:125–170`, `GridLevelRuntimeActorPersistence.cpp:262–266,855–858`, `GridLevelRuntimeActorWorldItems.cpp:536,618–626` | `FGridRuntimeItemState.ProjectileDefinitionAsset` et `FGridPendingWorldItemState.ItemDefinitionAsset` restent `UPROPERTY(SaveGame)` et sont encore lus pour les identités non enregistrées. | Scanner toutes les identités hors registre, préparer migration ponctuelle des copies v27, ne jamais retirer les champs par simple recherche. |
| **P1** | `GridCanonicalItemSpawnReadModel.h:25–61` | `ItemSpawns` peut copier `ItemDefinition` vers le loose matérialisé, même si `CanonicalItemId` validé domine l'identité. Les quatre vrais niveaux comptaient **0 spawn** lors du jalon historique. | Convertir le schéma authorable et les tests, pas supprimer la fonctionnalité de spawn. |
| **P1** | `GridThrownItemActor.cpp:177–194,670–753` et `GridLevelRuntimeActorWorldItems.cpp:1109+` | L'initialisation de projectile utilise encore des définitions/ID et la projection canonique partielle ; la conversion à l'impact conserve le pointeur pour hors-registre. | Comparaison canonique du lancement, mesh/axe, collisions, durée, munitions, PIT02, niveaux et GUID avant retrait. |
| **P1** | `GrimrockPartyPawnHeldItem.cpp:28–63,113–140` | Visualisation tenue : recherche `UGridItemDefinitionAsset` et `InitializeFromItemDefinition`. | Identifier objets réellement concernés (torche/ARM) et préserver la lumière et les mains. |
| **P1** | `GridWallLockActor.cpp:51–65`, `GridLevelPlacementTypes.h:85–90` | Union runtime des anciennes règles `AcceptedKeyItems` et des `AcceptedKeyIds` canoniques. Les deux serrures cuivre/fer ont déjà été migrées et validées en PIE. | Scanner toutes les règles/overrides avant retrait ; pas de nouvelle conversion binaire aveugle. |
| **P1** | `GridItemDefinitionAsset.h:109–163`, `GrimrockWeaponDetailsCustomization.cpp:54–91` | `bUseWeaponCatalogue`, `WeaponCatalogue`, `WeaponVisualCatalogue` et profil N1/N20 sont encore représentés/consultés dans les Details historiques. | Conversion des 24 assets et passage du Details à la définition canonique ; suppression seulement en 06.5. |
| **P1** | `GridMonsterDeathComponent.cpp:349–381` | Branche pour ItemId hors registre utilise `ItemDefinitionAsset`, `bUseWeaponCatalogue` et poids historique ; branche canonique prioritaire pour IDs connus. | Qualifier chaque loot réellement authoré, puis supprimer uniquement après conversion native. |
| **P2** | `Source/GrimrockPrototypeEditor/Private/ARMEMENT/` et `Scripts/ApplyARM_CLEAN06_*.ps1` | Les commandlets de migration et l'ancien pipeline d'authoring restent présents. Pas une preuve de lecture runtime ; risque de réintroduire des références si réexécutés. | Maintenir désarmés ; inventorier avant suppression Editor-only de 06.5. |

### Chemins déjà convergés ou protégés

- `GridLevelRuntimeActorWorldItems.cpp:941+` : dépôts de nouveaux items canoniques ARM/non-ARM via `InitializeFromCanonicalItem`, niveau physique fixe, GUID conservé.
- `GridLevelRuntimeActorPersistence.cpp:623–670` : restauration des objets libres **canoniques** via le registre ; `ProjectileDefinitionAsset` n'est pas utilisé pour ceux-ci.
- `GridLevelRuntimeActorWorldItems.cpp:536,618+` : transport inter-niveau d'une identité **canonique** par ItemId/Niveau/GUID, sans dépendre de l'ancien `DA_Weapon_*`.
- PIT02/TRAP-PIT01, commit `9e8331c1` : validation PIE du propriétaire le 17 novembre 2026, suite Windows et deux statuts `SilentTask`/`LocalAuto` réussis. **Ne pas rouvrir ce correctif pendant le cutover ARM.**
- Les deux réceptacles (TorchHolder/Gardien) et les deux serrures (Cuivre/Fer) ont reçu leur identité/règle canonique native dans les jalons X.16.7–8, avec retours PIE positifs. Leur présence ne prouve toutefois pas que toutes les propriétés legacy ont été effacées.

## 4. Blocages avant une suppression 06.5

1. **Preuve AssetRegistry/UE manquante :** inventaire actuel de chaque ancien pointeur et valeur dans les 24 armes, quatre niveaux, palette, tous presets et règles d'énigmes. Les 95 occurrences sont historiques, et `Content/` contient d'autres packages susceptibles de référencer les mêmes classes.
2. **ARM au premier placement/réceptacle :** plusieurs chemins de production passent par la présentation historique ; pas de preuve de parité complète pour leur remplacement.
3. **Éditeur :** Inspector, placement de palette, validation, prévisualisation et certains widgets/commandlets doivent pouvoir authorer des IDs sans dépendre des anciens pointeurs avant suppression.
4. **Persistance v27 :** deux pointeurs `SaveGame` et des lecteurs hors registre ; les tests `SaveGameToMemory` démontrent la structure, pas la reprise sûre de fichiers v27 sur disque.
5. **Autres consommateurs :** vérifier exhaustivement `Source/` hors tests (menus, hotbar, combat, loot, rendu, inventaire) par rapport au contrat du registre, avec attribution précise de chaque branche legacy.

**Décision : 06.5 BLOQUÉE.** Les quatre DataAssets de règles déjà convertis ne doivent pas être réécrits par les anciennes migrations.

## 5. Ordre de travail recommandé à Codex / portes de validation

1. **X.19 natif, lecture seule** (issue [#182](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/182)) : rapport JSON/CSV hors dépôt, matrice fichier:ligne des lecteurs, scan UE des packages et overrides, correspondance des 24 armes à la table de 22 ARM, état des 95 occurrences, inventaire des SaveGame v27 sur copies **seulement**. Publier le delta documenté si les chiffres changent.
2. **Conversion limitée des ARM placés et réceptacles**, avec preuve de présentation complète, équipement, réactions, état lumineux, Chaos, GUID et niveau N1/N20 ; aucun changement de gameplay masqué. Décomposer si le périmètre ne tient pas en un seul jalon atomique.
3. **Passage Editor/Palette/ItemSpawns à l'identité canonique**, migration binaire UE native avec manifeste de packages, sauvegardes SHA-256 et réouverture indépendante. Conserver les propriétés tant que leurs valeurs sont nécessaires à la conversion.
4. **Copies v27 protégées**, puis suppression 06.5 : lecture de tous les packages et tous les lecteurs avant retrait des UPROPERTY/classe/scripts ; zéro fallback seulement après cutover.
5. **06.6** : suite UE globale, Cook/Shipping, Save/Continue, PIE sur ARM représentatifs et régression PIT02, SFX/VFX/held icons et inventaire.

## 6. Verdict de ce commit de documentation

- Aucun C++, `.uasset`, `.umap`, `.sav` ou script modifié par l'audit distant.
- Aucun test UE/PIE exécuté **pour ce jalon documentaire** ; ne pas emprunter les 1 968 tests verts de `9e8331c1` comme preuve d'un scan X.19.
- L'issue #182 constitue l'ordre de travail natif restant ; **la sortie 06.4 n'est pas autorisée** tant que son relevé ne confirme pas les propriétés et la parité sur les vrais packages.
- Un seul commit de documentation, avec le point de reprise, est l'unité de publication prévue.
