# TD06.8 — PartyInventory Item Definition Registry / Rehydration audit

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD06.7 — PartyInventory Equipment Core extraction**  
Baseline GitHub : `3351ebb1a003db0a5ffccfe1f7450da4b674a195`  
Statut : **PRÊT À VALIDER LOCALEMENT**

## Objet

TD06.8 audite la frontière **Item Definition Registry / Rehydration** de `UGridPartyInventoryComponent` après les extractions Hotbar, Cursor et Equipment.

Contrairement aux tranches précédentes, l'objectif n'est pas de chercher mécaniquement un nouveau `.cpp`. L'audit doit déterminer si cette responsabilité est encore une frontière indépendante risquée ou si elle appartient désormais légitimement au cœur d'autorité/orchestration du composant.

`UGridPartyInventoryComponent` et `FGridPartyInventoryState` restent l'unique façade et l'unique autorité d'état.

## Périmètre audité

```text
RegisterItemDefinition
RehydrateOwnedItemDefinitions
FindItemDefinition
ApplyItemDefinitionToInstance
RuntimeItemDefinitionsById
```

Le bloc de production représente environ **110 lignes** dans le fichier principal de 1 357 lignes.

## Analyse architecturale

Le registre est `Transient` et sert de table de résolution runtime pour des identités persistées par `FName`. Il est utilisé transversalement par :

- l'ajout / stacking d'inventaire ;
- la restauration Save / Continue ;
- la Hotbar ;
- le Cursor Transfer ;
- l'Equipment Core ;
- les actions de combat ;
- les widgets / diagnostics qui ont besoin de retrouver la définition autoritaire.

`RehydrateOwnedItemDefinitions()` parcourt les sources possédées autoritaires et reconstruit le registre après restore. Le déplacer dans un fichier séparé resterait techniquement possible, mais ne réduirait pas aujourd'hui une dépendance ou un risque comparable aux frontières Hotbar/Cursor/Equipment.

**Décision provisoire TD06.8 : ne pas extraire le Registry/Rehydration.**

Cette décision devient définitive pour TD06 lorsque le contrat ci-dessous est vert et que TD06.9 confirme la stop condition.

## Contrats existants réutilisés

### CC5

`Grimrock.CharacterCreation.CC5.SaveMemoryRoundTrip` couvre déjà :

- le caractère transient du registre ;
- le restore de `FGridPartyInventoryState` ;
- le rehydrate d'une définition d'item équipé ;
- la conservation du `RuntimeObjectId` après restore ;
- l'ownership valide après rehydrate.

### MON18.8

`Grimrock.Magic.MON18.8.SpellBindingIsNotItemDefinition` verrouille déjà le fait qu'un binding Hotbar de policy `Spell` ne doit jamais être envoyé au resolver d'ItemDefinition.

## Nouveau contrat TD06.8

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridTD068PartyInventoryItemDefinitionRegistryTests.cpp
```

Filtre :

```text
Grimrock.TechnicalDebt.TD06_8.PartyInventoryItemDefinitionRegistry.Contract
```

### Invariants ajoutés

1. `RegisterItemDefinition(nullptr)` et un ID `None` sont refusés.
2. Réenregistrer un même ID retourne `true` mais **ne remplace pas** le premier asset enregistré.
3. `FindItemDefinition(NAME_None)` retourne `nullptr`.
4. `ApplyItemDefinitionToInstance()` :
   - copie le poids ;
   - remplit seulement un `DisplayName` vide ;
   - applique l'état lumineux par défaut ;
   - force un item non stackable à quantité 1 ;
   - clamp une pile entre 1 et `MaxStackSize` ;
   - échoue sans mutation si la définition est absente.
5. `RehydrateOwnedItemDefinitions()` résout uniquement les ItemDefinitions réellement possédées depuis :
   - inventaire des personnages actifs ;
   - inventaire de `CharacterPool` ;
   - équipement actif ;
   - Cursor ;
   - Hotbar `Equipment` ;
   - Hotbar `QuickItem`.
6. Les bindings Hotbar `Spell` et `Ability` ne sont jamais envoyés au resolver d'items.
7. Les IDs possédés sont dédupliqués avant résolution.
8. Un rehydrate réussi remplace le registre transient précédent par les seules définitions possédées.
9. Un resolver manquant ou retournant un asset avec le mauvais `ItemDefinitionId` fait échouer le rehydrate.
10. L'échec est **atomique vis-à-vis du registre existant** : l'ancien registre reste intact.

## Résidu diagnostics TD02.3

L'audit statique après TD06.7 confirme que les helpers suivants n'ont plus aucune référence en dehors de leur propre définition dans `GridPartyInventoryComponent.cpp` :

```text
GetItemTypeName
GetEquipmentSlotsText
GridInventoryCompatibilityDiagnosticsIsHandSlot
GridInventoryCompatibilityDiagnosticsIsExcludedPaperDollSlot
GridInventoryCompatibilityDiagnosticsIsNewPaperDollSlot
GridInventoryCompatibilityDiagnosticsLooksPotentiallyEquippable
GetEquipmentStatBonusText
GetDamageResistanceSetText
```

Ils sont donc **code mort confirmé**. Leur suppression est réservée à TD06.9, après validation TD06.8, afin de respecter la règle « caractériser avant changement » et de garder TD06.8 sans modification de production.

`GetEquipmentSlotName` et `ForEachEquipmentItem` ne sont pas morts : ils restent utilisés par le rehydrate, les diagnostics, le poids et l'ownership.

## Validation demandée

Depuis la racine du projet :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TechnicalDebt.TD06_8"
```

Puis régression Save/rehydration existante :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.CharacterCreation.CC5"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Magic.MON18.8.SpellBindingIsNotItemDefinition"
```

Critères :

```text
TD06_8 : 1 Success / 0 warning / 0 Failed
CC5    : 2 Success / 0 warning / 0 Failed
MON18.8 SpellBindingIsNotItemDefinition : 1 Success / 0 warning / 0 Failed
```

## Suite

Si cette validation reste verte :

```text
TD06.9 — PartyInventory final re-audit / dead diagnostic helper cleanup / stop condition
```
