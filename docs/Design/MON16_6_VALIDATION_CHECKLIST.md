# MON16.6 — Validation Checklist

## État

```text
Implémentation C++ : préparée
Documentation      : préparée
Compilation UE5    : EN ATTENTE
Automation MON16.6 : EN ATTENTE
Régressions        : EN ATTENTE
Clôture            : NON
```

Base : `b153a5d48f709f5b86d8d125e7cd61daa095966b`.

## Architecture

- [x] projection read-only `FGridStatusEffectPresentationView`
- [x] builder pur `FGridStatusEffectPresentationBuilder`
- [x] aucune règle gameplay dans le HUD
- [x] aucune comparaison hard-coded d'EffectId
- [x] aucun second lifecycle
- [x] aucun second CombatLog/ring-buffer
- [x] TurnManager reste propriétaire de `CombatLogEntries`
- [x] aucun tick/polling UI ajouté
- [x] aucun WBP/.uasset/.umap modifié

## Présentation

- [x] nom / description projetés
- [x] icône optionnelle projetée
- [x] disposition Buff/Debuff/Neutral projetée
- [x] Turns/Rounds/Permanent formatés
- [x] stacks projetés
- [x] initiative projetée sans modifier la règle MON16.4
- [x] périodicité et contrôle exposés au tooltip
- [x] fallback valide si DefinitionAsset manque
- [x] ordre déterministe par EffectId

## HUD groupe

- [x] `FGridCombatActionPanelView::StatusEffects`
- [x] `StatusSummary`
- [x] `LatestStatusFeedback`
- [x] `Text_StatusEffects` optionnel
- [x] `Text_StatusFeedback` optionnel
- [x] fallback natif C++ si bindings absents
- [x] rafraîchissement via `NotifyPartyInventoryChanged`

## Feedback

- [x] `StatusApplied`
- [x] `StatusRefreshed`
- [x] `StatusTicked`
- [x] `StatusExpired`
- [x] payload groupe/monstre commun
- [x] DoT présenté depuis le `FGridAttackResult` MON16.3
- [x] Tick avant Expire
- [x] dernier feedback transitoire sans historique parallèle

## Compilation UE5.5.4

Attendu : 0 erreur C++, UHT ou link.

- [ ] compilation / chargement confirmé par log utilisateur

## Automation ciblée

Exécuter :

```text
Automation RunTests Grimrock.RPG.MON16.6
```

- [ ] `PresentationProjection` — Success
- [ ] `DurationFormatting` — Success
- [ ] `DeterministicProjection` — Success
- [ ] `FallbackProjection` — Success
- [ ] `PartyApplyFeedback` — Success
- [ ] `RefreshFeedback` — Success
- [ ] `TickAndExpirationFeedback` — Success
- [ ] `MonsterFeedbackParity` — Success
- [ ] `PartyPanelProjection` — Success
- [ ] `NoParallelSystem` — Success

Attendu : **10/10 Success**.

## Régressions minimales

Après MON16.6 vert :

```text
Automation RunTests Grimrock.RPG.MON16.5
Automation RunTests Grimrock.RPG.MON16.4
Automation RunTests Grimrock.RPG.MON16.3
Automation RunTests Grimrock.RPG.MON16.2
Automation RunTests Grimrock.RPG.MON16.1
Automation RunTests Grimrock.RPG.MON15
Automation RunTests Grimrock.Monsters.MON14
```

Attendus :

```text
MON16.5 : 11/11
MON16.4 : 11/11
MON16.3 : 11/11
MON16.2 : 10/10
MON16.1 : 7/7
MON15   : 42/42
MON14   : 19/19
```

- [ ] MON16.5 : 11/11 Success
- [ ] MON16.4 : 11/11 Success
- [ ] MON16.3 : 11/11 Success
- [ ] MON16.2 : 10/10 Success
- [ ] MON16.1 : 7/7 Success
- [ ] MON15 : 42/42 Success
- [ ] MON14 : 19/19 Success

## Clôture

MON16.6 sera marqué **VALIDÉ ET CLOS** uniquement après compilation/chargement UE5.5.4, 10/10 MON16.6 et régressions appropriées sans nouvel échec.

Prochaine étape : `MON16.7 — Save / Restore des status effects`.
