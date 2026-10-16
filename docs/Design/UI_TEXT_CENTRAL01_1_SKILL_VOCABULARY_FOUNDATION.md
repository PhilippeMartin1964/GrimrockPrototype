# UI-TEXT-CENTRAL01.1 — Central Skill detail vocabulary foundation

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **SOURCE IMPLÉMENTÉE — matérialisation ST_UI et WBP requise**

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

UGridSkillDetailWidget prépare 17 bindings optionnels de labels Designer et
les remplit depuis ST_UI dans NativeConstruct.

Ils restent BindWidgetOptional dans ce ticket afin de ne pas casser
WBP_RPGSkillDetail avant la matérialisation manuelle dans Unreal Editor.

Aucun texte de contenu Skill n'est déplacé.

## 4. Matérialisation Designer requise

Après régénération de ST_UI, les TextBlocks statiques existants de
WBP_RPGSkillDetail devront être renommés / marqués Variable avec les noms
suivants :

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

## 5. Hors scope

Ce ticket ne :

- modifie aucun .uasset ;
- ne supprime pas DA_RPGTalentDetailTextSet ;
- ne migre pas encore le vocabulaire Talent ;
- ne traduit pas en anglais ;
- ne change aucune règle gameplay.

DA_RPGTalentDetailTextSet reste donc compatible pendant la migration.

## 6. Suite

~~~text
UI-TEXT-CENTRAL01.2 — Talent TextSet convergence
~~~

Ce ticket auditera toutes les clés réellement consommées par
DA_RPGTalentDetailTextSet, les copiera vers UI.csv avec leurs textes français
validés, migrera les consommateurs vers FUILocalizationText, puis retirera
l'ancienne autorité seulement après régression verte.

Les écrans Recettes, Codex et autres utiliseront ensuite la même règle :
vocabulaire d'interface dans UI.csv ; contenu métier dans leurs Data Assets.
