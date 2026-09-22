# UI-ASSET-ORG01 — Réorganisation logique des assets UI

Date : **22 septembre 2026**  
Statut : **ACTIF — migration UE5 manuelle par lots**

## Objectif

Réduire la racine de `Content/GrimrockPrototype/Blueprints/UI` sans modifier les responsabilités runtime.

Après UI-ASSET-CLEAN01, 22 assets restent encore directement à la racine. Ils sont regroupés en quatre domaines simples :

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

Les dossiers existants `Buttons`, `Combat`, `Cursor`, `Icons`, `MainMenu` et `RPG` ne sont pas déplacés dans ce ticket.

## Mapping canonique

### Batch A — Fonts (2)

```text
F_Carolingia -> Fonts/F_Carolingia
carolingia   -> Fonts/carolingia
```

### Batch B — Interaction (2)

```text
WBP_GridMouseCursor -> Interaction/WBP_GridMouseCursor
WBP_ReadableMessage -> Interaction/WBP_ReadableMessage
```

### Batch C — Inventory (10)

```text
WBP_CharacterSheet
WBP_InventoryBag
WBP_InventorySlot
WBP_PartyMember
WBP_ItemActionMenu
WBP_ItemActionButton
WBP_ItemReadPanel
WBP_ItemTooltip
WBP_ItemTooltipComparisonRow
WBP_ItemTooltipStatLine
```

Destination : `Blueprints/UI/Inventory/`.

### Batch D — InGameMenu (8)

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

Destination : `Blueprints/UI/InGameMenu/`.

## Règles de migration

Les déplacements sont réalisés **uniquement depuis Unreal Editor**.

Pour chaque batch :

```text
1. Créer le dossier cible dans Content Browser.
2. Déplacer les assets avec Move Here.
3. Save All.
4. UI -> Fix Up Redirectors in Folder.
5. Save All.
6. Fermer/réouvrir l'éditeur si un Blueprint reste chargé avec un ancien chemin.
7. Lancer l'audit UI-ASSET-ORG01.
8. Effectuer le smoke PIE du domaine avant de passer au batch suivant.
```

Ne pas déplacer les quatre batches en une seule opération.

## Audit automatique

Test :

```text
Grimrock.Editor.UIAssetOrg01.MigrationAudit
```

Pour chacun des 22 assets, le test accepte soit l'ancien chemin, soit le nouveau, mais **jamais les deux et jamais aucun**.

Sortie attendue avant migration :

```text
Location=SOURCE
```

Après déplacement + Fix Up Redirectors :

```text
Location=TARGET
```

Un `Location=INVALID` indique soit un doublon/redirector encore présent, soit un asset manquant.

## Chemins C++ codés en dur

La recherche repository ne trouve aucun chemin de production codé en dur du type :

```text
/Game/GrimrockPrototype/Blueprints/UI/WBP_...
```

pour les 22 assets déplacés.

Le chemin C++ connu vers :

```text
/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard
```

reste inchangé car le dossier `RPG` n'est pas déplacé.

Les références principales de ces WBP sont donc sérialisées dans les assets Unreal et doivent être mises à jour automatiquement par un déplacement effectué dans le Content Browser.

## Documentation

Les documents historiques ne sont pas réécrits aveuglément pendant la migration.

Après validation des quatre batches, mettre à jour les documents CURRENT/canoniques qui donnent encore les anciens chemins physiques. Les documents explicitement historiques peuvent conserver leurs chemins d'époque s'ils restent clairement marqués comme tels.

## Ordre recommandé

Commencer par **Batch A — Fonts** : seulement deux assets, faible surface fonctionnelle et validation très simple.

Ne passer au batch suivant qu'après validation du batch courant.
