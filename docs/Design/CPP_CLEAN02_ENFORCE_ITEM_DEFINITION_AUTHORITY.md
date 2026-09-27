# CPP-CLEAN02 — Enforce Item Definition Authority

Date : **27 septembre 2026**  
Parent : **CPP-AUDIT01 — Audit général du code C++**

## Objectif

Supprimer deux permissivités historiques qui empêchaient `UGridItemDefinitionAsset`
d'être l'autorité unique des items équipables :

1. un `FGridItemInstance` sans définition enregistrée pouvait être équipé dans
   n'importe quel slot Equipment supporté ;
2. deux assets différents pouvaient partager silencieusement le même
   `ItemDefinitionId`, le premier enregistré gagnant uniquement par ordre
   d'exécution.

Aucune nouvelle couche n'est introduite.

## Contrat après CPP-CLEAN02

### Compatibilité Equipment

```text
item invalide                                      -> false
personnage invalide                                -> false
slot Equipment non supporté                        -> false
ItemDefinition absente du registre                 -> false
ItemDefinition présente + slot non compatible      -> false
ItemDefinition présente + slot compatible          -> true
```

`CompatibleEquipmentSlots` de `UGridItemDefinitionAsset` est donc la seule
autorité de compatibilité.

Un item non résolu peut encore exister temporairement dans l'inventaire, par
exemple pendant une fixture ou avant rehydration. Cette présence ne lui confère
aucun droit d'équipement.

Le chemin normal de pickup reste compatible avec cette règle :
`AGrimrockPartyPawn::AddItemInstanceToSelectedCharacterInventory()` résout la
définition depuis le runtime de niveau et l'enregistre avant l'ajout.

### Unicité ItemDefinitionId

```text
ID nouveau + asset valide              -> register true
même ID + même asset                   -> true, idempotent
même ID + autre asset                  -> false, DuplicateId
```

Un conflit d'identité ne dépend donc plus de l'ordre silencieux
d'enregistrement.

## Modifications

### GridPartyInventoryComponentEquipment.cpp

Suppression du fallback :

```text
definition absente + slot supporté -> true
```

`CanEquipItemToSlot()` exige désormais une définition enregistrée.

`EquipItemFromInventorySlot()` fournit un diagnostic distinct :

```text
Reason=MissingDefinition
```

et échoue avant toute mutation.

### GridPartyInventoryComponent.cpp

`RegisterItemDefinition()` reste idempotent pour le même asset, mais rejette
un second asset portant le même `ItemDefinitionId` avec :

```text
Reason=DuplicateId
```

Le registre existant reste intact.

### Tests

`Grimrock.TechnicalDebt.TD06_6.PartyInventoryEquipmentCore.Contract` vérifie
désormais :

- qu'un item sans définition est non équipable ;
- que l'appel transactionnel d'équipement le rejette ;
- que le rejet est atomique ;
- que le slot source reste inchangé ;
- que MainHand reste vide.

`Grimrock.TechnicalDebt.TD06_8.PartyInventoryItemDefinitionRegistry.Contract`
vérifie désormais :

- réenregistrement du même asset = idempotent ;
- autre asset avec le même ID = rejet ;
- l'asset autoritaire initial reste enregistré.

Les documents TD06.6, TD06.7 et TD06.8 sont mis à jour pour indiquer que leurs
anciennes caractérisations historiques ont été explicitement remplacées par ce
contrat.

## Non-objectifs

CPP-CLEAN02 ne modifie pas :

- la structure de `FGridPartyInventoryState` ;
- le SaveGame ;
- le rehydrate des définitions possédées ;
- l'ajout générique d'un item à l'inventaire ;
- les transferts World/Cursor/Receptacle ;
- les stats ou résistances ;
- les DataAssets binaires ;
- les Blueprints.

## Validation demandée

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD06_6"
```

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD06_8"
```

Puis le chemin Cursor/Equipment qui consomme la même autorité :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD06_4"
```

Enfin, après validation ciblée, le run global `Grimrock` reste le critère de
non-régression final.

## Validation PIE recommandée

Avec des items réels déjà configurés :

- arme compatible -> équipement OK ;
- armure vers mauvais slot -> refus ;
- swap équipement -> inchangé ;
- drag Cursor -> Equipment -> inchangé.

Le ticket est validé uniquement après sortie UE locale fournie par l'utilisateur.
