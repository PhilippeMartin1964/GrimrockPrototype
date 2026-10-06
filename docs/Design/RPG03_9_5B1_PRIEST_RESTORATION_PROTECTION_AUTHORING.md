# RPG03.9.5B1 — Prêtre : Restauration + Protection

Date : **6 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : `RPG03.9.5A` validé localement 6/6  
Statut : **VALIDÉ — Prêtre Restauration/Protection couvert par Grimrock.RPG.RPG03, 193/193 le 6 octobre 2026**

## Périmètre

B1 authorise les deux premières branches du Prêtre, sans matérialiser de `.uasset` :

- **Restauration** : 5/5 talents ;
- **Protection** : 5/5 talents ;
- **10 Choice records** ;
- **9 actions Spell actives** ;
- **5 Status Effects Prêtre** configurés en mémoire pour validation.

### Restauration

1. Soin renforcé — +25 % de soin sortant pour les actions `SourcePolicy=Spell`.
2. Régénération — Ally R3, 2 PA / 5 mana, CD2, `Status_Regeneration` 3 Turns.
3. Soin de groupe — Party, 3 PA / 8 mana, CD3, `5 + WIS mod + Skill_Medicine Rank`.
4. Purification — Ally R3, 2 PA / 6 mana, CD2, jusqu'à 2 Debuffs explicitement listés ou tagués `Purifiable`.
5. Miracle — Ally R3, 4 PA / 15 mana, CD5, `12 + 2×WIS mod + Religion Rank`, plancher 50 % MaxHP, 3 Purifiable, +25 % MagicalArmor de référence.

### Protection

1. Bénédiction — Ally R3, 2 PA / 5 mana, CD2, +2 Accuracy / +4 Initiative, 2 rounds.
2. Égide — Ally R3, 2 PA / 6 mana, CD2, `8 + WIS mod + Religion Rank` MagicalArmor.
3. Protection sacrée — Ally R3, 2 PA / 7 mana, CD3, +25 % Holy/Necrotic/Arcane et +2 Evasion contre Spell non-Area, 3 rounds.
4. Sanctuaire — Ally R3, 3 PA / 10 mana, CD4, blocage du ciblage hostile direct, expiration à la prochaine activation ou après dégâts réellement infligés.
5. Bastion divin — Party, 4 PA / 12 mana, CD5, +35 % MagicalArmor de référence puis -20 % dégâts non-Physical, 2 rounds.

## Architecture

- aucune comparaison runtime sur `TalentId` ;
- les ChoiceIds ne servent qu'à la progression/requirements ;
- C1 reste l'autorité des Status ;
- C2 reste l'autorité des modificateurs ;
- C3 reste l'autorité de l'armure ;
- C4 reste l'autorité de rupture de Sanctuaire ;
- C8 reste l'autorité des cibles et purges ;
- aucun asset binaire n'est créé par B1.

## Validation historique

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.RPG03.9.5B1"
```

La réussite ne doit être déclarée qu'après lecture de la sortie utilisateur.
