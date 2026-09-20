# UI-INV02 — Drag Transfer Between Party Members

Date : **20 septembre 2026**  
Statut : **AUTOMATION FONCTIONNELLE AVEC WARNINGS — clean rerun demandé ; passe UMG non réalisée**

## Objectif

UI-INV02 permet de transférer un objet du sac du personnage sélectionné vers un autre membre du groupe par drag & drop sur son portrait.

```text
slot inventaire du personnage A
    -> drag
    -> portrait du personnage B
    -> item transféré dans l'inventaire de B
```

Le transfert ne change pas le personnage sélectionné. Il s'agit d'une mutation d'inventaire, pas d'une commande de navigation.

## Autorité et transaction

Aucune logique de propriété n'est ajoutée au Blueprint.

```text
UGridPartyMemberWidget::NativeOnDrop()
    -> UGridInventoryWidget::HandlePartyMemberItemDrop()
    -> UGridItemTransferService::TransferInventorySlotToCharacter()
    -> UGridPartyInventoryComponent
```

Le service valide la source, la destination, la quantité et la capacité du sac cible. La source n'est mutée qu'après le préflight de capacité. Si l'insertion destination échoue, le slot source est restauré.

## Identité runtime

Le drag capture désormais :

```text
SourceCharacterIndex
SourceSlotType
SourceSlotIndex
SourceItemDefinitionId
SourceRuntimeObjectId
```

Au drop, le slot source est relu. Si son `RuntimeObjectId` ou sa définition ne correspondent plus au drag initial, le transfert est refusé.

Un stack complet conserve son identité runtime lorsqu'il devient une stack distincte dans la destination. Un Ctrl-drag crée une nouvelle identité pour la quantité séparée et laisse l'identité originale au reliquat source.

## Ownership destination

`AddItemToCharacterInventory()` reste responsable de réécrire :

```text
OwnerType           = CharacterInventory
OwnerGuid           = CharacterId destination
OwnerCharacterIndex = index destination
EquipmentSlot       = None
```

UI-INV02 ne duplique pas cette règle.

## Interaction portrait

Les `UGridPartyMemberWidget` enregistrés par UI-CHAR01 reçoivent maintenant leur `OwningInventoryWidget` et deviennent des drop targets natifs pour `UGridInventoryDragDropOperation`.

Aucune mutation Blueprint n'est requise. Le Blueprint peut seulement gérer plus tard un feedback visuel hover/valid/invalid.

## Comportement

Un drop de A vers B transfère l'objet puis rafraîchit les portraits et le sac visible. A reste sélectionné afin de permettre plusieurs transferts successifs.

Un drop sur le portrait source n'est pas un transfert et ne modifie aucun état.

Une destination pleine refuse le transfert et conserve exactement la source.

UI-INV02 cible volontairement les **slots d'inventaire**. Un item équipé n'est pas implicitement déséquipé puis transféré.

## Contrat UMG

Dans `WBP_PartyMember`, la racine doit rester hit-testable pour que `NativeOnDrop()` reçoive le drag.

```text
WBP_PartyMember
└── Overlay_Root
    ├── surface hit-testable du widget
    ├── Image_Portrait
    ├── informations personnage
    └── Border_Selected
```

Les éléments décoratifs internes peuvent rester `HitTestInvisible`, mais pas toute la racine.

## Hors périmètre

UI-INV02 ne réalise pas :

- drag d'un équipement directement vers un autre personnage ;
- échange forcé lorsque le sac cible est plein ;
- tri automatique ;
- filtre ;
- changement automatique de sélection après transfert.

## Invariants

1. Le drag mémorise le personnage source.
2. Le drop relit et valide l'identité runtime source.
3. Le transfert passe par `UGridItemTransferService`.
4. Aucun item n'est dupliqué.
5. Une destination pleine ne consomme pas la source.
6. Un split reçoit une nouvelle identité runtime.
7. Le personnage sélectionné ne change pas lors du transfert.
8. Aucun Blueprint ne modifie directement les tableaux d'inventaire.

## Automation

Filtre :

```text
Grimrock.UI.Inventory02
```

Tests :

```text
Grimrock.UI.Inventory02.PartyTransferService
Grimrock.UI.Inventory02.PortraitDropRouting
```

Couverture :

- transfert de stack complet ;
- ownership destination ;
- conservation d'identité du stack complet ;
- split avec nouvelle identité ;
- rejet si destination pleine ;
- drop portrait ;
- conservation de la sélection ;
- rejet d'un drag obsolète ;
- rejet du drop sur le portrait source.

## Validation locale

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Inventory02"
```

Après la passe UMG, contrôler en PIE :

1. ouvrir le sac de A ;
2. drag d'un item sur le portrait de B ;
3. constater sa disparition de A ;
4. sélectionner B et constater sa présence ;
5. revenir à A et vérifier que la sélection n'avait pas changé pendant le drop ;
6. Ctrl-drag d'un stack > 1 et vérifier le split ;
7. remplir B puis vérifier le rejet sans perte ;
8. vérifier les drops slot-à-slot existants ;
9. aucun `BindWidget` critique.


## Validation reçue le 20 septembre 2026

Première exécution locale :

```text
Filter                 : Grimrock.UI.Inventory02
Succeeded              : 0
Succeeded with warnings: 2
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le comportement est passé, mais les deux tests ont été classés `Succeeded with warnings` parce qu'ils exercent volontairement des rejets normaux : destination pleine, drag obsolète et drop sur le même portrait.

Le follow-up UI-INV02.1 abaisse ces rejets attendus en `Log/Verbose`. Les véritables incohérences (index invalide, rollback impossible, ownership invalide) restent `Warning/Error`.

Une nouvelle exécution propre de `Grimrock.UI.Inventory02` est demandée avant clôture du contrat C++.
