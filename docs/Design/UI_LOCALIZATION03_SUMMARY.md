# UI-LOCALIZATION03 — WBP text ownership cleanup

Date : **13 octobre 2026**  
Projet : **Unreal Engine 5.5.4 dungeon crawler prototype**  
Ticket : **UI-LOCALIZATION03**  
État : **FERMÉ**

## 1. Objet

UI-LOCALIZATION03 ferme l'audit et le nettoyage de propriété des textes sérialisés
dans :

~~~text
Content/GrimrockPrototype/Blueprints/UI/*
~~~

Le but n'était pas de supprimer tous les `FText` des WBP, mais de supprimer les
**doubles autorités**, les placeholders runtime, le wording actif dans le bytecode
Blueprint et les données authorées placées dans une mauvaise autorité.

Le modèle final reste :

~~~text
ST_UI
    = vocabulaire UI générique réellement partagé

Data Assets
    = contenu RPG / gameplay / narratif authoré

C++ / read models
    = valeurs dynamiques, composition, mécanique, formats runtime

WBP
    = présentation et copy réellement propre à l'écran

glyphes / notation mécanique
    = peuvent rester sérialisés dans les WBP
~~~

## 2. Baseline UI-LOCALIZATION03.1

Commit :

~~~text
bef02e1c49d50d2aa5a61ce51fa3d274be824568
UI-LOCALIZATION03.1 audit serialized WBP text ownership
~~~

Le `Game.manifest` contenait alors :

~~~text
716 occurrences collectées au total

221 occurrences provenant de Blueprints/UI
175 textes source distincts
29 packages WBP

203 occurrences WidgetTree
  6 occurrences Script Bytecode
 12 occurrences Default__ / CDO
---
221
~~~

Classification de l'audit :

| Classe | Occurrences | Décision |
| --- | ---: | --- |
| Placeholders runtime sérialisés | 62 | vider |
| Textes actifs Blueprint Script Bytecode | 6 | sortir du Blueprint |
| Doublons exacts ST_UI existants | 14 | réutiliser ST_UI |
| Vocabulaire générique Talent | 13 | réutiliser le Talent TextSet |
| Contenu authoré illustration race | 12 | supprimer / rattacher à l'autorité correcte |
| Glyphes / notation mécanique | 12 | conserver |
| Autres textes statiques de présentation | 102 | WBP par défaut ; migrer uniquement si partage réel démontré |

Les 102 textes statiques incluaient les 14 occurrences de vocabulaire réellement
partagé traitées ensuite par UI-LOCALIZATION03.2D.

## 3. UI-LOCALIZATION03.2A — placeholders runtime

Commit :

~~~text
93ee64a5e7b94d3be7a106e2280cc657f75cede5
UI-LOCALIZATION03.2A remove runtime WBP text placeholders
~~~

Résultat :

~~~text
62 identités runtime sérialisées supprimées du manifest
~~~

Ces champs sont alimentés par le runtime et ne sont plus des autorités de texte
Designer.

## 4. UI-LOCALIZATION03.2B — frontière Blueprint bytecode

Commits :

~~~text
8ba065fb2eb375a8d5358ff3776fa2bc693c1fe1
UI-LOCALIZATION03.2B1 expose bytecode-free UI presentation texts

9ce13b0056f33b5d967848e93e6dac7aa9fe67a4
UI-LOCALIZATION03.2B2 remove player-facing UI bytecode text
~~~

Les six occurrences player-facing Script Bytecode identifiées par l'audit ont
été sorties des graphes Blueprint.

Le runtime possède désormais les formats / libellés concernés :

~~~text
(Δ {Delta})
({Quantity}x)
Lumière : allumée
Lumière : éteinte
vide
Impossible de charger cette sauvegarde.
~~~

Résultat final :

~~~text
Blueprints/UI [Script Bytecode] : 0
~~~

## 5. UI-LOCALIZATION03.2C — réutilisation des autorités existantes

### 5.1 ST_UI existant

Commit :

~~~text
2c88ca9ab51d738ae970ed311f3560e716a20533
UI-LOCALIZATION03.2C1 reuse existing ST_UI authorities
~~~

14 textes statiques WBP ont été reliés à des clés `ST_UI` déjà existantes,
notamment :

~~~text
Spellbook.School.*
CharacterCreation.Attribute.*
CharacterCreation.Wizard.Step.*
CharacterCreation.Identity.*
CharacterCreation.Portrait.*
~~~

### 5.2 Talent TextSet

Commits :

~~~text
a93999d0b21e17ba3486f33971dc317839dbff6d
UI-LOCALIZATION03.2C2A complete Talent generic text authority

f9169e3b618e91277f1c1db61a2953726c2c2955
UI-LOCALIZATION03.2C2B bind Talent generic text authority

cd8cdadba0e026516b7dcb7d814e0f0b9d499ae3
UI-LOCALIZATION03.2C2C remove Talent WBP text authorities
~~~

Le vocabulaire générique Talent appartient désormais à :

~~~text
DA_RPGTalentDetailTextSet
~~~

Les 13 anciennes autorités WBP ont disparu.

## 6. UI-LOCALIZATION03.2D — vocabulaire WBP partagé

Commits :

~~~text
733280597ebbb2259b3d21274eb35db8fbd04dde
UI-LOCALIZATION03.2D1 author shared WBP vocabulary

9ea823329c5ebc117dd3261d60cff63a1286002c
UI-LOCALIZATION03.2D2 reuse shared WBP vocabulary
~~~

Le recalcul après 03.2C n'a trouvé que six chaînes réellement partagées entre
plusieurs packages WBP.

Autorités ajoutées :

| Clé ST_UI | Source |
| --- | --- |
| `Navigation.Back` | Retour |
| `Menu.LoadGame` | Charger partie |
| `Menu.Options` | Options |
| `Menu.License` | Licence |
| `Inventory.Category.Equipment` | Equipement |
| `Resource.Mana` | Mana |

Ces six clés remplacent exactement 14 anciennes identités WBP.

## 7. UI-LOCALIZATION03.2E — illustration de race

Commits :

~~~text
5a8dc4b8ceb2858b9cd90700cc5991b6d3cd0a84
UI-LOCALIZATION03.2E1 remove unused race illustration display names

96cfb1798b0a377b1c9df6c43c6de1172277e04b
UI-LOCALIZATION03.2E2 remove serialized race illustration display names
~~~

L'analyse a montré que :

~~~text
FRPGRaceIllustrationOption::DisplayName
~~~

n'était lu par aucune logique de production.

L'illustration de race est résolue uniquement avec :

~~~text
RaceId
Gender
Illustration
RaceEnvironment
~~~

Le champ `DisplayName` a donc été supprimé au lieu d'être déplacé vers une
nouvelle autorité inutile.

Les 12 anciennes valeurs sérialisées dans
`WBP_CharacterCreationWizard.Default__...` ont disparu du manifest.

Le nom de race reste authoré par :

~~~text
URPGRaceAsset::DisplayName
~~~

et le genre utilise le vocabulaire localisé existant de création de personnage.

## 8. UI-LOCALIZATION03.3 — garde-fous anti-régression

Commits :

~~~text
deec44303fd716ee2e9e36e7a82eea1b4328f2ca
UI-LOCALIZATION03.3 guard WBP text ownership

f72290e1d04424521293d6370862a1549206da22
UI-LOCALIZATION03.3A fix WBP ownership guard build

74e3c12b49a81dcb696c35d09c6393d291010628
UI-LOCALIZATION03.3B fix ownership guard call sites
~~~

Test ajouté :

~~~text
Game.UI.Localization.LOCALIZATION03.WbpOwnershipRegression
~~~

Il verrouille :

- les 62 chemins de placeholders runtime supprimés ;
- les 4 chemins historiques représentant les 6 occurrences Script Bytecode ;
- les 14 chemins ayant convergé vers ST_UI existant ;
- les 13 chemins Talent ;
- les 14 chemins de vocabulaire WBP partagé ;
- les 12 chemins `AvailableRaceIllustrations.DisplayName`.

Cela représente :

~~~text
119 chemins historiques uniques
121 occurrences historiques supprimées
~~~

Le garde-fou vérifie également que les 12 glyphes mécaniques connus restent
intentionnellement présents :

~~~text
ESC
I K G M J H
↑ ↓
+ -
- / +
~~~

et que du copy local WBP légitime reste autorisé.

La règle est donc explicitement :

~~~text
PAS "zéro FText dans les WBP"
~~~

## 9. Mesure finale

Après UI-LOCALIZATION03.2E :

~~~text
605 occurrences collectées au total

100 occurrences provenant de Blueprints/UI
 99 textes source distincts
 23 packages WBP

100 occurrences WidgetTree
  0 occurrence Script Bytecode
  0 occurrence Default__ / CDO
~~~

Décomposition des 100 occurrences WBP restantes :

~~~text
88 textes statiques locaux au WBP
12 glyphes / notations mécaniques
---
100
~~~

Comparaison avec la baseline :

| Mesure | 03.1 | Final | Delta |
| --- | ---: | ---: | ---: |
| Occurrences Game totales | 716 | 605 | -111 |
| Occurrences Blueprints/UI | 221 | 100 | **-121** |
| Sources UI distinctes | 175 | 99 | -76 |
| Packages WBP collectés | 29 | 23 | -6 |
| WidgetTree | 203 | 100 | -103 |
| Script Bytecode | 6 | 0 | **-6** |
| Default__ / CDO | 12 | 0 | **-12** |

La baisse de 121 occurrences dans `Blueprints/UI` ne signifie pas une baisse
équivalente du total Game : plusieurs textes ont convergé vers des autorités
partagées `ST_UI` ou Data Asset et restent donc correctement collectés ailleurs.

## 10. Validations finales

Validations fournies pendant la campagne :

~~~text
Game.UI.Localization.LOCALIZATION03.SharedWbpVocabulary
    1/1

Game.UI.Localization.LOCALIZATION03.RaceIllustrationOwnership
    1/1

Game.UI.Localization.LOCALIZATION03.WbpOwnershipRegression
    inclus dans le filtre global 03

Game.UI.Localization.LOCALIZATION03
    3 succeeded
    0 warnings
    0 failed

Game.UI.Localization.LOCALIZATION02
    1/1

Grimrock.UI.RPG.CODEAUDIT01.BlueprintCompatibility
    1/1
~~~

Le pipeline de localisation complet a également été exécuté après les
matérialisations WBP :

~~~text
RunLocalizationSmoke.ps1
Runtime_Culture_Smoke : OK
volatile generated outputs restored : OK
~~~

## 11. Architecture finale

### ST_UI

Réservé au vocabulaire générique réellement partagé.

### Data Assets

Conservent le contenu authoré du domaine :

~~~text
DA_RPGTalentDetailTextSet
URPGRaceAsset
URPGCharacterPortraitSetAsset
...
~~~

### C++

Conserve :

- les valeurs runtime ;
- les formats mécaniques ;
- les read models ;
- les messages de présentation directement liés à la logique runtime.

### WBP

Peut conserver :

- du copy propre à un écran ;
- des labels purement locaux ;
- des glyphes / raccourcis mécaniques intentionnels.

Un texte player-facing présent dans un WBP n'est donc pas automatiquement une
dette d'architecture.

## 12. Hors périmètre

UI-LOCALIZATION03 ne transforme pas le smoke anglais en traduction anglaise
complète de production.

Le fixture anglais actuel reste destiné à la validation technique du pipeline.

Une traduction anglaise exhaustive devra être traitée séparément, par exemple :

~~~text
LOCALIZATION-EN01
~~~

## 13. Conclusion

UI-LOCALIZATION03 est fermé.

Le chantier a supprimé les doubles autorités démontrées sans centraliser
artificiellement tout le texte de présentation.

État final :

~~~text
runtime placeholders          : nettoyés
Blueprint Script Bytecode     : 0
doublons ST_UI connus         : supprimés
vocabulaire Talent dupliqué   : supprimé
vocabulaire WBP partagé       : centralisé
DisplayName illustration race : supprimé comme donnée morte
copy WBP local légitime       : conservé
glyphes mécaniques            : conservés
garde-fous                    : actifs
~~~

La prochaine campagne de localisation éventuelle doit donc être un sujet
fonctionnel distinct, et non une poursuite du nettoyage WBP.
