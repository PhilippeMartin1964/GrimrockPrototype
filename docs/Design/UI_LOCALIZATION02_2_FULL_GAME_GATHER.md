# UI-LOCALIZATION02.2 — Full Game gather configuration

Date : **13 octobre 2026**  
Projet : **Unreal Engine 5.5.4 dungeon crawler prototype**  
Ticket : **UI-LOCALIZATION02.2**  
État : **SOURCE CONFIGURATION — validation UE locale requise avant matérialisation**

## 1. Objet

Étendre le Localization Target `Game` aux autorités de texte joueur identifiées
par `UI-LOCALIZATION02.1`, sans changer leur ownership :

~~~text
UI.csv / ST_UI
    -> vocabulaire UI générique

C++ LOCTEXT / NSLOCTEXT
    -> wording C++ déjà localisable

Data Assets
    -> contenu RPG / gameplay authoré

Grid Level / Readables
    -> contenu authoré par niveau

UMG / runtime Blueprints / maps
    -> FText sérialisé runtime éventuel
~~~

Ce ticket ne migre aucun texte vers une nouvelle autorité et ne modifie aucun
`.uasset`.

## 2. Gather Source ajouté

`Game_Gather.ini` utilise maintenant `GatherTextFromSource` sur le module
runtime uniquement :

~~~ini
SearchDirectoryPaths=Source/GrimrockPrototype
FileNameFilters=*.h
FileNameFilters=*.cpp
~~~

Exclusions explicites :

~~~text
Source/GrimrockPrototype/Private/Tests/*
Source/GrimrockPrototype/Private/RPG/RPGProgressionPIESimulator.*
Source/GrimrockPrototypeEditor/*    -> hors SearchDirectoryPaths
~~~

Les `LOCTEXT/NSLOCTEXT` de production peuvent donc rejoindre le manifest sans
aspirer les tests ni le module Editor.

## 3. Gather Assets étendu

Les chemins collectés sont :

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

Fixtures explicitement exclues :

~~~text
DA_Item_TestNote*
Readables/*_Test*
~~~

Les dossiers d'art, audio, effets, meshes, icônes et outils Editor restent hors
périmètre par omission.

## 4. Metadata

Aucun `GatherTextFromMetaData` global n'est ajouté.

Les deux usages runtime connus de `UENUM DisplayName` restent une vérification
de résultat pour le manifest :

~~~text
EGridSpellSchool
ERPGSkillGoverningAttribute
~~~

Si ces labels ne sont pas collectés, leur correction devra rester bornée à ces
surfaces plutôt que d'aspirer toute la metadata technique.

## 5. Garde-fous Automation

Le contrat historique `LOCALIZATION01.LocalizationTargetContract` est adapté :
il continue de protéger les identifiants de localisation neutres sans confondre
ces identifiants avec les chemins historiques du projet.

Nouveau test :

~~~text
Game.UI.Localization.LOCALIZATION02.FullGameGatherContract
~~~

Il protège :

- présence de `GatherTextFromSource` ;
- limitation au module runtime ;
- exclusion des tests et de l'outil PIE ;
- absence de Gather metadata global ;
- présence de tous les scopes assets décidés par 02.1 ;
- collecte de `*.uasset` et `*.umap` ;
- exclusion des fixtures de test ;
- exclusion des Blueprints et icônes Editor.

## 6. Étapes de validation locale attendues

Validation source/configuration :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Game.UI.Localization.LOCALIZATION02"
~~~

Puis pipeline de localisation complet :

~~~powershell
.\Scripts\Localization\RunLocalizationSmoke.ps1 `
    -EngineRoot D:\UE_5.5
~~~

Le second passage doit produire un nouveau `Game.manifest` élargi. Il faudra
alors contrôler au minimum :

- maintien des 205 clés `ST_UI` ;
- présence de textes issus des `LOCTEXT/NSLOCTEXT` runtime ;
- présence des familles Data Assets représentatives ;
- présence de contenu authored/readable lorsqu'il est non vide ;
- delta UMG réel ;
- absence des tests et du module Editor ;
- résultat des deux labels `UENUM DisplayName`.

## 7. Matérialisation

Les fichiers générés suivants ne sont **pas** modifiés dans ce commit source :

~~~text
Content/Localization/Game/Game.manifest
Content/Localization/Game/fr/*
Content/Localization/Game/en/*
~~~

Ils doivent être matérialisés uniquement après exécution réussie du Gather UE
local et inspection du résultat.

Aucun test UE local n'est déclaré réussi tant que sa sortie n'a pas été fournie
par l'utilisateur.

## 8. UI-LOCALIZATION02.2A — premier retour de validation locale

Le premier Full Game gather local du 13 octobre 2026 a confirmé que le test
Automation `LOCALIZATION02` passe et que les deux étapes de Gather se
terminent avec un code de sortie nul.

Le pipeline s'est ensuite arrêté pendant l'application du fixture anglais sur
`CharacterCreation.Wizard.Step.Summary`. Le Full Game gather contient
désormais plusieurs entrées ayant le même `msgid` français `Résumé`, alors
que le fixture historique identifiait une entrée uniquement par son texte
source.

02.2A corrige le fixture pour cibler l'identité PO stable :

~~~text
msgctxt "ST_UI,<Key>"
+
msgid "<SourceString>"
~~~

Le fixture reste volontairement limité aux clés smoke de `UI.csv`; il ne
devient pas une traduction anglaise exhaustive.

Le même Gather a révélé un problème de contenu distinct : `DA_FloorCarpet`
et `DA_FloorRubble` partagent le même package Localization ID, ce qui crée
des conflits sur `DisplayName` et `Description`. Cette correction exige une
modification d'asset dans Unreal Editor et n'est pas effectuée à l'aveugle.

Enfin, `DA_BookPage_Test` et `DA_Scroll_Test` ont été parcourus malgré les
filtres nominaux `*_Test*`. Ce point sera traité séparément : une organisation
des fixtures sous un chemin de test explicitement excluable est préférable à
une nouvelle couche de filtrage ad hoc.

Une nouvelle exécution locale du pipeline complet est requise après 02.2A.
