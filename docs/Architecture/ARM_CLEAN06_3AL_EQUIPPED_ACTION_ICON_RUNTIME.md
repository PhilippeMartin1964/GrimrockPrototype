# ARM-CLEAN06.3AL — Icônes d'actions équipées depuis le catalogue canonique

**Base :** ARM-CLEAN06.3AK, validation Windows de l'étape précédente
obligatoire avant publication.

## Problème

Le système canonique distingue `Resolved.ItemIcon` (objet du sac)
de `WeaponVisualCatalogue.ActionIcons[EquippedActionKind]`
(action équipée). Pourtant, `GridTurnManagerPlayerActionCatalog.cpp`
affectait encore l'icône **d'objet** au transport transitoire de
l'attaque canonique. Le widget `GridCombatHudActionWidget`
pouvait ensuite écraser l'image par l'ancienne
`DA_EquippedWeaponActionIcons` en classifiant des champs obsolètes
de `DA_Weapon_*` : `WeaponArchetypeId` et
`bUseWeaponCatalogue`.

## Un seul chemin de lecture pour chaque catégorie d'objet

- `FGridCanonicalActionIconReadModel::Classify` dérive une
  catégorie d'icône uniquement de
  `FGridResolvedItem.ArchetypeId`, famille
  `Weapon.Kind` et de l'**action déjà sélectionnée**.
  Il exige un `ItemActionBinding` réel pour les attaques
  simples et défenses, ou le vrai `Attack_DualWield`
  composite du système de deux mains.
- `UGridItemSubsystem::FindEquippedActionIcon` ne copie ni
  ne retient de tableau : il lit directement la map unique
  `DA_WeaponVisualCatalogue.ActionIcons` de la racine
  `DA_ItemSystem`. `None` renvoie une référence vide.
- Le transport de la contribution d'attaque canonique
  `FGridCombatActionDefinition.Icon` prend désormais l'icône
  **d'action**, jamais `Resolved.ItemIcon`. Le calcul
  de dégâts, les coûts, la portée, la main source,
  les conditions et la disponibilité ne changent pas.
- Le HUD n'applique **plus** la palette historique si
  l'instance physique du slot source est reconnue comme
  canonique (ItemId + RuntimeObjectId + main).
  Elle emploie la même catégorie et le même registre
  pour produire `Image_ActionIcon`, en conservant
  exactement la hiérarchie existante du WBP.
- Un raccourci `Attack_DualWield` réellement offert
  utilise `MeleeDualWield`, puis, si sa catégorie
  n'est pas configurée, l'ancien `DualAttackIcon`
  individuel déjà présent. Cela ne crée pas de deuxième
  attaque. Les autres actions canoniques sans catégorie
  n'affichent pas une icône d'inventaire comme secours.
- Objets encore hors registre, sorts, potions et autres
  sources : parcours actuel temporaire inchangé.

Les catégories correspondent aux familles déjà vérifiées
dans `Grimrock.RPG.ARMEMENT.LOT02.20.EquippedActionIconCategories`.
Les catégories de défense ne sont jamais des permissions
d'action ; les boucliers et orbes passifs ne gagnent
pas d'attaque ou de raccourci.

## Tests et validation

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquippedActionIcons`
emploie la vraie racine `DA_ItemSystem` et couvre
ARM013/014/031/045/054 en N1/N20, ActionId réels,
icônes égales à la seule map canonique, action
fictive non liée, bouclier passif, sorts,
consommables et composite dual-wield.
Les tests historiques de HUD/hotbar, `MON12`,
`ARMEMENT.LOT02.19.2.1` et `LOT02.20` restent actifs.

Un seul commit atomique pour ce jalon.
La compilation UE5.5.4 et **toute la suite `Grimrock`**
doivent être vertes sur le SHA exact
(`UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`).

## Limites / localisation

La `DA_EquippedWeaponActionIcons.uasset` et
sa propriété WBP restent présentes **jusqu'à 06.4/06.5**
pour les fichiers binaires encore liés. Aucune texture,
palette, WBP ou niveau n'est édité dans ce jalon.
**UI.csv → ST_UI → WBP → Game/fr/en : inchangé.**
Une capture PIE est nécessaire pour juger du rendu
des 12 icônes ; le test `-NullRHI` ne démontre pas
leur composition visuelle.
