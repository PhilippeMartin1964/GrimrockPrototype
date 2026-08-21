# MON18.1 — Spell Data Model & Cast Contract

Statut : **IMPLÉMENTÉ — validation UE5.5.4 en attente du résultat utilisateur**  
Date : **21 août 2026**

---

## 1. Objectif

MON18.1 établit le contrat C++ minimal du futur système de magie sans implémenter encore le runtime de lancement de sort.

L'étape doit répondre à deux contraintes :

1. fournir une représentation data-driven et sérialisable d'un sort ;
2. réutiliser les systèmes existants plutôt que créer un second moteur de combat, de ciblage ou de Status Effects.

---

## 2. Audit des contrats existants réutilisés

### MON12 — actions / hotbar / ressources

Le catalogue de combat possède déjà :

- `EGridCombatActionSourcePolicy::Spell` ;
- `EGridCombatTargetingPolicy` avec `Self`, `Ally`, `FirstAxialTarget`, `Cell`, `Area` ;
- les raisons d'indisponibilité `InsufficientActionPoints`, `InsufficientMana`, `CooldownActive` ;
- un contexte de catalogue contenant `RemainingActionPoints`, `CurrentMana`, `MaximumMana` et les cooldowns.

`FGridCombatActionCatalog` est explicitement pur : il évalue les définitions et ressources mais ne résout aucun effet et ne consomme ni PA, ni mana, ni objet source.

MON18 reprend ce principe. Le coût d'un sort est une donnée ; le paiement transactionnel sera raccordé au runtime en MON18.3.

### MON16 — Status Effects

MON18.1 ne crée aucun état d'effet magique parallèle. Un effet de sort de type `ApplyStatusEffect` ou `RemoveStatusEffect` référence MON16 par un `StatusEffectId` stable.

L'application, le stacking, la durée, les ticks et la persistance restent sous l'autorité de MON16.

### MON17 — projectile

MON18.1 ne crée aucun projectile magique spécifique. Le contrat de projectile/présentation validé avec le Gobelin lanceur sera réutilisé lorsque MON18.6 abordera la présentation.

### MON15 / Save

Les identités persistantes utilisent des valeurs sérialisables (`FGuid`, `FName`, coordonnées de grille). Aucun pointeur d'acteur n'est stocké dans `FGridSpellCastRequest` ou `FGridSpellTarget`.

---

## 3. Nouveau contrat

Fichier :

```text
Source/GrimrockPrototype/Public/Magic/GridSpellTypes.h
```

### `EGridSpellSchool`

Vocabulaire initial volontairement léger :

```text
None
Arcane
Fire
Frost
Air
Earth
Life
Death
Protection
```

Ce vocabulaire est extensible et n'impose pas encore les écoles définitives du design de production.

### `EGridSpellEffectType`

MON18.1 définit uniquement des intentions déclaratives :

```text
Damage
Heal
ApplyStatusEffect
RemoveStatusEffect
```

Aucun de ces effets n'est exécuté dans MON18.1.

### `FGridSpellEffectDefinition`

Contient :

```text
Type
Magnitude
StatusEffectId
```

Règles structurelles :

- Damage / Heal : `Magnitude > 0` ;
- ApplyStatusEffect / RemoveStatusEffect : `StatusEffectId != None`.

### `FGridSpellDefinition`

Contient :

```text
SpellId
DisplayName
Description
School
ManaCost
ActionPointCost
MinRangeCells
MaxRangeCells
TargetingPolicy
bRequiresLineOfSight
CooldownRounds
Effects[]
```

`TargetingPolicy` réutilise directement `EGridCombatTargetingPolicy` du combat existant.

### `FGridSpellTarget`

Cible sérialisable sans pointeur d'acteur :

```text
TargetId
GridCell
bHasGridCell
```

`TargetId` est destiné aux membres du groupe / monstres identifiés de façon stable. `GridCell` sert aux sorts visant une cellule ou une zone.

### `FGridSpellCastRequest`

```text
CasterCharacterId
SpellId
Target
```

Ce contrat transporte l'intention de lancement. Il ne dépense aucune ressource et ne résout aucun effet.

### `FGridSpellContract`

Service pur de validation structurelle :

```text
ValidateDefinition()
ValidateRequest()
```

Il vérifie notamment :

- identité du sort ;
- nom ;
- coûts non négatifs ;
- portée cohérente ;
- politique de ciblage ;
- présence et validité des effets ;
- identité stable du lanceur ;
- présence d'une cible selon le `TargetingPolicy`.

---

## 4. Ce que MON18.1 ne fait volontairement pas

MON18.1 ne contient pas :

- paiement mana ;
- paiement PA ;
- vérification de la mana courante ;
- vérification du tour actif ;
- résolution réelle LOS / portée dans le monde ;
- dégâts ;
- soins ;
- application MON16 ;
- projectile ;
- VFX / audio ;
- Spellbook ;
- apprentissage ;
- hotbar runtime ;
- migration SaveGame ;
- Blueprint / DataAsset / WBP de production.

Ces responsabilités sont volontairement différées aux sous-jalons suivants.

---

## 5. Tests Automation ajoutés

Fichier :

```text
Source/GrimrockPrototype/Private/Tests/GridMagicMON181SpellContractTests.cpp
```

Tests :

```text
Grimrock.Magic.MON18.1.DefinitionValidation
Grimrock.Magic.MON18.1.StatusEffectBridge
Grimrock.Magic.MON18.1.CastRequestValidation
Grimrock.Magic.MON18.1.ContractIsPure
```

Ils verrouillent :

- définition valide / invalide ;
- identité obligatoire ;
- portée cohérente ;
- cible obligatoire ;
- effets obligatoires ;
- référence MON16 par `StatusEffectId` ;
- cible par identifiant stable ou coordonnées de grille ;
- absence de mutation des coûts/effets pendant la validation.

---

## 6. Trous architecturaux identifiés pour les étapes suivantes

L'audit fait ressortir plusieurs raccords à traiter sans créer de systèmes parallèles :

1. `EGridCombatActionSourcePolicy::Spell` existe, mais l'exécution runtime de cette source doit être branchée au TurnManager en MON18.3.
2. Le catalogue expose déjà mana, PA et cooldown ; MON18.3 doit réutiliser la transaction existante plutôt que payer directement depuis le modèle de sort.
3. `EGridCombatTargetingPolicy` fournit le vocabulaire de cible ; MON18.4 doit raccorder ses résolveurs aux règles de grille/LOS existantes.
4. MON16 fournit le runtime d'effets d'état ; le résolveur de sort devra seulement router `StatusEffectId` vers ce système.
5. Le pipeline projectile MON17 est une présentation ; les dégâts magiques devront rester dans la résolution de sort et non dans le projectile.
6. La connaissance des sorts par personnage n'existe pas encore comme contrat persistant : c'est le périmètre de MON18.2.

---

## 7. Porte de sortie MON18.1

MON18.1 pourra être déclaré **VALIDÉ ET CLOS** uniquement après :

1. compilation du projet sous UE5.5.4 ;
2. exécution des quatre tests `Grimrock.Magic.MON18.1` ;
3. résultat `Success` fourni par l'utilisateur.

Aucune compilation ou exécution UE n'est revendiquée dans ce document avant ce retour.

---

## 8. Suite prévue

Après validation MON18.1 :

```text
MON18.2 — Spell Knowledge / Spellbook
```

Cette étape introduira les sorts connus par personnage, apprentissage/anti-duplication, contraintes de classe/niveau et base de persistance, sans encore déplacer l'autorité du runtime de combat.
