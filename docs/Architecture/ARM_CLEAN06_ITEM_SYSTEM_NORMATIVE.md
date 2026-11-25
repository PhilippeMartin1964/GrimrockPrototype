# Contrat normatif des objets et de l’armement

Contrat approuvé le 13 novembre 2026, implémenté et clôturé. Le [modèle relationnel](DataModel.md) décrit les types, cardinalités et variantes réellement présentes. Le [contrat d’édition](../Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md) indique les points d’écriture. Les anciennes procédures sont reliées à leur [historique](../History/ARM_CLEAN06.md).

## Autorité et identité

1. `DA_ItemSystem` (`UGridItemSystemAsset`) indexe les définitions par `ItemId` et référence exactement une fois les quatre catalogues. `UGridItemSubsystem::ResolveItem(ItemId, ItemLevel, OutItem)` fournit une `FGridResolvedItem` transitoire. Les consommateurs évitent les lectures parallèles, les copies persistantes de données statiques et les fallbacks historiques.
2. `UGridItemDefinitionAsset` porte `ItemId`, `DisplayName`, `Description`, `WeightKg` et `Tags`. `UGridArmamentDefinitionAsset` ajoute `ArchetypeId`. Une arme matérialisée référence l’archétype par son identité. Elle n’embarque ni référence de catalogue, ni copie du profil N1, de ses icônes ou de ses meshes.
3. La résolution valide `ItemId` et le niveau physique exact. ARM exige N1–N20, non ARM N0. Une identité absente, une quantité hors contrat ou une référence invalide entraîne un refus sans état de remplacement. `RuntimeObjectId` identifie l’instance, `ItemDefinitionId` désigne sa définition.
4. Un objet physique a un seul propriétaire ou emplacement à la fois. Inventaire, équipement, curseur, monde, projectile et réceptacle transmettent la même identité selon les règles de quantité, de séparation et de rollback. La source à deux mains réserve l’autre emplacement sans seconde instance. Les transactions vérifient les identités stables à travers les callbacks.
5. SaveGame v27 décrit l’état mutable. La masse, le nom et les profils sont reconstruits par le registre. Les deux Readables et la compatibilité des anciens chemins légitimes restent protégés. Les redirections et alias ne recréent aucune ancienne autorité.

## Catalogue et présentation

| Autorité | Propriétés |
| --- | --- |
| `DA_WeaponCatalogue` | `FGridWeaponArchetype`, politique de main, munitions, progression de dégâts, précision, coût, portée et protections, autorisations, réactions et politique de projectile. Les préférences de classe orientent les choix. |
| `DA_WeaponVisualCatalogue` | Liaisons `ArchetypeId → VisualId`, profils visuels et `ActionIcons`. Icône de l’objet, meshes et transformations, finitions, VFX persistants et représentation du projectile restent des données visuelles. |
| `DA_ActionCatalogue` | Actions mécaniques compactes et détaillées. Les bindings combinent `ActionId` et `PresentationId`. Les consommateurs non ARM utilisent la forme détaillée lorsque le contrat l’exige. |
| `DA_ActionPresentationCatalogue` | Présentations compactes et détaillées, audio et Niagara selon les événements et résultats effectifs. Aucune autorisation d’action ou règle de dégâts n’est déduite du rendu. |

Les profils N1–N20 sont obtenus par règles compactes et exceptions absolues. Les 82 archétypes produisent 1 640 profils, dont 260 passifs. La racine comprend 48 définitions concrètes, dont 22 ARM. Les autres archétypes sont des données de catalogue. Les slots et bonus actuels servent de socle à l’équipement futur, qui reste conceptuel.

Un bouclier passif ne reçoit pas d’attaque factice pour représenter une absorption ou réaction. Main gauche, main droite et dual-wield gardent leurs politiques distinctes. La balistique relève du modèle mécanique, la collision de ses acteurs et politiques. Les visuels du lancer relèvent du modèle de présentation.

Les objets non ARM portent leurs données spécialisées, dont lumière, lecture, interactions, données de lancer générique et visuels propres. La politique physique d’une pierre N0 vaut aussi pour sa barre rapide, sans attaque spécialisée. Les sorts, talents, statuts et compétences gardent leurs autorités. Les icônes d’objet et d’action équipée sont distinctes. Les états de présentation ne changent pas les droits, dégâts ou coûts.

## Préservation et validation

Conserver les identités, le schéma v27, les trois redirections de classe, 53 redirections d’objet et 50 alias Primary Asset IDs. Les 48 packages historiques retirés restent exclus. Les types Asset Manager `GridCanonicalWeaponCatalogueAsset` et `GridCanonicalWeaponVisualCatalogueAsset` sont des identités de compatibilité. Les contrôles chiffrés et l’oracle indépendant figurent dans la [consolidation](../Technical/Consolidation.md).

Une évolution normative exige une décision explicite, des tests sur le chemin métier et une validation de contenu. Les preuves historiques ne servent pas de modèles d’authoring. La phase de migration est [clôturée](ARM_CLEAN06_MIGRATION_PLAN.md).
