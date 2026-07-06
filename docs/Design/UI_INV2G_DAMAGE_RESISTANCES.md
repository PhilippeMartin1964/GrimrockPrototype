# UI-INV2G Damage Resistances

## Objet

UI-INV2G ajoute une base minimale pour les resistances de degats apportees par l'equipement.

Le systeme sert d'abord a porter et afficher des bonus simples issus des DataAssets d'items. Il ne modifie pas encore la resolution complete des degats, les deplacements, la lumiere ou les assets Unreal.

## Types de degats supportes

`EGridDamageType` expose les types suivants :

- `Physical` ;
- `Fire` ;
- `Ice` ;
- `Lightning` ;
- `Poison` ;
- `Holy` ;
- `Necrotic` ;
- `Arcane`.

## Resistances

`FGridDamageResistanceSet` contient une valeur entiere par type :

```text
PhysicalResistance
FireResistance
IceResistance
LightningResistance
PoisonResistance
HolyResistance
NecroticResistance
ArcaneResistance
```

Ces valeurs sont des bonus numeriques simples. Leur interpretation exacte en mitigation de degats reste a definir dans une etape de combat ulterieure.

## ArmorBonus et PhysicalArmor

`ArmorBonus` dans `FGridEquipmentStatBonus` continue d'alimenter `FRPGDerivedStats::PhysicalArmor`.

`PhysicalResistance` est separee de `PhysicalArmor`. Elle represente une resistance de type degat, pas une valeur d'armure derivee. Les deux notions restent distinctes pour permettre plus tard des regles de mitigation differentes.

## DataAssets

`UGridItemDefinitionAsset::EquipmentResistanceBonus` porte les resistances apportees par un item equipe.

Exemples :

```text
Anneau ignifuge
CompatibleEquipmentSlots = Ring1, Ring2
EquipmentResistanceBonus.FireResistance = 5

Cape du venin
CompatibleEquipmentSlots = Cloak
EquipmentResistanceBonus.PoisonResistance = 10

Amulette arcanique
CompatibleEquipmentSlots = Amulet
EquipmentResistanceBonus.ArcaneResistance = 8
```

Codex ne modifie aucun DataAsset dans cette etape. Les valeurs seront renseignees manuellement dans Unreal Editor.

## Calcul

`UGridPartyInventoryComponent::ComputeCharacterEquipmentResistances()` :

- parcourt l'equipement actif du personnage ;
- ignore les slots vides ;
- resout la definition d'item ;
- additionne `EquipmentResistanceBonus` ;
- retourne le total.

`FGridInventoryCharacterSummary` expose :

- `EquipmentResistances` ;
- `FinalResistances`.

Pour l'instant, il n'existe pas de resistances de base de personnage. Donc :

```text
FinalResistances = EquipmentResistances
```

## Affichage

`UGridInventoryWidget` prepare des `BindWidgetOptional` :

```text
Text_ResistanceFire
Text_ResistanceIce
Text_ResistanceLightning
Text_ResistancePoison
Text_ResistanceHoly
Text_ResistanceNecrotic
Text_ResistanceArcane
```

Si ces TextBlocks existent dans `WBP_GridInventory`, `RefreshSelectedCharacterDetails()` affiche les valeurs de `FinalResistances`. Si les widgets n'existent pas, rien ne se passe.

Aucun WBP n'est modifie dans UI-INV2G-A.

## Diagnostic

`UGridPartyInventoryComponent::LogSelectedCharacterResistanceDiagnostics()` logue manuellement :

- `CharacterIndex` ;
- le total `Physical`, `Fire`, `Ice`, `Lightning`, `Poison`, `Holy`, `Necrotic`, `Arcane` ;
- chaque item equipe qui apporte au moins une resistance.

Le diagnostic n'est pas appele automatiquement et ne s'execute pas en `Tick`.

## Travail manuel UE5 futur

- Ajouter les TextBlocks de resistance dans `WBP_GridInventory`.
- Ajouter les icones Feu, Glace, Foudre, Poison, Sacre, Necrotique et Arcane.
- Renseigner `EquipmentResistanceBonus` dans les DataAssets d'equipement.
- Decider plus tard de la formule de mitigation utilisee par le combat.
