# GrimrockPrototype — MON18.1 — Spell Data Model & Cast Contract

Statut : **VALIDÉ ET CLOS sous UE5.5.4**  
Date de validation : **21 août 2026**

---

## 1. Objectif

MON18.1 pose le contrat de données générique du système de magie sans introduire de seconde pile parallèle pour les actions, le ciblage, les ressources, les effets de statut, les projectiles, la hotbar ou la persistance.

Le jalon définit :

- l'identité stable d'un sort ;
- ses métadonnées de gameplay ;
- son coût en mana et PA ;
- ses contraintes de portée et de LOS ;
- sa politique de ciblage ;
- son cooldown ;
- ses effets déclaratifs ;
- le contrat sérialisable d'une requête de lancement.

MON18.1 ne résout aucun sort et ne consomme aucune ressource. Ces responsabilités sont reportées aux sous-jalons runtime de MON18.

---

## 2. Réutilisation des systèmes existants

MON18.1 réutilise explicitement les contrats existants :

- `EGridCombatTargetingPolicy` pour le ciblage ;
- les PA et la mana déjà exposés par le catalogue d'actions MON12 ;
- `EGridCombatActionSourcePolicy::Spell` pour l'identité d'origine d'une action ;
- `StatusEffectId` comme pont vers MON16 ;
- le pipeline projectile/presentation déjà validé avec MON17 ;
- les identifiants stables (`FGuid`, `FName`) pour préparer Spellbook, hotbar et Save/Continue.

Aucune mutation de PA/mana, aucune recherche de cible et aucune application d'effet n'est effectuée par le contrat de données.

---

## 3. Types introduits

### `EGridSpellSchool`

Vocabulaire initial des écoles de magie.

### `EGridSpellEffectType`

Types d'effets déclaratifs initiaux :

```text
Damage
Heal
ApplyStatusEffect
RemoveStatusEffect
```

Les effets d'état référencent uniquement un `StatusEffectId`. Le système MON16 reste l'autorité sur la durée, le stacking, DoT, Haste/Slow, Stun/Silence/Immobilize et la persistance de l'effet.

### `FGridSpellEffectDefinition`

Décrit un effet élémentaire d'un sort.

### `FGridSpellDefinition`

Contient le contrat de gameplay d'un sort :

```text
SpellId
DisplayName
Description
School
ManaCost
ActionPointCost
MinRangeCells
MaxRangeCells
bRequiresLineOfSight
TargetingPolicy
CooldownRounds
Effects[]
```

### `FGridSpellTarget`

Cible purement data-driven, sans pointeur d'acteur.

### `FGridSpellCastRequest`

Requête de lancement sérialisable :

```text
CasterCharacterId
SpellId
Target
```

### `FGridSpellContract`

Service pur de validation structurelle du contrat. Il ne possède aucun état runtime et ne consomme aucune ressource.

---

## 4. Asset data-driven

`UGridSpellDefinitionAsset : UPrimaryDataAsset` expose `FGridSpellDefinition` dans l'éditeur et suit le pattern déjà utilisé par les autres définitions data-driven du projet.

Aucun DataAsset de production n'est requis pour valider MON18.1 ; les premiers sorts de production arriveront en MON18.5.

---

## 5. Invariants verrouillés

MON18.1 impose les invariants suivants :

1. `SpellId` est obligatoire et constitue l'identité stable du sort.
2. Les coûts de mana et de PA ne peuvent pas être négatifs.
3. Les portées ne peuvent pas être négatives et `MinRangeCells <= MaxRangeCells`.
4. Le cooldown ne peut pas être négatif.
5. Un sort doit déclarer une politique de ciblage cohérente.
6. Les effets d'état doivent référencer un `StatusEffectId` valide.
7. Les requêtes de lancement doivent fournir un `CasterCharacterId` valide et un `SpellId` valide.
8. Le contrat reste pur : aucune consommation de ressource, aucun accès monde, aucun acteur requis.

---

## 6. Validation automatisée UE5.5.4

Commande exécutée :

```text
Automation RunTests Grimrock.Magic.MON18.1
```

Résultat fourni le 21 août 2026 :

```text
Grimrock.Magic.MON18.1.CastRequestValidation   Success
Grimrock.Magic.MON18.1.ContractIsPure          Success
Grimrock.Magic.MON18.1.DefinitionValidation    Success
Grimrock.Magic.MON18.1.StatusEffectBridge      Success

Total                                           4/4 Success
```

---

## 7. Conclusion

**MON18.1 — Spell Data Model & Cast Contract est VALIDÉ ET CLOS sous UE5.5.4.**

Prochain travail autoritaire :

```text
MON18.2 — Spell Knowledge / Spellbook
```
