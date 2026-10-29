# ARMEMENT01.15 — Potion de soin N1 commune au Prêtre et à l'Apothicaire

## Objectif

Produire **un seul** `UGridItemDefinitionAsset` réel :
`/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01`
(`ItemDefinitionId = Item_Potion_Healing_01`).

Cette définition est consommée par **deux kits approuvés**, sans imposer une
branche ou attribuer un talent de niveau 2 :

- `Kit_Priest_Restoration` : sceptre sacré N1 + potion de soin ×1 (sac).
- `Kit_Alchemist_Apothecary` : dague N1 + potion de soin ×1 (sac).

Les autres kits ne changent pas ; aucun nouvel inventaire ni moteur de potion n'est introduit.

## Contrat de gameplay N1

| Propriété | Valeur initiale |
|---|---|
| ItemDefinitionId | `Item_Potion_Healing_01` |
| ItemType | `Potion` |
| Quantité au départ | 1 dans chacun des deux kits |
| Stackable | Oui, maximum 10 par pile |
| Poids | 0,2 kg |
| Action QuickItem | `Use_Item_Potion_Healing_01` |
| Targeting / Resolution | `Self` / `Effect` |
| Restauration | **20 PV** |
| Coût en PA | **1** |
| Coût source | **1 objet** |
| Tags | `QuickItem.Potion`, `QuickItem.Potion.Positive` |
| Restriction | Inutile à pleine santé ; aucun objet ni PA consommé en cas de refus |
| Compétence | Aucun rang ni talent automatiquement accordé |

**Équilibrage provisoire :** 20 PV / 1 PA est la configuration type existante de
`docs/Design/MON12_8_4_QUICK_ITEM_COMBAT_ACTIONS.md`. Ce montant est
conservé pour le premier essai en jeu ; sa valeur finale exige une revue d'équilibrage.
Le talent `EnhancedPotion` N2, s'il est acquis ultérieurement, reconnaît le tag
`QuickItem.Potion.Positive` via le système C2/C7 existant.

**Localisation :** `Localization/Source/UI.csv` définit les deux clés
`Item.StartingPotion.Healing01.Name` et
`Item.StartingPotion.Healing01.Description`. L'asset conserve des références
`FText` à `ST_UI`. La présentation n'est pas codée en dur dans le runtime.

## Production

Le script **existant** `Scripts/AuthorRPGAlchemist.ps1` offre désormais
`-StartingHealthPotion -Apply`, un mode isolé qui :

1. Compile le Development Editor avec `ValidateUE.ps1` ;
2. Synchronise l'asset existant `ST_UI.uasset` depuis `UI.csv` via
   `UIStringTableAuthoring` ;
3. Utilise le commandlet **existant** `RPGAlchemistAuthoring`
   en mode `-StartingHealthPotion -Apply` ;
4. Vérifie que la définition est valide et **refuse de remplacer** un package
   existant incompatible ;
5. Exécute les tests `Grimrock.RPG.ARMEMENT01.15.HealthPotion`.

Une seconde exécution sur un package identique est idempotente
(`PRESENT_SAME`), et les autres assets Alchimiste ne sont pas modifiés.
Le mode normal `AuthorRPGAlchemist.ps1` est conservé inchangé.

### Commande locale unique

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\AuthorRPGAlchemist.ps1 -EngineRoot D:\UE_5.5 -StartingHealthPotion -Apply
```

Après le succès des tests, vérifier les deux packages locaux dans le Content Browser :
`Content/Localization/StringTables/ST_UI.uasset` et
`Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.uasset`.
**Ces binaires ne peuvent pas être créés dans GitHub sans lancer UE** ; ils
doivent être committés depuis la machine Windows après acceptation PIE.

## Critères de validation

- Tests `AuthoringAndQuickItemContract` : définition réelle du QuickItem,
  action affichée pour un blessé, bloquée à pleine santé et en absence d'objets ;
- Tests `PriestAndApothecaryNewGame` : véritables deux classes et kits,
  attribution d'une potion par héros, arme N1 conservée, GUID,
  identité/quantité dans snapshot de sauvegarde et aucun changement
  des six DataAssets de classe ;
- Tests de régression `Grimrock.RPG.ARMEMENT01.13.ApprovedKits` et
  `Grimrock.RPG.ARMEMENT01.9` regroupés après matérialisation ;
- PIE : `New Game`, essai avec PV réduits, consommation unique,
  refus à pleine santé, `Save → Quitter → Continue` sur disque ;
- Icône/mesh définitifs de potion : **passe visuelle ultérieure**, ne pas
  substituer un mesh non validé et déclarer l'art terminé.

### État

Code C++ / configuration auteur / tests enregistrés sur `master` ;
**compilation et génération du package UE locales en attente**.
