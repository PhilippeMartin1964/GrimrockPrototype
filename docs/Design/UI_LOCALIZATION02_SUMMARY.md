# UI-LOCALIZATION02 — Full Game localization closure summary

Date de clôture : **13 octobre 2026**  
Projet : **Unreal Engine 5.5.4 dungeon crawler prototype**  
Statut : **CLOSED**  
Baseline finale : **fe02113d60e55ffd00e0cb2ead73d18d976b3674**

## 1. Objet de la campagne

UI-LOCALIZATION02 a transformé le target `Game`, initialement limité à
`ST_UI`, en target de localisation joueur couvrant les autorités de texte
runtime réellement utilisées par le projet.

La campagne n'a pas cherché à centraliser tous les `FText` dans une seule
autorité. Elle a conservé la séparation établie par UI-LOCALIZATION01 :

~~~text
vocabulaire UI générique partagé
    -> Localization/Source/UI.csv
    -> ST_UI générée

contenu RPG / gameplay authoré
    -> Data Assets

LOCTEXT / NSLOCTEXT de production
    -> C++ collecté par GatherTextFromSource

contenu authoré de niveau
    -> Grid Level / Readable / World Object Data Assets

valeurs / mécanique
    -> C++

diagnostics techniques / tests / outils développeur
    -> hors target joueur
~~~

Le résultat de la campagne est un pipeline de localisation complet, borné et
régressable, sans activation d'un `GatherTextFromMetaData` global.

## 2. Architecture finale du target Game

Le pipeline final est :

~~~text
Localization/Source/UI.csv
    -> UIStringTableAuthoring
    -> /Game/Localization/StringTables/ST_UI

Source runtime
    -> GatherTextFromSource

Data Assets / UMG / Runtime Blueprints / Maps
    -> GatherTextFromAssets

ensemble
    -> Game.manifest
    -> Game.archive fr/en
    -> Game.po fr/en
    -> Game.locres fr/en
    -> runtime culture smoke
~~~

Culture native : `fr`.  
Culture de smoke : `en`.

`ApplyEnglishSmokeTranslations.ps1` reste un **fixture de validation**. Il ne
constitue pas une traduction anglaise éditoriale complète.

## 3. Couverture finale

### 3.1 Source C++

Le Gather source couvre :

~~~text
Source/GrimrockPrototype
    *.h
    *.cpp
~~~

avec exclusions explicites :

~~~text
Source/GrimrockPrototype/Private/Tests/*
Source/GrimrockPrototype/Private/RPG/RPGProgressionPIESimulator.*
~~~

Le module Editor reste hors périmètre du target joueur.

### 3.2 Assets runtime

Le Gather assets couvre :

~~~text
Content/Localization/StringTables/*
Content/GrimrockPrototype/Core/DataAssets/*
Content/GrimrockPrototype/Monsters/Categories/*
Content/GrimrockPrototype/Monsters/*/Data/*
Content/GrimrockPrototype/Blueprints/UI/*
Content/GrimrockPrototype/Blueprints/Runtime/*
Content/GrimrockPrototype/Maps/*
~~~

Packages collectés :

~~~text
*.uasset
*.umap
~~~

### 3.3 Fixtures de test

Les fixtures ont été regroupées sous une racine dédiée :

~~~text
Content/GrimrockPrototype/Core/DataAssets/Tests/*
~~~

Cette racine est explicitement exclue du target joueur.

Le pipeline ne dépend donc plus de filtres fragiles basés sur des noms
d'assets comme `*_Test*`.

## 4. Corrections de contenu révélées par le Full Game gather

Le Full Game gather a révélé plusieurs problèmes qui n'étaient pas visibles
lorsque le target ne couvrait que `ST_UI`.

### 4.1 Conflit de Localization ID

`DA_FloorCarpet` et `DA_FloorRubble` partageaient un Localization ID de
package, produisant des conflits sur leurs textes authorés.

Le Localization ID de `DA_FloorCarpet` a été réinitialisé via Unreal Editor,
puis la génération a été revalidée.

Le résultat final attendu et protégé est :

~~~text
Content/Localization/Game/Game_Conflicts.txt
    -> vide
~~~

### 4.2 Placeholder UMG `Text Block`

Plusieurs Widget Blueprints de production contenaient encore la valeur par
défaut Unreal :

~~~text
Text Block
~~~

Le Gather la matérialisait sous l'identité :

~~~text
UMG,TextBlockDefaultValue
~~~

Un nettoyage ciblé des `UTextBlock` dont la valeur source était exactement
`Text Block` a été effectué dans les WBP de production.

Le manifest final ne contient plus :

~~~text
TextBlockDefaultValue
"Text": "Text Block"
~~~

### 4.3 Fixtures Readable

Les anciennes exclusions nominatives de `DA_Item_TestNote`,
`DA_BookPage_Test` et `DA_Scroll_Test` ne suffisaient pas à garantir leur
absence du manifest.

Le déplacement sous `Core/DataAssets/Tests/*` fournit désormais une frontière
structurelle et durable.

## 5. Présentation des enums runtime

Deux surfaces joueur dépendaient auparavant directement de metadata réfléchie :

~~~text
EGridSpellSchool
ERPGSkillGoverningAttribute
    -> GetDisplayNameTextByValue()
~~~

Le Full Game gather a confirmé que ces labels n'étaient pas couverts par le
pipeline choisi.

La correction finale reste bornée :

- `ERPGSkillGoverningAttribute` réutilise les clés
  `CharacterCreation.Attribute.*` déjà présentes dans `ST_UI` ;
- `EGridSpellSchool` utilise neuf clés `Spellbook.School.*` ajoutées à
  `UI.csv` ;
- aucun `GatherTextFromMetaData` global n'a été introduit.

Le manifest matérialisé contient notamment :

~~~text
CharacterCreation.Attribute.Dexterity
Spellbook.School.Fire
~~~

sous :

~~~text
/Game/Localization/StringTables/ST_UI.ST_UI
~~~

et ne dépend plus des deux noms d'enums réfléchis pour cette présentation.

## 6. Jalons fermés

| Jalon | Objet | Commit |
| --- | --- | --- |
| 02.1 | Localization coverage audit | `bb4bc45597029f47257b9f8d9a3ac202aa949840` |
| 02.2 | Full Game gather configuration | `4e291bc95571c9d22d3ec8339efff97f2d0c8763` |
| 02.2A | Stable PO smoke identity via `msgctxt` | `3f1ceb6ce595270d3208b66f5a48227ebbdb8f02` |
| 02.2B | Clean Full Game gather / fixtures / UMG placeholders | `832db2b0ccf99765829407da5aa3b5b2456e8da3` |
| 02.2C | Localize reflected enum presentation | `eea73fa7c477545bdd5bb260426cdb6b7399bdb2` |
| 02.2D | Materialize validated Game localization | `656e6778c87eb6a31a41b11ae42de45b2c436fb4` |
| 02.3 | Localization regression guards | `0d74c34584b862f76a45b7269cc26ae982dffd15` |
| 02.3A | Keep localization smoke idempotent | `ee61566d0b1827a7fe7af5460962436b90e90f15` |
| 02.3B | Fix smoke snapshot initialization order | `fe02113d60e55ffd00e0cb2ead73d18d976b3674` |

## 7. Garde-fous finaux

Le filtre Automation :

~~~text
Game.UI.Localization.LOCALIZATION02
~~~

protège notamment :

- présence du Gather source et de ses exclusions ;
- périmètre du Gather assets ;
- exclusion de `Core/DataAssets/Tests/*` ;
- absence de Gather metadata global ;
- présence des clés `Spellbook.School.*` ;
- usage de `ST_UI` pour les deux présentations d'enums ;
- absence de `GetDisplayNameTextByValue()` dans ces deux surfaces ;
- absence de `TextBlockDefaultValue` dans le manifest matérialisé ;
- absence des fixtures de test dans le manifest matérialisé ;
- absence des deux identités d'enums réfléchies dans le manifest ;
- présence des clés UI stables représentatives ;
- `Game_Conflicts.txt` vide.

Le pipeline `RunLocalizationSmoke.ps1` vérifie en plus le
**manifest fraîchement régénéré** juste après le Gather.

Il échoue si réapparaissent :

~~~text
TextBlockDefaultValue
"Text": "Text Block"
Core/DataAssets/Tests/
DA_Item_TestNote
DA_BookPage_Test
DA_Scroll_Test
EGridSpellSchool
ERPGSkillGoverningAttribute
~~~

ou si `Game_Conflicts.txt` n'est plus vide.

## 8. Smoke idempotent et matérialisation volontaire

Un smoke normal doit pouvoir être exécuté sans salir le dépôt :

~~~powershell
.\Scripts\Localization\RunLocalizationSmoke.ps1 `
    -EngineRoot D:\UE_5.5
~~~

Par défaut, le script restaure après validation les sorties volatiles qui
changent seulement à cause des timestamps ou du report de Gather :

~~~text
Content/Localization/Game/Game.csv
Content/Localization/Game/en/Game.po
Content/Localization/Game/fr/Game.po
~~~

La validation locale finale utilisateur a confirmé qu'après le smoke :

~~~text
git status --short
    -> vide
~~~

Pour une matérialisation volontaire des sorties générées, utiliser :

~~~powershell
.\Scripts\Localization\RunLocalizationSmoke.ps1 `
    -EngineRoot D:\UE_5.5 `
    -KeepGeneratedOutputs
~~~

Ce mode est réservé aux tickets qui ont explicitement pour objet de versionner
une nouvelle génération.

## 9. Validation finale

Les validations locales fournies par l'utilisateur ont confirmé :

~~~text
Game.UI.Localization.LOCALIZATION02
    -> succès
    -> 0 warning
    -> 0 échec

Grimrock.UI.RPG06.Skills.Renderer
    -> 3 succès
    -> 0 warning
    -> 0 échec

Runtime_Culture_Smoke
    -> succès
    -> 0 erreur
    -> 0 warning
~~~

Le smoke runtime vérifie notamment :

~~~text
fr CharacterCreation.Attribute.Dexterity -> Dextérité
en CharacterCreation.Attribute.Dexterity -> Dexterity

fr Spellbook.School.Fire -> Feu
en Spellbook.School.Fire -> Fire
~~~

La validation finale de 02.3B a également confirmé le caractère idempotent du
pipeline et un working tree propre après exécution.

## 10. Ce que UI-LOCALIZATION02 ne signifie pas

La fermeture de cette campagne ne signifie pas :

- que toutes les chaînes anglaises du jeu sont traduites ;
- que tous les textes sérialisés dans les WBP ont l'autorité architecturale
  idéale ;
- que chaque texte UI doit être déplacé vers `ST_UI` ;
- que les diagnostics techniques doivent devenir localisables ;
- que le projet doit activer un Gather global de metadata.

Elle signifie que le target `Game` couvre maintenant les autorités joueur
retenues, que les frontières de test sont explicites, que les sorties sont
matérialisées et que les régressions observées pendant la campagne sont
automatiquement surveillées.

## 11. Règle d'ownership conservée

La règle finale reste :

~~~text
ST_UI
    = vocabulaire UI générique partagé

Data Assets
    = contenu RPG / gameplay / narratif authoré

C++ LOCTEXT / NSLOCTEXT
    = texte runtime localisable lorsque C++ est l'autorité naturelle

C++
    = valeurs, calculs, mécanique et diagnostics

WBP
    = présentation
~~~

La présence technique d'un `FText` localisable dans un WBP ne suffit pas à
décider qu'il doit être migré vers `ST_UI`. Cette décision appartient à un
audit d'ownership séparé.

## 12. Prochaine campagne recommandée

~~~text
UI-LOCALIZATION03.1 — Audit serialized player-facing WBP text ownership
~~~

Objectif : inventorier les textes joueur encore authorés directement dans les
Widget Blueprints et les classifier sans modification d'asset au premier
ticket.

Pour chaque occurrence :

~~~text
Asset / Widget
Texte actuel
Autorité actuelle
Autorité cible
Décision
~~~

Classification attendue :

~~~text
vocabulaire UI générique partagé
    -> candidat ST_UI

texte propre à un écran et purement présentation
    -> peut rester WBP

texte dynamique / calculé
    -> C++

contenu RPG / gameplay / narratif
    -> Data Asset
~~~

UI-LOCALIZATION03.1 doit être un **audit uniquement**. Aucune migration UMG ne
doit être effectuée avant d'avoir établi cette cartographie.

## 13. Documents de référence

- `UI_LOCALIZATION01_SUMMARY.md`
- `UI_LOCALIZATION02_1_LOCALIZATION_COVERAGE_AUDIT.md`
- `UI_LOCALIZATION02_2_FULL_GAME_GATHER.md`
- `UI_LOCALIZATION01_12_FINAL_P0_CLOSURE_AUDIT.md`

## 14. Conclusion

`UI-LOCALIZATION02` est fermé.

Le target `Game` n'est plus limité à `ST_UI` : il couvre désormais le
texte joueur de production provenant des sources C++, des Data Assets, des WBP,
des Runtime Blueprints et des maps dans le périmètre décidé.

Les problèmes découverts par cette extension — conflits de Localization ID,
placeholders UMG, fixtures de test et dépendance à des enum DisplayName — ont
été corrigés et protégés par des garde-fous automatiques.

Toute nouvelle migration d'ownership de texte appartient désormais à
`UI-LOCALIZATION03` ou à une campagne ultérieure, et ne doit pas rouvrir
UI-LOCALIZATION02 sans régression explicitement démontrée.
