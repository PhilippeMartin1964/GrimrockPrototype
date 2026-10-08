# RPG-SKILL01.3 — Safe Skill Allocation Undo

Date : **8 octobre 2026**  
Parent : **RPG-SKILL01 — Skill Point Economy & Allocation**  
État : **VALIDÉ / CLOS — Automation 8/8 ; matérialisation UMG poussée ; PIE Safe Undo validé**

## Objectif

Permettre de corriger immédiatement un clic `+` erroné sans transformer
l'écran COMPÉTENCES en système de respec gratuit.

Règle :

```text
rang existant avant la session
        = plancher non remboursable

rangs achetés depuis l'ouverture courante
        = annulables avec -
```

Fermer puis rouvrir COMPÉTENCES démarre une nouvelle session et valide donc les
rangs précédemment achetés.

## Autorité

`FRPGSkillPointService` reste l'unique autorité économique.

```text
+ -> TryPurchaseNextRank()
- -> TryRefundPurchasedRank(SessionFloorRank)
```

Le remboursement :

- décrémente exactement un Rank ;
- rembourse exactement un Skill Point puisque le solde reste dérivé de la
  somme des `SkillRanks` ;
- refuse de franchir le rang présent avant la session ;
- réutilise `FRPGSkillService::TrySetSkillRank()` pour la mutation sparse ;
- notifie `UGridPartyInventoryComponent` comme une mutation normale.

Aucun compteur de remboursement n'est sauvegardé.

## Session UI

`UGridSkillsWidget` conserve uniquement deux maps transitoires non persistées :

```text
SessionPurchasedSkillRanks
SessionSkillRankFloors
```

La clé combine `CharacterId + SkillId`, ce qui permet de changer de personnage
dans la même fenêtre sans perdre l'historique d'annulation de l'autre membre du
groupe.

`AGrimrockPartyPawn::ShowSkillsWidget()` appelle :

```text
BeginSkillAllocationSession()
```

à chaque réouverture du panneau.

## Projection

Chaque `FGridSkillEntryView` expose désormais :

```text
bCanIncreaseRank
bCanDecreaseRank
```

Le second vaut vrai uniquement si le personnage possède encore au moins un rang
acheté dans la session et que le rang courant reste supérieur au plancher.

## Feedback

Succès :

```text
Attribution annulée
« Crochetage » revient au rang 1.
Points de compétence disponibles : 3.
```

Refus hors session :

```text
Annulation impossible
Seuls les rangs attribués depuis l'ouverture actuelle de COMPÉTENCES
peuvent être annulés.
```

## RPG-SKILL01.3B — matérialisation UMG

Aucun `.uasset` n'est modifié par le commit source.

Dans `WBP_RPGSkillEntry`, conserver `Button_IncreaseSkill` et transformer
la dernière zone en :

```text
SB_SkillIncrease                  [SizeBox]
└── HB_SkillAllocation            [HorizontalBox]
    ├── Button_DecreaseSkill      [Button] Is Variable = YES
    │   └── Text_DecreaseSkill    [TextBlock]
    └── Button_IncreaseSkill      [Button] Is Variable = YES
        └── Text_IncreaseSkill    [TextBlock]
```

Réglages :

```text
SB_SkillIncrease.Width Override = 88

Button_DecreaseSkill Slot
Size                           = Fill 1.0
Vertical Alignment             = Center

Button_IncreaseSkill Slot
Size                           = Fill 1.0
Vertical Alignment             = Center

Text_DecreaseSkill
Text                           = −
Font                           = Alegreya Sans
Font Size                      = 20
Justification                  = Center
Auto Wrap Text                 = false
Visibility                     = Not Hit-Testable (Self & All Children)
Is Variable                    = NO

Text_IncreaseSkill
Text                           = +
Font                           = Alegreya Sans
Font Size                      = 20
Justification                  = Center
Auto Wrap Text                 = false
Visibility                     = Not Hit-Testable (Self & All Children)
```

Dans `WBP_GridSkills`, garder le header existant mais passer :

```text
SB_HeaderIncrease.Width Override = 88
Text_HeaderIncrease.Text          = − / +
Text_HeaderIncrease.Font Size     = 16
Text_HeaderIncrease.Justification = Center
```

Aucun Event Graph ni binding Blueprint.

## Automation

Le filtre parent devient :

```text
Grimrock.RPG.SKILL01
```

Attendu après RPG-SKILL01.3A : **8 tests**.

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.SKILL01"
```

PIE attendu après matérialisation :

```text
ouvrir K
+ sur une Skill -> rang +1, point -1, bouton - actif
-               -> rang -1, point +1
- au plancher   -> désactivé
fermer K
rouvrir K       -> ancien rang devient non remboursable
```


## Clôture

Validation source :

```text
Grimrock.RPG.SKILL01
8/8
0 warning
0 échec
exit code 0
```

Validation PIE :

- `+` achète un rang et consomme un point ;
- `−` annule uniquement un rang acheté pendant la session courante ;
- le point est remboursé automatiquement ;
- le bouton `−` se désactive au plancher de session ;
- fermer puis rouvrir COMPÉTENCES crée une nouvelle frontière non remboursable.

Matérialisation :

```text
aba11f5502376a77ab6b98749a3a0cbbfb9e4f7f
RPG-SKILL01.3B materialize safe Skill allocation undo
```

Le commit binaire final contient uniquement `WBP_RPGSkillEntry.uasset` ; l'état
de `WBP_GridSkills` requis par RPG-SKILL01.2/01.3 a été confirmé en PIE sans
diff binaire supplémentaire dans ce commit.

Le parent `RPG-SKILL01` est **CLOS**.
