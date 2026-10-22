# ARMEMENT01.9.2 — Douze kits de départ pour les six classes

> **Statut au 22.10.2026 — LEGACY RUNTIME MAINTENU.** Les 12 kits (deux par classe) décrivent toujours les recettes *effectivement distribuées* par `FRPGStartingEquipmentPresetCatalog`. Leur choix d'armes ne constitue **plus** la référence de conception pour le futur. Les 18 recettes orientées branches figurent dans [ARMEMENT01.9.6](ARMEMENT01_9_6_EIGHTEEN_STARTING_KITS.md), avec les règles de transition dans [ARMEMENT01.9.6.1](ARMEMENT01_9_6_1_CLEANUP_CONSOLIDATION.md). **Ne pas retirer les identifiants historiques ni les tests liés aux sauvegardes.**

## Portée

Complète le socle transactionnel validé sous `Grimrock.RPG.ARMEMENT01.9.StartingKits` (4/4).
Cette tranche ne modifie aucun `.uasset` ni aucun widget UMG.

Le catalogue déterministe C++ `FRPGStartingEquipmentPresetCatalog` fournit **deux kits
par classe de production**. Les objets référencés existent sous
`/Game/GrimrockPrototype/Core/DataAssets/Weapons` et `.../Items`.

## Kits initiaux

| Classe | Kit (par défaut en premier) | Équipement | Sac |
|---|---|---|---|
| Guerrier (`Warrior`) | `Warrior_Defender` — Défenseur | Dague, bouclier rond | — |
| | `Warrior_Skirmisher` — Escarmoucheur | Dague | Couteau de lancer |
| Voleur (`Rogue`) | `Rogue_Shadow` — Ombre | Dague | Couteau de lancer |
| | `Rogue_Explorer` — Éclaireur | Dague | Torche |
| Rôdeur (`Ranger`) | `Ranger_Archer` — Archer | Arc court à deux mains | Dague |
| | `Ranger_Pathfinder` — Pisteur | Dague | Torche |
| Mage (`Mage`) | `Mage_Scholar` — Érudit | — | Dague, gemme bleue |
| | `Mage_Wanderer` — Vagabond | — | Dague, torche |
| Prêtre (`Priest`) | `Priest_Pilgrim` — Pèlerin | Dague | Torche |
| | `Priest_Protector` — Protecteur | Dague, bouclier rond | — |
| Alchimiste (`Alchemist`) | `Alchemist_Apothecary` — Apothicaire | — | Dague, fiole vide |
| | `Alchemist_Firestarter` — Artificier | — | Dague, bombe incendiaire |

Tous les objets ont `Quantity=1` et `ItemLevel=0` en configuration :
le niveau est résolu lors de leur attribution par
`FRPGStartingEquipmentService::TryBuildKit`.
Chaque kit possède un identifiant stable, réutilisable par le futur wizard.

Ces kits sont des **premières propositions jouables** utilisant les ressources
disponibles, et non la liste définitive des armes magiques, sacerdotales ou
d'alchimie. Ils pourront être remplacés par des données d'auteur lorsqu'un
catalogue d'objets adapté à chaque classe sera produit.

## Autorité et compatibilité

- Seules les six instances `DA_Class_*` de production (chemin exact) obtiennent
  automatiquement un kit par défaut lors de `CreateInitialCharacter()`.
- Pour ces classes, le runtime duplique la définition dans la mémoire de
  l'inventaire et peuple **la copie uniquement**. Les `.uasset` d'origine
  restent intacts. Aucune écriture de package et aucun réenregistrement.
- Si un asset de classe définit déjà ses propres `StartingEquipmentKits`
  et `DefaultStartingEquipmentKitId`, ceux-ci ont priorité ; les recettes
  C++ ne les remplacent pas.
- Une classe créée en mémoire ou des tests hérités ayant simplement
  `ClassId=Warrior` ne reçoivent **pas** ces kits par surprise.
- Une définition d'objet manquante/incompatible, un niveau impossible
  ou une attribution qui dépasse la capacité provoque le refus atomique
  de création du personnage, sans kit partiel.
- L'arc court ne possède qu'une instance physique en `MainHand`,
  `OffHand` étant réservée logiquement (ARMEMENT01.7.2).
- `StartingEquipmentKitId` reste dans `FGridCharacterInventoryState` et
  est donc inclus dans l'état du personnage à sauvegarder.

## Limitations explicites

- **Pas encore d'écran de choix** : le kit par défaut est octroyé.
- Pas de modification de widgets UMG ni du flux de chargement New Game.
- Les variantes sont consultables et matérialisables depuis la couche C++ ;
  le choix utilisateur et sa transaction viendront dans ARMEMENT01.9.3.
- La saisie de kits en dur dans les assets reste possible et fait autorité.

## Validation ciblée

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ARMEMENT01.9.SixClassKits"
```

Quatre tests : `CatalogueAndOverride`, `ProductionNewGameGrants`,
`RangerHandsAndAlternative`, `NonProductionUnchanged`.

La validation UE5.5.4 doit être confirmée par le journal utilisateur.
