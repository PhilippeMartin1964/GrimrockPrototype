# RPG03.7 — Alchemy / QuickItem Modifiers (C7)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : **RPG03.6 validé 8/8**  
Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

RPG03.7 complète le contrat générique nécessaire aux consommables d'Alchimiste sans créer un moteur d'objets parallèle.

Les QuickItems restent des `UGridItemDefinitionAsset` réels :

- l'inventaire reste l'autorité de quantité ;
- l'action consomme `SourceItemQuantityCost` ;
- le catalogue et le TurnManager restent les seules voies d'exécution combat ;
- aucune logique de production ne compare un `TalentId`.

## 1. Sémantique par tags de source

Les `ItemTags` d'un item QuickItem sont désormais projetés dans :

```text
FGridCombatActionDefinition::SourceTags
```

Les profils C2 peuvent filtrer avec :

```text
RequiredSourceTags
```

Tous les tags requis doivent être présents.

Convention d'authoring prévue, sans hard-code runtime :

```text
QuickItem.Alchemy
QuickItem.Bomb
QuickItem.Potion
QuickItem.Potion.Positive
QuickItem.Flask
QuickItem.Catalyst
```

Les variantes peuvent ajouter des tags spécialisés, par exemple un type élémentaire. Le code ne compare aucun de ces noms pour décider d'un effet ; ils sont uniquement des identités de données consommées par les filtres C2.

## 2. QuickItem explicite, pas inféré depuis ItemType

Avant C7, `BuildQuickItemCombatActionDefinition()` limitait les QuickItems aux types `Potion` et `Scroll`.

C7 retire cette inférence.

La seule autorité devient :

```text
bProvidesQuickItemCombatAction = true
```

Cela permet à une bombe, une flasque, un catalyseur ou un futur consommable d'utiliser le même pipeline, quel que soit son `EGridItemType`.

Le builder normalise toujours :

```text
ActionId = Use_<ItemDefinitionId>
SourcePolicy = QuickItem
SourceItemQuantityCost >= 1
SourceTags += ItemTags
```

Les tags sont dédupliqués et triés.

## 3. Authoring Cell / Area préparé pour C8

Le builder n'interdit plus un QuickItem valide uniquement parce qu'il cible :

```text
Cell
Area
Ally
```

L'asset peut donc déjà représenter Fire Bomb, Toxic Bomb, Oil Slick ou Corrosive Cloud.

L'autorité d'exécution reste toutefois le catalogue :

- les formes déjà exécutables restent disponibles ;
- une forme qui nécessite encore C8 reste `ExecutionNotImplemented` ;
- aucun exécuteur Alchimiste temporaire n'est ajouté.

## 4. Scaling par rang de compétence

`FGridCombatQuickItemScalingProfile` fournit :

```text
ScalingSkillId
DirectDamageSkillRankScale
RestoreHealthSkillRankScale
RestoreManaSkillRankScale
```

`FGridQuickItemResolver` utilise les `FRPGSkillRank` durables du personnage.

Exemples RPG02 représentables :

```text
Fire Bomb
base direct damage = 6
ScalingSkillId = Skill_Alchemy
DirectDamageSkillRankScale = 1
→ 6 + Alchemy Rank
```

```text
Corrosive Cloud
base direct damage = 4
DirectDamageSkillRankScale = 1
→ 4 + Alchemy Rank
```

```text
Panacea
RestoreHealth = 10
RestoreHealthSkillRankScale = 2
→ 10 + 2 × Alchemy Rank
```

Ce scaling est appliqué avant les multiplicateurs de dégâts C2 et avant le bonus positif de potion.

## 5. Potion renforcée

C2 gagne :

```text
PositiveEffectPercentModifier
```

Un profil filtré par :

```text
SourcePolicy = QuickItem
RequiredSourceTags = QuickItem.Potion.Positive
PositiveEffectPercentModifier = +25
```

représente **Potion renforcée**.

Le bonus s'applique à :

- restauration Health ;
- restauration Mana ;
- restauration PhysicalArmor ;
- restauration MagicalArmor.

Il s'applique après le scaling de compétence.

Il ne modifie pas :

- les dégâts offensifs ;
- les DoT ;
- les surfaces ;
- la durée des Status Effects.

Ainsi le contrat RPG02 « magnitude +25 %, durée inchangée » reste explicite.

## 6. Grenadier

Les primitives existantes C2 deviennent filtrables par tag de source.

### Charge précise

```text
RequiredSourceTags = QuickItem.Bomb
RangeCellsModifier = +1
FriendlyDirectDamagePercentModifier = -50
```

Le premier effet est déjà projeté dans l'action.

`FriendlyDirectDamagePercentModifier` est volontairement un hook C7+C8 : C8 l'appliquera uniquement aux alliés couverts par une bombe Area. Les surfaces et Status ne seront pas réduits.

### Maître grenadier

```text
RequiredSourceTags = QuickItem.Bomb
ActionPointCostModifier = -1
OutgoingDamagePercentModifier = +20
```

Le coût d'item n'est jamais modifié.

Pour une action Attack, le clamp existant conserve un coût minimal de 1 PA.

## 7. Diffusion

C2/C7 expose :

```text
QuickItemSecondaryTargetCount
QuickItemSecondaryMagnitudePercent
QuickItemSecondaryDurationPercent
```

Le contrat RPG02 devient :

```text
RequiredSourceTags = QuickItem.Potion.Positive
QuickItemSecondaryTargetCount = 1
QuickItemSecondaryMagnitudePercent = 50
QuickItemSecondaryDurationPercent = 50
```

`FGridQuickItemResolver::ResolveSecondaryEffect()` produit cette projection.

La sélection du second allié et l'application batch attendent C8. Aucun item supplémentaire ne sera consommé : la transaction source reste celle de l'action primaire.

## 8. Antidote, Élixir défensif et Panacée

C7 rend leurs **items et actions** représentables sans format spécial :

- Antidote : QuickItem réel, coût 1 item ;
- Élixir défensif : quatre ItemDefinitions/variantes data-driven possibles, avec C3 pour +4 MagicalArmor et C1/C2 pour la résistance ;
- Panacée : QuickItem réel, soin `10 + 2×Alchemy Rank`, C3 pour +8 MagicalArmor.

Le retrait filtré de Debuffs et le ciblage Ally restent C8.

Aucune liste Poison/Burning/etc. n'est codée dans C7.

## 9. Recettes

Le projet ne possède pas encore de moteur d'ingrédients/recettes autoritaire.

RPG03.7 n'invente donc ni ingrédients ni coûts absents de RPG02.

Les identités déjà prévues :

```text
Recipe_Bomb_Fire
Recipe_Bomb_Toxic
Recipe_Antidote
Recipe_Flask_Oil
Recipe_Panacea
...
```

peuvent être accordées par le mécanisme existant :

```text
FRPGClassProgressionChoiceDefinition::GrantedRequirementIds
```

Cela évite une seconde liste d'unlocks. Un futur écran d'artisanat pourra interroger ces RequirementIds et des définitions de recettes lorsqu'elles seront spécifiées.

## 10. Atomicité

Le pipeline existant reste conservé :

```text
préflight
→ vérification PA / mana / quantité
→ dépense PA
→ consommation d'une quantité d'item
→ application des effets
→ cooldown
```

Si la consommation échoue après réservation des PA, le TurnState est restauré.

Un effet Self sans bénéfice reste rejeté avant consommation via `NoApplicableEffect`.

C7 ne crée pas de chemin de consommation spécifique aux potions ou bombes.

## 11. Frontière avec C8

C7 fournit les données et projections requises pour :

- bombes Area ;
- réduction du friendly fire direct ;
- Diffusion ;
- Antidote/Panacée avec filtres de statuts ;
- Self/Ally ;
- batch multi-cible ;
- surfaces C6 issues d'actions Cell/Area.

C8 doit maintenant brancher ces primitives sur un **unique exécuteur Effect/Batch générique**.

## Automation ajoutée

Filtre :

```text
Grimrock.RPG.RPG03.7
```

Tests :

1. `SourceTagFiltering`
2. `QuickItemNormalization`
3. `AlchemyDamageScaling`
4. `PositivePotionScaling`
5. `GrenadierProjection`
6. `DiffusionProjection`
7. `CatalogSkillScaledEffect`
8. `ModifierValidation`

La validation n'est considérée réussie qu'après exécution du harness UE5.5.4 et fourniture de la sortie utilisateur.
