# RPG03.9.6B1 — Alchimiste : Grenadier + Apothicaire

Date : **6 octobre 2026**  
Dépendance : `RPG03.9.6A` validé 8/8  
Statut : **VALIDÉ — Grenadier/Apothicaire couvert par Grimrock.RPG.RPG03, 193/193 le 6 octobre 2026**

## Périmètre

B1 authorise sans matérialiser de binaire :

- Grenadier 5/5 ;
- Apothicaire 5/5 ;
- 10 Choice records ;
- 8 QuickItems concrets ;
- 5 Status configurables, dont `Status_Poison` partagé.

Les actions QuickItem restent dans `UGridItemDefinitionAsset`, pas dans `URPGClassAsset::CombatActions`.

## Identité QuickItem

C7 conservait historiquement `Use_<ItemDefinitionId>`.

B1 ajoute un override optionnel générique :

```text
QuickItemActionIdOverride
```

`NAME_None` conserve le comportement existant. Les items Alchimiste peuvent ainsi respecter les ActionIds canoniques de la règle sans hard-code runtime.

## Grenadier

- Fire Bomb : Area1 R4, 2 PA, 6 + Alchemy, Burning 2 Turns derrière MagicalArmor=0, Surface Fire 2.
- Toxic Bomb : Area1 R4, 2 PA, 6 + Alchemy, Poison 3 Turns derrière MagicalArmor=0, Surface Poison 3.
- Charge précise : bombes +1 portée, -50 % dégâts directs aux alliés.
- Réaction en chaîne : C4 SurfaceReaction OncePerAction, +25 % dégâts / +1 rayon, pas de récursion.
- Maître grenadier : bombes -1 PA, +20 % dégâts directs.

## Apothicaire

- Potion renforcée : potions positives +25 % Health/Mana/Armor, pas de durée.
- Antidote : Ally R1 AP1, Poison + un Toxin.
- Élixir défensif : quatre items Fire/Ice/Lightning/Poison, +4 MagicalArmor et +25 % résistance 3 rounds.
- Diffusion : un second allié, 50 % magnitude et durée, pas de consommation supplémentaire.
- Panacée : Ally R1 AP2 CD3, 10 + 2×Alchemy HP, +8 MagicalArmor, jusqu'à 3 Debuffs.

## Recettes

Les Choice records accordent des `GrantedRequirementIds` `Recipe_*` comme contrat futur. Aucun moteur de crafting n'est ajouté.

## Validation

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.6B1"
```
