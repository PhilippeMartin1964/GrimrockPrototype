# UI-SKILLS-SUMMARY01 — Split Skill summary

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **01.1 SOURCE PRÉPARÉE — matérialisation ST_UI et WBP requise**

## 1. Objectif

Remplacer le résumé monolithique :

~~~text
Points de compétence : {SkillPoints} - Rang max : {RankCap}
~~~

par deux informations indépendantes :

~~~text
Points de compétence disponibles : <valeur>
Rang maximum actuel possible :      <valeur>
~~~

Cette séparation permet d’ajouter ultérieurement une icône, un espacement ou
un style propre à chaque information sans reconstruire une grosse chaîne.

## 2. Autorités

Libellés statiques :

~~~text
UI.csv
  -> ST_UI
  -> Text_LabelAvailableSkillPoints
  -> Text_LabelCurrentSkillRankCap
~~~

Clés :

~~~text
Skill.Summary.AvailablePoints
Skill.Summary.CurrentRankCap
~~~

Valeurs dynamiques :

~~~text
View.RemainingSkillPoints -> Text_AvailableSkillPoints
View.SkillRankCap         -> Text_CurrentSkillRankCap
~~~

En l’absence de personnage valide, les deux valeurs affichent `—`.

## 3. Phase 01.1

Le C++ expose provisoirement deux nouveaux `BindWidgetOptional` :

~~~text
Text_AvailableSkillPoints
Text_CurrentSkillRankCap
~~~

`Text_SkillPoints` et les anciennes clés `Talent.Screen.SkillPointsFormat` /
`Talent.Screen.SkillPointsUnavailable` restent uniquement comme bridge de
matérialisation afin de ne pas casser le WBP existant sur `master`.

Après matérialisation du nouveau WBP, la phase 01.2 supprimera entièrement :

~~~text
Text_SkillPoints
Talent.Screen.SkillPointsFormat
Talent.Screen.SkillPointsUnavailable
GridSkillsWidgetPrivate::FormatSkillPoints()
~~~

Aucune compatibilité durable ne sera conservée.

## 4. WBP cible

~~~text
HB_AvailableSkillPoints
├── [Image_AvailableSkillPoints]        futur / optionnel
├── Text_LabelAvailableSkillPoints      -> ST_UI / Skill.Summary.AvailablePoints
└── Text_AvailableSkillPoints           -> valeur C++

HB_CurrentSkillRankCap
├── [Image_CurrentSkillRankCap]         futur / optionnel
├── Text_LabelCurrentSkillRankCap       -> ST_UI / Skill.Summary.CurrentRankCap
└── Text_CurrentSkillRankCap            -> valeur C++
~~~

Les labels ne doivent pas être `Is Variable` pour le C++ : ils restent
strictement Designer / ST_UI.

## 5. Validation 01.1

Après récupération du commit :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Le filtre doit inclure le nouveau test :

~~~text
Grimrock.UI.Skills02.Summary.WidgetContract
~~~

Ensuite : matérialiser les deux nouvelles clés dans `ST_UI`, puis modifier
`WBP_GridSkills` avant de supprimer le bridge legacy en 01.2.
