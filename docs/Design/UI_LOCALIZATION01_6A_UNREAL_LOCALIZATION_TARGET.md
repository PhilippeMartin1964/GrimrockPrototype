# UI-LOCALIZATION01.6A — Unreal Localization Target + English smoke infrastructure

Date : **13 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**

## Objet

Brancher la String Table générée ST_UI sur le pipeline natif de localisation
Unreal et préparer un smoke test réel fr -> en.

## Target

~~~text
Target        Game
Native        fr
Foreign       en
Config        Config/Localization/Game_*.ini
Data          Content/Localization/Game/
~~~

Pour 01.6, le gather est volontairement limité à :

~~~text
Content/Localization/StringTables/*
~~~

Le but est de valider le nouveau pipeline avant de l élargir aux anciens
LOCTEXT, Data Assets et autres contenus.

## Pipeline

~~~text
ST_UI.uasset
   -> Gather
   -> Game.manifest / Game.archive
   -> Export Game.po
   -> traduction anglaise de smoke
   -> Import
   -> Compile
   -> fr/Game.locres + en/Game.locres
   -> changement de culture runtime
~~~

## Packaging/runtime

Config/DefaultGame.ini charge Content/Localization/Game au runtime et stage
explicitement fr et en. Le preset EFIGS fournit les données
d internationalisation nécessaires à ces deux cultures.

## Smoke anglais

Le script Scripts/Localization/ApplyEnglishSmokeTranslations.ps1 modifie
uniquement le Game.po anglais exporté et traduit cinq entrées simples.
Ce script est un fixture de validation, pas une autorité de traduction
production.

Le commandlet -run=UILocalizationEnglishSmoke vérifie ensuite le texte source
français et cinq traductions anglaises, puis restaure l état de culture.

## Exécution locale

~~~powershell
.\Scripts\Localization\RunLocalizationSmoke.ps1 -EngineRoot D:\UE_5.5
~~~

Le script resynchronise ST_UI, gather, exporte les PO, applique le fixture
anglais, importe, compile les locres et lance le smoke runtime.

## Validation source

~~~text
Game.UI.Localization.LOCALIZATION01.LocalizationTargetContract
~~~

Ce test valide la culture native fr, les cultures fr/en, les noms neutres,
le gather limité à ST_UI, export/import PO, compilation locres et staging.

## 01.6B

Après réussite locale, les artefacts générés sous Content/Localization/Game/
seront inspectés puis committés séparément. Aucun manifest, archive, PO,
locmeta ou locres n est fabriqué manuellement dans 01.6A.
