# ARM-CLEAN06 — Plan de refonte et de migration sans legacy final

**Statut : PLAN NORMATIF APPROUVÉ SUR LES OBJECTIFS ; EXÉCUTION NON DÉMARRÉE.** 13 novembre 2026. Source de vérité : [contrat ARM-CLEAN06](ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md). **Ne pas présenter le modèle cible comme déjà implémenté.**

## 1. Invariants de migration

- **Zéro perte** : les 24 définitions `DA_Weapon_*` présentes dans `Content/GrimrockPrototype/Core/DataAssets/Weapons` à l'audit, plus les autres `DA_Item_*`, niveaux, palettes, 82 archetypes × 20 niveaux et états persistants sont inventoriés **avant** la moindre suppression.
- **Pas de changement de gameplay masqué** : 1 640 profils numériques et 260 profils passifs, protections (dont déplétion), coûts PA, portée, combat mains nues, MainHand/OffHand, deux mains, dual-wield, lancers, munitions, parade, blocage, action HUD 1 et compétences réelles restent équivalents.
- **Un seul modèle final** : aucun commutateur legacy, profil N1 copié, double pointeur, fallback, sérialisation d'anciennes variantes dans le runtime final. Les outils de conversion sont Editor-only et **retirés au cutover**.
- **Sécurité Unreal** : migration de .uasset/.umap par commandlets UE5.5.4, pas modification binaire artisanale ; Git LFS, données non commitées utilisateur et sauvegardes existantes protégées ; contrôle de listes de fichiers, sha et diff.
- **Traçabilité** : un jalon = au maximum un commit atomique sur master ; validate Windows `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA exact ; PIE pour rendu non prouvé par NullRHI.

## 2. Séquence imposée et portes de validation

| Étape | Produit principal | Condition stricte de sortie |
|---|---|---|
| **06.0 — Baseline lecture seule** | inventaire des 24 DA_Weapon, autres DA_Item, 4 DA_GridLevel, ObjectPalette, WBP, SaveGame ; cartographie de chaque référence | rapport d'inventaire reproductible avec valeurs et dépendances des .uasset, notamment deux ThrowingKnife, Shuriken et DA_WeaponCatalogue répété dans Details |
| **06.1 — Modèle canonique C++** | `UGridItemSubsystem`, `DA_ItemSystem`, 4 catalogues, classes d'item spécialisées et `FResolvedItem` transient | validation des IDs, de l'unicité des sources, aucun fallback pour le **nouveau** modèle |
| **06.2 — Contenus des catalogues** | extraction de DisplayName/WeightKg depuis catalogues et copies vers seules définitions ; ActionIcons → WeaponVisualCatalogue ; Action/Presentation normalisés | parité 82×20 (1640), 260 passifs, toutes icônes d'objet et commande et toutes références SFX/VFX existantes conservées |
| **06.3 — Lecteurs unifiés** | inventaire, HUD/hotbar, tooltips, mains, combat, effets, held item, monde/pickups, projectiles, plaques de pression, prévisualisation éditeur | aucun accès direct à anciens champs; preuves tests multi-lecteurs (poids, iconographie, meshes, niveaux) |
| **06.4 — Conversion binaire** | resauvegarde réelle de DA_Weapon_*, DA_Item_*, DA_GridLevel_*, DA_ObjectPalette_Default, presets et Blueprints touchés ; conversion des SaveGame existants via utilitaire ponctuel | diff exact, référentiel complet, aucune référence ancienne, chargement/placement réussis |
| **06.5 — Suppression définitive** | retirer champs et classes de compatibilité, anciens catalogues de raccourcis et générateurs qui réintroduisent des copies ; effacer outil de conversion Editor | recherche statique et scan AssetRegistry sans anciens champs/refs; **zéro fallback runtime** |
| **06.6 — Recette intégrale** | compilation Win64 UE5.5.4, Automation Grimrock globale, sessions Save/Continue, PIE sur armes représentatives | zéro échec, tests explicites couverture, validation visuelle par le propriétaire sur contenu réel |

Les étapes sont **séquencées par dépendances**, pas par suppression brutale des anciennes UPROPERTY. La compatibilité nécessaire pendant la conversion n'est autorisée **que dans un outil Editor de migration temporaire**, pas comme seconde autorité runtime pérenne. Aucun jalon intermédiaire n'est étiqueté « architecture terminée ».

## 3. Matrice de migration des champs

| Ancien stockage | Destination unique | Ancien stockage retiré après contrôle |
|---|---|---|
| `DA_WeaponCatalogue[].DisplayName` et `DA_Weapon_*.DisplayName` | `UGridItemDefinitionAsset.DisplayName` | nom dans profil d'archétype |
| `DA_WeaponCatalogue[].WeightKg` et `DA_Weapon_*.Weight` | `UGridItemDefinitionAsset.WeightKg` | poids dans profil catalogue, copies d'instance |
| `DA_Weapon_*.WeaponCatalogue` (et doublon Details signalé) | `DA_ItemSystem.WeaponCatalogue` | 0 pointeur catalogue dans DA_Weapon |
| `DA_Weapon_*.WeaponVisualCatalogue` | `DA_ItemSystem.WeaponVisualCatalogue` | pointeur et opt-in local |
| `DA_Weapon_*.Icon, WorldMesh, EquippedMesh` | `DA_WeaponVisualCatalogue.VisualProfiles` | fallback mesh/icon du DA_Weapon |
| `DA_EquippedWeaponActionIcons.Icons` | `DA_WeaponVisualCatalogue.ActionIcons` | ancien DA de palette et propriété WBP associée |
| `CombatActions[]` et `QuickItemCombatAction` embarqués | `DA_ActionCatalogue` + `ItemActionBinding` | arbre générique d'action dans arme standard |
| `CombatActions[].OffensiveProfile` (N1), PA, Range | `DA_WeaponCatalogue.Archetypes/LevelProgression` | toutes copies N1 et dérivées |
| `PlayerAttackPresentationProfile`, SFX, Niagara locaux | `DA_ActionPresentationCatalogue` | réglages de frappe dans le DA_Weapon |
| `bCombatThrowWeapon, ThrowSpeed, ThrowArc…` | `ProjectilePolicy` (physique) et `VisualProfile.ProjectileVisual` (cosmétique) | dizaines de paramètres de lancer locaux |
| `EquipmentStatBonus.ArmorBonus/MagicalArmorBonus` sur arme cataloguée | `DA_WeaponCatalogue` (par niveau) | protections N1 doublées |
| `FGridItemInstance.DisplayName/Weight` | résolution via `ItemId` et `ItemLevel` | snapshots statiques SaveGame |
| `FGridLooseItemInstance.ItemDefinition` et palette d'éditeur `DefaultItemDefinition` | `ItemId` stable dans niveau/palette | références UObject multiples en niveau |
| `WeaponArchetypeId` dans l'item | `ArchetypeId` dans `UGridArmamentDefinitionAsset` | ancien champ legacy |
| `ItemType/HandUsage/CompatibleEquipmentSlots` saisis | sous-classe + `HandPolicy` de l'archétype | double représentation de la règle de mains |

## 4. Scénarios minimum de tests automatisés

1. **Données** : ouvrir tous les assets existants en UE, 24 identités d'armes et autres items ; 82 archétypes × N1…N20, 1640 profils égaux à la baseline, aucun profil passif offensif ; exceptions des progressions compactes.
2. **Références** : chaque ItemId, ArchetypeId, VisualId, ActionId, PresentationId et AmmoKindId résolu une seule fois ; zéro WeaponCatalogue/WeaponVisualCatalogue dans les définitions ; un seul DA_ItemSystem ; contrôle de tous les niveaux et palettes.
3. **Objets** : poids inventaire/équipement, plaque de pression, jet depuis la main, physique Chaos, NPC loot et objets au sol ; même valeur calculée quel que soit le lecteur.
4. **Combat** : bouclier ARM075, parade ARM013, armes une/deux mains, DualWield, 2 attaques physiques indépendantes, SFX du résultat réel, critique lourd et impact matière, contrôle de munitions arc/arbalète.
5. **Interactions** : inventaire/curseur/hotbar/tooltip, chargement du donjon, ramassage et placement, objet de niveau fixe ; Saved → Continue à travers arrêt/reprise du processus, sans anciens snapshots.
6. **Présentation** : une seule autorité Icon objet et ActionIcon, held mesh et world mesh, VFX persistent distinct de Niagara d'attaque, locales fr/en via workflow UI ; test visuel PIE séparé de `-NullRHI`.
7. **Hygiène** : analyse C++ des lectures obsolètes, AssetRegistry pour références anciennes, absence de classes/commandlets/scripts qui reconstruisent du legacy ; tests non-régression complets.

## 5. Règles opérationnelles

- Toute création de nouvelle arme **doit** suivre le [contrat d'authoring cible](../Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md), mais **aucune production via l'ancien pipeline** n'est considérée conforme tant que sa migration n'est pas réalisée.
- Les contrats actuels de sauvegarde ne peuvent être supposés directement compatibles : générer une conversion ponctuelle pour les vraies versions existantes, sauvegarder les sources en sûreté, vérifier lecture/écriture, puis détruire le lecteur transitoire.
- Un échec de parité ou de référence **bloque** l'étape ; aucun fallback legacy n'est ajouté pour masquer le défaut.
- Les modifications d'assets et leurs tests doivent appartenir au même jalon atomique, sous réserve des règles Git/Git LFS et des commits d'assets produits via l'agent UE. Les étapes documentaires ne modifient **aucun** contenu binaire.
