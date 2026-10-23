# UI-INV2F Equipment Stat Bonuses

## Objet

UI-INV2F ajoute un premier systeme minimal de bonus d'equipement pour le paper doll.

> **Mise à jour ARMEMENT01.10.3.2 (23.10.2026)** — Depuis ARMEMENT01.10.3.1, la projection d'armure physique tient compte de l'absorption déjà subie par chaque équipement. Voir [le contrat des boucliers](../Rules/RPG_Shields_Protection_Contract_v0_1.md). Les autres bonus d'équipement restent inchangés.

Les bonus sont portes par `UGridItemDefinitionAsset::EquipmentStatBonus` et sont additionnes depuis les items actuellement equipes sur le personnage affiche. Les DataAssets seront renseignes manuellement dans Unreal Editor lors des etapes de contenu suivantes.

## Stats de base et stats finales

Les stats de base du personnage restent stockees dans `FGridCharacterInventoryState` :

- `Attributes` ;
- `DerivedStats` ;
- `MaxCarryWeight` ;
- `CurrentWeight`.

Les bonus d'equipement ne modifient pas directement les valeurs de base. Le résumé `FGridInventoryCharacterSummary` expose les valeurs effectives. Pour l'armure physique, la quantité déjà absorbée est conservée sur `FGridItemInstance::AbsorbedEquipmentPhysicalArmor`, y compris après retrait et rééquipement.

Le resume final inclut :

- attributs finaux = attributs de base + bonus d'equipement ;
- `MaxHealth` final = `DerivedStats.MaxHealth` + `MaxHealthBonus` ;
- `MaxMana` final = `DerivedStats.MaxMana` + `MaxManaBonus` ;
- `CurrentPhysicalArmor` effectif = `Resources.CurrentPhysicalArmor` durable + somme des contributions d'armure physique **encore disponibles** sur les objets équipés (`ArmorBonus - AbsorbedEquipmentPhysicalArmor` pour chaque bonus positif) ;
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

L'armure physique est projetée via le binding canonique `Text_CharacterPhysicalArmor` de `WBP_CharacterSheet`. L'ancien alias `Text_CharacterArmor` a été supprimé par UI-CLEAN03.

## Limites actuelles

- Les resistances de degats sont traitees separement dans `docs/Design/UI_INV2G_DAMAGE_RESISTANCES.md`.
- Pas de bonus conditionnels.
- Pas de recalcul de `DerivedStats` complet depuis les attributs finaux.
- Pas de modification persistante des stats de base.
- Pas d'effet automatique sur le deplacement ou le combat hors valeurs deja exposees au resume.
