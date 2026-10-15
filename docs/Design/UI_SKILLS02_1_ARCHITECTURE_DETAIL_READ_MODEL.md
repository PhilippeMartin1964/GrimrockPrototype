# UI-SKILLS02.1 — Architecture & Detail Read Model

Date : **15 octobre 2026**  
Parent : **UI-SKILLS02 — Skill Browser & Detail Panel**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## 1. Objectif

Préparer la refonte graphique de la page COMPÉTENCES sans modifier encore les
Widgets Blueprint ni créer d'asset binaire.

Cette tranche introduit uniquement :

- un catalogue de métadonnées de présentation des Skills ;
- un read model détaillé prêt à alimenter le futur panneau latéral ;
- les invariants structurels associés ;
- des tests Automation ciblés.

Aucune règle gameplay n'est ajoutée.

## 2. Autorités conservées

Les autorités existantes restent inchangées :

~~~text
URPGSkillAsset
    SkillId
    DisplayName
    Description
    GoverningAttribute
    MaxRank
    bAllowUntrainedChecks
    RequirementGrants

FGridCharacterInventoryState::SkillRanks
FRPGSkillService
FRPGSkillPointService
FRPGSkillCheckService
~~~

Le nouveau catalogue ne doit jamais dupliquer ces champs.

## 3. Catalogue de présentation

Nouveau type : URPGSkillPresentationAsset.

Entrée :

~~~text
FRPGSkillPresentationDefinition
    SkillId
    Icon
    UsageDescription
    BuildAdvice
    RecommendedClassIds
    UsefulClassIds
    SynergySkillIds
~~~

RecommendedClassIds et UsefulClassIds sont des conseils de build uniquement.
Ils n'interdisent jamais l'utilisation ou la progression d'un Skill.

Le futur asset de production sera :

~~~text
/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGSkillPresentation
~~~

Il n'est volontairement pas créé dans UI-SKILLS02.1.

### Validation structurelle

Le catalogue rejette :

- SkillId=None ;
- les SkillId dupliqués ;
- les ids de classe None ou dupliqués dans une même liste ;
- une classe présente simultanément dans RecommendedClassIds et UsefulClassIds ;
- une auto-synergie ;
- une synergie dupliquée ;
- un SynergySkillId absent du catalogue.

La complétude de production (25 entrées, icônes obligatoires et références vers
les vraies classes canoniques) reste réservée à UI-SKILLS02.6, lorsque l'asset
sera authoré.

## 4. Detail read model

Nouveau type : FGridSkillDetailView.

Il contient des valeurs prêtes à afficher :

~~~text
identité / nom / description / icône
attribut directeur + valeur + modificateur
rang / rang max / plafond actuel
points restants
prochain rang + coût
état d'entraînement
disponibilité + / -
bonus de base du test
usage / conseil de build
classes recommandées / classes utiles
synergies
~~~

BaseCheckBonus signifie strictement :

~~~text
Rank + GoverningAttributeModifier
~~~

Il exclut volontairement les modificateurs de Talent, de groupe et de contexte.
Le panneau pourra ainsi présenter la base stable puis expliquer séparément les
bonus contextuels.

## 5. Sémantique d'entraînement

Le read model introduit :

~~~text
EGridSkillTrainingState
    UntrainedAllowed
    TrainingRequired
    Trained
~~~

Projection attendue :

~~~text
Rank > 0
    -> Trained

Rank == 0 && bAllowUntrainedChecks
    -> UntrainedAllowed

Rank == 0 && !bAllowUntrainedChecks
    -> TrainingRequired
~~~

Cette enum est une sémantique UI ; elle ne remplace pas
bAllowUntrainedChecks, qui reste la règle canonique dans URPGSkillAsset.

## 6. Affinités et synergies

Le detail read model reçoit des références déjà résolues :

~~~text
FGridSkillClassAffinityView
    ClassId
    DisplayName

FGridSkillSynergyView
    SkillId
    DisplayName
~~~

UMG n'aura donc pas à rechercher des classes ou des Skills ni à convertir des
ids techniques en libellés joueur.

La résolution elle-même sera implémentée dans UI-SKILLS02.2.

## 7. Hors scope

UI-SKILLS02.1 ne doit pas :

- modifier WBP_GridSkills ;
- modifier WBP_RPGSkillEntry ;
- créer WBP_RPGSkillDetail ;
- créer DA_RPGSkillPresentation ;
- sélectionner encore un Skill dans UGridSkillsWidget ;
- modifier les achats/remboursements de rang ;
- modifier la formule des tests ;
- modifier les plafonds ou les points ;
- authorer les 25 icônes ;
- ajouter des textes joueur en dur dans le C++.

## 8. Fichiers source

~~~text
Source/GrimrockPrototype/Public/UI/RPGSkillPresentationAsset.h
Source/GrimrockPrototype/Private/UI/RPGSkillPresentationAsset.cpp
Source/GrimrockPrototype/Public/UI/GridSkillsUiTypes.h
Source/GrimrockPrototype/Private/Tests/UISKILLS021ArchitectureTests.cpp
~~~

## 9. Validation locale demandée

Après pull :

~~~powershell
cd D:\Development\GrimrockPrototype
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.Skills02"
~~~

Puis régression :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON20.8.SkillsPage"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.RPG06.Skills"
~~~

Ne pas déclarer UI-SKILLS02.1 validé tant que les sorties locales n'ont pas été
fournies.

## 10. Critères de sortie

~~~text
catalogue présentation structurellement valide        -> accepté
SkillId dupliqué                                      -> rejeté
classe recommandée + utile simultanément              -> rejetée
auto-synergie                                         -> rejetée
synergie vers Skill absent                            -> rejetée
detail view cohérent                                  -> valide
état entraînement incohérent                          -> invalide
bonus de base fabriqué                                -> invalide
aucun changement UMG / uasset                         -> confirmé
aucune nouvelle autorité gameplay                     -> confirmé
~~~

## 11. Suite

UI-SKILLS02.2 devra construire réellement FGridSkillDetailView à partir de :

~~~text
FGridSkillsPageView
+ personnage sélectionné
+ URPGSkillAsset canonique
+ URPGSkillPresentationAsset
+ services RPG existants
~~~

et introduire SelectedSkillId dans UGridSkillsWidget comme état de session
UI uniquement.
