# ARM-CLEAN01 — Audit de l'autorité des données d'armes (UE 5.5.4)

**Date :** 12 novembre 2026. **Statut :** audit statique de `master` ; aucune migration des assets binaires ni validation PIE revendiquée.
**Objet :** commencer le nettoyage demandé de l'armement ARM001–ARM082, sans perdre les données enregistrées ni modifier le gameplay silencieusement.
**Sources actives :** `docs/Architecture/ARMEMENT_PIPELINE_GUIDE.md`, `docs/Architecture/ARMEMENT_PIPELINE_CONTRACT.md`, `docs/Design/ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md`.

## Inventaire quantifié du schéma

- `UGridItemDefinitionAsset` (`Public/Runtime/GridItemDefinitionAsset.h`) : **51 UPROPERTY au premier niveau**. Ce type universel doit aussi servir à des consommables, torches, livres et projectiles : leur présence ne justifie pas de tout exposer comme une fiche d'arme.
- `FGridCombatActionDefinition` (`Public/Runtime/Combat/GridCombatTypes.h`) : **44 UPROPERTY**, pour des actions universelles, compétences, sorts, surfaces et attaques ; une arme standard n'en utilise qu'une sous-partie.
- `FGridWeaponLevelProfile` : **15 propriétés par niveau**, répétées 20 fois pour chacun des **82 archétypes** (`DA_WeaponCatalogue`, 1 640 entrées, 260 passives). Les invariants de même archétype sont imposés par `IsValidCatalogue()` : nom, famille, `HandUsage`, type/sous-type, scaling, précision, PA, portée et poids.
- `DA_WeaponVisualCatalogue` possède les bindings et les profils visuels ; l'item recopie actuellement `Icon`, `WorldMesh`, `EquippedMesh` lors de sa fabrication.
- La progression SFX/VFX N1–N20 de `ARMEMENT-FX01` est **une spécification**, pas un système runtime entièrement implémenté.

## Cartographie des autorités et doublons réels

| Domaine | Autorité cible | Copies / consommateurs existants | Action |
|---|---|---|---|
| Identité d'instance | `FGridItemInstance.ItemDefinitionId` + `ItemLevel` + GUID | Le niveau fixe est enregistré dans inventaire/SaveGame | **Préserver** |
| Dégâts NdM+K, type et précision | Profil d'arme par `ArchetypeId + ItemLevel` | `CombatActions[].OffensiveProfile.AttackDefinition` contient une copie N1. `ResolveEquippedOffensiveProfile` la remplace au runtime, mais `GridWeaponPipelineCommandlet::ItemMatches` exige les valeurs N1 identiques | **Supprimer la double autorité**, pas l'identité/coût de l'action |
| Protection physique/magique | Profil par niveau | `EquipmentStatBonus.ArmorBonus/MagicalArmorBonus` est initialisé depuis N1, puis lu par `GridPartyInventoryComponentEquipment.cpp` (`GetEffectiveCurrent*Armor`, `Apply*ArmorDamage`, `Restore*Armor`, `ComputeCharacterEquipmentStatBonus`) | **P0 fonctionnel** : faire lire la protection réelle de l'instance avant toute suppression |
| Poids | Profil par niveau | `ItemDefinitionAsset.Weight` initialisé avec N1 ; `ApplyItemDefinitionToInstance` écrase déjà le poids d'instance avec `LevelProfile.WeightKg` | Conserver fallback legacy, retirer la contrainte de synchronisation N1 pour les armes catalogue |
| PA et portée | Catalogue (base), action (conditions, ciblage, ressources, éventuels coûts de talents) | `CombatActions[].ActionPointCost/RangeCells`, valeurs du catalogue | Distinguer action normale/bonus et profil de combat ; ne pas effacer les coûts de talents |
| Stats autres qu'armure | `EquipmentStatBonus` si réellement authorées | Force/Dextérité/etc. sur la définition | Conserver pour effets explicites ; pas d'affixes ou bonus fictifs |
| Noms, textes et catégories | Identité item/description depuis la feuille Narratif | `FGridWeaponLevelProfile.DisplayName/Family/HandUsage` répétés, `ItemDefinitionAsset.DisplayName` | Unifier à la migration, garder les champs affichables correctement |
| Visuels de l'arme | `DA_WeaponVisualCatalogue` | `ItemDefinitionAsset.Icon/WorldMesh/EquippedMesh` sont consommés par certains chemins legacy (`LoadHeldMesh`) | Centraliser après audit complet des lecteurs ; préserver les références soft |
| Sons et VFX d'attaque | `PlayerAttackPresentationProfile` (actuel) puis éventuel FX catalogue | `AttackAudio`, `ImpactHitAudio`, `ImpactMissAudio` + Niagara ; FX01 est proposition | Conserver l'écoute PIE validée ; ne pas fabriquer un nouveau catalogue sans migration |
| Utilitaires et propriétés communes | `UGridItemDefinitionAsset` partagé | QuickItem, Lecture, LightEmitter, WorldSparkle, Throw, Physics, Tags | **Masquer dans une vue spécialisée**, ne pas supprimer ces propriétés globales |

## Chemins d'exécution à préserver

1. `GridTurnManagerPlayerActionCatalog.cpp` parcourt les `CombatActions` d'un objet équipé et résout son profil de niveau par `ResolveEquippedOffensiveProfile`.
2. `GridTurnManagerPlayerActions.cpp` résout l'attaque, les dés, la caractéristique et les modificateurs ; `GridCombatResolver.cpp` calcule le résultat.
3. `GridCombatHudWidget.cpp` projette une seule attaque contextuelle : Dual Wield → MainHand → OffHand → mains nues ; une arme invalide ne doit **pas** être maquillée en une arme à 0 dégât.
4. `GridPlayerAttackPresentationComponent.cpp` émet `AttackAudio/ImpactHitAudio/ImpactMissAudio` depuis les événements de frappe ; le son d'impact dépend de `Result.bHit`, pas de la quantité de PV perdus.
5. `GridPartyInventoryComponentEquipment.cpp` lit encore la protection sérialisée sur l'item, y compris pour un objet de niveau 20 : à corriger **avant** de supprimer les anciennes copies.
6. `WeaponPipeline.ps1 -Action Validate -All` compare explicitement les attaques N1 enregistrées dans les DataAssets : adapter les validations après la nouvelle résolution, sans auto-écraser les assets.

## Décisions de refactoring

- **CLEAN02 :** créer des accesseurs de statistiques **effectives par instance** ; faire transiter les lectures d'armure, de tooltip et de combat par eux. Conserver pour l'instant les champs sérialisés legacy et un fallback uniquement lorsque `bUseWeaponCatalogue=false`.
- **CLEAN03 :** un modèle de progression compact doit porter les invariants, la base N1, les règles et les exceptions. Un résolveur pur/testable restitue `FGridWeaponLevelProfile`. Ne pas introduire la migration dans `IsValidCatalogue()` sans avoir comparé l'ensemble des 1 640 profils source.
- **CLEAN04 :** présenter un panneau de travail d'arme court (dégâts, protection, action, SFX/VFX, finition niveau), et une section avancée dépliable. Le type `UGridItemDefinitionAsset` reste générique pour ne pas casser tous les autres objets.
- **Source de vérité :** pendant la migration, le XLSX canonique demeure la référence éditoriale et `DA_WeaponCatalogue` l'autorité runtime. Ne jamais entretenir une double édition libre bidirectionnelle Excel/DataAsset.
- **Données de test :** ne jamais utiliser `DamageDice.bEnabled=false` pour demander 0 dégât d'une arme offensive. Le multiplicateur de réception 0 sur un monstre de test permet l'écoute audio tout en gardant une attaque valide.

## Contrôles de migration obligatoires

- Comparaison exacte des 1 640 triplets `DiceCount/DiceSides/FlatBonus`, protections, poids, PA, portée, scaling, précision, type/sous-type et passivité, puis vérification du résultat `MinDamage/MaxDamage` dérivé.
- Tests unitaires des 20 niveaux et des profils passifs ; tests loot/SaveGame/Continue, arme à deux mains, OffHand, double frappe, bouclier/égide/orbe, projectile et attaque de secours.
- Validation locale UE5.5.4 via `Scripts/ValidateUE.ps1` et PIE ; ne jamais annoncer PASS pour du code uniquement poussé sur GitHub.
- Migration des `.uasset` avec l'éditeur/commandlet après sauvegarde et contrôle Git LFS, **avant** toute suppression de champs `UPROPERTY` existants.
- Un jalon = un commit sur `master` ; aucun branchage parallèle ni héritage legacy permanent.

## Ce qui reste à mesurer

La lecture intégrale des valeurs du XLSX et la comparaison algorithmique de ses 1 640 lignes exigent une extraction binaire ou le script local du projet. **Cet audit de code ne démontre pas** l'existence d'une fonction unique reproduisant les progressions actuelles. Ne pas modifier les profils de production au jugé.
