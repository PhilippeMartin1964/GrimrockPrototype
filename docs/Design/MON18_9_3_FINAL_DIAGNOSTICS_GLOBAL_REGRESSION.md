# MON18.9.3 — Final Diagnostics / Global Regression / Closure

## Statut

**IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE.**

Base :

```text
9e6fe3971a48a870066fb4898325b300609de497
Close MON18.9.2 spell balance validation
```

## 1. Objectif

MON18.9.3 est la dernière passe de stabilisation avant la clôture du jalon majeur MON18 — Magic & Spellbook.

Aucun nouveau gameplay n'est introduit. Le sous-jalon se limite à :

- rendre les diagnostics de slots SaveGame résiduels attribuables à un slot précis ;
- garantir que le checkpoint pré-combat `_AutoCombat` ne devient pas un slot de chargement normal ;
- lancer une campagne Automation globale `Grimrock` ;
- corriger uniquement d'éventuelles régressions réellement observées ;
- effectuer une courte validation PIE finale avant clôture de MON18.

## 2. Diagnostic des anciens slots

Le menu principal sonde les slots configurés :

```text
GrimrockParty
GrimrockParty_2
GrimrockParty_3
```

`UGrimrockPartySaveGame::Serialize()` peut détecter et journaliser une migration/restauration invalide, mais il ne connaît pas le nom du slot passé à `UGameplayStatics::LoadGameFromSlot()`.

MON18.9.3 complète donc l'observabilité dans `UGrimrockGameInstance::HasPartySaveGame()` : lorsqu'un slot existant n'est pas chargeable, un diagnostic complémentaire inclut :

```text
[MON18.9.3] SlotProbe
Slot=<nom>
UserIndex=<index>
Result=Rejected
Reason=<raison>
```

Raisons distinguées :

```text
LoadFailedOrWrongClass
IncompatibleSave
PartyInventoryStateNotLoadable
```

Aucun ancien slot n'est supprimé, migré de force ou écrasé par cette passe. Le diagnostic sert uniquement à identifier clairement le slot responsable.

## 3. Isolation du checkpoint pré-combat

Le checkpoint MON18.9.1 reste :

```text
<slot courant>_AutoCombat
```

Il ne doit pas être ajouté à `ConfiguredPartySaveSlotNames` et ne doit donc pas apparaître comme une sauvegarde manuelle normale dans le menu de chargement.

## 4. Automation MON18.9.3

Namespace :

```text
Grimrock.Magic.MON18.9.3
```

Tests :

```text
SaveSlotDiagnostics
CheckpointIsolation
```

Attendu : **2/2 Success**.

## 5. Campagne globale

Après succès des deux tests ciblés :

```text
Automation RunTests Grimrock
```

Cette campagne est l'autorité de non-régression finale du projet avant clôture de MON18.

Toute occurrence de :

```text
Result={Fail}
Error:
Ensure condition failed
Assertion failed
```

sera analysée. Seules les régressions réellement observées seront corrigées ; aucun refactor préventif n'est autorisé dans MON18.9.3.

## 6. Validation PIE finale après campagne verte

Scénario court :

1. New Game avec Mage.
2. `Grimrock.Spellbook.SeedProduction`.
3. Vérifier les quatre sorts dans Spellbook.
4. Affecter au moins Arcane Bolt et Lesser Heal à la hotbar.
5. Déclencher un combat automatique et constater le checkpoint pré-combat.
6. Exécuter Arcane Bolt ou Lesser Heal selon la situation.
7. Vérifier qu'une sauvegarde régulière est refusée pendant le combat.
8. Sauvegarder hors combat dans un scénario stable.
9. Stop PIE puis `Continue`.
10. Vérifier Spellbook et bindings hotbar restaurés.
11. Relancer `Grimrock.Spellbook.SeedProduction` et obtenir `Added=0 AlreadyKnown=4`.

## 7. Critères de clôture MON18

MON18 pourra être déclaré **VALIDÉ ET CLOS** uniquement lorsque :

```text
Grimrock.Magic.MON18.9.3     2/2 Success
Automation RunTests Grimrock campagne globale sans Fail
PIE final                    validé par l'utilisateur
```

Ensuite seulement seront mis à jour le project overview, la roadmap et le document de clôture de MON18.
