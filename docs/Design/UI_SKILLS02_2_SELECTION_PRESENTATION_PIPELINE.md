# UI-SKILLS02.2 — Skill Selection & Presentation Pipeline

Date : **15 octobre 2026**  
Parent : **UI-SKILLS02 — Skill Browser & Detail Panel**  
Prérequis : **UI-SKILLS02.1 validé localement : 3/3 ; MON20.8 SkillsPage 8/8 ; RPG06 Skills 7/7**  
État : **SOURCE IMPLÉMENTÉE — validation locale requise**

## 1. Objectif

Relier le contrat UI-SKILLS02.1 aux autorités runtime existantes sans modifier encore les Widgets Blueprint.

Cette tranche ajoute :
- une sélection de Skill persistante uniquement pendant la session UI ;
- une politique déterministe de sélection par défaut ;
- la construction réelle de FGridSkillDetailView ;
- la résolution des affinités de classes et des synergies ;
- l'exposition du coût canonique d'un rang depuis FRPGSkillPointService ;
- le maintien de la sélection lors des refresh et des changements de personnage.

Aucune nouvelle règle gameplay n'est créée.

## 2. Sélection UI

UGridSkillsWidget expose désormais :

~~~text
SelectedSkillId
SelectedSkillDetail
OnSkillSelectionChanged

SelectSkill()
ClearSkillSelection()
GetSelectedSkillDetail()
~~~

SelectedSkillId est Transient et ne quitte jamais l'UI.

Politique :

~~~text
SkillId préféré encore présent -> conserver
sinon liste non vide           -> première entrée triée
sinon                          -> NAME_None
~~~

Comme la page Skills est déjà triée par DisplayName puis SkillId, la première entrée correspond à l'ordre joueur existant.

Un changement de personnage conserve le même SkillId si ce Skill existe toujours. Aucun état de sélection n'est écrit dans le personnage ou le SaveGame.

Les boutons + et - sélectionnent également le Skill concerné avant d'exécuter la transaction existante. Le clic sur le corps de la ligne sera câblé dans UI-SKILLS02.3.

## 3. Construction du détail

FGridSkillsPageService ajoute :

~~~text
ResolveSkillSelection(...)
TryBuildSkillDetailView(...)
~~~

TryBuildSkillDetailView croise :

~~~text
FGridSkillsPageView
FGridCharacterInventoryState
URPGSkillAsset
FRPGSkillService
FRPGSkillPointService
FRPGSkillCheckService
URPGCharacterRulesLibrary
URPGSkillPresentationAsset optionnel
~~~

La projection vérifie que le PageView correspond bien au CharacterId, ClassId et Level du personnage avant de produire le détail.

## 4. Valeurs gameplay projetées

Le détail expose réellement :

~~~text
Rank
MaxRank
CurrentRankCap
RemainingSkillPoints
NextRank
NextRankCost
bCanIncreaseRank
bCanDecreaseRank
GoverningAttributeValue
GoverningAttributeModifier
BaseCheckBonus
TrainingState
~~~

BaseCheckBonus reste strictement :

~~~text
Rank + GoverningAttributeModifier
~~~

Il ne contient aucun bonus de Talent, de groupe ou de contexte.

## 5. Coût d'un rang

FRPGSkillPointService est désormais l'unique API exposant :

~~~text
GetRankPointCost() -> 1
~~~

Les calculs internes de balance, achat et remboursement utilisent eux aussi cette méthode. Le read model n'encode donc pas une seconde constante UI.

Il s'agit d'une factorisation de la règle existante, sans changement de comportement.

## 6. Rang existant supérieur au plafond actuel

Le plafond de niveau est un plafond d'achat, pas une invalidation d'un état déjà existant.

FGridSkillDetailView accepte donc désormais :

~~~text
Rank > CurrentRankCap
~~~

dans ce cas :

~~~text
bAtCurrentRankCap = true
bCanIncreaseRank = false
~~~

Cette règle reste cohérente avec le read model MON20.8 existant, qui accepte déjà ce cas.

## 7. Métadonnées de présentation

SkillPresentationAsset est un champ Designer optionnel de UGridSkillsWidget.

Tant que DA_RPGSkillPresentation n'est pas authoré :
- le détail gameplay reste constructible ;
- icône, UsageDescription, BuildAdvice, affinités et synergies restent vides ;
- aucun chargement forcé d'un asset inexistant n'est effectué.

Lorsqu'un catalogue est fourni, le pipeline :
- copie Icon, UsageDescription et BuildAdvice ;
- résout RecommendedClassIds et UsefulClassIds vers les DisplayName canoniques ;
- résout SynergySkillIds vers les DisplayName des URPGSkillAsset canoniques ;
- rejette les références impossibles à résoudre au lieu d'afficher des ids techniques.

## 8. Hors scope

UI-SKILLS02.2 ne modifie pas :
- WBP_GridSkills ;
- WBP_RPGSkillEntry ;
- aucun uasset ;
- le rendu visuel d'une ligne ;
- le futur WBP_RPGSkillDetail ;
- les icônes de production ;
- les textes détaillés de production ;
- la formule d'un Skill Check ;
- les règles de points, rangs ou plafonds.

## 9. Tests ajoutés

Le filtre Grimrock.UI.Skills02 couvre maintenant UI-SKILLS02.1 et UI-SKILLS02.2.

Nouveaux cas :

~~~text
detail projection dynamique + métadonnées
training states
rang existant au-dessus du plafond d'achat
rejet projection incohérente
rejet référence de classe inconnue
politique de sélection
API de sélection UGridSkillsWidget
~~~

## 10. Validation locale demandée

~~~powershell
cd D:\Development\GrimrockPrototype
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.Skills02"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.SKILL01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.MON20.8.SkillsPage"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.RPG06.Skills"
~~~

Ne pas considérer UI-SKILLS02.2 comme validé avant réception de ces sorties.

## 11. Suite

UI-SKILLS02.3 — Compact Skill Entry devra :
- ajouter l'icône 48x48 à la ligne ;
- rendre la ligne cliquable ;
- diffuser une demande de sélection vers UGridSkillsWidget ;
- distinguer Normal / Hovered / Selected ;
- condenser le statut d'entraînement ;
- conserver les boutons - / + et leurs transactions actuelles.

Aucun panneau de détail UMG ne sera encore nécessaire avant UI-SKILLS02.4.
