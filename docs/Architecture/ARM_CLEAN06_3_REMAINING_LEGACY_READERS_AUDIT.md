# ARM-CLEAN06.3 — Audit des lectures historiques restantes (14.11.2026)

**Point de référence :** `3fe7ad0` (06.3AN) ; analyse statique des sources
C++ sous `Source/` avant 06.3AO. L'objectif de 06.3 est la convergence
des **consommateurs**, non la suppression prématurée des vieux UPROPERTY.
La présence d'un symbole dans la source ne prouve pas un fallback
effectivement exécuté pour un ItemId canonique.

## Lecteurs de production à examiner avant 06.4

| Zone | Ancienne dépendance | Lecture/risque à qualifier | Priorité |
|---|---|---|---|
| Kits de départ | `CanEquipToSlot`, `GetEffectiveHandUsage` | l'item N20 canonique peut rester traité avec une politique de mains historique ; ciblé 06.3AO | haute |
| Dual-wield | `GridDualWieldPolicy.cpp` | classification des deux sources par définition historique ; vérifier parité avec `ResolveBoundAttackForHand` et refus des passifs | haute |
| Widget d'inventaire | `GridInventorySlotWidget.cpp` | branche historique `GetEffectiveHandUsage` ; distinguer les ItemId enregistrés avant conversion | moyenne |
| Menus d'actions | `GridItemContextActionLibrary.cpp` | permissions de mains issues de la définition ancienne ; confronter à équipement canonique | moyenne |
| Actions de combat | `GridTurnManagerPlayerActionCatalog.cpp` | chemin historique `CombatActions` / `TryGetWeaponCatalogueProfile` ; prouver qu'il est totalement isolé des ARM enregistrés | haute |
| Sauvegardes et instances | `GridItemInstanceMaterializationReadModel.h` et `GridPartyInventoryComponent.cpp` | anciens snapshots/valeurs maintenus tant que 06.4 n'a pas converti les binaires | haute |
| Butins et monstres | `GridMonsterLootResolver.cpp`, `GridMonsterDeathComponent.cpp` | distinction entre chemin canonique déjà converti et branche hors registre ; vérifier transferts/Save | moyenne |
| Réceptacles/monde | `GridReceptacleItemMetadataReadModel.h` | branche ancienne uniquement pour les identités encore absentes du registre | moyenne |
| Affichage des armes | `GridWeaponTooltipProjection.cpp`, `GridEquippedWeaponActionIconPalette.cpp` | préférer catalogue visuel canonique et ne jamais afficher une icône d'objet comme icône d'action | moyenne |
| Éditeur / commandlets | `GridWeaponPipelineCommandlet.cpp`, `GridWeaponPilotItemAuthoringCommandlet.cpp`, `GrimrockWeaponDetailsCustomization.cpp` | authoring historique qui réintroduirait des copies après bascule ; supprimer après conversion, sans écrire les binaires directement | haute |

## Critères de sortie 06.3 → 06.4

1. Tous les lecteurs d'ItemId connu sont passés par le registre unique
   ou ont une preuve explicite de priorité canonique testée.
2. Toute branche historique résiduelle est limitée à des ItemId absents
   du `DA_ItemSystem`, en attendant conversion contrôlée UE.
3. Les 82 archétypes × 20 niveaux (1 640 profils), 260 profils passifs,
   24 DataAssets d'armes inventoriés et 18 kits restent intacts.
4. Aucun `.uasset`, niveau, palette ni sauvegarde n'est supprimé à cette
   étape ; préparer la conversion ponctuelle 06.4 par commandlet UE.
5. Aucun verdict « zéro legacy final » n'est prononcé avant 06.5 :
   ce rapport est une cartographie de travail et non un audit exhaustif
   des références binaires (`AssetRegistry` et commandlets requis).

Les scénarios non couverts par tests sur le vrai jeu doivent être
acceptés visuellement en PIE après la conversion de contenus.

## Complément 06.3AR — HUD d'action

Le lecteur HUD `GridCombatHudWidget.cpp` pouvait copier `ItemDefinition.Icon`
et `ApplyCatalogueIcon` de la palette ancienne dans une action d'arme
pourtant enregistrée. Le jalon 06.3AR isole les actions d'équipement
canoniques et interdit toute réutilisation d'icône d'objet pour ces IDs.
Les raccourcis quick item/unknown ID restent provisoirement legacy ;
les données d'authoring en `.uasset` relèvent toujours de 06.4.

## Complément 06.3AS — inventaire/piles sans définition historique

`FGridItemInstanceMaterializationReadModel` pouvait uniquement
matérialiser les identités connues si un asset historique était
encore enregistré. Le jalon 06.3AS retire cette obligation pour
les instances ARM explicitement nivelées et les objets N0 non-ARM,
et rend `MaxStack` du registre prioritaire dans les entrées, les
fusions et la séparation des piles. L'héritage de niveau authoré N0,
la reconstitution SaveGame et les références d'assets binaires
restent des obligations de 06.4.

## Complément 06.3AT — reconstitution de sauvegarde

`RehydrateOwnedItemDefinitions` vérifie désormais les instances physiques
canoniques de l'inventaire actif, de la réserve, de l'équipement et du
curseur contre `DA_ItemSystem` et leur capacité de pile avant de toucher
le registre temporaire. Les anciennes définitions sont **optionnelles**
pour les IDs connus et **obligatoires** pour les IDs absents du registre.
Leurs présences facultatives maintiennent les consommateurs transitoires.
Le format SaveGame 27 n'est pas modifié et les conversions des
enregistrements N0 historiques appartiennent toujours à 06.4.

## Complément 06.3AU — catégories de l'inventaire et dette runtime qualifiée

Le filtre, le tri par type et le libellé d'infobulle dérivaient encore
leur catégorie de `DA_Weapon_*.ItemType`. Le modèle
`FGridCanonicalInventoryItemTypeReadModel` projette maintenant le
`Weapon.Kind` du seul `DA_ItemSystem` pour les ARM connus N1–N20,
sans fallback vers les vieux types contradictoires. Munitions N0 et
autres non-ARM enregistrés sont provisoirement `Misc` car le type
`Ammo` n'existe pas dans `EGridItemType`. Les ID hors registre
conservent le comportement historique.

L'audit détaillé et les dépendances runtime encore bloquantes
avant 06.4 figurent dans
`docs/Architecture/ARM_CLEAN06_3AU_RUNTIME_ITEM_CATEGORIES_AUDIT.md`.
Le présent complément **n'affirme ni la fin de 06.3 ni une
conversion binaire déjà réalisée**.

## Complément 06.3AV — vols ARM spécialisés et restitution au sol

Le préflight d'un lancer spécialisé était encore bloqué par l'ancien
`DA_Weapon_*` même lorsque `DA_ItemSystem` enregistrait l'identité et
les paramètres `ProjectilePolicy`. Les modèles de Force et de
balistique, le contrôleur (y compris le curseur), les cinq sources de
lancer du Pawn, le spawner, le projectile et le monde/les fosses
acceptent maintenant les **ARM N1–N20 ayant un projectile spécialisé
réel** sans ancienne définition. La masse, l'arc, la vitesse,
la présentation et la durée de vie proviennent du système canonique ;
les GUID et niveaux ne sont pas reconstitués artificiellement.

**La transition n'est pas finie :** les lancers physiques ordinaires
sans `ProjectilePolicy`, les flèches N0 du chemin de combat à distance,
les interactions `Consume/Read/Hotbar` et les autres objets non ARM
restent à qualifier avant 06.4. Une valeur de vitesse générique
non authorée n'est pas inventée.

Voir `docs/Architecture/ARM_CLEAN06_3AV_CANONICAL_SPECIALIZED_THROWS.md`.
Aucun `.uasset`, SaveGame, WBP ni texte localisé n'a été modifié.

## Complément 06.3AW — capacités d'interaction et raccourcis physiques

Le menu `GridItemContextActionLibrary`, l'assignation des raccourcis
`GridPartyInventoryComponentHotbar`, leur affichage
`GridCombatHudWidget` et les exécuteurs de lecture/consommation
d'inventaire classent maintenant d'abord l'identité canonique
`ItemId + ItemLevel`.

Un projectile spécialisé enregistré N1–N20 est affectable à la
barre comme **lancer physique** `ThrowItem_<ItemId>`, sans
`DA_Weapon_*`. Les anciens `ItemType=Potion/Book`, contenus
`ReadText` et `QuickItemCombatAction` ne deviennent pas des
capacités de l'arme. Une lecture reste autorisée si le texte
appartient à l'instance. Les objets non enregistrés conservent
leurs exécuteurs de consommation et de lecture historiques.

**Dette explicite :** `UGridConsumableDefinitionAsset.UseActions`
existe dans le schéma canonique, mais son exécution
Combat/Exploration n'est pas encore branchée. Ne pas
annoncer une migration de potion canonique avant la preuve
du véritable exécuteur. Les conversions binaires UE sont
toujours prévues en 06.4.

Voir `docs/Architecture/ARM_CLEAN06_3AW_CANONICAL_CONTEXT_HOTBAR_READ.md`.
Aucun asset Unreal ni clé localisée n'est changé.

## Complément 06.3AX — soins directs canoniques UseActions

Le catalogue d'actions vérifie désormais le coût d'objet
`SourceItemQuantityCost` sur `Kind=Use`, en plus des attaques ;
les autres genres restent sans consommation d'objet.

`FGridCanonicalQuickItemUseReadModel` convertit **en mémoire,
sans ancien DataAsset et sans copie persistée** une unique
action authorée `Use / Self / Heal(Magnitude > 0)` en
`FGridCombatActionDefinition` pour le catalogue QuickItem
et les exécuteurs combat/exploration existants. Le menu,
l'affectation du raccourci et sa projection dans le HUD
utilisent cette même capacité. Un soin canoniquement
enregistré ne peut jamais récupérer d'anciens effets.
Les effets avancés ou multiples ne sont pas inventés.

**Dette restante :** les autres `UseActions` (statuts,
surfaces, téléportation, soin d'un allié, etc.) exigent
une projection et des tests dédiés ; le coût PA de
`Use` n'est pas encore authoré et vaut zéro lors de
la projection, sans valeur arbitraire. Aucun ancien
`.uasset` ni SaveGame converti à ce stade.

Voir
`docs/Architecture/ARM_CLEAN06_3AX_CANONICAL_SELF_HEAL_USE_ACTIONS.md`.
Localisation `UI.csv → ST_UI → WBP → Game/fr/en` inchangée.

## Complément 06.3AY — retrait d'un statut identifié, uniquement en combat

Après AX (soin personnel), `FGridCanonicalQuickItemUseReadModel`
projette également `Use / Self / RemoveStatus(StatusId, Magnitude=1)`
avec une **identité de statut précise**, `SourceItemQuantityCost=1`,
et un seul binding d'usage. `FGridCombatActionCatalog` et le
`GridTurnManager` disposent déjà des contrôles de disponibilité et
du retrait via `UGridStatusEffectLifecycleSubsystem`.
Le raccordement se limite à une projection transitoire.
Aucun effet ou dépense de PA n'est déduit d'un ancien
`DA_Item_*`.

Les actions de nettoyage de statut sont affectables en hotbar
de combat mais ne deviennent pas des actions `Consume`
en exploration. L'absence du statut ciblé rend l'action
`NoApplicableEffect` avant toute dépense d'objet.
Les multi-effets, les cibles autres que `Self` et les valeurs
ambiguës de `Magnitude` restent refusés.

**Dettes :** les autres `UseActions`, leur ciblage et
la preuve runtime PIE de la transaction complète ;
les contenus binaires et les sauvegardes restent hors 06.3.
Voir `docs/Architecture/ARM_CLEAN06_3AY_CANONICAL_USE_REMOVE_STATUS.md`.

Localisation `UI.csv → ST_UI → WBP → Game/fr/en` inchangée.

## Complément 06.3AZ — application canonique d'un seul statut sur soi

`FGridCanonicalQuickItemUseReadModel` projette maintenant
`Use / Self / ApplyStatus(StatusId, Magnitude=1)` sur les
`StatusApplications` de l'exécuteur QuickItem existant.
Le statut doit posséder une **définition de statut valide**
résolue par `FGridCombatStatusApplicationResolver` ;
sa politique de cumul, sa durée et sa puissance ne sont
jamais dupliquées dans `DA_ItemSystem`.

L'instance canonique est affectable à la barre de combat
sans `DA_Item_*`, mais ne devient pas un `Consume`
d'exploration. Le modèle conserve `Use / Heal` (AX)
et `Use / RemoveStatus` (AY). Le catalogue désactive
le nouvel usage avec `NoApplicableEffect` lorsque
l'application de statut ne changerait rien, ce qui
empêche de dépenser l'objet pour un effet nul.

**Reste volontairement exclu :** cibles Ally/Party/Hostile,
plusieurs effets ou niveaux invalides, effet de statut
en exploration, coût PA non authoré et transaction
de combat PIE non encore vérifiée par essai visuel.
Les autres `UseActions` nécessitent un contrat de
projection séparé.

Voir `docs/Architecture/ARM_CLEAN06_3AZ_CANONICAL_USE_APPLY_STATUS.md`.
Aucun asset UE ou texte `UI.csv/ST_UI/WBP/Game-fr-en`
n'est changé. Les conversions binaires relèvent de 06.4.
