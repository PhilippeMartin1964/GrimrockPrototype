# UI-RPG04.4 — Generic N-Variant Talent Acquisition

Date: 7 octobre 2026

## Goal

Support every conceptual Talent node containing 2..N progression ChoiceIds without introducing class-specific UI.

Covered production families include:

- Warrior Martial Specialization (3)
- Ranger Favored Enemy (N)
- Mage Elemental Affinity (4)
- Mage Surface Weaver Imbuement (4)

## Flow

```text
conceptual node selected
-> CHOISIR UNE VARIANTE
-> generic ComboBoxString containing N concise labels
-> choose concrete variant
-> CONFIRMER / ANNULER
-> OnAcquireConfirmed(concrete ChoiceId)
-> UGridSkillsWidget::CommitConfirmedTalentChoice()
-> FRPGClassProgressionTransactionService::TryCommitChoices()
-> refresh
```

The UMG never decides cost, level, prerequisites or exclusivity. The transaction service remains authoritative.

## Concise labels

Variant authoring may expose names such as:

```text
Spécialisation martiale — Tranchant
```

The detail widget strips the conceptual prefix for selector/list presentation:

```text
Tranchant
```

After acquisition the detail displays:

```text
Variante choisie : Tranchant
```

## Designer migration

Add only two controls to the existing WBP_RPGTalentDetail:

```text
Button_ChooseVariant [Button, Is Variable]
└── Text_ChooseVariant = CHOISIR UNE VARIANTE

Combo_VariantChoice [ComboBoxString, Is Variable]
```

Recommended placement:

```text
Text_DetailVariants
Spacer_AcquireTop
Button_AcquireTalent
Button_ChooseVariant
Combo_VariantChoice
Spacer_AcquirePrompt
Text_AcquirePrompt
HB_AcquireConfirm
...
```

Initial visibility:

```text
Button_ChooseVariant = Collapsed
Combo_VariantChoice  = Collapsed
```

The C++ controls both visibilities and fills the ComboBox dynamically.

No Blueprint graph, no manually authored ComboBox options.

## Tests

```text
Grimrock.UI.RPG04.Variants.GenericNSelection
Grimrock.UI.RPG04.Variants.ShortLabels
Grimrock.UI.RPG04.Variants.Transaction
```

Together with the existing RPG04 tests, the expected filter total becomes 12.
