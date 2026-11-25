# ARM-CLEAN06 — Contrat d'édition des armes, objets, actions et effets

**Contrat approuvé le 13 novembre 2026 et désormais implémenté.** Les [invariants](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md) et le [modèle réel](../Architecture/DataModel.md) précisent les autorités. Éditer les assets actifs par Unreal Editor.

## 1. Une fiche d'objet lisible

### Arme, bouclier ou focaliseur (un DataAsset par objet concret)

~~~text
DA_Weapon_StraightSword_01 : UGridArmamentDefinitionAsset
  ItemId = Weapon_StraightSword_01
  DisplayName = Épée droite
  Description = ... (FText localisable)
  WeightKg = ... (une seule valeur)
  Tags = []  (facultatifs)
  ArchetypeId = ARM014
~~~

**Exactement six champs éditables au maximum.** Ne pas ajouter : WeaponCatalogue, WeaponVisualCatalogue, GameAssetRef, ItemType, Icon, CombatActions, OffensiveProfile, DisplayName d'action répétitif, WorldMesh, EquippedMesh, N1/PA/dés, SFX/VFX, bUseWeaponCatalogue, bUseWeaponVisualCatalogue ou de nouveaux pointeurs par arme. L'éditeur affiche les détails calculés N1/N20 **en lecture seule** et propose une navigation vers les catalogues sans stocker de référence.

### Autres types de DataAsset

| Type | Champs de la base | Extensions autorisées |
|---|---|---|
| `UGridConsumableDefinitionAsset` | ItemId, DisplayName, Description, WeightKg, Tags | MaxStack ; liaisons de vrais effets/ActionId/PresentationId |
| `UGridAmmoDefinitionAsset` | idem | AmmoKindId, MaxStack |
| `UGridOtherEquipmentDefinitionAsset` | idem | slot(s), bonus intrinsèques ne recouvrant aucun archétype |
| Autres objets (torches, livres, clés, composants) | idem | attributs spécialisés et icône/mesh non-armement ; lumière, lecture, interaction selon besoin |

Le champ d'équipement et la taxonomie `ItemType` ne sont **pas** à saisir deux fois : la sous-classe fournit la famille de gameplay.

## 2. Où changer chaque propriété ?

| Je souhaite modifier… | Emplacement éditable | Notes |
|---|---|---|
| Nom, description, poids d'une épée | `DA_Weapon_StraightSword_01` | FText localisable ; poids fixe N1–N20 tant qu'aucune décision contraire |
| Famille, mains, portée, PA, dés, accuracy, scaling | `DA_WeaponCatalogue.Archetypes[ARM014]` | valeur N1 + règles jusqu'à N20 ; aucun N1 copié dans la fiche |
| Protection du bouclier | `DA_WeaponCatalogue.Archetypes[ARM075]` | absorption passive et niveau ; ne pas créer de fausse attaque |
| Parade passive / blocage réactif | Politique de réaction de l’archétype dans `DA_WeaponCatalogue` | exige vraie condition d'événement ; aucune icône n'active une réaction |
| Vitesse / arc du jet | Politique de projectile de l’archétype dans `DA_WeaponCatalogue` | données physiques uniquement |
| Arme nécessitant des flèches | `RequiredAmmoKindId` sur archétype ; `AmmoKindId` sur muni | identité logique, pas lien DA_Weapon → DA_Item_Arrows |
| Icône du sac et mesh de l'épée | `DA_WeaponVisualCatalogue.VisualProfiles[VisualId]` | obtenu via liaison ArchetypeId → VisualId |
| Icône de la frappe / parade active | `DA_WeaponVisualCatalogue.ActionIcons[EquippedActionKind]` | catégorie **dérivée**, ne pas dupliquer dans CombatActions |
| Finition N1–N20 / ornement | Finitions de niveau du profil dans `DA_WeaponVisualCatalogue.VisualProfiles` | cosmétique uniquement, non rareté implicite |
| Rotation/échelle du mesh lancé, VFX idle | `DA_WeaponVisualCatalogue.VisualProfiles` | pas de paramètres physiques |
| Nouvelle attaque lourde ou action spéciale | `DA_ActionCatalogue` par `ActionId` | action authorée + liaison vers archétype ; règles typées |
| Son de coup, impact métal/bois/chair, critique | `DA_ActionPresentationCatalogue` par `PresentationId` | sélection après résultat combat ; ne modifie pas dégâts |
| Niagara de frappe / pare / bloque | `DA_ActionPresentationCatalogue` | distinct du VFX persistant d'une arme |
| Recommandation de classe / kit | Affinités de l’archétype et `DA_WeaponCatalogue.ClassPreferences` | jamais interdiction d'équiper par défaut |
| Quantité, niveau fixe, usure d'armure réelle | `FGridItemInstance` / état mutable | SaveGame et transferts |
| Placer un objet dans un niveau | `DA_GridLevel_*` : ItemId, ItemLevel, Quantity, pose | pas de pointeur UObject par placement |

## 3. Actions et présentation (sans usine à gaz)

Une arme n'embarque **aucun** `TArray<FGridCombatActionDefinition>` universel. L'archétype déclare seulement la liste des `ItemActionBinding(ActionId, PresentationId)` correspondant à de **vraies** actions. `DA_ActionCatalogue` définit un comportement une fois ; `DA_ActionPresentationCatalogue` définit son rendu audio/Niagara une fois. Les actions normales utilisent les stats du `DA_WeaponCatalogue` au niveau de l'instance.

- **Attaque, critique, miss, impact matière, parade, blocage et lourd** correspondent à des **issues du gameplay**. Les FX ne sont jamais des permissions d'action.
- Les boucliers et les focaliseurs passifs peuvent avoir zéro action ; les réactions passives ne créent pas de raccourci actif.
- Une même présentation peut servir à plusieurs armes/actions ; ses variations sont données par les profils, pas par un second tableau de sons sur chaque arme.
- Les icônes de sorts/talents et les icônes d'objets consommables gardent leurs autorités spécialisées, sans passer par la palette d'actions **équipées**.

## 4. Cycle éditorial d’un nouvel objet

1. Créer **une** définition concrète (DA_Weapon_*, DA_Item_*), attribuer un ItemId stable et unique.
2. Si arme : enregistrer son archétype (ARMxxx) et ses règles dans `DA_WeaponCatalogue`, puis la liaison visuelle dans `DA_WeaponVisualCatalogue`. Pas de nouvelle référence d'asset dans la définition.
3. Définir les actions réellement disponibles et leurs présentations dans les deux catalogues correspondants ; réutiliser les presets existants.
4. Vérifier `DA_ItemSystem` : index exact des définitions et quatre références globales. Aucun alias anonyme ni duplication de clés.
5. Faire passer validations d'intégrité, tests gameplay, N1/N20, inventaire/SaveGame/monde/PIE. Un asset qui manque d'une référence obligatoire **échoue** à la validation, sans fallback.

Les anciens outils d’import ARMEMENT et de migration sont archivés. Utiliser les définitions et quatre catalogues actifs. Les contrôles spécialisés et le comparateur historique sont décrits dans la [référence des scripts](../SCRIPTS_REFERENCE.md#controles-specialises).

## 5. Conditions qualité / revues

- **Simplicité vérifiable :** six champs au plus sur une fiche d'arme, un seul identifiant ARM et zéro champ catalogue.
- **Unicité vérifiable :** une source pour nom, masse, statistique, icône d'objet et icône d'action ; aucune duplication entre classes/instances/pipelines.
- **Exactitude :** 82 × 20 profils, 260 passifs, préservation NdM+K, positions, passifs, munitions, réactions, expérience et progression.
- **SFX/VFX :** aucune action, défense, critique ou blessure simulée visuellement sans événement réel.
- **Runtime :** seul `UGridItemSubsystem` résout les données ; C++ pour les contrats/mécaniques, WBP Designer pour la présentation, textes via le circuit de localisation approuvé.
- **Zéro legacy :** aucune autorité historique parallèle. Les Core Redirects et alias encore nécessaires protègent les références légitimes.

Voir [historique de la migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Cette spécification remplace les prescriptions de structure d'assets des documents historiques ARMEMENT et INDUS ; elle ne modifie pas automatiquement leurs **règles de gameplay**.
