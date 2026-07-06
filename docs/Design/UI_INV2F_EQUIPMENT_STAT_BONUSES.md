# UI-INV2F Equipment Stat Bonuses

## Objet

UI-INV2F ajoute un premier systeme minimal de bonus d'equipement pour le paper doll.

Les bonus sont portes par `UGridItemDefinitionAsset::EquipmentStatBonus` et sont additionnes depuis les items actuellement equipes sur le personnage affiche. Les DataAssets seront renseignes manuellement dans Unreal Editor lors des etapes de contenu suivantes.

## Stats de base et stats finales

Les stats de base du personnage restent stockees dans `FGridCharacterInventoryState` :

- `Attributes` ;
- `DerivedStats` ;
- `MaxCarryWeight` ;
- `CurrentWeight`.

Les bonus d'equipement ne modifient pas ces valeurs de base. Ils sont appliques au resume calcule pour l'affichage inventaire via `FGridInventoryCharacterSummary`.

Le resume final inclut :

- attributs finaux = attributs de base + bonus d'equipement ;
- `MaxHealth` final = `DerivedStats.MaxHealth` + `MaxHealthBonus` ;
- `MaxMana` final = `DerivedStats.MaxMana` + `MaxManaBonus` ;
- `PhysicalArmor` final = `DerivedStats.PhysicalArmor` + `ArmorBonus` ;
- charge maximale finale = `MaxCarryWeight` + `CarryWeightBonus`.

`CurrentHealth` et `CurrentMana` ne sont pas augmentes directement. Ils sont seulement limites au maximum final si necessaire.

## Structure de bonus

`FGridEquipmentStatBonus` contient actuellement :

```text
StrengthBonus
DexterityBonus
ConstitutionBonus
IntelligenceBonus
WisdomBonus
CharismaBonus
MaxHealthBonus
MaxManaBonus
CarryWeightBonus
ArmorBonus
```

Le systeme reste volontairement simple. Les resistances avancees, bonus elementaires, vitesse, critique ou effets conditionnels seront traites dans une etape ulterieure, par exemple UI-INV2G.

## Exemples de contenu

```text
Anneau de force
CompatibleEquipmentSlots = Ring1, Ring2
EquipmentStatBonus.StrengthBonus = 1

Bottes du voleur
CompatibleEquipmentSlots = Feet
EquipmentStatBonus.DexterityBonus = 1

Amulette vitale
CompatibleEquipmentSlots = Amulet
EquipmentStatBonus.MaxHealthBonus = 10

Ceinture de portage
CompatibleEquipmentSlots = Belt
EquipmentStatBonus.CarryWeightBonus = 15.0
```

## Travail manuel UE5

Pour chaque DataAsset d'equipement, verifier :

- `CompatibleEquipmentSlots` ;
- `EquipmentStatBonus` ;
- `ItemType` ;
- `DisplayName` en francais ;
- `Description` en francais ;
- `Weight` ;
- `Icon` ;
- `WorldMesh` ;
- `EquippedMesh` si l'item doit etre visible ou tenu.

Les items sans bonus peuvent conserver `EquipmentStatBonus` a zero.

## Diagnostics

`UGridPartyInventoryComponent::ComputeCharacterEquipmentStatBonus()` additionne les bonus d'un personnage.

`UGridPartyInventoryComponent::LogSelectedCharacterEquipmentStatBonusDiagnostics()` logue manuellement :

- l'index du personnage selectionne ;
- chaque item equipe qui donne au moins un bonus ;
- le total des bonus d'attributs ;
- le total `MaxHealth`, `MaxMana`, `CarryWeight` et `Armor`.

Ce diagnostic ne s'execute pas en `Tick` et ne modifie aucun DataAsset.

## UI-INV2F-C : affichage des bonus dans la fiche personnage

`FGridInventoryCharacterSummary` expose maintenant les valeurs de base, les valeurs finales et le total des bonus :

- `BaseAttributes` ;
- `BaseDerivedStats` ;
- `BaseMaxWeight` ;
- `EquipmentStatBonus` ;
- `Attributes` final ;
- `DerivedStats` final ;
- `MaxWeight` final.

La fiche personnage affiche les valeurs finales avec le bonus d'equipement entre parentheses :

```text
Force : 13 (+1)
Dexterite : 12 (+2)
PV : 35 / 45 (+10)
Mana : 20 / 25 (+5)
Poids : 12.5 / 65.0 (+15.0)
```

Il n'y a pas encore de `RichText`, de couleur ou de style specifique pour les bonus. L'objectif est d'abord de rendre la contribution de l'equipement visible et verifiable.

L'armure est prete cote C++ via le `BindWidgetOptional` `Text_CharacterArmor`. Aucun WBP n'est modifie dans cette etape. Le travail visuel manuel UE5 consistera a ajouter `Text_CharacterArmor` dans la section des stats derivees de `WBP_GridInventory` si l'armure doit etre affichee.

## Limites actuelles

- Pas de resistances avancees.
- Pas de bonus conditionnels.
- Pas de recalcul de `DerivedStats` complet depuis les attributs finaux.
- Pas de modification persistante des stats de base.
- Pas d'effet automatique sur le deplacement ou le combat hors valeurs deja exposees au resume.
