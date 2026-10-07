# UI-RPG04.1 — Talent Node Selection Routing

Date : **7 octobre 2026**  
État : **SOURCE PRÊTE — validation locale UE5.5.4 requise**

## Objectif

Relier le clic d'un `WBP_RPGTalentNode` à `UGridSkillsWidget` sans aucune mutation gameplay.

Le ticket introduit uniquement une sélection UI transient d'un **TalentNodeId conceptuel**.

## Flux

```text
WBP_RPGTalentNode
└── UGridTalentNodeWidget
    └── OnTalentNodeClicked(TalentNodeId)
        ↓
WBP_RPGTalentBranch
└── UGridTalentBranchWidget
    └── OnTalentNodeClicked(TalentNodeId)
        ↓
WBP_GridSkills
└── UGridSkillsWidget
    ├── SelectedTalentNodeId
    └── OnTalentSelectionChanged(TalentNodeId)
```

## Invariant critique

UI-RPG04.1 ne fait **aucune acquisition** et n'appelle jamais :

```text
FRPGClassProgressionTransactionService::TryCommitChoices()
```

Il ne modifie jamais `SelectedClassProgressionChoiceIds` ni le budget de Talent Points.

Le clic sert uniquement à préparer le panneau de détail UI-RPG04.2.

## Sélection

`SelectTalentNode()` accepte tout nœud présent dans le read model courant, quel que soit son état. C'est volontaire : un talent verrouillé doit pouvoir être inspecté.

Un `TalentNodeId` absent du read model est refusé. La sélection est effacée lorsque le personnage change ou lorsque le nœud n'existe plus après refresh.

## Tests

```text
Grimrock.UI.RPG04.Selection.KnownNode
Grimrock.UI.RPG04.Selection.UnknownNode
Grimrock.UI.RPG04.Selection.RoutingContract
```

Validation :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG04.Selection"
```

Résultat attendu : 3 réussites, 0 warning, 0 échec.

## Suite

UI-RPG04.2 ajoutera `WBP_RPGTalentDetail`, consommateur de `GetSelectedTalentNode()` et `OnTalentSelectionChanged`.

Aucune transaction n'est introduite avant la phase de confirmation d'acquisition.
