# UI-SKILLS02.4 — Skill Detail Widget

Date : **15 octobre 2026**  
Parent : **UI-SKILLS02 — Skill Browser & Detail Panel**  
Prérequis : **UI-SKILLS02.3 validé localement : Skills02 10/10 ; RPG06 Skills 7/7 ; RPG.SKILL01 8/8 ; MON20.8 SkillsPage 8/8**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## 1. Objectif

Introduire le widget C++ de détail qui consommera le read model
FGridSkillDetailView préparé par UI-SKILLS02.1 et UI-SKILLS02.2.

Ce jalon reste purement source :

- aucun WBP n'est créé ou modifié ;
- aucun uasset n'est modifié ;
- aucune donnée de gameplay n'est déplacée ;
- aucun texte détaillé de production n'est inventé.

Le widget ne fait que formater et afficher les données déjà projetées.

## 2. Nouveau widget

Nouveau type :

~~~text
UGridSkillDetailWidget
~~~

Fichiers :

~~~text
Source/GrimrockPrototype/Public/UI/GridSkillDetailWidget.h
Source/GrimrockPrototype/Private/UI/GridSkillDetailWidget.cpp
~~~

Le futur Blueprint Designer sera :

~~~text
WBP_RPGSkillDetail
    parent C++ = UGridSkillDetailWidget
~~~

Sa matérialisation est réservée à UI-SKILLS02.5.

## 3. Donnée reçue

Le widget reçoit uniquement :

~~~text
FGridSkillDetailView
~~~

via :

~~~text
InitializeSkillDetail(...)
ClearSkillDetail()
RefreshDetailVisual()
~~~

Il n'accède directement ni à SkillRanks, ni aux services de points, ni au
personnage, ni aux DataAssets gameplay.

FGridSkillsPageService reste responsable de construire le read model.

## 4. Sections préparées

Le contrat Designer prévoit les zones suivantes :

~~~text
HEADER
    Image_SkillIcon
    Text_DetailName
    Text_DetailAttribute
    Text_DetailRank

DESCRIPTION
    VB_DetailDescription
    Text_DetailDescription

ENTRAÎNEMENT
    VB_DetailTraining
    Text_DetailTraining

UTILISATION
    VB_DetailUsage
    Text_DetailUsage

CONSEIL DE BUILD
    VB_DetailBuildAdvice
    Text_DetailBuildAdvice

TEST DE COMPÉTENCE
    VB_DetailCheck
    Text_DetailCheckRank
    Text_DetailCheckAttributeModifier
    Text_DetailCheckBaseBonus

AFFINITÉS
    VB_DetailAffinities
    VB_DetailRecommendedClasses
    Text_DetailRecommendedClasses
    VB_DetailUsefulClasses
    Text_DetailUsefulClasses

SYNERGIES
    VB_DetailSynergies
    Text_DetailSynergies

PROGRESSION
    VB_DetailProgression
    Text_DetailCurrentRankCap
    Text_DetailRemainingSkillPoints
    VB_DetailNextRank
    Text_DetailNextRank
    VB_DetailNextRankCost
    Text_DetailNextRankCost
~~~

Tous ces bindings restent BindWidgetOptional jusqu'à UI-SKILLS02.5.

## 5. Présentation calculée

Le widget prépare les valeurs suivantes :

~~~text
ResolvedAttributeText
ResolvedRankText
ResolvedTrainingText

ResolvedCheckRankText
ResolvedCheckAttributeModifierText
ResolvedCheckBaseBonusText

ResolvedRecommendedClassesText
ResolvedUsefulClassesText
ResolvedSynergiesText

ResolvedCurrentRankCapText
ResolvedRemainingSkillPointsText
ResolvedNextRankText
ResolvedNextRankCostText
~~~

La section TEST DE COMPÉTENCE n'invente aucune formule.

Elle affiche seulement les composantes déjà validées :

~~~text
rang                    -> valeur signée
modificateur attribut   -> valeur signée
bonus de base           -> Rank + GoverningAttributeModifier
~~~

Le texte statique expliquant la formule complète restera une responsabilité de
présentation/localisation lors de la matérialisation du WBP.

## 6. Textes et localisation

Les noms d'attribut réutilisent l'autorité UI existante :

~~~text
FUILocalizationText
CharacterCreation.Attribute.*
~~~

Les libellés de rang et d'entraînement utilisent les mêmes namespace/keys que
la ligne compacte :

~~~text
GridSkills / SkillRankFormat
GridSkills / SkillTrained
GridSkills / SkillUntrainedAllowed
GridSkills / SkillTrainingRequired
~~~

Aucun nouveau paragraphe explicatif n'est codé en dur dans C++.

Description, UsageDescription et BuildAdvice proviennent exclusivement du read
model et des données de présentation prévues.

## 7. Affinités et synergies

Les noms déjà résolus dans FGridSkillDetailView sont concaténés uniquement pour
l'affichage :

~~~text
RecommendedClasses -> "Voleur, Éclaireur"
UsefulClasses      -> "Guerrier"
Synergies          -> "Crochetage, Athlétisme"
~~~

Le widget ne résout aucun ClassId ou SkillId lui-même.

## 8. Progression

La section PROGRESSION projette :

~~~text
CurrentRankCap
RemainingSkillPoints
NextRank
NextRankCost
~~~

Lorsque NextRank == 0 :

~~~text
VB_DetailNextRank     -> Collapsed
VB_DetailNextRankCost -> Collapsed
~~~

Le rang maximum reste donc représentable sans texte artificiel du type
"Maximum atteint" codé en C++.

## 9. Intégration dans UGridSkillsWidget

UGridSkillsWidget prépare maintenant un binding optionnel :

~~~text
Detail_Skill : UGridSkillDetailWidget
~~~

et une méthode interne :

~~~text
ApplySkillDetailPresentation()
~~~

Cette méthode est appelée :

- au NativeConstruct ;
- après RefreshSkills ;
- après SelectSkill ;
- après ClearSkillSelection.

Tant que WBP_RPGSkillDetail n'existe pas, Detail_Skill est null et le
comportement actuel reste inchangé.

## 10. Tests ajoutés

Quatre tests supplémentaires :

~~~text
Grimrock.UI.Skills02.DetailWidget.Formatting
Grimrock.UI.Skills02.DetailWidget.TrainingAndMaximumRank
Grimrock.UI.Skills02.DetailWidget.RejectAndClear
Grimrock.UI.Skills02.DetailWidget.WidgetContract
~~~

Ils vérifient :

- le formatage de l'attribut ;
- le rang ;
- les trois données du Skill Check ;
- les affinités et synergies ;
- les données de progression ;
- l'état TrainingRequired ;
- le comportement au rang maximum ;
- le rejet d'un read model incohérent ;
- ClearSkillDetail ;
- le contrat BindWidgetOptional ;
- le binding parent Detail_Skill.

Le filtre Grimrock.UI.Skills02 doit passer de 10 à **14 tests**.

## 11. Hors scope

UI-SKILLS02.4 ne :

- crée pas WBP_RPGSkillDetail ;
- ne modifie pas WBP_GridSkills ;
- ne modifie pas WBP_RPGSkillEntry ;
- ne crée pas DA_RPGSkillPresentation ;
- ne crée aucune icône ;
- ne renseigne aucune affinité de production ;
- ne modifie aucune règle de Skill Check ;
- ne modifie aucun coût ou plafond ;
- ne crée aucun nouveau système de textes.

## 12. Validation locale

~~~powershell
cd D:\Development\GrimrockPrototype

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.Skills02"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.RPG06.Skills"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SKILL01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON20.8.SkillsPage"
~~~

Ne pas considérer UI-SKILLS02.4 validé avant réception des sorties locales.

## 13. Suite

UI-SKILLS02.5 — Designer Materialization devra être réalisé dans Unreal Editor
avec contrôle visuel :

- créer WBP_RPGSkillDetail ;
- placer Detail_Skill dans WBP_GridSkills ;
- convertir la page en layout liste + détail ;
- matérialiser Image_SkillIcon et les états hover/selected de la ligne ;
- retirer physiquement l'ancien Text_SkillDescription ;
- créer les sections du panneau détail et leurs libellés ;
- vérifier la largeur 62/38 et le scroll du panneau.

UI-SKILLS02.6 authorera ensuite DA_RPGSkillPresentation et les icônes de
production.
