# ALIGN-A — Dead Code & Asset Hygiene

Date : 2026-09-09

Statut : **implémentation terminée ; validation finale élargie encore à exécuter avant clôture définitive**.

## 1. Objectif

ALIGN-A intervient après WORLDOBJ-MIG10 pour supprimer du code mort, corriger des références sérialisées obsolètes et retirer des assets devenus inutiles, sans relancer une migration d’architecture ni toucher aux bridges encore nécessaires.

Le principe suivi a été conservateur : aucune suppression de `.uasset` sur simple intuition, aucune réécriture large des systèmes runtime et aucune suppression de compatibilité sans preuve d’inutilisation.

## 2. Exclusions explicites

ALIGN-A n’a pas supprimé ni refactoré les éléments suivants :

```text
PlacementKind
PlacementZOffset
WallInset
LocalOffsetAlongWall
LocalOffsetVertical
RefreshPlacementRuntimeProjection()
IsCenterPlaced()
IsEdgePlaced()
FGridWorldObjectInstance::Type
```

Sont également conservés :

- les anciens paramètres/tableaux audio de porte, migration distincte ;
- `bThrowable` ;
- `ItemActorClass` ;
- les API Blueprint non prouvées mortes ;
- les fallbacks runtime hors périmètre direct ;
- les packages historiques dont le nom contient encore `GridObjectArchetypeAsset` ou `DA_Archetype_*` mais dont le contenu reste actif.

## 3. Étapes réalisées

| Étape | Commit | Résultat |
|---|---|---|
| ALIGN-A1 | `732e009b7d316b48c146eea0874e03aef15ee5ca` | suppression de 5 helpers morts de placement/visibilité |
| ALIGN-A2 | `b19a9fdf59821a27e5bc25a66d9cf5ab7b64af56` | suppression de 8 helpers morts de groupes de paramètres |
| ALIGN-A3 | `4fd3afe65317a574605fa6505da0d6d138f1b49a` | audit AssetRegistry read-only des assets suspects |
| ALIGN-A4 | `435215e265fe661d9aec7893f66de2a0896236bf` | synchronisation canonique des définitions du preview depuis la palette |
| ALIGN-A5 | `24ddbf2796f338a7f4cd84cc5e2f9f142c6693de` | resave de `L_GrimrockEditor` et retrait des références pickup résiduelles |
| ALIGN-A6-B | `8e91d7b6a328dcb21b897a51ac71b210e1ab1416` | test d’audit mis à jour avec les attentes de présence/absence |
| ALIGN-A6 | `e4cfe23e19d26e2d56b58514472b157531a722da` | suppression des 6 anciens world-object pickup assets |
| ALIGN-A7 | commit de cette documentation | cohérence documentaire et rapport de clôture technique |

## 4. Code mort supprimé

### ALIGN-A1

Supprimés :

```text
RequiresEdgePlacement()
SupportsCenterPlacement()
SupportsWallPlacement()
AllowsInvisibleRuntimeObject()
IsCeilingPlaced()
```

Le test PIT01 qui appelait encore `SupportsCenterPlacement()` a été réécrit pour vérifier directement l’autorité de placement courante.

### ALIGN-A2

Supprimés :

```text
UsesWallPlacementParams()
UsesCenterPlacementParams()
UsesReadableParams()
UsesItemParams()
UsesTriggerParams()
UsesMovingMeshParams()
UsesFixedMeshParams()
UsesRuntimeActorClass()
```

Restent volontairement utilisés/conservés :

```text
RequiresRuntimeActorClass()
UsesLightParams()
UsesReceptacleParams()
UsesTeleporterParams()
UsesButtonAnimationParams()
```

## 5. Audit et nettoyage des assets

ALIGN-A3 a audité neuf packages via AssetRegistry.

Après nettoyage de la map en A5, six anciens pickups sont passés à zéro referencer on-disk :

```text
DA_Object_BlueGemPickup
DA_Object_KeyCopperPickup
DA_Object_KeyIronPickup
DA_Object_ShurikenPickup
DA_Object_StonePickup
DA_Object_TestNotePickup
```

Ils ont ensuite été supprimés via Unreal Editor puis commités en A6.

Trois définitions restent volontairement actives :

```text
DA_MonsterSpawn
DA_Archetype_CustomRecruiter_Service
DA_Archetype_StoryCompanion_Recruit
```

Lors du dernier audit local A6, chacune de ces trois définitions était encore référencée par :

```text
/Game/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default
/Game/GrimrockPrototype/Maps/L_GrimrockEditor
```

## 6. Cause racine des références pickup résiduelles

Avant ALIGN-A4, `SyncPreviewRuntimeWorldObjectDefinitionsFromPalette()` ajoutait les définitions présentes dans la palette avec `AddUnique()` mais ne retirait jamais celles qui avaient disparu de la palette.

Conséquence : `L_GrimrockEditor` pouvait conserver indéfiniment des références sérialisées vers d’anciens pickup world-object definitions.

ALIGN-A4 remplace ce comportement par une projection canonique :

```text
ObjectPalette.Entries[].DefaultWorldObjectDefinition
        |
        v
PaletteDefinitions uniques
        |
        v
PreviewRuntimeActor.WorldObjectDefinitions
```

La liste n’est remplacée que si le contenu diffère, afin d’éviter de salir inutilement la map.

ALIGN-A5 a ensuite chargé/reconstruit/sauvegardé `L_GrimrockEditor`, ce qui a effectivement supprimé les six références résiduelles.

## 7. Architecture collectible obtenue

Le pipeline de production est désormais cohérent de bout en bout :

```text
UGridItemDefinitionAsset
        |
        +--> FGridObjectPaletteEntry.DefaultItemDefinition
        |
        +--> FGridLooseItemInstance.ItemDefinition
        |
        +--> AGridItemActor
        |
        +--> inventory / equipment / drop / receptacle
```

Un collectible direct ne doit pas avoir de `DefaultWorldObjectDefinition` compagnon.

La validation de `FGridObjectPaletteEntry::IsValidEntry()` encode ce contrat : une entrée item directe est valide avec `DefaultItemDefinition`, sans `DefaultWorldObjectDefinition` ni icône de palette séparée.

## 8. Validation locale déjà obtenue

Les validations UE5.5.4 suivantes ont été fournies localement pendant ALIGN-A :

| Étape / filtre | Résultat |
|---|---|
| A1 — `Grimrock.WorldObjects` | 37 success / 0 warning / 0 failed |
| A1 — `Grimrock.Pit.PIT01` | 1 success / 1 success-with-warning / 0 failed |
| A2 — `Grimrock.WorldObjects` | 37 success / 0 warning / 0 failed |
| A2 — `Grimrock.Pit` | 0 failed, process exit 0 |
| A3 — `Grimrock.Editor.ALIGN_A3.AssetReferenceAudit` | 1 success / 0 warning / 0 failed |
| A4 — `Grimrock.Editor.ALIGN_A4.PreviewDefinitionSync` | 1 success / 0 warning / 0 failed |
| A4 — `Grimrock.WorldObjects` | 37 success / 0 warning / 0 failed |
| A6 — `Grimrock.Editor.ALIGN_A3.AssetReferenceAudit` | 1 success / 0 warning / 0 failed |
| A6 — `Grimrock.WorldObjects` | 37 success / 0 warning / 0 failed |
| A6 — `Grimrock.TechnicalDebt.TD02_1.WorldItemsContract` | 1 success / 0 warning / 0 failed |
| A6 — `Grimrock.TechnicalDebt.TD02_7.PartyItemTransfer` | 3 success / 0 warning / 0 failed |

Les filtres `Grimrock.ItemPickup` et `Grimrock.ItemTooltip` ne sont pas des filtres Automation existants dans le Source courant ; ils ne doivent pas être utilisés comme preuve de régression. Le contrat pickup est couvert par `TD02_1.WorldItemsContract` et les transferts par `TD02_7.PartyItemTransfer`.

## 9. Gate final avant fermeture définitive

Après récupération du commit ALIGN-A7, exécuter la validation élargie suivante :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.WorldObjects"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Pit"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Monsters.MON13"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD02_1.WorldItemsContract"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD02_7.PartyItemTransfer"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Editor.ALIGN_A3.AssetReferenceAudit"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Editor.ALIGN_A4.PreviewDefinitionSync"
```

Si ces filtres passent sans échec, ALIGN-A peut être marqué **CLOSED** par une dernière mise à jour documentaire courte du présent rapport, sans nouvelle modification runtime/content.

## 10. Conclusion technique

ALIGN-A a supprimé 13 helpers morts, corrigé une accumulation de références sérialisées dans la preview, supprimé six assets pickup devenus réellement orphelins et aligné la documentation sur le modèle post-MIG10.

Le résultat recherché est atteint : un item ramassable est défini une fois, par `UGridItemDefinitionAsset`, et n’a plus besoin d’un `UGridWorldObjectDefinitionAsset` compagnon uniquement pour être placé dans un niveau.
