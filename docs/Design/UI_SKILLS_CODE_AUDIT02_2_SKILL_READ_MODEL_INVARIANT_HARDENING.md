# UI-SKILLS-CODE-AUDIT02.2 — Skill Read-Model Invariant Hardening

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **VALIDÉ / CLOS**

## 1. Défaut

Le cap de rang par niveau est une règle autoritaire de l'économie Skill :

~~~text
niveaux 1–4   -> rang maximum 2
niveaux 5–9   -> rang maximum 3
niveaux 10–14 -> rang maximum 4
niveaux 15–20 -> rang maximum 5
~~~

Avant 02.2, un état externe ou restauré pouvait contenir par exemple :

~~~text
Character.Level = 3
Skill.Rank       = 4
CurrentRankCap   = 2
~~~

La transaction d'achat empêchait correctement de créer cet état, mais
`FRPGSkillPointService::TryGetBalance()` l'acceptait tant que le total de
points dépensés restait dans le budget. Le page service pouvait ensuite émettre
un `FGridSkillEntryView` et un `FGridSkillDetailView` cohérents entre eux,
mais incohérents avec la règle de niveau.

## 2. Correction autoritaire

`FRPGSkillPointService::TryGetBalance()` refuse désormais toute entrée
`FRPGSkillRank` dont le rang dépasse `GetRankCapForLevel(Character.Level)`.

Cette validation se situe avant la projection UI et protège aussi les appels
transactionnels qui dépendent d'une balance valide.

Le service ne corrige ni ne tronque jamais l'état durable : il le rejette.

## 3. Invariants des read-models

`FGridSkillEntryView::IsValid()` est maintenant l'autorité structurelle de la
ligne Skill. Il vérifie notamment :

~~~text
SkillId valide
MaxRank >= 1
1 <= CurrentRankCap <= MaxRank
0 <= Rank <= CurrentRankCap
bTrained == (Rank > 0)
bCanIncreaseRank impossible au MaxRank ou au CurrentRankCap
bCanDecreaseRank impossible à rang zéro
~~~

`FGridSkillDetailView::IsValid()` refuse explicitement :

~~~text
Rank > CurrentRankCap
~~~

et `bAtCurrentRankCap` correspond désormais exactement à :

~~~text
Rank == CurrentRankCap
~~~

puisque l'état `Rank > CurrentRankCap` n'est plus valide.

`FGridSkillsPageView::IsValid()` vérifie maintenant la cohérence de la partie
Skills : identité de page, niveau/classe, balance Skill, cap global, unicité des
SkillId et validité de chaque entrée. Chaque `CurrentRankCap` de ligne doit
également être égal à :

~~~text
min(Skill.MaxRank, Page.SkillRankCap)
~~~

## 4. Construction atomique

`FGridSkillsPageService::TryBuildCharacterView()` valide chaque
`FGridSkillEntryView` avant insertion puis valide le page read-model complet
avant de l'exposer.

Un état durable invalide provoque donc :

~~~text
TryGetBalance          -> false
TryBuildCharacterView  -> false
OutView                -> reset / invalide
SkillRanks durable     -> inchangé
~~~

Le read-model n'essaie jamais de réparer silencieusement la donnée.

## 5. Tests renforcés

Les tests existants couvrent maintenant :

~~~text
Grimrock.RPG.SKILL01.Economy.DerivedBalance
    -> balance refusée si un rang dépasse le cap de niveau

Grimrock.UI.Skills02.Architecture.DetailReadModel
    -> DetailView refuse Rank > CurrentRankCap
    -> EntryView refuse Rank > CurrentRankCap

Grimrock.UI.Skills02.Pipeline.TrainingAndRankCap
    -> stockage externe au-dessus du cap accepté par la couche brute
    -> page read-model refusé
    -> balance refusée
    -> état durable non muté
~~~

Deux anciennes fixtures `MON20.8` utilisaient des rangs 3/4 sur un personnage
niveau 3. Elles sont ramenées au rang 2 afin de ne plus encoder un état que la
règle de progression interdit.

## 6. Non-objectifs

02.2 ne modifie pas :

~~~text
coût d'un rang
planning des points Skill
planning des caps
algorithme d'achat
undo de session
affinités
tri
UMG
.uasset
SaveGame / migration automatique
~~~

Aucune donnée invalide n'est automatiquement corrigée ou sauvegardée.

## 7. Validation locale

Validation ciblée Skills UI :

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Validation de l'autorité d'économie Skill :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.SKILL01"
~~~

Puis la régression historique des read-models Skills :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.MON20.8.SkillsPage"
~~~

Validation locale reçue :

~~~text
Grimrock.UI.Skills02       25/25 PASS
Grimrock.RPG.SKILL01        8/8 PASS
Grimrock.MON20.8.SkillsPage 8/8 PASS
warnings                    0
failures                    0
~~~

État final : **UI-SKILLS-CODE-AUDIT02.2 VALIDÉ / CLOS**.
