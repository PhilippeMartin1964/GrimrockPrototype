# UI-LOCALIZATION02.1 — Localization coverage audit

Date : **13 octobre 2026**  
Projet : **Unreal Engine 5.5.4 dungeon crawler prototype**  
Ticket : **UI-LOCALIZATION02.1**  
État : **AUDIT UNIQUEMENT — aucun changement du pipeline de Gather, aucun asset binaire modifié**

## 1. Objet

Mesurer la couverture réelle du Localization Target `Game` après la fermeture de
`UI-LOCALIZATION01`, puis définir le plus petit périmètre sûr pour
`UI-LOCALIZATION02.2 — Full Game gather`.

Ce ticket ne change aucune autorité de texte. Il ne migre pas de
`LOCTEXT/NSLOCTEXT` vers `UI.csv`, ne modifie aucun `.uasset`, ne produit
pas de traduction anglaise exhaustive et ne modifie pas
`Config/Localization/Game_Gather.ini`.

Baseline de travail vérifiée :

~~~text
master
8c3f9871d5bfa539414e8e0f641d3d2cb6cf0ed3
UI-LOCALIZATION01.DOC close P0 documentation
~~~

Baseline technique P0 fermée :

~~~text
6e5cb5f14f532e2685117e4f803d5ae7a13c6e7e
UI-LOCALIZATION01.12B materialize final P0 localization
~~~

## 2. Architecture de référence conservée

La séparation établie par `UI-LOCALIZATION01` reste valide :

~~~text
contenu RPG authoré
    -> Data Assets

vocabulaire UI générique partagé
    -> Localization/Source/UI.csv
    -> ST_UI générée

LOCTEXT / NSLOCTEXT déjà localisables
    -> C++ tant qu'aucune décision d'authoring ne justifie une migration

mécanique / valeurs
    -> C++

diagnostics techniques
    -> C++ / logs / stable reasons

contenu authoré de niveau
    -> Grid Level / Readable / World Object Data Assets
~~~

La campagne de couverture ne doit pas confondre **mécanisme de localisation**
et **autorité d'authoring**. Un `LOCTEXT` valide n'a pas à être déplacé dans
`UI.csv` uniquement pour être traduit.

## 3. Couverture actuelle du target Game

La configuration actuelle contient une seule étape de collecte :

~~~ini
[GatherTextStep0]
CommandletClass=GatherTextFromAssets
IncludePathFilters=Content/Localization/StringTables/*
PackageFileNameFilters=*.uasset
ShouldExcludeDerivedClasses=false
ShouldGatherFromEditorOnlyData=false
SkipGatherCache=false
~~~

Il n'existe actuellement :

- aucun `GatherTextFromSource` ;
- aucun autre chemin Content dans `GatherTextFromAssets` ;
- aucune collecte des `*.umap`.

Conséquence : le target `Game` couvre **ST_UI uniquement**. Les textes
localisables présents dans les sources C++ et dans les autres assets runtime
sont hors Gather.

## 4. ST_UI / UI.csv

### 4.1 Autorité

Le contrat 01.3 reste inchangé :

~~~text
Localization/Source/UI.csv
    = autorité d'authoring

/Game/Localization/StringTables/ST_UI
    = projection générée
~~~

L'asset `ST_UI` ne devient pas une seconde autorité et ne doit pas être édité
manuellement comme source.

### 4.2 Cohérence observée

État du dépôt audité :

~~~text
UI.csv
    1 ligne d'en-tête
    205 entrées de données

Game.manifest
    namespace : ST_UI
    205 clés
    path unique : /Game/Localization/StringTables/ST_UI.ST_UI
~~~

La projection actuelle est donc cohérente pour la couverture existante.

### Décision

**Conserver sans changement.**  
Priorité : **haute**, car cette couverture doit rester stable lors de 02.2.

## 5. C++ LOCTEXT / NSLOCTEXT

### 5.1 État général

Les macros restantes dans le module runtime sont déjà des autorités Unreal
localisables. Elles sont cependant **non couvertes par `Game`**, car aucune
étape `GatherTextFromSource` n'existe.

L'inventaire par domaine suivant retient les fichiers de production contenant
des surfaces joueur.

### 5.2 Combat

~~~text
Private/Runtime/Combat/GridCombatActionCatalog.cpp
Private/Runtime/Combat/GridCombatLog.cpp
Private/Runtime/Combat/GridPlayerAttackPresentationComponent.cpp
Private/Runtime/Combat/GridPlayerAttackPresentationTypes.cpp
Private/Runtime/Combat/GridTurnManagerComponent.cpp
Private/Runtime/GridLevelRuntimeActorFeedbackUI.cpp
~~~

Surfaces :

- nom/description de l'attaque universelle à mains nues ;
- combat log ;
- raisons de refus d'attaque ;
- noms joueur de repli ;
- formats de feedback.

Classification :

~~~text
player-facing : oui
autorité      : C++ LOCTEXT / NSLOCTEXT
localisable   : oui
Game Gather   : non
action        : conserver l'autorité et Gather depuis Source
priorité      : haute
~~~

### 5.3 Inventaire / actions objet

~~~text
Private/Runtime/GridItemContextActionLibrary.cpp
Private/UI/GridInventorySlotWidget.cpp
Private/UI/GridInventoryWidget.cpp
Public/UI/GridInventoryUiTypes.h
~~~

Surfaces :

- actions contextuelles ;
- types et slots d'équipement ;
- tri ;
- formats et fallbacks de présentation.

Décision : **conserver les macros existantes et les Gather**. Une migration
ultérieure vers `ST_UI` ne se justifierait que pour une vraie volonté de
centraliser un vocabulaire partagé, pas pour corriger la couverture.

### 5.4 Progression / Skills / Talents

~~~text
Private/UI/GridCharacterSheetWidget.cpp
Private/UI/GridSkillEntryWidget.cpp
Private/UI/GridTalentNodeWidget.cpp
Private/UI/RPGProgressionFeedbackService.cpp
~~~

Surfaces :

- points de caractéristiques ;
- rangs et états de Skill ;
- tooltip Talent ;
- feedback de progression.

Décision : **Gather depuis Source**.

### 5.5 Spellbook

~~~text
Private/UI/GridSpellbookEntryWidget.cpp
~~~

Surfaces :

- hints d'assignation ;
- formats génériques du spellbook.

Décision : **Gather depuis Source**.

### 5.6 Startup / chargement

~~~text
Private/UI/GridDungeonBuildProgressWidget.cpp
Private/Runtime/GrimrockStartupModeComponent.cpp
~~~

Surfaces :

- construction du donjon ;
- préparation / chargement ;
- progression runtime.

Décision : **Gather depuis Source**.

### 5.7 Recruitment

~~~text
Private/UI/RPGStoryCompanionRecruitmentWidget.cpp
Private/UI/RPGStoryCompanionRecruitmentWidgetSlate.cpp
~~~

Surfaces :

- Recruter / Refuser / Voir la fiche ;
- états de recrutement ;
- feedback de présentation.

Décision : **Gather depuis Source**.

### 5.8 Carte

~~~text
Private/UI/GridMapWidget.cpp
~~~

Surface :

- format du niveau courant.

Décision : **Gather depuis Source**.

### 5.9 Faux positifs / fichiers à ne pas Gather

La recherche des tokens `LOCTEXT(` / `NSLOCTEXT(` trouve aussi des tests
Editor dont les chaînes testent littéralement la présence ou l'absence de
macros :

~~~text
Source/GrimrockPrototypeEditor/Private/Tests/UILOCALIZATION0112FinalP0ClosureTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/UIRPGCONTENTTEXT051RemainingAuthorityAuditTests.cpp
~~~

Ces occurrences ne sont pas des autorités joueur.

Le module Editor complet doit rester hors du Gather joueur.

## 6. Cas particulier : UENUM DisplayName utilisé au runtime

Deux chemins joueur utilisent actuellement directement le `DisplayName`
réfléchi d'un enum :

~~~text
GridSpellbookEntryWidget.cpp
    EGridSpellSchool
    -> GetDisplayNameTextByValue()

GridSkillEntryWidget.cpp
    ERPGSkillGoverningAttribute
    -> GetDisplayNameTextByValue()
~~~

Ce texte ne passe ni par `ST_UI` ni par un `LOCTEXT/NSLOCTEXT` explicite du
widget.

Cette surface doit être considérée comme **player-facing** et **à vérifier
explicitement** dans 02.2. Il ne faut pas activer un Gather global de metadata
par réflexe : le projet contient de nombreux `UMETA(DisplayName=...)`
strictement techniques ou Editor.

Décision recommandée :

1. effectuer le Full Game gather avec Source + Assets ;
2. vérifier si ces deux familles de labels apparaissent réellement dans le
   manifest joueur ;
3. si elles n'y apparaissent pas, traiter seulement ces usages runtime dans un
   ticket borné, via une autorité locale déjà existante (`LOCTEXT` ou
   `ST_UI`) ;
4. ne pas introduire de nouvelle couche ni de Gather global de metadata sans
   nécessité démontrée.

Priorité : **haute**, car ce sont des surfaces visibles.

## 7. Data Assets RPG / gameplay

Les types de production suivants possèdent des `FText` authorés destinés au
joueur. Ils sont sérialisés dans des assets Unreal et sont donc éligibles à
`GatherTextFromAssets`, mais **aucun n'est couvert actuellement** par
`Game`.

### 7.1 Race

~~~text
URPGRaceAsset
    DisplayName
    Description

6 assets DA_Race_*
~~~

Autorité : Data Asset.  
Action : **Gather, ne pas migrer vers UI.csv**.

### 7.2 Class / Talents

~~~text
URPGClassAsset
    DisplayName
    Description

FRPGClassProgressionChoiceDefinition
    DisplayName
    Description

6 assets DA_Class_*
~~~

Les textes de Choices/Talents restent contenus dans les classes.

Autorité : Data Asset.  
Action : **Gather**.

### 7.3 Skill

~~~text
URPGSkillAsset
    DisplayName
    Description

25 assets DA_Skill_*
~~~

Autorité : Data Asset.  
Action : **Gather**.

### 7.4 Talent presentation / Talent generic text

~~~text
DA_RPGTalentPresentation
    noms / descriptions des branches
    overrides conceptuels de nœuds

DA_RPGTalentDetailTextSet
    vocabulaire générique Talent existant
~~~

Ces deux assets sont actuellement hors `Game`.

Décision :

- les Gather comme assets ;
- conserver leurs autorités actuelles ;
- ne pas faire converger le TextSet vers `ST_UI` dans 02.1/02.2.

### 7.5 Item

~~~text
UGridItemDefinitionAsset
    DisplayName
    Description
    ReadText

21 assets DA_Item_*
~~~

Autorité : Data Asset.  
Action : **Gather**, à l'exception explicite des fixtures de test.

### 7.6 Status Effect

~~~text
UGridStatusEffectDefinitionAsset
    DisplayName
    Description

32 assets DA_Status_*
~~~

Autorité : Data Asset.  
Action : **Gather**.

### 7.7 Spell

~~~text
FGridSpellDefinition
    DisplayName
    Description

4 assets DA_Spell_*
~~~

Autorité : Data Asset.  
Action : **Gather**.

### 7.8 Monster / category

~~~text
UGridMonsterCategoryAsset
    DisplayName
    Description

UGridMonsterDefinitionAsset
    DisplayName
    Description

2 catégories DA_MONCAT_*
2 définitions de monstres de production observées
~~~

Autorité : Data Asset.  
Action : **Gather**.

### 7.9 Story companion

~~~text
URPGStoryCompanionAsset
    DisplayName
    ShortDescription
    RecruitmentConditionText

DA_StoryCompanion_Scout
~~~

Autorité : Data Asset.  
Action : **Gather**.

### 7.10 Quests

Le schéma existe déjà :

~~~text
UGridQuestDefinitionAsset
    DisplayName
    Description

FGridQuestObjectiveDefinition
    DisplayName
    Description
~~~

Aucun `DA_Quest_*` de production n'a été trouvé dans l'arbre audité.

Conclusion :

- la famille est **localisable et doit être couverte lorsqu'elle existe** ;
- le filtre 02.2 sur `Core/DataAssets/*` la couvrira sans ajouter de règle
  spéciale ;
- aucun contenu Quest fictif ne doit être créé pour ce ticket.

## 8. Contenu authored de niveau

Le projet possède déjà du texte joueur authoré au niveau et non du simple
vocabulaire système.

### 8.1 Autorités observées

Les placements de niveau portent notamment :

~~~text
ReadableTextOverride
ReadTitleOverride
ReadTextOverride
~~~

Les définitions de World Object possèdent également :

~~~text
DisplayName
Description
ReadableText
~~~

Les Readable Content Assets possèdent :

~~~text
Title
BodyText
ShortDescription
~~~

Le runtime transporte ces `FText` et les affiche via le flux
`ShowReadableMessage`.

Il s'agit donc d'une autorité joueur réelle, notamment pour :

- inscriptions ;
- notes / livres / parchemins ;
- textes lisibles ;
- contenu narratif posé dans un niveau ;
- overrides propres à un niveau.

### 8.2 Assets de niveau présents

~~~text
DA_GridLevel_00
DA_GridLevel_01
DA_GridLevel_New_Level_03
DA_GridLevel_Pillars_of_Light
~~~

Ces assets sont actuellement hors Gather.

### 8.3 Décision

**Le contenu authored par niveau doit rester dans les Data Assets de niveau et
être Gather par `Game`.**

Il ne doit pas être déplacé vers `UI.csv`, car il ne s'agit pas de vocabulaire
UI partagé.

Les assets actuellement identifiés comme fixtures :

~~~text
DA_Item_TestNote
DA_BookPage_Test
DA_Scroll_Test
~~~

doivent être exclus du target joueur tant qu'ils restent des contenus de test.

## 9. UMG / autres assets

### 9.1 Widgets runtime

L'arbre contient **41 widgets `WBP_*`** sous :

~~~text
Content/GrimrockPrototype/Blueprints/UI/
~~~

Ils couvrent notamment :

- Combat ;
- InGameMenu / Codex / Journal / Map / Recipes / Spellbook ;
- Interaction ;
- Inventory / Character Sheet ;
- Main Menu / Load / Options / Credits / Licenses ;
- Character Creation ;
- Story Companion recruitment ;
- Skills / Talents.

Tous ces packages sont actuellement hors Gather.

### 9.2 Limite de l'audit GitHub

Les `.uasset` sont stockés via Git LFS. Le dépôt Git accessible à l'audit
expose leur pointeur LFS, pas une représentation textuelle fiable des propriétés
Designer sérialisées.

Il serait donc incorrect d'affirmer que les WBP ne contiennent plus aucun
`FText` authoré directement dans UMG.

La bonne opération de mesure est précisément le futur
`GatherTextFromAssets` : il révélera les textes sérialisés réellement
collectables sans modification aveugle des assets.

### 9.3 Runtime Blueprints et maps

Le dépôt contient également :

~~~text
Content/GrimrockPrototype/Blueprints/Runtime/
    15 Blueprints runtime observés

Content/GrimrockPrototype/Maps/
    L_Dungeon.umap
    L_MainMenu.umap
~~~

Ils ne sont pas des autorités textuelles principales, mais leur inclusion est
peu coûteuse et ferme le risque de `FText` sérialisé résiduel.

Décision 02.2 :

- Gather `Blueprints/UI/*` ;
- Gather `Blueprints/Runtime/*` ;
- Gather les deux maps runtime ;
- ne pas Gather `Blueprints/Editor/*`.

## 10. Exclusions explicites

Les familles suivantes ne doivent **pas** entrer dans le target joueur au seul
motif qu'elles contiennent du texte.

### 10.1 Source technique

Exclure conceptuellement :

- `UE_LOG` ;
- diagnostics Save / persistence ;
- validations d'assets ;
- diagnostics runtime ;
- erreurs Lua techniques ;
- commandes PIE / console ;
- simulateurs développeur ;
- raisons techniques non présentées directement ;
- noms d'assets ;
- IDs ;
- fallbacks de contenu invalide ;
- tests Automation.

Chemins à exclure explicitement du Gather Source :

~~~text
Source/GrimrockPrototype/Private/Tests/*
Source/GrimrockPrototype/Private/RPG/RPGProgressionPIESimulator.*
~~~

Le module complet suivant ne doit pas être recherché par le target joueur :

~~~text
Source/GrimrockPrototypeEditor/*
~~~

### 10.2 Content Editor / test

Ne pas inclure :

~~~text
Content/GrimrockPrototype/Blueprints/Editor/*
Content/GrimrockPrototype/Icons/DungeonEditorIcons/*
~~~

Ne pas inclure les fixtures nommées test observées :

~~~text
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_TestNote*
Content/GrimrockPrototype/Core/DataAssets/Readables/*_Test*
~~~

### 10.3 Valeurs mécaniques

Ne pas externaliser par réflexe :

~~~text
+10
-25
10 %
x3
12 / 20
IDs stables
noms internes
notation de tiers
séparateurs purement mécaniques
~~~

## 11. Matrice de couverture

| Source / famille | Player-facing ? | Autorité | Localisable Unreal ? | Couvert actuellement par Game ? | Doit être couvert ? | Action recommandée | Priorité |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ST_UI | Oui | `UI.csv` -> ST_UI | Oui | **Oui** | Oui | Conserver ; protéger le 1:1 CSV/manifest | Haute |
| C++ `LOCTEXT/NSLOCTEXT` runtime | Oui selon occurrence | Source C++ | Oui | **Non** | Oui | Ajouter `GatherTextFromSource` | Haute |
| UENUM `DisplayName` lu par UI | Oui pour 2 usages connus | Metadata C++ | À vérifier dans ce pipeline | **Non constaté** | Oui | Vérifier manifest 02.2 ; pas de Gather metadata global par défaut | Haute |
| Race `DA_Race_*` | Oui | Data Assets | Oui | **Non** | Oui | Gather assets | Haute |
| Class / Talent Choices `DA_Class_*` | Oui | Data Assets | Oui | **Non** | Oui | Gather assets | Haute |
| Skills `DA_Skill_*` | Oui | Data Assets | Oui | **Non** | Oui | Gather assets | Haute |
| Talent presentation | Oui | `DA_RPGTalentPresentation` | Oui | **Non** | Oui | Gather asset, conserver autorité | Haute |
| Talent generic TextSet | Oui | `DA_RPGTalentDetailTextSet` | Oui | **Non** | Oui | Gather asset, pas de migration dans ce ticket | Haute |
| Items `DA_Item_*` | Oui | Data Assets | Oui | **Non** | Oui | Gather, exclure fixtures test | Haute |
| Status Effects `DA_Status_*` | Oui | Data Assets | Oui | **Non** | Oui | Gather assets | Haute |
| Spells `DA_Spell_*` | Oui | Data Assets | Oui | **Non** | Oui | Gather assets | Haute |
| Monster categories / definitions | Oui | Data Assets | Oui | **Non** | Oui | Gather assets | Haute |
| Story companion | Oui | Data Asset | Oui | **Non** | Oui | Gather asset | Haute |
| Quest definitions/objectives | Oui par contrat | Data Assets | Oui | **Non** | Oui lorsqu'ils existent | Couvrir via racine DataAssets ; aucun contenu fictif | Moyenne |
| Level authored readable text | Oui | Grid Level / placement FText | Oui | **Non** | Oui | Gather level assets | Haute |
| Readable / inscription content | Oui | Readable / World Object assets | Oui | **Non** | Oui | Gather assets | Haute |
| WBP runtime | Potentiellement oui | UMG sérialisé | Oui | **Non** | Oui | Gather `Blueprints/UI/*` ; inventorier le delta | Haute |
| Runtime Blueprints | Potentiellement | Blueprint sérialisé | Oui | **Non** | Oui par garde-fou | Gather chemin runtime | Moyenne |
| Runtime maps | Potentiellement | Map package | Oui | **Non** | Oui par garde-fou | Gather `*.umap` runtime | Moyenne |
| Save / persistence diagnostics | Non dans la frontière actuelle | C++ technique | Sans objet pour le joueur | Non | Non | Exclure | Basse |
| Asset/runtime validation | Non | C++ technique | Sans objet pour le joueur | Non | Non | Exclure | Basse |
| Lua technical errors | Non | C++ / Lua technique | Sans objet pour le joueur | Non | Non | Exclure | Basse |
| PIE / developer tools | Non | C++ Editor/dev | Sans objet pour le joueur | Non | Non | Exclure | Basse |
| Tests | Non | C++ tests | Sans objet pour le joueur | Non | Non | Exclure | Basse |
| Editor tools/assets | Non | Editor | Sans objet pour le joueur | Non | Non | Exclure | Basse |
| Mechanical notation | Non linguistique | C++ | N/A | N/A | Non | Conserver en C++ | Basse |

## 12. Recommandation précise pour UI-LOCALIZATION02.2

### 12.1 Principe

02.2 doit combiner :

~~~text
GatherTextFromSource
+
GatherTextFromAssets
~~~

sans changer les autorités de texte.

Le but est de faire de `Game` le target complet du **contenu réellement
joueur**, pas un aspirateur de tout le dépôt.

### 12.2 GatherTextFromSource recommandé

Périmètre candidat :

~~~ini
CommandletClass=GatherTextFromSource
SearchDirectoryPaths=Source/GrimrockPrototype
FileNameFilters=*.h
FileNameFilters=*.cpp
ExcludePathFilters=Source/GrimrockPrototype/Private/Tests/*
ExcludePathFilters=Source/GrimrockPrototype/Private/RPG/RPGProgressionPIESimulator.*
ShouldGatherFromEditorOnlyData=false
~~~

Points importants :

- ne pas ajouter `Source/GrimrockPrototypeEditor` ;
- les raw `TEXT(...)`, logs et diagnostics ne deviennent pas localisables par
  magie : seuls les mécanismes collectables sont concernés ;
- si un futur diagnostic adopte un `LOCTEXT`, sa présence devra être
  reclassifiée plutôt que d'élargir aveuglément le target.

### 12.3 GatherTextFromAssets recommandé

Le plus petit ensemble stable identifié est :

~~~text
Content/Localization/StringTables/*
Content/GrimrockPrototype/Core/DataAssets/*
Content/GrimrockPrototype/Monsters/Categories/*
Content/GrimrockPrototype/Monsters/*/Data/*
Content/GrimrockPrototype/Blueprints/UI/*
Content/GrimrockPrototype/Blueprints/Runtime/*
Content/GrimrockPrototype/Maps/*
~~~

Filtres packages :

~~~text
*.uasset
*.umap
~~~

Exclusions candidates :

~~~text
Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_TestNote*
Content/GrimrockPrototype/Core/DataAssets/Readables/*_Test*
~~~

Les grandes familles suivantes restent hors périmètre par omission :

~~~text
Art/*
Audio/*
Effects/*
Environment/*
Icons/*
Meshes/*
Blueprints/Editor/*
UI/*                 (textures / présentation visuelle hors WBP)
~~~

La racine `Core/DataAssets/*` est volontairement préférée à une liste
manuelle de dizaines de familles : elle correspond à l'autorité gameplay
authorée existante et couvre automatiquement les futures Quest definitions
sans créer une nouvelle règle à chaque type de Data Asset.

### 12.4 Pas de Gather metadata global dans 02.2 initial

Ne pas ajouter par défaut un `GatherTextFromMetaData` large.

Le gain attendu ne concerne actuellement que deux usages runtime connus de
`UENUM DisplayName`, alors que le coût serait de collecter beaucoup de
metadata Editor/technique.

02.2 doit d'abord **mesurer** ce cas dans le manifest produit. Une correction
bornée des deux surfaces est préférable si elles restent hors couverture.

## 13. Critères d'acceptation recommandés pour 02.2

Le prochain ticket devrait prouver au minimum :

1. `ST_UI` reste couvert sans changement d'autorité et ses 205 clés restent
   présentes tant que `UI.csv` n'a pas changé ;
2. le manifest contient des entrées issues des `LOCTEXT/NSLOCTEXT` de
   production ;
3. le manifest contient des entrées issues de familles représentatives :
   Race, Class/Talent, Skill, Item, Status, Spell, Monster, Companion ;
4. au moins un texte authored de niveau/readable est collecté lorsqu'un tel
   texte non vide existe ;
5. les WBP runtime sont réellement parcourus, et le delta de manifest sert
   d'inventaire des éventuels `FText` Designer résiduels ;
6. aucune entrée ne provient de
   `Source/GrimrockPrototype/Private/Tests/*` ;
7. aucune entrée ne provient de `Source/GrimrockPrototypeEditor/*` ;
8. aucun asset `Blueprints/Editor` ou icône d'éditeur n'est aspiré ;
9. les deux usages runtime de `UENUM DisplayName` sont explicitement vérifiés ;
10. le smoke fr/en continue de tester le pipeline, sans être présenté comme
    une traduction anglaise exhaustive.

Une validation locale UE devra être fournie par l'utilisateur avant de
déclarer les tests 02.2 réussis.

## 14. Résultat de UI-LOCALIZATION02.1

La couverture actuelle est **fonctionnelle mais volontairement partielle** :

~~~text
COUVERT
    UI.csv -> ST_UI -> Game

NON COUVERT MAIS DOIT L'ÊTRE
    C++ LOCTEXT / NSLOCTEXT runtime
    Data Assets RPG / gameplay
    Talent presentation / TextSet
    Items / Status / Spells / Monsters / Companion
    contenu authored de niveau / Readables
    UMG runtime
    Blueprints runtime / maps comme garde-fou

À VÉRIFIER SPÉCIFIQUEMENT
    UENUM DisplayName consommé comme texte joueur

À EXCLURE
    tests
    module Editor
    outils PIE
    diagnostics techniques
    Save/persistence diagnostics
    validations
    Lua technical errors
    assets Editor
    fixtures de contenu test
    notation mécanique
~~~

La prochaine étape recommandée est donc bien :

~~~text
UI-LOCALIZATION02.2 — Full Game gather
~~~

avec **deux collecteurs complémentaires, Source + Assets**, dans les périmètres
bornés ci-dessus.

Aucune migration de wording n'est nécessaire pour commencer 02.2.
