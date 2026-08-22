# MON18.9.2 — Spell Balance & Cross-System Regression

## Statut

**IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE.**

Base :

```text
a315672c18999f2d413fd9077ad25866f2e4186d
Close MON18.9.1 combat save policy validation
```

## 1. Objectif

MON18.9.2 ne crée aucun nouveau système de magie. Il fige la première baseline de balance des quatre sorts de production et sécurise leurs interactions transactionnelles avec la hotbar, les PA/mana et MON16.

Baseline conservée :

```text
Spell_ArcaneBolt   Damage 4            Mana 3  PA 2  portée 1..5  cooldown 0
Spell_LesserHeal   Heal 5              Mana 4  PA 2  portée 0..3  cooldown 0
Spell_Haste        Apply Status_Haste  Mana 5  PA 2  portée 0..3  cooldown 0
Spell_CurePoison   Remove Status_Poison Mana 4 PA 2  portée 0..3  cooldown 0
```

Les valeurs ne sont pas modifiées arbitrairement avant playtest. Elles deviennent un contrat testé afin que toute future modification de balance soit volontaire et visible.

## 2. Règle transactionnelle ajoutée

Un sort qui ne peut produire aucune mutation utile sur sa cible est rejeté avant commit runtime.

Cas couverts :

```text
Lesser Heal sur cible à PV max
    -> NoEffectWouldApply
    -> 0 PA consommé
    -> 0 mana consommée

Cure Poison sur cible sans Status_Poison
    -> NoEffectWouldApply
    -> 0 PA consommé
    -> 0 mana consommée
```

Le resolver MON18.5 reste déterministe et peut toujours produire un résultat où tous les effets ont `bMutatedTarget=false`. La décision de ne pas payer un sort inutile appartient au niveau d'orchestration `FGridSpellHotbarExecutionService`, après résolution sur copies et avant publication du résultat autoritaire.

## 3. Nouveau contrat de résultat

`EGridSpellEffectResolutionRejectReason` reçoit :

```text
NoEffectWouldApply
```

`FGridSpellEffectResolutionResult::DidMutateTarget()` centralise la détection d'au moins une mutation utile dans le batch d'effets.

Cette règle conserve l'atomicité MON18.3/MON18.5 : les coûts calculés ne vivent encore que dans des copies locales lorsque le rejet survient.

## 4. Cas qui restent payants

Les casts utiles conservent le comportement existant :

```text
Cure Poison + Status_Poison présent
    -> poison retiré
    -> 2 PA + 4 mana débités

Haste + définition MON16 valide
    -> Status_Haste appliqué/réappliqué
    -> 2 PA + 5 mana débités
```

Arcane Bolt et Lesser Heal utiles ne changent pas de comportement.

## 5. Tests Automation

Namespace :

```text
Grimrock.Magic.MON18.9.2
```

Tests ajoutés :

```text
BalanceContract
FullHealthHealNoCost
CurePoisonCleanTargetNoCost
CurePoisonCommit
HasteCommit
```

Attendu : **5/5 Success**.

Régressions recommandées après succès ciblé :

```text
Grimrock.UI.UI01.4.3e.2
Grimrock.Magic.MON18.5
Grimrock.Magic.MON18.8
Grimrock.RPG.MON16.4
Grimrock.RPG.MON16.7
```

## 6. Fichiers

Modifiés :

```text
Source/GrimrockPrototype/Public/Magic/GridSpellEffectResolver.h
Source/GrimrockPrototype/Private/Magic/GridSpellHotbarExecution.cpp
```

Ajoutés :

```text
Source/GrimrockPrototype/Private/Tests/GridMagicMON1892SpellBalanceRegressionTests.cpp
docs/Design/MON18_9_2_SPELL_BALANCE_CROSS_SYSTEM_REGRESSION.md
```

Aucun `.uasset`, `.umap`, SaveGame version bump ou nouvelle abstraction parallèle.

## 7. Validation attendue

Après compilation UE5.5.4 :

```text
Automation RunTests Grimrock.Magic.MON18.9.2
```

Puis les régressions listées ci-dessus. La clôture de MON18.9.2 attend les résultats fournis par l'utilisateur.
