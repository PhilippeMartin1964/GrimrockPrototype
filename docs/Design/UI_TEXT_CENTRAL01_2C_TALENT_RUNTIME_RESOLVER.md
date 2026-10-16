# UI-TEXT-CENTRAL01.2C — Talent runtime resolver migration

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## Autorité runtime

Le vocabulaire Talent statique passe désormais par `FRPGTalentLocalizationText` :

~~~text
Talent.Recipe.* -> DA_RPGTalentDetailTextSet (transition)
autres clés     -> FUILocalizationText -> ST_UI
~~~

Les quinze noms de recettes restent volontairement hors ST_UI jusqu'à
l'existence d'une autorité Recipe/Crafting.

## Consommateurs migrés

~~~text
GridTalentDetailWidget.cpp
GridTalentNodeWidget.cpp
GridSkillsWidget.cpp
RPGProgressionFeedbackService.cpp
~~~

Le détail Talent utilise directement `UI.Section.Usage`. Le read-model
utilise `Status.Duration.Permanent` pour la durée permanente partagée.

## Formats nommés

Les six formats migrés vers UI.csv utilisent désormais leurs arguments nommés :
`Level`, `Class`, `TalentPoints`, `SkillPoints`, `RankCap`,
`TalentName` et `RemainingPoints`.

## Validation locale

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Game.UI.TextCentral.CENTRAL01.TalentRuntimeResolver"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG.DESC01"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG05.Feedback"
~~~

Aucun uasset n'est modifié par 01.2C.
