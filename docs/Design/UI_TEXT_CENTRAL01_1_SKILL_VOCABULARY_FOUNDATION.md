# UI-TEXT-CENTRAL01.1 — Central Skill detail vocabulary foundation

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **01.1C SOURCE CLOSURE IMPLÉMENTÉE — validation locale finale requise**

## 1. Décision

Le projet converge vers une autorité centrale pour le vocabulaire statique
d'interface :

~~~text
Localization/Source/UI.csv
    -> UIStringTableAuthoring
    -> /Game/Localization/StringTables/ST_UI
    -> FUILocalizationText::Get(Key)
    -> widgets runtime
~~~

Cette autorité peut être réutilisée par plusieurs écrans lorsqu'ils expriment
réellement la même notion.

Les Data Assets métier restent l'autorité du contenu du jeu :

~~~text
DA_Skill_*                  -> noms et descriptions de compétences
DA_RPGSkillPresentation     -> usage / conseil de build / affinités / synergies
DA_Class_* / Talents        -> contenu RPG authoré
DA_Item_* / recettes / codex -> contenu métier correspondant
~~~

Le WBP reste responsable de la composition visuelle, pas du wording statique
important.

## 2. Pilote Skills

UI-TEXT-CENTRAL01.1 ajoute 17 clés au CSV central.

Clés réutilisables :

~~~text
UI.Section.Description
UI.Section.Usage
UI.Section.Progression
~~~

Clés spécifiques au domaine Skill :

~~~text
Skill.Section.Training
Skill.Section.BuildAdvice
Skill.Section.Check
Skill.Section.Affinities
Skill.Section.Synergies

Skill.Check.Rank
Skill.Check.AttributeModifier
Skill.Check.BaseBonus

Skill.Affinity.RecommendedClasses
Skill.Affinity.UsefulClasses

Skill.Progression.CurrentRankCap
Skill.Progression.RemainingSkillPoints
Skill.Progression.NextRank
Skill.Progression.NextRankCost
~~~

Les libellés demandés sont notamment :

~~~text
Rang de compétence :
Modificateur d'attribut :
Bonus de base :
~~~

## 3. Runtime

UGridSkillDetailWidget prépare 17 bindings de labels Designer et les remplit
depuis ST_UI dans NativeConstruct.

La matérialisation de WBP_RPGSkillDetail a été réalisée dans
UI-TEXT-CENTRAL01.1B et validée visuellement. Les 17 bindings sont donc
désormais BindWidget obligatoires dans UI-TEXT-CENTRAL01.1C.

Aucun texte de contenu Skill n'est déplacé.

## 4. Matérialisation Designer

ST_UI a été régénérée et WBP_RPGSkillDetail a été matérialisé avec les
TextBlocks statiques renommés / marqués Variable avec les noms suivants :

~~~text
Text_LabelDescription
Text_LabelTraining
Text_LabelUsage
Text_LabelBuildAdvice
Text_LabelCheck
Text_LabelCheckRank
Text_LabelCheckAttributeModifier
Text_LabelCheckBaseBonus
Text_LabelAffinities
Text_LabelRecommendedClasses
Text_LabelUsefulClasses
Text_LabelSynergies
Text_LabelProgression
Text_LabelCurrentRankCap
Text_LabelRemainingSkillPoints
Text_LabelNextRank
Text_LabelNextRankCost
~~~

Les widgets de valeur existants Text_Detail* ne changent pas.

La matérialisation binaire a été versionnée dans le commit
`0e3e22c7 UI-TEXT-CENTRAL01.1B materialize Skill labels`.

Le filtre `Grimrock.UI.Skills02` a ensuite été validé localement à
**15/15, 0 warning, 0 échec** avant le resserrement des bindings en 01.1C.

## 5. Hors scope

Ce ticket ne :

- modifie aucun .uasset ;
- ne supprime pas DA_RPGTalentDetailTextSet ;
- ne migre pas encore le vocabulaire Talent ;
- ne traduit pas en anglais ;
- ne change aucune règle gameplay.

DA_RPGTalentDetailTextSet reste donc compatible pendant la migration.

## 6. Validation finale 01.1C

Après récupération du commit source 01.1C :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Le jalon peut être fermé définitivement si le filtre reste à 15/15.

## 7. Suite

~~~text
UI-TEXT-CENTRAL01.2 — Talent TextSet convergence
~~~

Ce ticket auditera toutes les clés réellement consommées par
DA_RPGTalentDetailTextSet, les copiera vers UI.csv avec leurs textes français
validés, migrera les consommateurs vers FUILocalizationText, puis retirera
l'ancienne autorité seulement après régression verte.

Les écrans Recettes, Codex et autres utiliseront ensuite la même règle :
vocabulaire d'interface dans UI.csv ; contenu métier dans leurs Data Assets.
