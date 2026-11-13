# ARM-CLEAN06.1 — Modèle canonique C++ isolé (sans bascule des assets)

**Point de départ :** commit utilisateur `a60031db0d05aa444a7453f061e7463bd4e0b4d5` sur `master`.
**Autorité :** [ARM-CLEAN06](ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md),
[contrat d'édition](../Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md) et
[plan de migration](ARM_CLEAN06_MIGRATION_PLAN.md).

## Contenu de l'étape

Un modèle C++ isolé, **non raccordé aux lecteurs historiques** :

| Nouveau composant | Responsabilité unique |
|---|---|
| `UGridCanonicalItemDefinitionAsset` | Base à exactement cinq champs éditables : `ItemId`, `DisplayName`, `Description`, `WeightKg`, `Tags` |
| `UGridArmamentDefinitionAsset` | Une seule extension : `ArchetypeId`, aucun catalogue ni champ N1/mesh/icône/son |
| `UGridNonArmamentDefinitionAsset` et spécialisations | Visuels propres aux objets non-ARM ; `Consumable` (MaxStack, usages), `Ammo` (AmmoKindId, MaxStack), `OtherEquipment` (slots/bonus) |
| `UGridCanonicalWeaponCatalogueAsset` | Règles de 82 archétypes : cinq croissances compactes, dégâts, armures, PA, portée, mains, munitions, réactions, balistique, classes et actions |
| `UGridCanonicalWeaponVisualCatalogueAsset` | Liaison `ArchetypeId → VisualId`, icône objet distincte des `ActionIcons`, meshes, finitions et visuels de projectiles |
| `UGridActionCatalogueAsset` | Définition par `ActionId` des effets typés, cibles, mana, cooldown et éventuel coefficient de dégâts d'arme ; **aucune copie de dés/PA/portée N1** |
| `UGridActionPresentationCatalogueAsset` | Présentation par `PresentationId` : mouvement, sons Swing/impact matière/parade/blocage/critique/lourde et Niagara |
| `UGridItemSystemAsset` | Un seul index d'items et quatre références globales de catalogue |
| `UGridItemSubsystem` | Configuration unique à l'initialisation du GameInstance et `ResolveItem(ItemId, ItemLevel)` avec erreurs explicites |
| `FGridResolvedItem` | Projection transitoire non sérialisée pour les futurs lecteurs combat, inventaire, monde et UI |

Aucune modification de `DA_Weapon_*`, `DA_Item_*`, `DA_GridLevel_*`,
`DA_ObjectPalette_Default`, audio Sword, RatGiant, `L_Dungeon` ou sauvegardes.
Aucune modification WBP, HUD ou texte joueur ; le workflow UI de localisation
n'est **pas déclenché** dans ce jalon.

## Pourquoi le nom de la base est temporairement différent

Le nom final approuvé `UGridItemDefinitionAsset` est **encore occupé** par la
classe historique aux nombreuses UPROPERTY sérialisées dans les vrais assets.
La remplacer immédiatement ferait perdre des propriétés ou des références
avant la conversion Editor. Le nouveau type porte donc provisoirement le nom
`UGridCanonicalItemDefinitionAsset`. **Ce n'est pas une autorisation de garder
deux architectures dans l'état final** : la migration de classe et l'élimination
du modèle historique sont impératives aux étapes ARM-CLEAN06.4/06.5.

De même, les nouveaux catalogues d'armes sont typés `UGridCanonicalWeapon*`
pour ne pas reclasser prématurément les deux DataAssets binaires historiques.
Les noms de fichiers d'assets `DA_WeaponCatalogue` et
`DA_WeaponVisualCatalogue` seront conservés lors de leur conversion UE.
Aucun alias ni fallback vers les anciens catalogues n'existe **dans ce nouveau
résolveur**. Le fonctionnement historique reste inchangé jusqu'au cutover.

## Démarrage et contrat fail-closed

`UGridItemSubsystem` est un `UGameInstanceSubsystem`, créé par Unreal.
Sa propriété `ConfiguredItemSystem` (Config=Game) est l'unique chemin de
configuration. **Elle reste vide dans ARM-CLEAN06.1**, parce que
`DA_ItemSystem.uasset` n'existe pas encore. L'asset sera authoré par UE5.5.4
et raccordé en ARM-CLEAN06.2, jamais généré artificiellement.

Sans registre, `ResolveItem` échoue avec `RegistryNotConfigured`.
Une référence inconnue ou un niveau hors 1–20 échoue avec un code distinct.
Aucune charge répétée de catalogue ni fallback aux champs des armes historiques.
Une tentative de deuxième configuration est refusée.

`RegisterItemSystem` effectue avant acceptation les validations
d'unicité, de clés étrangères, des quatre catalogues, des 82 archétypes,
des 1 640 niveaux valides, des 260 profils passifs, des valeurs de
protection, des actions réellement liées, des visuels et des `AmmoKindId`.
Les sources des noms, poids et statistiques sont distinctes. Une icône
n'autorise jamais une action.

**Limites volontaires à cette phase :** les données des 56 assets historiques
n'ont **pas encore été converties** ; l'intégrité des ID externes de statuts
n'est vérifiée que structurellement (ID présent et type d'effet adapté),
leurs assets seront reliés lors de l'authoring 06.2. Le système de combat, les
actions spécifiques et les effets ne sont pas migrés ni exécutés par ces
nouvelles classes en 06.1. Les snapshots `FGridItemInstance` et les SaveGame
historiques ne sont pas modifiés avant l'utilitaire Editor de 06.4.

## Tests automatiques

- `Grimrock.RPG.ARM.CLEAN06_1.CanonicalSchema` : nombre de UPROPERTY
  (5 + 1), catalogue de 82 archétypes, 260 passifs et références valides.
- `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Registry` : configuration unique et défaut explicite sans registre.
- `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.SwordN20` : frappe N20 sans copie N1, poids par objet et action authorée.
- `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Shield` : bouclier N20 sans dés artificiels et armure passive.
- `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Consumable` : résolution des usages et du MaxStack.
- `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoResolve` : existence et résolution de munition.
- `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.AmmoKind` : identité `AmmoKindId` restituée.
- `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.Rejections` : identifiants/niveaux invalides refusés sans fallback.
- `Grimrock.RPG.ARM.CLEAN06_1.ReferentialIntegrity` : doublons ItemId,
  ArchetypeId, visuel inconnu, action inconnue et attaque fictive de bouclier.

Validation automatique : `UE5.5.4/SilentTask` et
`UE5.5.4/LocalAuto` **sur le SHA exact** après push. Le succès des tests
C++ ne vaut ni conversion des assets, ni preuve de comportement en PIE.

## Suite imposée

**ARM-CLEAN06.2 :** tenir compte du delta utilisateur `a60031db` postérieur à la baseline 06.0 (notamment les quatre DataAssets Sword, les sons et RatGiant) avant toute conversion, puis produire via Unreal Editor la racine
`DA_ItemSystem`, les catalogues de destination et leurs valeurs migrées,
y compris les nouvelles données Sword et RatGiant qui ne doivent pas
disparaître ; comparer chaque profil N1–N20 à la baseline 06.0.
Ne pas raccorder les lecteurs combat/inventaire/SaveGame avant la validation
des références et des assets convertis. Les étapes 06.3–06.6 restent
nécessaires avant l'élimination des classes historiques.
