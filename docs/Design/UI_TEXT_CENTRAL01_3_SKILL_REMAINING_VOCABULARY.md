# UI-TEXT-CENTRAL01.3 — Skill remaining vocabulary

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **SOURCE IMPLÉMENTÉE — matérialisation ST_UI et validation locale requises**

## Objectif

Retirer les cinq derniers textes joueur statiques du pipeline Skill C++.

~~~text
ancien C++
UnknownSkill
SkillRankFormat
SkillTrained
SkillUntrainedAllowed
SkillTrainingRequired

nouvelle autorité
Localization/Source/UI.csv
    -> ST_UI
    -> FUILocalizationText
    -> UGridSkillEntryWidget / UGridSkillDetailWidget
~~~

## Clés

~~~text
Skill.Fallback.Unknown
    Compétence inconnue

Skill.Rank.Format
    Rang {Rank} / {MaxRank}

Skill.Training.Trained
    Entraînée

Skill.Training.UntrainedAllowed
    Utilisable sans entraînement

Skill.Training.Required
    Entraînement requis
~~~

Le format de rang utilise volontairement des placeholders **nommés**, conformes
au contrat `UI.csv`. Les anciens placeholders ordonnés `{0}` / `{1}`
disparaissent.

## Fallback de nom

Lorsque le read-model ne fournit pas de `DisplayName`, l'UI ne transforme plus
le `SkillId` technique en texte joueur. Elle affiche
`Skill.Fallback.Unknown`. Les Data Assets Skill restent l'autorité des vrais
noms de compétences.

## Garde anti-régression

~~~text
Game.UI.TextCentral.CENTRAL01.SkillRemainingVocabulary
Game.UI.TextCentral.CENTRAL01.SkillVocabularySourceGuard
~~~

Le garde source refuse le retour des anciens `NSLOCTEXT("GridSkills", ...)`
et exige les cinq clés ST_UI dans les deux renderers Skill.

## Matérialisation

Après récupération de la source, exécuter :

~~~powershell
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
  "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" `
  -run=UIStringTableAuthoring `
  -unattended -nop4 -nosplash -nullrhi
~~~

Le premier passage doit produire `Changed=true` car cinq clés sont ajoutées.
Après versionnement de `ST_UI.uasset`, un second passage doit produire
`Changed=false`.

## Régressions

~~~text
Game.UI.TextCentral.CENTRAL01.SkillRemainingVocabulary
Game.UI.TextCentral.CENTRAL01.SkillVocabularySourceGuard
Grimrock.UI.Skills02
Grimrock.UI.RPG06.Skills
UI-LOCALIZATION01.6
~~~
