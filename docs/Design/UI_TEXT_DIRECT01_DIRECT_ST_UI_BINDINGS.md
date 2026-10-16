# UI-TEXT-DIRECT01 — Direct ST_UI bindings for static UMG text

Date : **16 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**  
État : **VALIDÉ / CLOS — bindings ST_UI directs matérialisés, relais C++ statiques retirés**

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
01.1  contrat + mapping                                      CLOS
01.2  matérialisation WBP_RPGSkillDetail dans UE            CLOS
01.3  matérialisation WBP_RPGTalentDetail dans UE           CLOS
01.4  suppression des relais C++ statiques + tests          CLOS
01.5  régressions / smoke / clôture                         CLOS
~~~

Les `.uasset` sont volontairement édités dans Unreal Editor avec contrôle
visuel. Aucun script aveugle ne réécrit leur sérialisation.

## 9. Validation finale — 16 octobre 2026

Validations locales fournies après retrait des relais C++ :

~~~text
Grimrock.UI.Skills02
    15 succeeded
    0 warnings
    0 failed

Grimrock.UI.RPG.DESC01
    23 succeeded
    0 warnings
    0 failed

Game.UI.Localization.LOCALIZATION03.WbpOwnershipRegression
    1 succeeded
    0 warnings
    0 failed

UI-LOCALIZATION01.6 smoke
    Build / Gather / Export / Import / Compile : OK
    Runtime_Culture_Smoke                     : OK
    source-controlled configs unchanged       : OK
    volatile generated outputs restored       : OK
    final git status --short                   : clean
~~~

Rapport smoke final :

~~~text
Saved/Localization/UI-LOCALIZATION01.6/20261016-144758
~~~

La stop-condition est atteinte :

- les 25 propriétés statiques ci-dessus référencent directement `ST_UI` ;
- aucun setter C++ ne réinjecte leur wording ;
- les `BindWidget` natifs devenus inutiles ont été supprimés ;
- `Button_AcquireTalent` et `Button_ConfirmAcquire` restent liés au C++ pour l'interaction ;
- aucune valeur dynamique/formatée n'a été déplacée dans le Designer ;
- Automation, ownership regression et smoke sont verts ;
- le smoke laisse désormais le working tree propre, y compris pour les artefacts de localisation suivis par Git.

## 10. Commits de la campagne

~~~text
f7d05952 UI-TEXT-DIRECT01.1 define direct ST_UI binding contract
202b31bf UI-TEXT-DIRECT01.2 materialize Skill detail direct ST_UI bindings
799e9824 UI-TEXT-DIRECT01.3 materialize Talent detail direct ST_UI bindings
6758c81f UI-TEXT-DIRECT01.4 remove static C++ text relays
607a3b3b UI-TEXT-DIRECT01.4A relax mechanical glyph manifest guard
f3fd07f4 UI-TEXT-DIRECT01.5A keep localization smoke working tree clean
92d1431b UI-TEXT-DIRECT01.5B handle empty localization snapshots
dd96f6c2 UI-TEXT-DIRECT01.5C make empty snapshot restore scalar-safe
~~~

UI-TEXT-DIRECT01 est **clos**.
