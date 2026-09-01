# HOTBAR01.2.1 — Remove ThrowMainHand Legacy Compatibility

Date : 01.09.2026

## Objectif

Supprimer la compatibilité arrière introduite temporairement pour l'ancien binding synthétique de lancer MainHand.

Le projet étant encore en phase prototype, cette compatibilité ajoute de la complexité sans bénéfice produit réel.

## Suppressions

Le refactor retire :

- le générateur d'identité du binding historique de lancer MainHand dans `FGridCombatHotbarBinding` ;
- l'action synthétique correspondante dans `GridCombatHudWidget.cpp` ;
- sa résolution spéciale dans `RequestHotbarSlot()` ;
- son affectation spéciale dans `AssignCombatActionToHotbarSlot()` ;
- le bypass clavier dans `AGrimrockPartyPawn::TryExecuteCombatHotbarSlot()` ;
- les assertions de test qui validaient cette identité persistante.

Le helper runtime servant réellement à lancer l'objet **actuellement équipé** est conservé, mais renommé `ResolvePhysicalThrowEquippedMainHand()` afin d'éviter toute confusion avec l'ancien binding.

## Chemins autoritaires après HOTBAR01.2.1

### Objet équipé

```text
MainHand
  -> menu contextuel Lancer
  -> Cursor_Aim
  -> projectile physique
```

### Objet d'inventaire affecté à la hotbar

```text
Inventaire
  -> drag vers slot 2–9,0
  -> ThrowItem_<ItemDefinitionId>
  -> raccourci
  -> Cursor_Aim
  -> projectile physique
```

Aucune action synthétique « lancer ce qui est actuellement en MainHand » n'est persistée dans la hotbar.

## Politique de sauvegarde prototype

Tant que GrimrockPrototype reste en phase prototype :

- une sauvegarde ancienne peut être invalidée par un refactor structurel ;
- aucun shim, alias ou migration de sauvegarde n'est ajouté automatiquement ;
- une compatibilité arrière ne sera introduite que sur décision explicite.

Si une sauvegarde locale contient un binding ancien supprimé, elle peut être réinitialisée.

## Validation

Filtres recommandés :

```text
Grimrock.TechnicalDebt.TD02_7.PartyItemTransfer
Grimrock.TechnicalDebt.TD06_2.PartyInventoryHotbar
Grimrock.Monsters.MON12.8
Grimrock.Hotbar.HOTBAR01_1
```
