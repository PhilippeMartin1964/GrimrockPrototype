# UI-RPG04.3A — Simple Talent Acquisition Contract

Date: 7 octobre 2026
Status: source ready; local UE5.5.4 validation required.

## Flow

```text
selected simple Talent
-> ACQUERIR
-> confirmation pending
-> CONFIRMER / ANNULER
-> UGridTalentDetailWidget::OnAcquireConfirmed(ChoiceId)
-> UGridSkillsWidget::CommitConfirmedSimpleTalent()
-> FRPGClassProgressionTransactionService::TryCommitChoices()
-> NotifyPartyInventoryChanged()
-> existing Skills refresh
```

## Authority

No cost, level, prerequisite, exclusivity or point-budget rule is reimplemented in UMG.
The final mutation authority remains `FRPGClassProgressionTransactionService::TryCommitChoices()`.

The detail widget only permits the simple confirmation UX when the current read model is
`Available` and contains exactly one concrete ChoiceId. The transaction service validates
the request again at commit time.

Multi-variant nodes are deliberately deferred to UI-RPG04.4.

## 04.3B Designer controls

The C++ contract already accepts these optional widgets in WBP_RPGTalentDetail:

```text
Button_AcquireTalent
Button_ConfirmAcquire
Button_CancelAcquire
Text_AcquirePrompt
Text_AcquireFeedback
```

They remain BindWidgetOptional in 04.3A so source validation does not require a binary asset edit.

## Tests

```text
Grimrock.UI.RPG04.Acquisition.ConfirmationState
Grimrock.UI.RPG04.Acquisition.VariantDeferred
Grimrock.UI.RPG04.Acquisition.SimpleCommit
```
