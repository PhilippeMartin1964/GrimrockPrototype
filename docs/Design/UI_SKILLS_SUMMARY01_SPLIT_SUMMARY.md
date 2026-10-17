# UI-SKILLS-SUMMARY01 — Split Skill summary

Date : **17 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **VALIDÉ CÔTÉ DESIGNER — fermeture native UI-SKILLS-CODE-AUDIT02.6 implémentée**

## 1. Contrat final

Le résumé Skills utilise deux informations indépendantes :

~~~text
Points de compétence disponibles : <valeur>
Rang maximum actuel possible :      <valeur>
~~~

Autorités :

~~~text
UI.csv / ST_UI
    Skill.Summary.AvailablePoints
    Skill.Summary.CurrentRankCap

View.RemainingSkillPoints
    -> Text_AvailableSkillPoints

View.SkillRankCap
    -> Text_CurrentSkillRankCap
~~~

En l'absence de personnage valide, les deux valeurs dynamiques affichent le
glyphe mécanique `—`.

## 2. Designer matérialisé

Le WBP est déjà matérialisé et confirmé dans le projet avec les deux valeurs
séparées. Le contrat natif final est donc :

~~~text
Text_AvailableSkillPoints   -> BindWidget
Text_CurrentSkillRankCap    -> BindWidget
~~~

Les labels statiques restent sous autorité Designer / ST_UI :

~~~text
Text_LabelAvailableSkillPoints
    -> Skill.Summary.AvailablePoints

Text_LabelCurrentSkillRankCap
    -> Skill.Summary.CurrentRankCap
~~~

Le C++ ne possède ni ces labels, ni leur style, ni leur mise en page.

## 3. Legacy supprimé

UI-SKILLS-CODE-AUDIT02.6 retire définitivement :

~~~text
Text_SkillPoints
GridSkillsWidgetPrivate::FormatSkillPoints()
Talent.Screen.SkillPointsFormat
Talent.Screen.SkillPointsUnavailable
~~~

Les deux anciennes clés quittent `Localization/Source/UI.csv` et ne sont plus
requises par les tests runtime Talent.

Aucune couche de compatibilité n'est conservée.

## 4. Propriété des données

Le split ne change aucune règle de progression :

~~~text
FRPGSkillPointService
    -> Granted / Spent / Remaining / RankCap

FGridSkillsPageView
    -> RemainingSkillPoints
    -> SkillRankCap

UGridSkillsWidget
    -> projection des deux valeurs uniquement
~~~

Aucun compteur de points Skill supplémentaire n'est persisté.

## 5. Validation

Le test existant :

~~~text
Grimrock.UI.Skills02.Summary.WidgetContract
~~~

verrouille maintenant les deux `BindWidget` obligatoires et l'absence du
symbole natif `Text_SkillPoints`.

La régression de production `Grimrock.UI.Skills02.Production.CatalogIconsAndWiring`
recompile `WBP_GridSkills` et détecte donc une divergence entre le Designer et
les bindings natifs durcis.

Validation locale finale requise après le commit 02.6 :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Skills02"
~~~

Puis :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.RPG.DESC01"
~~~
