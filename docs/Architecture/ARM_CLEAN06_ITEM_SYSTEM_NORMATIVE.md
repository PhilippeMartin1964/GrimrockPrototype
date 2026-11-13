# ARM-CLEAN06 — Architecture normative du système d'objets et d'armement

**Décision approuvée par le propriétaire le 13 novembre 2026. Version 1.0.** UE5.5.4 — C++ autorité ; UMG présentation.

**STATUT : CIBLE OBLIGATOIRE POUR LES DÉVELOPPEMENTS FUTURS, NON ENCORE IMPLÉMENTÉE.** La compilation et les tests déjà réussis couvrent l'ANCIEN système, pas la refonte proposée ici.

> **Clause de primauté.** Pour le schéma des objets, des armes, des icônes, des actions et de leurs références, ce document annule toute prescription incompatible des anciens rapports ARMEMENT, INDUS, ITEM, MON, RPG, ARM-CLEAN et des guides de pipeline. Les règles de gameplay approuvées restent valides : ce contrat change les propriétaires des données, **pas les effets des armes**. Les anciens documents décrivent des jalons historiques et servent au diagnostic, pas à créer de nouvelles doubles autorités.

## 1. Dix invariants non négociables

1. **Un registre unique.** Un seul `DA_ItemSystem` (classe cible `UGridItemSystemAsset`) indexe `ItemId -> UGridItemDefinitionAsset` et contient une et une seule référence vers chacun des quatre catalogues : `DA_WeaponCatalogue`, `DA_WeaponVisualCatalogue`, `DA_ActionCatalogue` et `DA_ActionPresentationCatalogue`. Configuration une seule fois dans `UGridItemSubsystem` au démarrage.
2. **Une définition par objet concret.** Les `DA_Weapon_*` et `DA_Item_*` demeurent des DataAssets individuels. Base `UGridItemDefinitionAsset` : **cinq champs** `ItemId`, `DisplayName`, `Description`, `WeightKg`, `Tags`. La sous-classe `UGridArmamentDefinitionAsset` ajoute **seulement `ArchetypeId`**. Boucliers et focaliseurs passifs inclus.
3. **Zéro référence de catalogue dans les armes.** Un `DA_Weapon_xxx` n'enregistre jamais `WeaponCatalogue`, `WeaponVisualCatalogue`, `bUseWeaponCatalogue`, `bUseWeaponVisualCatalogue`, ni pointeur direct vers action/son/mesh/icône. Les deux apparitions du `DA_WeaponCatalogue` signalées dans Details deviennent **zéro**. Un lien `ArchetypeId` suffit.
4. **Identité et masse en un seul endroit.** Seul l'asset concret porte `DisplayName` et `WeightKg`. Les lignes N1 du catalogue ne possèdent plus ces champs. Le XLSX source peut conserver ces valeurs comme données éditoriales d'import, mais n'est pas une deuxième autorité runtime. Aucune progression de masse par niveau n'est présumée.
5. **Armes : seules les règles métier dans le catalogue.** `DA_WeaponCatalogue` possède `ARM001–ARM082`, type d'arme, `HandPolicy`, profils numériques N1–N20, dégâts, armure physique/magique, PA, portée, munitions, balistique, réactions passives, recommandations de classe, et liaisons aux actions. **Jamais** nom/masse/icône/mesh/son.
6. **Une autorité visuelle.** `DA_WeaponVisualCatalogue` possède `ArchetypeId -> VisualId`, `ItemIcon`, `WorldMesh`, `EquippedMesh`, finitions, décoration, visuel de projectile et la palette `ActionIcons[EquippedActionKind]`. L'ancienne map `DA_EquippedWeaponActionIcons` y est **migrée**, puis son DataAsset est supprimé. L'icône de l'objet et celle de l'action restent **deux rôles distincts**, jamais des copies.
7. **Actions et FX séparés.** `DA_ActionCatalogue` décrit `ActionId`, mécanismes, ciblage, coûts spécifiques, cooldown et effets **typés** ; aucune copie des dés, PA ou portée N1 de l'arme. `DA_ActionPresentationCatalogue` décrit les mouvements, sons par type d'impact, parade, blocage, critique, lourde, Niagara. Une seule liaison `ItemActionBinding(ActionId, PresentationId)` par action authorée dans l'archétype ou le consommable.
8. **États d'instance réduits.** `FGridItemInstance` : `RuntimeObjectId`, `ItemId`, `ItemLevel`, `Quantity`, état **mutable** (p. ex. armure absorbée, lumière activée). Ni `DisplayName`, ni `Weight`, ni `Icon`, ni stats N1 persistées. Les niveaux, palettes et sauvegardes référencent des `ItemId` stables, pas des pointeurs UE vers la définition dans les nouvelles données.
9. **Une résolution commune.** Tous les consommateurs passent par `UGridItemSubsystem::ResolveItem(ItemId, ItemLevel)` ; résultat `FResolvedItem` transient. Ni widgets, ni acteurs, ni scripts de combat ne chargent des catalogues en parallèle ni ne conservent une deuxième autorité statique. Les erreurs de contenu sont explicites à la validation et au runtime.
10. **AUCUN legacy dans l'état final.** Ni `bUse*` de compatibilité, ni snapshots N1, ni branche fallback, ni ancienne palette d'icônes, ni migration SaveGame dans le jeu. La conversion des anciennes données est une **opération Editor ponctuelle** vérifiée avant bascule, puis les adaptateurs/outils de migration sont éliminés. Pas de compatibilité silencieuse.

## 2. Propriétaires exclusifs et cardinalités

| Entité / DataAsset | Données qui lui appartiennent exclusivement | Multiplicité / interdictions |
|---|---|---|
| `DA_ItemSystem` | Index des `ItemId` et 4 références globales | 1 asset, 1 entrée par ItemId, **aucune copie de catalogue** |
| `UGridItemDefinitionAsset` (base) | ItemId, DisplayName, Description, WeightKg, Tags | 1 définition par objet ; pas de champ `ItemType` saisi |
| `UGridArmamentDefinitionAsset` | ArchetypeId | 1 seule propriété métier ajoutée ; 0 pointeur de catalogue |
| `UGridConsumableDefinitionAsset` | MaxStack, liaisons d'usage | 0–n usages, pas de profil d'attaque N1 |
| `UGridAmmoDefinitionAsset` | AmmoKindId, MaxStack | lie des munitions aux archétypes par clé, pas par pointeur |
| `UGridOtherEquipmentDefinitionAsset` | Slots et bonus non déjà catalogués | armures/bijoux hors famille ARM ; pas de double armure |
| Visuels d'items non-armement | Icon, WorldMesh | dans leur définition spécialisée, sans catalogue d'armes |
| `DA_WeaponCatalogue` | Archetypes, LevelProgression, ReactionPolicies, ProjectilePolicies, ClassAffinities, ItemActionBindings, AmmoKind | 82 archétypes × 20 niveaux, pas de visuel ni nom/poids |
| `DA_WeaponVisualCatalogue` | VisualBindings, VisualProfiles, ActionIcons | plusieurs ARM peuvent partager un VisualId |
| `DA_ActionCatalogue` | ActionId, ActionKind, ciblage, coût, cooldown, ActionEffects | 0–n effets typés, pas d'icône ni de dés d'arme |
| `DA_ActionPresentationCatalogue` | PresentationId, animation, SoundBase, Niagara | profils partagés par plusieurs actions |
| `DA_Status_*` | Règles des statuts | lien logique depuis ActionEffect via StatusId |
| `FGridItemInstance` / `ItemPlacement` | Identité, niveau, quantité, état mutable, position | aucune duplication des propriétés statiques |

**Intégrité référentielle :** unicité de `ItemId`, `ArchetypeId`, `VisualId`, `ActionId`, `PresentationId`, `AmmoKindId` ; validation croisée avant cook et interdiction de lier un ID inexistant. Tout `DA_Weapon_*` actif choisit exactement un ArchetypeId ; tout archetype possède une progression ; un bouclier/focaliseur passif a **zéro action offensive** sauf action réelle explicitement authorée. `ItemActionBinding` est unique par couple propriétaire/ActionId et ne duplique jamais les réglages de l'action.

## 3. Diagramme entité–relation cible (Mermaid)

~~~mermaid
erDiagram
    ITEM_SYSTEM {
        asset DA_ItemSystem PK
        array ItemDefinitions
        ref WeaponCatalogue
        ref WeaponVisualCatalogue
        ref ActionCatalogue
        ref PresentationCatalogue
    }
    ITEM_DEFINITION {
        FName ItemId PK
        FText DisplayName
        FText Description
        float WeightKg
        array Tags
    }
    ARMAMENT_DEFINITION {
        FName ItemId PK,FK
        FName ArchetypeId FK
    }
    CONSUMABLE_DEFINITION {
        FName ItemId PK,FK
        int MaxStack
    }
    AMMO_DEFINITION {
        FName ItemId PK,FK
        FName AmmoKindId FK
        int MaxStack
    }
    OTHER_EQUIPMENT_DEFINITION {
        FName ItemId PK,FK
        enum EquipmentSlot
        struct EquipmentBonuses
    }
    GENERIC_ITEM_VISUAL {
        FName ItemId PK,FK
        texture Icon
        mesh WorldMesh
    }
    WEAPON_CATALOGUE {
        asset DA_WeaponCatalogue PK
        array Archetypes
    }
    WEAPON_ARCHETYPE {
        FName ArchetypeId PK
        enum ArmamentKind
        enum HandPolicy
        enum DamageType
        FName RequiredAmmoKindId FK
        struct BaseCombatStats
    }
    LEVEL_PROGRESSION {
        FName ArchetypeId PK,FK
        struct Level1Baseline
        struct DamageGrowth
        struct ProtectionGrowth
        int MaxLevel_20
    }
    PROGRESSION_EXCEPTION {
        int Level
        string StatKey
        int OverrideValue
    }
    REACTION_POLICY {
        FName ReactionId PK
        enum TriggerEvent
        enum ReactionKind
        float Chance
        enum AbsorptionPolicy
    }
    PROJECTILE_POLICY {
        FName ArchetypeId PK,FK
        float InitialSpeed
        float Arc
        float FlightLifetime
        enum CollisionBehavior
    }
    CLASS_AFFINITY {
        FName ArchetypeId FK
        FName ClassId FK
        enum RecommendationTier
    }
    AMMO_KIND {
        FName AmmoKindId PK
    }
    WEAPON_VISUAL_CATALOGUE {
        asset DA_WeaponVisualCatalogue PK
        array VisualBindings
        array VisualProfiles
        map ActionIcons
    }
    VISUAL_BINDING {
        FName ArchetypeId PK,FK
        FName VisualId FK
    }
    VISUAL_PROFILE {
        FName VisualId PK
        texture ItemIcon
        mesh WorldMesh
        mesh EquippedMesh
        struct CosmeticProgression
        struct ProjectileVisual
        struct IdleLightAndVFX
    }
    ACTION_ICON {
        enum EquippedActionKind PK
        texture ActionTexture
    }
    ACTION_CATALOGUE {
        asset DA_ActionCatalogue PK
        array Actions
    }
    ACTION_DEFINITION {
        FName ActionId PK
        enum ActionKind
        enum TargetingPolicy
        enum CostPolicy
        int CooldownRounds
    }
    ACTION_EFFECT {
        enum EffectKind
        struct TypedPayload
        FName StatusId FK
    }
    ITEM_ACTION_BINDING {
        FName ActionId FK
        FName PresentationId FK
    }
    PRESENTATION_CATALOGUE {
        asset DA_ActionPresentationCatalogue PK
        array PresentationProfiles
    }
    PRESENTATION_PROFILE {
        FName PresentationId PK
        struct MotionPreset
        struct SwingAudio
        struct ImpactByMaterial
        struct CriticalParryBlockAudio
        struct NiagaraEffects
    }
    STATUS_EFFECT_ASSET {
        FName StatusId PK
        asset DA_StatusEffect_xxx
    }
    ITEM_INSTANCE {
        Guid RuntimeObjectId PK
        FName ItemId FK
        int ItemLevel
        int Quantity
        struct MutableState
    }
    LEVEL_ASSET {
        asset DA_GridLevel_xxx PK
        array LooseItems
    }
    ITEM_PLACEMENT {
        FName ItemId FK
        int ItemLevel
        int Quantity
        struct GridPlacement
    }

    ITEM_SYSTEM ||--o{ ITEM_DEFINITION : "ItemId index"
    ITEM_SYSTEM ||--|| WEAPON_CATALOGUE : "global ref"
    ITEM_SYSTEM ||--|| WEAPON_VISUAL_CATALOGUE : "global ref"
    ITEM_SYSTEM ||--|| ACTION_CATALOGUE : "global ref"
    ITEM_SYSTEM ||--|| PRESENTATION_CATALOGUE : "global ref"
    ITEM_DEFINITION ||--o| ARMAMENT_DEFINITION : "armament"
    ITEM_DEFINITION ||--o| CONSUMABLE_DEFINITION : "consumable"
    ITEM_DEFINITION ||--o| AMMO_DEFINITION : "ammo"
    ITEM_DEFINITION ||--o| OTHER_EQUIPMENT_DEFINITION : "armor jewelry"
    ITEM_DEFINITION ||--o| GENERIC_ITEM_VISUAL : "non armament"
    WEAPON_CATALOGUE ||--|{ WEAPON_ARCHETYPE : "ARM001 to ARM082"
    ARMAMENT_DEFINITION }o--|| WEAPON_ARCHETYPE : "ArchetypeId"
    WEAPON_ARCHETYPE ||--|| LEVEL_PROGRESSION : "N1 to N20"
    LEVEL_PROGRESSION ||--o{ PROGRESSION_EXCEPTION : "exceptions"
    WEAPON_ARCHETYPE ||--o{ REACTION_POLICY : "passive reactions"
    WEAPON_ARCHETYPE ||--o| PROJECTILE_POLICY : "physics"
    WEAPON_CATALOGUE ||--o{ CLASS_AFFINITY : "advisory"
    CLASS_AFFINITY }o--|| WEAPON_ARCHETYPE : "ArchetypeId"
    WEAPON_ARCHETYPE }o--o| AMMO_KIND : "requires"
    AMMO_DEFINITION }o--|| AMMO_KIND : "provides"
    WEAPON_VISUAL_CATALOGUE ||--o{ VISUAL_BINDING : "ArchetypeId"
    VISUAL_BINDING }o--|| WEAPON_ARCHETYPE : "joins"
    WEAPON_VISUAL_CATALOGUE ||--o{ VISUAL_PROFILE : "visuals"
    VISUAL_BINDING }o--|| VISUAL_PROFILE : "VisualId"
    WEAPON_VISUAL_CATALOGUE ||--o{ ACTION_ICON : "action icons"
    ACTION_CATALOGUE ||--o{ ACTION_DEFINITION : "mechanics"
    ACTION_DEFINITION ||--o{ ACTION_EFFECT : "typed effects"
    ACTION_EFFECT }o--o| STATUS_EFFECT_ASSET : "StatusId"
    WEAPON_ARCHETYPE ||--o{ ITEM_ACTION_BINDING : "authored actions"
    CONSUMABLE_DEFINITION ||--o{ ITEM_ACTION_BINDING : "uses"
    ITEM_ACTION_BINDING }o--|| ACTION_DEFINITION : "ActionId"
    ITEM_ACTION_BINDING }o--o| ACTION_ICON : "derived kind"
    PRESENTATION_CATALOGUE ||--o{ PRESENTATION_PROFILE : "presets"
    ITEM_ACTION_BINDING }o--o| PRESENTATION_PROFILE : "PresentationId"
    ITEM_INSTANCE }o--|| ITEM_DEFINITION : "ItemId"
    LEVEL_ASSET ||--o{ ITEM_PLACEMENT : "placements"
    ITEM_PLACEMENT }o--|| ITEM_DEFINITION : "ItemId"
~~~

Le diagramme est conceptuel et décrit **les relations autorisées**, non des UPROPERTY existantes à cette date. Les liens vers ArchetypeId, VisualId, ActionId et StatusId sont des **jointures logiques par valeur** ; aucune deuxième référence UObject cachée n'est autorisée.

## 4. Lecture unique au runtime

~~~mermaid
flowchart LR
    A["Inventaire, niveau, SaveGame : ItemId + ItemLevel + état"] --> B["UGridItemSubsystem::ResolveItem"]
    B --> C["DA_ItemSystem : index + quatre catalogues"]
    C --> D["FResolvedItem : modèle transient"]
    D --> E["Combat, HUD, UI, monde, save, palette"]
~~~

Le sous-système initialise et valide son index au chargement, effectue les résolutions sans chargement synchrone répété au Tick, et ne conserve pas de doubles copies statiques dans le SaveGame. Une erreur produit un diagnostic explicite avant packaging et au runtime, **sans retour caché à un profil ancien**.

### Visuals et actions

- **Icône d'objet :** `DA_WeaponVisualCatalogue.VisualProfiles[VisualId].ItemIcon` ; **icône de commande :** `DA_WeaponVisualCatalogue.ActionIcons[EquippedActionKind]`. Le `EquippedActionKind` est déduit du `ActionKind` et de la `HandPolicy` ; l'icône ne peut créer une action.
- **Attaque normale :** dégâts/dés, portée et coût proviennent de l'archétype et du niveau ; les actions avancées empruntant les dégâts d'arme portent un coefficient explicite, pas un snapshot N1.
- **Défense :** les réactions **passives** de parade ARM013 et absorption de bouclier sont distinctes des actions **actives**. Un bouclier ne reçoit jamais d'attaque fictive.
- **Munition :** jointure `RequiredAmmoKindId`/`AmmoKindId` sans pointeur direct d'arme vers un `DA_Item_Arrows_*`.
- **Projectile :** balistique, collisions et durée dans le catalogue combat ; pose du mesh, visuel de vol et Niagara dans le catalogue visuel.
- **SFX/VFX :** attaque, impact par matériau, miss, parade/blocage, critique et lourd sont sélectionnés **sur événements réels** via le profil de présentation ; ils ne modifient ni dégâts ni PA.
- **Autres items :** consommables, munitions, armures/bijoux, livres/torches ont des sous-classes spécialisées ; leurs icônes non-armement restent dans leur définition. Sorts et talents gardent leurs autorités propres.

## 5. Exemple normatif de définition d'arme

~~~text
DA_Weapon_StraightSword_01  : UGridArmamentDefinitionAsset
    ItemId       = Weapon_StraightSword_01
    DisplayName  = Épée droite
    Description  = [FText localisable]
    WeightKg     = [valeur unique migrée depuis les données réelles]
    Tags         = [] si nécessaire
    ArchetypeId  = ARM014
~~~

**Au maximum six champs éditables** pour cette fiche standard, hors métadonnées techniques UE. Un bouton d'éditeur « Ouvrir l'archétype / les visuels » résout l'ID et **ne sérialise aucun nouveau champ**. Un résumé N1/N20 peut être affiché en lecture seule.

## 6. Préservation du gameplay et conditions de sortie

Les **82 archétypes × 20 niveaux**, les NdM+K, les protections, coûts/portées, règles main gauche/deux mains, munitions, actions réellement authorées, dual wield, lanceurs, statuts, consommables, save/continue et visuels validés doivent rester équivalents. Les **260 profils passifs** n'acquièrent aucun dégât fictif. Le statut « compressé » de `DA_WeaponCatalogue` est préservé si les valeurs reconstruites sont identiques.

Avant tout retrait d'UPROPERTY, réaliser une **conversion Editor hors runtime** des vrais `.uasset`, niveaux, palettes et sauvegardes, avec parité, audits de références Git LFS et tests Windows UE5.5.4 ; l'outil de migration est ensuite supprimé. Le cutover n'est accepté qu'après absence de doubles autorités, absence de fallback, succès de tous les tests puis PIE. Voir [plan de migration](ARM_CLEAN06_MIGRATION_PLAN.md) et [contrat d'authoring](../Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md).

## 7. Architecture présente et documentation contradictoire

Le présent modèle **n'est pas encore exécuté**. L'état C++/DataAsset constaté et ses doublons figurent dans [ARM-CLEAN05](ARM_CLEAN05_WEAPON_DATA_ASSET_AUDIT.md) et [l'ERD historique](ARM_CLEAN05_WEAPON_ASSET_ERD.md). Le pipeline existant `Scripts/WeaponPipeline.ps1` fabrique encore des copies N1 : **ne pas le réutiliser pour créer de nouvelles armes suivant ARM-CLEAN06 avant sa refonte**. Les anciens documents sont répertoriés dans [l'index de Design](../Design/ARM_CLEAN06_DOCUMENTATION_INDEX.md) ; aucun ancien « contrat actif » ne prévaut sur celui-ci pour le modèle des données.

**Interdictions de revue :** pointeur de catalogue dans DA_Weapon, copie d'icône/mesh/N1/nom/poids, commutateur de compatibilité, lecture directe des catalogues par widget/acteur, action de défense déduite d'une texture, fallback silencieux, nouveau champ persisté non propriétaire, changement gameplay sous couvert de refonte. Toute dérogation exige une décision explicite modifiant d'abord ce contrat.
