# UI-SKILLS-CODE-AUDIT02.6 — Skill Summary Migration Closure

Date : **17 octobre 2026**  
État : **IMPLÉMENTÉ — validation locale requise**

## Défaut / dette

La migration `UI-SKILLS-SUMMARY01` avait déjà matérialisé le nouveau résumé
dans `WBP_GridSkills`, mais le C++ conservait encore trois éléments de
compatibilité :

~~~text
Text_SkillPoints                         BindWidgetOptional
Text_AvailableSkillPoints                BindWidgetOptional
Text_CurrentSkillRankCap                 BindWidgetOptional
~~~

ainsi qu'un formatter et deux anciennes clés de localisation.

Cette compatibilité n'avait plus de consommateur Designer et contredisait la
règle du projet : supprimer les chemins legacy une fois la matérialisation
effectuée.

## Correction

Contrat final :

~~~text
Text_AvailableSkillPoints   BindWidget
Text_CurrentSkillRankCap    BindWidget
Text_SkillPoints            supprimé
~~~

`RefreshSkillSummaryPresentation()` écrit directement les deux valeurs
obligatoires à partir du read-model.

Supprimés :

~~~text
GridSkillsWidgetPrivate::FormatSkillPoints()
Talent.Screen.SkillPointsFormat
Talent.Screen.SkillPointsUnavailable
~~~

Les deux clés quittent `Localization/Source/UI.csv`, et les tests Talent ne les
considèrent plus comme du vocabulaire requis.

## Garanties

La modification ne touche pas :

~~~text
FRPGSkillPointService
formules de points / caps
SkillRanks
undo de session
affinités
tri
catalogue canonique
WBP_GridSkills.uasset
ST_UI.uasset
~~~

Aucun `.uasset` n'est modifié par 02.6.

Le test de production Skills recompile le Blueprint existant contre les deux
`BindWidget` obligatoires.

## Validation locale

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG.DESC01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Game.UI.TextCentral.CENTRAL01.TalentStringTableRuntime"
~~~

Ne pas déclarer 02.6 clos avant les résultats locaux.
