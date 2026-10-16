# UI-TEXT-DIRECT01 — Direct ST_UI bindings for static UMG text

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **01.1 CONTRAT OUVERT — matérialisation UMG manuelle requise**

## 1. Décision

Le projet possède déjà le bon mécanisme Unreal pour consommer un texte statique
centralisé : une propriété `FText` d'un widget peut référencer directement une
entrée de `ST_UI` depuis le Designer.

Le contrat devient :

~~~text
Localization/Source/UI.csv
    -> UIStringTableAuthoring
    -> ST_UI

ST_UI
    ├──> WBP directement
    │      pour les textes strictement statiques
    │
    └──> FUILocalizationText
           pour les textes dynamiques, conditionnels ou formatés
~~~

`UI.csv` reste l'unique autorité d'authoring du vocabulaire central. Le WBP
ne copie pas le français : il conserve la référence `String Table + Key`.

## 2. Pourquoi retirer le relais C++ statique

Le pattern suivant est désormais interdit lorsqu'il ne sert qu'à recopier un
texte fixe :

~~~text
ST_UI
    -> FUILocalizationText::Get(...)
    -> NativeConstruct()
    -> TextBlock->SetText(...)
~~~

Il ajoute un binding C++ sans apporter de logique.

De plus, en UE 5.5, `UTextBlock::SetText()` remplace le contenu de la propriété
Text. Il ne faut donc pas conserver simultanément une référence String Table
Designer et un setter runtime sur la même propriété.

## 3. Règle d'ownership

| Nature du texte | Autorité / consommation |
| --- | --- |
| label/titre/bouton/tooltip statique avec clé ST_UI | WBP -> ST_UI direct |
| texte conditionnel | C++ / read-model -> ST_UI si nécessaire |
| format avec valeurs runtime | C++ + FText::Format + pattern ST_UI |
| valeur purement mécanique | C++ / read-model |
| nom/description Skill, Talent, Item, Spell, Recipe | Data Asset du domaine |
| copy local intentionnel sans clé centrale | WBP local autorisé |
| glyphe/raccourci mécanique | WBP local autorisé |

Règle absolue : **une seule autorité par propriété FText**.

## 4. Précédent déjà présent

UI-LOCALIZATION03.2C1 et 03.2D2 ont déjà relié des textes statiques WBP à
`ST_UI`, notamment des attributs de Character Creation et du vocabulaire
partagé. La propriété « Charisme » observée dans le Designer avec :

~~~text
String Table : ST_UI
Key          : CharacterCreation.Attribute.Charisma
~~~

est donc conforme au contrat et n'est pas une nouvelle technique introduite par
ce ticket.

UI-TEXT-DIRECT01 formalise cette pratique et retire les relais C++ statiques
restants là où ils sont inutiles.

## 5. Migration pilote — WBP_RPGSkillDetail

Ces 17 propriétés doivent référencer directement `ST_UI` :

| TextBlock | Clé |
| --- | --- |
| Text_LabelDescription | UI.Section.Description |
| Text_LabelTraining | Skill.Section.Training |
| Text_LabelUsage | UI.Section.Usage |
| Text_LabelBuildAdvice | Skill.Section.BuildAdvice |
| Text_LabelCheck | Skill.Section.Check |
| Text_LabelCheckRank | Skill.Check.Rank |
| Text_LabelCheckAttributeModifier | Skill.Check.AttributeModifier |
| Text_LabelCheckBaseBonus | Skill.Check.BaseBonus |
| Text_LabelAffinities | Skill.Section.Affinities |
| Text_LabelRecommendedClasses | Skill.Affinity.RecommendedClasses |
| Text_LabelUsefulClasses | Skill.Affinity.UsefulClasses |
| Text_LabelSynergies | Skill.Section.Synergies |
| Text_LabelProgression | UI.Section.Progression |
| Text_LabelCurrentRankCap | Skill.Progression.CurrentRankCap |
| Text_LabelRemainingSkillPoints | Skill.Progression.RemainingSkillPoints |
| Text_LabelNextRank | Skill.Progression.NextRank |
| Text_LabelNextRankCost | Skill.Progression.NextRankCost |

Après matérialisation, `UGridSkillDetailWidget` ne doit plus posséder
`ApplyLocalizedStaticLabels()` ni de `BindWidget` natif uniquement destiné
à ces 17 labels.

## 6. Migration pilote — WBP_RPGTalentDetail

Ces 8 propriétés doivent référencer directement `ST_UI` :

| TextBlock | Clé |
| --- | --- |
| Text_LabelType | Talent.Section.Type |
| Text_LabelStatus | Talent.Section.Status |
| Text_LabelPrinciple | Talent.Section.Principle |
| Text_LabelEffects | Talent.Section.Effects |
| Text_LabelUsage | UI.Section.Usage |
| Text_LabelAcquisition | Talent.Section.Acquisition |
| Text_AcquireTalent | Talent.Action.Acquire |
| Text_ConfirmAcquire | Talent.Action.Confirm |

Les boutons restent des `BindWidget` natifs car ils portent de l'interaction.
Seuls leurs enfants texte statiques quittent le contrat C++.

## 7. Procédure UMG

Pour chaque TextBlock :

1. ouvrir le WBP dans Unreal Editor ;
2. sélectionner le TextBlock ;
3. dans `Content > Text`, utiliser le drapeau / menu de localisation ;
4. choisir `Referenced Text` ;
5. choisir `ST_UI` ;
6. choisir la clé indiquée ci-dessus ;
7. sauvegarder puis compiler le WBP.

Ne pas modifier le texte directement dans `ST_UI.uasset`. Toute reformulation
se fait dans `Localization/Source/UI.csv`, puis via le materializer.

## 8. Phasage sûr

~~~text
01.1  contrat + mapping
01.2  matérialisation WBP_RPGSkillDetail dans UE
01.3  matérialisation WBP_RPGTalentDetail dans UE
01.4  suppression des relais C++ statiques + tests
01.5  régressions / smoke / clôture
~~~

Les `.uasset` sont volontairement édités dans Unreal Editor avec contrôle
visuel. Aucun script aveugle ne réécrit leur sérialisation.

## 9. Validation finale cible

~~~text
Grimrock.UI.Skills02
Grimrock.UI.RPG.DESC01
Game.UI.Localization.LOCALIZATION03.WbpOwnershipRegression
UI-LOCALIZATION01.6 smoke
~~~

La stop-condition est atteinte lorsque :

- les 25 propriétés statiques ci-dessus référencent directement `ST_UI` ;
- aucun setter C++ ne réinjecte leur wording ;
- les `BindWidget` natifs devenus inutiles ont été supprimés ;
- aucune valeur dynamique/formatée n'a été déplacée dans le Designer ;
- Automation et smoke restent verts.
