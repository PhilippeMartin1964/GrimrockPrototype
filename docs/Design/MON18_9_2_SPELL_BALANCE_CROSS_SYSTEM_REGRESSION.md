# MON18.9.2 — Spell Balance & Cross-System Regression

## Statut

**VALIDÉ ET CLOS sous UE5.5.4 — 22 août 2026.**

Base d'implémentation :

```text
5da47790ea0a2f2c03c3bf0c2a09bf5e711b40d6
Implement MON18.9.2 spell balance regressions
```

## 1. Objectif

MON18.9.2 ne crée aucun nouveau système de magie. Il fige la première baseline de balance des quatre sorts de production et sécurise leurs interactions transactionnelles avec la hotbar, les PA/mana et MON16.

Baseline conservée et désormais couverte par Automation :

```text
Spell_ArcaneBolt    Damage 4              Mana 3  PA 2  portée 1..5  cooldown 0
Spell_LesserHeal    Heal 5                Mana 4  PA 2  portée 0..3  cooldown 0
Spell_Haste         Apply Status_Haste    Mana 5  PA 2  portée 0..3  cooldown 0
Spell_CurePoison    Remove Status_Poison  Mana 4  PA 2  portée 0..3  cooldown 0
```

Les valeurs ne sont pas modifiées arbitrairement avant playtest. Toute future modification de balance devra être intentionnelle et mettre à jour ce contrat.

## 2. Contrat transactionnel

Un sort qui ne peut produire aucune mutation utile sur sa cible est rejeté avant commit runtime.

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

Le resolver MON18.5 reste déterministe. La décision de ne pas payer un sort inutile appartient à `FGridSpellHotbarExecutionService`, après résolution sur copies et avant publication du résultat autoritaire.

`EGridSpellEffectResolutionRejectReason` contient désormais `NoEffectWouldApply` et `FGridSpellEffectResolutionResult::DidMutateTarget()` centralise le test de mutation utile.

## 3. Cas utiles conservés

```text
Cure Poison + Status_Poison présent
    -> poison retiré
    -> 2 PA + 4 mana débités

Haste + définition MON16 valide
    -> Status_Haste appliqué/réappliqué
    -> 2 PA + 5 mana débités
```

Arcane Bolt et Lesser Heal utiles conservent leur comportement précédent.

## 4. Validation UE5.5.4

Campagne fournie par l'utilisateur le 22 août 2026.

### MON18.9.2 ciblé

```text
Grimrock.Magic.MON18.9.2.BalanceContract                 Success
Grimrock.Magic.MON18.9.2.CurePoisonCleanTargetNoCost     Success
Grimrock.Magic.MON18.9.2.CurePoisonCommit                Success
Grimrock.Magic.MON18.9.2.FullHealthHealNoCost            Success
Grimrock.Magic.MON18.9.2.HasteCommit                     Success
```

Bilan : **5/5 Success**.

### Régressions cross-system

```text
Grimrock.UI.UI01.4.3e.2      6/6 Success
Grimrock.Magic.MON18.5       6/6 Success
Grimrock.Magic.MON18.8      12/12 Success
Grimrock.RPG.MON16.4        11/11 Success
Grimrock.RPG.MON16.7        11/11 Success
```

Bilan cumulé de la campagne demandée : **51/51 Success**.

Les résultats confirment notamment :

- l'exécution hotbar d'Arcane Bolt et Lesser Heal ;
- l'absence de coût sur les échecs transactionnels ;
- la résolution Damage/Heal/ApplyStatus/RemoveStatus de MON18.5 ;
- la persistance Spellbook v6 et les protections SAVEFIX.1 ;
- le réordonnancement d'initiative Haste/Slow de MON16.4 ;
- la persistance et migration des Status Effects de MON16.7.

## 5. Fichiers

Modifiés par l'implémentation :

```text
Source/GrimrockPrototype/Public/Magic/GridSpellEffectResolver.h
Source/GrimrockPrototype/Private/Magic/GridSpellHotbarExecution.cpp
```

Ajoutés :

```text
Source/GrimrockPrototype/Private/Tests/GridMagicMON1892SpellBalanceRegressionTests.cpp
docs/Design/MON18_9_2_SPELL_BALANCE_CROSS_SYSTEM_REGRESSION.md
```

Aucun `.uasset`, `.umap`, SaveGame version bump ou système parallèle.

## 6. Contrat gelé

À partir de cette clôture :

- les quatre valeurs de production constituent la baseline de balance initiale ;
- un cast totalement sans effet utile ne doit jamais consommer PA/mana ;
- les sorts de statut réutilisent MON16 ;
- les bindings restent ceux de la hotbar MON12 ;
- la persistance Spellbook reste celle de MON18.8 ;
- tout changement ultérieur de coûts, portée, cooldown ou magnitude doit être explicite et testé.

Prochaine étape : **MON18.9.3 — diagnostics résiduels, campagne globale `Grimrock` et préparation de la clôture de MON18.**
