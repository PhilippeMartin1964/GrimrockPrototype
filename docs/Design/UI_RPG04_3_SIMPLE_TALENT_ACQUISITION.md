# UI-RPG04.3 — Acquisition simple d’un talent

Date : **7 octobre 2026**  
État : **SOURCE VALIDÉE LOCALEMENT — 9/9 RPG04 avant UI-RPG04.4**

## Objectif

Permettre l’acquisition réelle d’un talent simple, c’est-à-dire d’un nœud conceptuel contenant exactement un `ChoiceId`.

Le flux UI est :

```text
talent simple sélectionné
→ ACQUÉRIR
→ demande de confirmation
→ CONFIRMER / ANNULER
→ UGridTalentDetailWidget::OnAcquireConfirmed(ChoiceId)
→ UGridSkillsWidget::CommitConfirmedTalentChoice()
→ FRPGClassProgressionTransactionService::TryCommitChoices()
→ NotifyPartyInventoryChanged()
→ RefreshSkills()
```

## Autorité métier

L’UMG ne recalcule jamais :

- le coût en points de talent ;
- le niveau requis ;
- les prérequis ;
- les exclusions ;
- le budget de points restant.

L’autorité finale reste exclusivement :

```cpp
FRPGClassProgressionTransactionService::TryCommitChoices(...)
```

Le panneau de détail ne fait que décider si l’UX d’acquisition simple peut être proposée à partir du read model courant.

## Conditions d’affichage du bouton ACQUÉRIR

`UGridTalentDetailWidget::CanRequestSimpleAcquisition()` exige :

```text
bInitialized == true
NodeView.State == Available
NodeView.Variants.Num() == 1
ChoiceId valide
variante non déjà sélectionnée
```

Si le nœud possède plusieurs variantes, l’acquisition simple est masquée et UI-RPG04.4 prend le relais.

## Contrôles Designer de WBP_RPGTalentDetail

Contrôles déjà matérialisés :

```text
Button_AcquireTalent
Button_ConfirmAcquire
Button_CancelAcquire
Text_AcquirePrompt
Text_AcquireFeedback
```

Ces contrôles sont pilotés par le C++.

Aucun Event Graph Blueprint n’est nécessaire.

## Comportement attendu

Talent simple disponible :

```text
[ ACQUÉRIR ]
```

Après clic :

```text
Confirmer l’acquisition de « Nom du talent » ?

[ CONFIRMER ] [ ANNULER ]
```

ANNULER :

```text
aucune mutation
retour à ACQUÉRIR
```

CONFIRMER :

```text
TryCommitChoices(ChoiceId)
→ point consommé
→ talent acquis
→ refresh du read model
→ compteur de points mis à jour
```

## Tests

```text
Grimrock.UI.RPG04.Acquisition.ConfirmationState
Grimrock.UI.RPG04.Acquisition.VariantDeferred
Grimrock.UI.RPG04.Acquisition.SimpleCommit
```

Le test `SimpleCommit` vérifie la transaction réelle via le service autoritaire.
