# UI-RPG-TEXT01.10 — Final hardcoded French audit and closure

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Ticket : **UI-RPG-TEXT01.10**

## Résultat

Le pipeline runtime de présentation Talent est fermé pour TEXT01.

Les fichiers runtime Talent audités sont notamment :

~~~text
GridSkillsPageService.cpp
GridTalentDetailWidget.cpp
GridTalentVariantBlockWidget.cpp
GridTalentDetailLineWidget.cpp
GridTalentNodeWidget.cpp
GridTalentBranchWidget.cpp
GridSkillsWidget.cpp
RPGProgressionFeedbackService.cpp (partie Talent uniquement)
~~~

Les textes dépendants de la langue utilisent désormais des identités stables
résolues par `DA_RPGTalentDetailTextSet`. Les nombres, signes et pourcentages
restent des valeurs mécaniques C++.

## Clés ajoutées à la fermeture

~~~text
Talent.Node.LevelFormat
Talent.Node.State.*

Talent.Screen.NoCharacterSelected
Talent.Screen.SelectedCharacterFallback
Talent.Screen.ClassLevelFormat
Talent.Screen.TalentPointsFormat
Talent.Screen.SkillPointsUnavailable
Talent.Screen.SkillPointsFormat

Talent.Feedback.Reject.*
Talent.Feedback.Success.*
Talent.Feedback.Rejected.*
~~~

Le test `Grimrock.UI.RPG.DESC01.ReadModel.PresentationTextAuthority` exige
désormais la présence de ces clés dans le Data Asset de production.

## Hors périmètre identifié par l'audit

L'audit a également trouvé du français dans :

- `Source/GrimrockPrototypeEditor/Private/RPG/RPG*Authoring.cpp` ;
- les branches Skill/Attribute/LevelUp de `RPGProgressionFeedbackService.cpp` ;
- d'autres UI sans rapport avec les Talents.

Ces occurrences ne sont **pas** déclarées corrigées par TEXT01. Les fichiers
Editor Authoring contiennent le contenu narratif/gameplay authoré (noms,
principes, descriptions, descriptions d'actions). Ils devront être traités par
un chantier ultérieur si une localisation intégrale du contenu du jeu est demandée.

## Validation attendue

Valider `Grimrock.UI.RPG.DESC01` puis `Grimrock.UI.RPG05.Feedback` avec
`Scripts/ValidateUE.ps1` après ajout des nouvelles entrées dans le Data Asset.

TEXT01.10 n'est validé qu'après retour utilisateur des sorties locales.