# MON16.6 — Validation Checklist

## État

```text
Implémentation C++ : VALIDÉE
Documentation      : VALIDÉE
Compilation UE5    : VALIDÉE
Automation MON16.6 : 10/10 SUCCESS
Régressions        : 111/111 SUCCESS
Clôture            : OUI — 17 août 2026
```

Base : `b153a5d48f709f5b86d8d125e7cd61daa095966b`.

Implémentation : `969839d2546eea399cd2403dee2c977628efe2fc`.

Correctif compilation UE5.5.4 : `8f11c2641e64f46f5ebe31a162404fd58ffae22e`.

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

Le premier build MON16.6 a révélé deux erreurs locales :

- format non littéral transmis à `FString::Printf` ;
- variables locales `Slot` masquant `UWidget::Slot`.

Elles ont été corrigées dans :

```text
8f11c2641e64f46f5ebe31a162404fd58ffae22e
Fix MON16.6 UE5.5 compilation
```

L'exécution ultérieure de toute la campagne Automation dans l'éditeur confirme le chargement du module corrigé.

- [x] compilation / chargement confirmé après correctif UE5.5.4

## Automation ciblée

Exécuté :

```text
Automation RunTests Grimrock.RPG.MON16.6
```

- [x] `PresentationProjection` — Success
- [x] `DurationFormatting` — Success
- [x] `DeterministicProjection` — Success
- [x] `FallbackProjection` — Success
- [x] `PartyApplyFeedback` — Success
- [x] `RefreshFeedback` — Success
- [x] `TickAndExpirationFeedback` — Success
- [x] `MonsterFeedbackParity` — Success
- [x] `PartyPanelProjection` — Success
- [x] `NoParallelSystem` — Success

Résultat : **10/10 Success**.

## Régressions

Campagne finale `Test Run 8` :

```text
MON16.5 : 11/11 Success
MON16.4 : 11/11 Success
MON16.3 : 11/11 Success
MON16.2 : 10/10 Success
MON16.1 :  7/7 Success
MON15   : 42/42 Success
MON14   : 19/19 Success
```

- [x] MON16.5 : 11/11 Success
- [x] MON16.4 : 11/11 Success
- [x] MON16.3 : 11/11 Success
- [x] MON16.2 : 10/10 Success
- [x] MON16.1 : 7/7 Success
- [x] MON15 : 42/42 Success
- [x] MON14 : 19/19 Success

Régressions hors MON16.6 : **111/111 Success**.

Avec MON16.6 : **121/121 Success** sur la campagne finale pertinente.

Le fichier utilisateur complet contient quatre campagnes successives :

```text
Test Completed : 293
Success        : 293
Fail           : 0
Error          : 0
```

## Clôture

- [x] compilation/chargement UE5.5.4 après correctif
- [x] 10/10 MON16.6
- [x] régressions MON16.5 → MON14 vertes
- [x] aucun nouvel échec Automation
- [x] documentation de validation mise à jour
- [x] **MON16.6 VALIDÉ ET CLOS — 17 août 2026**

Prochaine étape : `MON16.7 — Save / Restore des status effects`.
