# ARMEMENT01.6.4 — Visuels d'armement : inventaire, tooltip, hotbar

**Date :** 21 octobre 2026. **État :** code/tests publiés ; compilation et validation locale en attente.
**Baseline validée :** TD04-20261021-110400, Grimrock 1486/1486, aucun avertissement ni échec.

## Contrat

ARMEMENT01.6.4 intègre uniquement la **présentation**. Pas de changement aux règles de combat, au calcul RPG, à PrimaryAttack, aux sauvegardes, aux équipements, ni aux objets Shuriken / ThrowingKnife.

Le petit adaptateur C++ FGridWeaponVisualUiResolver appelle l'autorité FGridWeaponVisualResolver existante. En contexte événementiel UI seulement, il charge le DataAsset visuel si nécessaire. Le résolveur métier ne charge toujours rien. Sans opt-in, ou si le catalogue manque : fallback exact du visuel Legacy. Une hotbar ne remplace une icône d'action que si un profil catalogue complet est résolu.

- UGridInventorySlotWidget::SetItem présente l'icône du profil réel et conserve son VisualId temporaire ; ClearItem invalide les caches.
- FGridItemTooltipView expose Icon, VisualId, bHasItemLevel, ItemLevel. Son icône est la même que le slot. Son niveau vient de FGridItemInstance et jamais du niveau du personnage. Les items Legacy de niveau 0 n'affichent pas de faux niveau.
- UGridCombatHudWidget::EnrichHotbarActionPresentation traite aussi PrimaryAttack : son binding système ne stocke pas de SourceDefinitionId, mais l'action résolue possède la source réelle de l'arme de main directrice. Les sources Equipment et QuickItem opt-in utilisent donc l'icône du catalogue. Les actions Universal (poing), les sorts, les capacités et les actions Legacy gardent leurs icônes.
- Deux tests sous Grimrock.RPG.ARMEMENT01.6.UI vérifient les trois silhouettes réelles, l'égalité de l'icône slot-tooltip, les niveaux fixes et Legacy, le vidage du slot, les icônes d'actions, le repli et le bouclier passif.

## Assets de référence

| Archetype | VisualId | Icon | StaticMesh |
|---|---|---|---|
| ARM001 | VIS_Dagger_01 | T_Weapon_Dagger_01 | SM_Weapon_Dagger_01 |
| ARM045 | VIS_ShortBow_01 | T_Weapon_ShortBow_01 | SM_Weapon_ShortBow_01 |
| ARM075 | VIS_RoundShield_01 | T_Weapon_RoundShield_01 | SM_Weapon_RoundShield_01 |

## Deux limites explicites

**Définitions d'items :** dans Content/GrimrockPrototype/Core/DataAssets/Weapons, les seuls collectibles individuels actuellement versionnés sont DA_Weapon_Shuriken et DA_Weapon_ThrowingKnife (plus les deux catalogues). Aucun DA_Weapon_Dagger, DA_Weapon_ShortBow ou DA_Weapon_RoundShield n'est encore authoré. Ils seront traités séparément, sans modifier Shuriken / ThrowingKnife.

Quand ces nouveaux items existent, régler explicitement bUseWeaponVisualCatalogue=true, WeaponArchetypeId=ARM001/ARM045/ARM075 et WeaponVisualCatalogue=DA_WeaponVisualCatalogue. L'opt-in n'est jamais supposé automatiquement par l'UI.

**UMG :** ce commit C++ ne modifie aucun .uasset de widget. WBP_InventorySlot utilise GetIconTexture, la hotbar le champ Action.Definition.Icon. Le corps du tooltip WBP_ItemTooltip pourra consommer les nouveaux champs Icon et bHasItemLevel/ItemLevel de GetTooltipView. Tant qu'il n'a pas été branché manuellement dans UE, aucun badge visuel n'est présenté comme terminé.

## Validation UE5.5.4

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.6"
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock"

Objectif : 2 nouveaux tests, 1488/1488 tests globaux si le reste de la suite est inchangé, aucun avertissement ni échec. Les tests ne sont PAS déclarés réussis sans rapport local.

## Suite

ARMEMENT01.6.5 : acteur pickup et editor preview. L'apparition des trois équipements dans l'UI en jeu demandera ensuite de créer leurs définitions d'items et, si souhaité, d'ajouter le badge au WBP existant.
