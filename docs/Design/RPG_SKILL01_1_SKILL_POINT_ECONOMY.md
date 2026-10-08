# RPG-SKILL01.1 — Skill Point Economy & Allocation — source

Date : **8 octobre 2026**  
Parent : **RPG-SKILL01 — Skill Point Economy & Allocation**  
État : **VALIDÉ / CLOS — RPG-SKILL01.1/.2/.3 terminés ; Automation 8/8 ; PIE + / − validé**

## Objectif

Implémenter l'économie de points de compétence sans ajouter de seconde autorité
persistante.

```text
Level + SkillRanks
      -> FRPGSkillPointService
      -> Granted / Spent / Remaining / RankCap
      -> FGridSkillsPageView
      -> WBP_GridSkills / WBP_RPGSkillEntry
```

## Règles

```text
Niveau 1      = 4 points accordés
Niveau 2..20  = +1 par niveau
Total niv. 20 = 23

1 point = +1 Rank

Rank cap :
1..4   -> 2
5..9   -> 3
10..14 -> 4
15..20 -> 5
```

Le `MaxRank` du `URPGSkillAsset` reste également contraignant.

## Aucune monnaie persistée

```text
Granted   = Level + 3
Spent     = somme des SkillRanks
Remaining = Granted - Spent
```

Les points non dépensés sont donc conservés naturellement par la sauvegarde.

## Transaction

Toute allocation joueur passe par :

```text
FRPGSkillPointService::TryPurchaseNextRank()
```

UMG ne modifie jamais `SkillRanks` directement.

## Projection UI

`FGridSkillsPageView` expose :

```text
GrantedSkillPoints
SpentSkillPoints
RemainingSkillPoints
SkillRankCap
```

Chaque ligne expose :

```text
CurrentRankCap
bCanIncreaseRank
```

## RPG-SKILL01.2 — matérialisation UMG

Aucun `.uasset` n'est modifié dans ce commit.

### WBP_GridSkills

Ajouter sur la page COMPÉTENCES :

```text
Text_SkillPoints [TextBlock] — Is Variable = YES
```

Réglages :

```text
Font           = Alegreya Sans
Font Size      = 18
Auto Wrap Text = false
Justification  = Left
Visibility     = Not Hit-Testable (Self & All Children)
Designer Text  = Points de compétence : 4 — Rang max : 2
```

### WBP_RPGSkillEntry

Ajouter à droite de `SB_SkillTrainingPolicy` :

```text
SB_SkillIncrease                 [SizeBox]
└── Button_IncreaseSkill         [Button] Is Variable = YES
    └── Text_IncreaseSkill       [TextBlock]
```

Réglages :

```text
SB_SkillIncrease.Width Override = 56
Slot HorizontalBox              = Auto
Vertical Alignment              = Center

Button_IncreaseSkill
Horizontal Alignment            = Fill
Vertical Alignment              = Fill

Text_IncreaseSkill
Text                            = +
Font                            = Alegreya Sans
Font Size                       = 20
Justification                   = Center
Visibility                      = Not Hit-Testable (Self & All Children)
```

Dans `HB_SkillsColumns`, ajouter :

```text
SB_HeaderIncrease [SizeBox Width Override = 56]
└── Text_HeaderIncrease [TextBlock]
```

avec :

```text
Text          = +
Font          = Alegreya Sans
Font Size     = 16
Justification = Center
Visibility    = Not Hit-Testable (Self & All Children)
```

Le bouton reste visible mais désactivé si aucun achat n'est possible.

## Feedback

Un achat réussi utilise le toast RPG existant :

```text
Compétence améliorée
« Crochetage » passe au rang 1.
Points de compétence restants : 3.
```

## Automation

```text
Grimrock.RPG.SKILL01
```

Attendu : **5 tests**.

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.SKILL01"
```

Régression ensuite :

```text
Grimrock.MON20.6.Skills
Grimrock.MON20.8.SkillsPage
Grimrock.UI.RPG06.Skills
```

RPG-SKILL01.1 a établi l'économie et la transaction d'achat.
RPG-SKILL01.2 a matérialisé `Text_SkillPoints` et le bouton `+`.
RPG-SKILL01.3 ajoute l'annulation sûre des seules attributions de la session
courante.

Validation finale locale :

```text
Grimrock.RPG.SKILL01
Succeeded              : 8
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

PIE final validé :

```text
+  -> Rank +1, Skill Point -1, bouton - actif
-  -> Rank -1, Skill Point +1
plancher de session -> - désactivé
fermer/réouvrir K -> ancien rang non remboursable
```

Matérialisation finale poussée :

```text
aba11f5502376a77ab6b98749a3a0cbbfb9e4f7f
RPG-SKILL01.3B materialize safe Skill allocation undo
```

Références :

- `RPG_SKILL01_3_SAFE_SKILL_ALLOCATION_UNDO.md`
- `RPG_SKILL01_FINAL_CLOSURE.md`
