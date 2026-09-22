# UI-ASSET-AUDIT01 — Audit de Content/GrimrockPrototype/Blueprints/UI

Date : **22 septembre 2026**  
Statut : **AUDIT REPOSITORY TERMINÉ — suppressions physiques à valider dans Unreal Reference Viewer**

## Portée et méthode

Le dossier contient **88 assets versionnés** :

| Zone | Nombre |
|---|---:|
| racine UI | 22 |
| Buttons | 22 |
| Combat | 4 |
| Cursor | 11 |
| Icons | 19 |
| MainMenu | 6 |
| RPG | 4 |

Les fichiers `.uasset` sont stockés via Git LFS. Le repository permet de vérifier les noms, les références textuelles C++/documentation et les contrats runtime, mais **pas de prouver qu'un asset binaire n'a aucun referencer sérialisé**.

Règle de suppression : un candidat n'est supprimé qu'après **Reference Viewer / AssetRegistry dans UE5**, puis `Fix Up Redirectors`, sauvegarde des referencers et validation locale.

## Assets à conserver — runtime ou contrat courant confirmé

### Inventaire / personnage

```text
WBP_CharacterSheet
WBP_InventoryBag
WBP_InventorySlot
WBP_PartyMember
WBP_ItemActionMenu
WBP_ItemActionButton
WBP_ItemReadPanel
WBP_ItemTooltip
WBP_ItemTooltipStatLine
```

Ces assets appartiennent aux chaînes UI-CHAR/UI-INV/UI-ITEM/UI-WEIGHT/UI-FILTER actuelles.

`WBP_ItemReadPanel` et `WBP_ReadableMessage` ne sont pas des doublons : le premier lit un item d'inventaire, le second affiche un message lisible du monde.

### Shell / pages RPG en jeu

```text
WBP_GrimrockMenu
WBP_GridSkills
WBP_GridSpellbook
WBP_GridSpellbookEntry
WBP_GridJournal
WBP_GridMap
WBP_GridRecipes
WBP_GridCodex
```

`WBP_GrimrockMenu` est temporaire mais encore requis pour les six pages non-Inventaire. Journal/Map/Recipes/Codex restent des shells prévus par la roadmap ; ils ne doivent pas être supprimés comme "vides".

### Combat

```text
Combat/WBP_GridCombatActionPanel
Combat/WBP_GridCombatHud
Combat/WBP_GridCombatHudAction
Combat/WBP_GridCombatHudInitiativeSlot
```

Ils correspondent aux classes dynamiques du HUD : party panel, action/hotbar et initiative.

### Menu principal

```text
MainMenu/WBP_MainMenu
MainMenu/WBP_LoadGameMenu
MainMenu/WBP_LoadGameSlotRow
MainMenu/WBP_OptionsMenu
MainMenu/WBP_CreditsMenu
MainMenu/WBP_LicenseMenu
```

Ils correspondent au flux MM actuel.

### Création / recrutement RPG

```text
RPG/WBP_CharacterCreationWizard
RPG/WBP_CCStep_Attributes
RPG/WBP_RPGStoryCompanionRecruitment
RPG/T_CharacterCreationWizard_RGB
```

Le wizard est utilisé par New Game et recrutement personnalisé ; le widget Story Companion appartient au recrutement runtime.

### Curseur / interaction monde

```text
WBP_GridMouseCursor
WBP_ReadableMessage
Cursor/Cursor_Aim
Cursor/Cursor_CannotPlaceItem
Cursor/Cursor_Default
Cursor/Cursor_Forbidden
Cursor/Cursor_None
Cursor/Cursor_PlaceItem
Cursor/Cursor_Pull
Cursor/Cursor_Push
Cursor/Cursor_Read
Cursor/Cursor_Take
Cursor/Cursor_Use
```

Les états courants incluent notamment `AimThrow` et `CannotPlaceItem`. `Locked` réutilise volontairement `Cursor_Forbidden`, donc l'absence d'un `Cursor_Locked` séparé est normale.

### Police Carolingia

```text
F_Carolingia
carolingia
```

Ils sont traités comme le couple de ressources Carolingia utilisé par le texte lisible. Ne pas supprimer l'un des deux sans vérifier le type d'asset et les referencers dans UE.

### Icônes et cadres à conserver par défaut

```text
Icons/T_Border_Character_Combat
Icons/T_Border_CombatHUD
Icons/T_Border_GoldFine
Icons/T_Hand_01
Icons/T_Icon_PA
Icons/T_Icon_Unarmed
Icons/T_Light_01
Icons/T_Mana
Icons/T_PV
Icons/T_Resistance_Arcane
Icons/T_Resistance_Fire
Icons/T_Resistance_Holy
Icons/T_Resistance_Ice
Icons/T_Resistance_Lightning
Icons/T_Resistance_Necrotic
Icons/T_Resistance_Poison
Icons/T_Weight
```

PV, Mana, poids et résistances sont explicitement réutilisés par la feuille/tooltip. Les autres cadres et icônes appartiennent au HUD/hotbar actuel ; ne pas les retirer sans audit binaire.

### Styles de boutons à conserver par défaut

```text
Buttons/Action/T_Border_CombatHUD_Normal
Buttons/Action/T_Border_CombatHUD_Hovered
Buttons/Action/T_Border_CombatHUD_Pressed
Buttons/Action/T_Border_CombatHUD_Disabled

Buttons/Wizard/T_ButtonMenu_Normal_480x100
Buttons/Wizard/T_ButtonMenu_Hovered_480x100
Buttons/Wizard/T_ButtonMenu_Pressed_480x100
Buttons/Wizard/T_ButtonMenu_Disabled_480x100
Buttons/Wizard/T_GenderFemale_ButtonIcon_Normal_512x512
Buttons/Wizard/T_GenderFemale_ButtonIcon_Hovered_512x512
Buttons/Wizard/T_GenderFemale_ButtonIcon_Pressed_512x512
Buttons/Wizard/T_GenderFemale_ButtonIcon_Disabled_512x512
Buttons/Wizard/T_GenderMaleButtonIcon_Normal_512x512
Buttons/Wizard/T_GenderMaleButtonIcon_Hovered_512x512
Buttons/Wizard/T_GenderMaleButtonIcon_Pressed_512x512
Buttons/Wizard/T_GenderMaleButtonIcon_Disabled_512x512
```

Combat Action et Wizard sont encore des surfaces actives.

## Candidats de nettoyage — ne pas supprimer à l'aveugle

### 1. Buttons/TopTabs — candidat fort

```text
T_ButtonTab_Normal_480x100
T_ButtonTab_Hovered_480x100
T_ButtonTab_Pressed_480x100
T_ButtonTab_Disabled_480x100
T_ButtonTab_Selected_480x100
```

Motif : UI-NAV01 a déplacé la navigation globale vers la barre basse et UI-CLEAN01 a retiré les anciens onglets Inventory du shell. La documentation courante interdit de recréer une barre d'onglets supérieure.

Action : Reference Viewer sur les cinq textures. Si aucun referencer runtime courant : suppression groupée.

### 2. WBP_ItemTooltipComparisonRow — candidat fort

Aucune référence textuelle courante n'a été trouvée. Le tooltip actuel documente `WBP_ItemTooltip` et `WBP_ItemTooltipStatLine`, avec les comparaisons portées par le read model structuré.

Action : Reference Viewer. Si `WBP_ItemTooltip` ne l'instancie plus, supprimer.

### 3. T_BorderCharacter / T_Border_Character — doublon de nom à résoudre

```text
Icons/T_BorderCharacter
Icons/T_Border_Character
```

Les deux assets sont distincts côté Git et aucun usage textuel ne permet de déterminer lequel est canonique.

Action : ouvrir les deux, comparer visuellement, Reference Viewer, conserver le réellement utilisé et supprimer l'autre si zéro referencer.

### 4. Buttons/T_RootFrame — usage à confirmer

Aucune autorité C++ ne dépend de ce nom. Il peut cependant être utilisé directement dans un WBP.

Action : Reference Viewer avant toute décision.

## Assets à ne PAS classer comme morts malgré peu de références textuelles

- textures `Cursor_*` : références généralement sérialisées dans `WBP_GridMouseCursor` ;
- textures `Icons/*` et `Buttons/*` : références généralement sérialisées dans les styles UMG ;
- `WBP_GridJournal/Map/Codex/Recipes` : shells futurs explicitement conservés ;
- `WBP_ReadableMessage` : classe concrète configurée sur le RuntimeActor ;
- `WBP_GridCombatActionPanel` : classe dynamique du HUD combat.

## Organisation cible proposée

Ne pas déplacer 88 assets en une seule passe. Les déplacements doivent être faits depuis l'Unreal Editor afin de préserver les références.

Organisation minimale recommandée :

```text
Blueprints/UI/
├── Inventory/
│   ├── WBP_CharacterSheet
│   ├── WBP_InventoryBag
│   ├── WBP_InventorySlot
│   ├── WBP_PartyMember
│   ├── WBP_ItemActionMenu
│   ├── WBP_ItemActionButton
│   ├── WBP_ItemReadPanel
│   ├── WBP_ItemTooltip
│   ├── WBP_ItemTooltipStatLine
│   └── WBP_ItemTooltipComparisonRow   [si conservé]
│
├── InGameMenu/
│   ├── WBP_GrimrockMenu
│   ├── WBP_GridSkills
│   ├── WBP_GridSpellbook
│   ├── WBP_GridSpellbookEntry
│   ├── WBP_GridJournal
│   ├── WBP_GridMap
│   ├── WBP_GridRecipes
│   └── WBP_GridCodex
│
├── Interaction/
│   ├── WBP_GridMouseCursor
│   ├── WBP_ReadableMessage
│   └── Cursor/...
│
├── Fonts/
│   ├── F_Carolingia
│   └── carolingia
│
├── Combat/       [conserver]
├── MainMenu/     [conserver]
├── RPG/          [conserver pour l'instant]
├── Buttons/      [conserver, après suppression éventuelle TopTabs]
└── Icons/        [conserver]
```

Cette organisation retire le principal problème actuel : les 22 assets hétérogènes à la racine, sans multiplier artificiellement les sous-dossiers.

## Ordre de nettoyage recommandé

1. Exécuter un audit AssetRegistry/Reference Viewer pour les candidats.
2. Supprimer uniquement les candidats à zéro referencer confirmés.
3. Fix Up Redirectors.
4. Valider PIE : menu principal, inventaire, tooltip, lecture, curseur, combat, skills/spellbook.
5. Déplacer ensuite les assets racine vers Inventory / InGameMenu / Interaction / Fonts, **dans UE5**, par petits lots.
6. Fix Up Redirectors après chaque lot.
7. Mettre à jour les chemins C++ codés en dur éventuels après déplacement. Le chemin actuellement confirmé codé en dur vers `WBP_CharacterCreationWizard` impose de ne pas déplacer cet asset sans correction C++.
8. Relancer `ValidateUE.ps1` sur les filtres UI concernés et effectuer un smoke PIE.

## Documentation vérifiée / corrections appliquées

- `UI_ARCHITECTURE_CURRENT.md` : statut UI-FILTER01 remis à jour.
- `UI_GRIMROCK_MENU_CURRENT.md` : date/statut remis à jour et lien vers cet audit.
- `INVENTORY_INTERACTION_ROUTING.md` : ancien `WBP_GridInventory` remplacé par `WBP_InventoryBag` pour la lecture d'item.
- `MON21_1_QUESTS_JOURNAL_MAP_CODEX_ARCHITECTURE_AUDIT.md` : note ajoutée pour signaler que sa page Inventory du shell est historique depuis UI-CLEAN01.
- `MAIN_MENU_AND_GAME_FLOW.md` : marqué historique MM0 pour éviter de réintroduire les anciens noms `WBP_Credits` / `WBP_License`.
- `UI_FILTER01_CATEGORY_MAPPING.md` : validation PIE UI-FILTER01.3.1 enregistrée.

## Conclusion

Aucune suppression binaire n'est effectuée par cet audit.

Candidats prioritaires à vérifier dans Unreal :

```text
Buttons/TopTabs/*         5 assets
WBP_ItemTooltipComparisonRow
Icons/T_BorderCharacter
Icons/T_Border_Character
Buttons/T_RootFrame
```

Le reste possède soit une responsabilité runtime/documentée actuelle, soit une dépendance binaire UMG plausible qui interdit une suppression sans Reference Viewer.


## UI-ASSET-CLEAN01 — lancé le 22 septembre 2026

Le nettoyage passe désormais par un audit AssetRegistry read-only avant ouverture du Reference Viewer.

Test :

```text
Grimrock.Editor.UIAssetClean01.ReferenceAudit
```

Référence :

```text
docs/Design/UI_ASSET_CLEAN01_REFERENCE_VIEWER_CLEANUP.md
```

Aucune suppression n'est autorisée avant la sortie locale de cet audit.


### Résultat UI-ASSET-CLEAN01 / AssetRegistry

L'audit local du 22 septembre 2026 classe six assets à zéro referencer : les cinq textures `Buttons/TopTabs/*` et `Buttons/T_RootFrame`.

Trois candidats initiaux sont finalement référencés et sont donc conservés :

```text
WBP_ItemTooltipComparisonRow
  -> WBP_ItemTooltip

Icons/T_BorderCharacter
  -> Combat/WBP_GridCombatActionPanel

Icons/T_Border_Character
  -> RPG/WBP_CharacterCreationWizard
```

Aucune suppression n'est encore effectuée avant confirmation Reference Viewer des six assets à zéro referencer.


### Post-suppression confirmé

Le second audit local confirme que les cinq `Buttons/TopTabs/*` et `Buttons/T_RootFrame` n'existent plus dans l'AssetRegistry (`Exists=false`).

Les trois assets conservés gardent chacun leur referencer attendu :

```text
WBP_ItemTooltipComparisonRow -> WBP_ItemTooltip
T_BorderCharacter            -> Combat/WBP_GridCombatActionPanel
T_Border_Character           -> RPG/WBP_CharacterCreationWizard
```

Le nettoyage binaire est donc terminé ; seule la smoke validation PIE reste nécessaire avant clôture formelle de UI-ASSET-CLEAN01.


## UI-ASSET-ORG01 — lancé le 22 septembre 2026

Après clôture de UI-ASSET-CLEAN01, les 22 assets encore à la racine de `Blueprints/UI` sont migrés par lots vers :

```text
Inventory/
InGameMenu/
Interaction/
Fonts/
```

Test de migration :

```text
Grimrock.Editor.UIAssetOrg01.MigrationAudit
```

Ce test accepte temporairement ancien ou nouveau chemin pour chaque asset, mais impose une seule localisation après `Fix Up Redirectors`.

Référence : `docs/Design/UI_ASSET_ORG01_UI_FOLDER_REORGANIZATION.md`.


## UI-ASSET-ORG01 — clôture

La réorganisation physique est terminée sur `master`.

Racine UI normalisée :

```text
Blueprints/UI/
├── Inventory/
├── InGameMenu/
├── Interaction/
├── Fonts/
├── Buttons/
├── Combat/
├── Cursor/
├── Icons/
├── MainMenu/
└── RPG/
```

Les 22 anciens assets racine ont tous été migrés vers leur domaine. Smoke PIE global validé par l'utilisateur le 22 septembre 2026.
