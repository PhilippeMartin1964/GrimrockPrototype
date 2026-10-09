# UI-RPG06.3B — Clôture finale Compétences + Talents

> **DOC-AUDIT02 — CLÔTURE HISTORIQUE.** UI-RPG06 reste validé/clos, mais deux
> éléments explicitement conservés à l'époque ne sont plus actuels :
> la projection plate `FGridTalentEntryView / FGridSkillsPageView::Talents`
> a été supprimée par UI-RPG-CODE-AUDIT01, et l'économie de Skill Points a
> depuis été livrée par RPG-SKILL01. Le reste du résultat UI-RPG06 demeure un
> jalon historique valide.

Date : **7 octobre 2026**  
Parent : **UI-RPG06 — Unification Compétences + Talents UX**  
État : **VALIDÉ / CLOS**

## Résultat

`WBP_GridSkills` est l'unique surface autonome pour Compétences et Talents.

```text
K
 -> WBP_GridSkills
    ├── COMPÉTENCES
    │   └── 25 Skills de production
    │       -> liste scrollable
    │       -> nom / attribut / rang / statut
    │
    └── TALENTS
        ├── 3 branches × 5 nœuds
        ├── détail
        ├── acquisition simple
        ├── acquisition à variantes
        └── notification de progression
```

Aucune logique gameplay n'est déplacée dans UMG.

## Validation finale locale

Build Development Editor :

```text
OK
```

Automation :

```text
Grimrock.UI.RPG06.Skills        7/7
Grimrock.MON20.8.SkillsPage     8/8
Grimrock.UI.RPG04              12/12
Grimrock.UI.RPG05               5/5
```

Pour les quatre filtres :

```text
Succeeded with warnings = 0
Failed                  = 0
Not run                 = 0
Process exit code        = 0
```

## Validation PIE

Validé manuellement :

```text
K ouvre WBP_GridSkills
COMPÉTENCES / TALENTS commute correctement
25 compétences accessibles par scroll
ordre alphabétique joueur par DisplayName
changement de personnage rafraîchit la projection
acquisition simple fonctionnelle
acquisition à variantes fonctionnelle
toast de progression fonctionnel
```

## Invariants finaux

- `UGridPartyInventoryComponent::SelectedCharacterIndex` reste l'autorité de sélection ;
- `FGridCharacterInventoryState::SkillRanks` reste l'autorité des rangs ;
- `URPGSkillAsset` reste la définition canonique d'un Skill ;
- `FRPGClassProgressionTransactionService` reste la seule autorité d'acquisition de Talent ;
- `ChoiceId` reste l'identité gameplay/persistante des Talents ;
- UMG ne calcule ni coût, ni prérequis, ni disponibilité ;
- aucun Skill Point, XP de Skill ou achat de rang n'est inventé tant que ce système n'existe pas.

## Dette volontairement conservée

La projection Talent plate historique :

```text
FGridTalentEntryView
FGridSkillsPageView::Talents
GetTalentEntryCount()
GetTalentEntry()
```

n'est plus consommée par la présentation C++ actuelle, mais reste
Blueprint-readable. Elle ne doit pas être supprimée sans audit explicite des
références dans les assets binaires.

## Statut des jalons UI-RPG

```text
UI-RPG01   CLOS
UI-RPG02   CLOS
UI-RPG03   CLOS
UI-RPG04   CLOS
UI-RPG05   CLOS
UI-RPG06   CLOS
```

Les anciens sous-documents qui conservent un libellé de type « source prête »
doivent être lus comme historiques ; ce document et les références CURRENT
portent le statut final.

La prochaine évolution Skills devra être un nouveau jalon fonctionnel
(explicitement l'économie de Skill Points / achat de rang si elle est décidée),
et non une extension implicite de UI-RPG06.
