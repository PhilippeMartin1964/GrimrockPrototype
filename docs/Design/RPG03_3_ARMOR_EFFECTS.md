# RPG03.3 — Armor Effects (C3)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : **RPG03.2 validé 7/7**  
Statut : **VALIDÉ — TD04.2 / Grimrock.RPG.RPG03.3 / 8 réussis / 0 warning / 0 échec / 5 octobre 2026**

> **Addendum ARMEMENT01.10.3.2 (23.10.2026)** — Ce document conserve l'historique de RPG03.3. Pour l'autorité **courante** de l'armure d'équipement épuisable, consulter [le contrat des boucliers](../Rules/RPG_Shields_Protection_Contract_v0_1.md) et [ARMEMENT01.10.3.1](ARMEMENT01_10_3_1_EQUIPMENT_PHYSICAL_ARMOR.md). La régression `Grimrock.RPG.RPG03.3` a été rejouée le 23.10.2026 : **8/8**, sans avertissement ni échec. La projection historique sans consommation de bonus d'équipement n'est plus la règle complète en vigueur.

## Objectif

RPG03.3 ajoute une primitive générique et data-driven pour modifier directement les pools `PhysicalArmor` et `MagicalArmor` déjà existants.

Aucune seconde armure n'est créée. Les autorités durables restent :

```text
FGridCharacterInventoryState::Resources
    CurrentPhysicalArmor
    CurrentMagicalArmor

AGridMonsterActor
    CurrentPhysicalArmor
    CurrentMagicalArmor
```

Le contrat TD07.3.3.3 `DerivedStats / mutable Resources` reste inchangé.

## Profil C3

`FGridCombatArmorEffectProfile` contient :

- `Pool = Physical | Magical` ;
- `Operation = Restore | Damage` ;
- `Magnitude = Flat | ReferencePercent | RawDamagePercent` ;
- `Trigger = AfterResolution | AfterSuccessfulHit` ;
- `Amount` ;
- scaling Flat optionnel :
  - `ScalingAttribute`,
  - `AttributeModifierScale`,
  - `ScalingSkillId`,
  - `SkillRankScale`.

Une magnitude Flat peut donc exprimer directement, en utilisant les attributs effectifs projetés du personnage (bonus d'équipement compris) et les rangs de Skill durables :

```text
Amount
+ AttributeModifierScale × floor((Attribute - 10) / 2)
+ SkillRankScale × SkillRank
```

avec un résultat minimal de 1 lorsque le profil est valide.

Exemple Bouclier arcanique :

```text
Amount                 = 6
ScalingAttribute       = Intelligence
AttributeModifierScale = 1
ScalingSkillId         = Skill_Arcana
SkillRankScale         = 1

→ 6 + INT mod + Arcana Rank
```

## Pool de référence

La restauration n'utilise pas un maximum sauvegardé supplémentaire.

Pour un personnage, RPG03.3 reconstruit la référence depuis les autorités déjà présentes :

```text
Physical reference =
    Class.BasePhysicalArmor
    + EquipmentStatBonus.ArmorBonus

Magical reference =
    Class.BaseMagicalArmor
```

Puis les modificateurs C2 de référence sont appliqués.

**Autorité actuelle après ARMEMENT01.10.3.1 :** le pool de référence maximal conserve `EquipmentStatBonus.ArmorBonus`, tandis que le pool courant tient compte de la **protection déjà absorbée** par chaque objet. `GetCharacterSummary()` lit `GetEffectiveCurrentPhysicalArmor()`, utilisé aussi pour le combat. `ApplyPhysicalArmorDamage()` consomme d'abord les contributions restantes des équipements puis la réserve de base ; `RestorePhysicalArmor()` restaure prioritairement les réserves d'objets épuisées, dans la limite du montant permis par l'effet de restauration.

La protection consommée est sérialisée dans `FGridItemInstance::AbsorbedEquipmentPhysicalArmor` ; cela ne crée **aucun maximum global d'armure supplémentaire**, mais étend bien l'état persisté des objets par rapport au texte historique de RPG03.3. Sa relecture effective depuis une sauvegarde disque reste à vérifier en PIE.

Pour un monstre, la référence est directement :

```text
MonsterDefinition.PhysicalArmor
MonsterDefinition.MagicalArmor
```

## Modificateurs C2 associés

`FGridCombatModifierProfile` ajoute :

```text
PhysicalArmorReferencePercentModifier
MagicalArmorReferencePercentModifier
PhysicalArmorRestorationPercentModifier
MagicalArmorRestorationPercentModifier
```

Exemples RPG02 :

```text
Rempart
PhysicalArmorReferencePercentModifier = +25

Corroded
PhysicalArmorRestorationPercentModifier = -20
```

Ces valeurs utilisent le même agrégateur C2 que Accuracy, Evasion, résistances, dégâts et coûts.

## Restauration

Les profils `Restore` :

1. calculent leur magnitude ;
2. appliquent le modificateur de restauration reçue ;
3. clampent le résultat au pool de référence.

Exemple :

```text
Reference MagicalArmor = 10
Current MagicalArmor   = 8
Restore                = 6

→ Applied = 2
→ Current = 10
```

Une action Self `ResolutionProfile=Effect` peut désormais être **armor-only** : aucun soin, aucune mana restaurée et aucun Status ne sont requis si un C3 peut réellement modifier le pool.

À armure déjà pleine, le catalogue retourne `NoApplicableEffect`.

## Dégâts directs d'armure

Pour une attaque, les profils C3 `Damage` utilisent `AfterSuccessfulHit`.

Ordre autoritaire :

```text
FGridCombatResolver::ResolveAttack()
        ↓
dégâts primaires
        ↓
armure restante
        ↓
C3 ArmorEffects
        ↓
même FGridAttackResult
        ↓
CombatLog
        ↓
ApplyAttackResult()
        ↓
C1 ArmorGate
```

Le C3 ajoute uniquement à :

```text
PhysicalArmorDamage
ou
MagicalArmorDamage
```

et ne modifie jamais `HealthDamage`.

### Brise-armure / Tir perforant

```text
Magnitude = RawDamagePercent
Amount    = 50
Pool      = Physical
Operation = Damage
```

Le bonus vaut 50 % du `RawDamage`, mais seulement jusqu'à concurrence de l'armure encore présente après les dégâts primaires.

L'excédent est perdu ; il ne déborde jamais sur les PV.

## Interaction avec C1

Comme le C3 enrichit le même `FGridAttackResult`, l'ArmorGate C1 voit l'état final correct.

Exemple :

```text
PhysicalArmor avant = 5
dégâts primaires armure = 3
C3 direct armor damage = 2
PhysicalArmor après = 0

→ PhysicalArmorDepleted = vrai
→ le Status C1 peut être appliqué
```

Il n'existe donc aucun ordre parallèle `damage → status → armor effect`.

## Couverture directe des besoins RPG02

RPG03.3 fournit les primitives nécessaires pour :

- Rempart : modification % du pool physique de référence ;
- Forteresse : restauration % du pool physique de référence ;
- Brise-armure : dégâts physiques d'armure = % RawDamage ;
- Tir perforant : même primitive ;
- Bouclier arcanique : Flat + INT mod + Arcana Rank ;
- Miracle : restauration % du pool magique de référence ;
- Égide : Flat + WIS mod + Religion Rank ;
- Bastion divin : restauration % du pool magique de référence ;
- Élixir défensif : restauration Flat ;
- Panacée : restauration Flat ;
- Flasque acide : dégâts Flat + Alchemy Rank ;
- Corroded : réduction C2 des restaurations physiques reçues.

## Limites volontaires

RPG03.3 ne remplace pas les autres dépendances RPG02 :

- `C8` reste nécessaire pour Self/Ally R3, Front-row party, All living party et autres batchs ;
- un Effect hostile ciblé tel que Flasque acide nécessite encore le chemin de ciblage C8, même si sa magnitude C3 est désormais représentable ;
- `C7` reste nécessaire pour recettes et variantes de QuickItems ;
- `C4` reste nécessaire pour réactions/triggers ;
- aucune donnée des 90 Talents n'est injectée à l'aveugle dans des `.uasset`.

## Automation ajoutée

Filtre :

```text
Grimrock.RPG.RPG03.3
```

Tests :

1. `ProfileValidation`
2. `FlatRestoreClamp`
3. `ReferenceAndRestorationModifiers`
4. `RawDamageNoOverflow`
5. `ScaledFlatMagnitude`
6. `ArmorGateAfterC3`
7. `ArmorOnlyCatalog`
8. `SelfActionExecution`

Validation utilisateur reçue le 5 octobre 2026 : 8 tests réussis, 0 warning, 0 échec, 0 not run, process exit code 0.
