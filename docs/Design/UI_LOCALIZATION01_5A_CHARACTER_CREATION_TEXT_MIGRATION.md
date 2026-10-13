# UI-LOCALIZATION01.5A — Character Creation text migration

> **Statut : CLOSED.** Migration : `975188e74f56897621cf272083c8d98f10b4cf68`, correction : `2b2817f9d4bbdfbf767f59a119ad7431b1075d01`. Matérialisation 01.5B : `f962071868cf7098ad9f8117a2f792e81f5f3948`.


Date : **13 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**

## Objet

Migrer le wording joueur de la création de personnage vers l'autorité commune
définie par UI-LOCALIZATION01.3 :

~~~text
Localization/Source/UI.csv
    -> ST_UI générée
    -> FText runtime
~~~

Aucun Data Asset RPG de race, classe ou portrait ne change d'autorité.

## Périmètre

Fichiers runtime migrés :

~~~text
RPGCharacterCreationWidget.cpp
RPGCharacterCreationWizardWidget.cpp
RPGCharacterCreationAttributesStepWidget.cpp
~~~

Le texte joueur n'est plus construit dans ces fichiers avec des phrases
françaises ou anglaises brutes.

Les valeurs mécaniques restent en C++ :

~~~text
-
+N
N / N
valeurs de caractéristiques
IDs
noms saisis par le joueur
~~~

## Résolution runtime

Le helper neutre :

~~~text
FUILocalizationText
~~~

charge la String Table générée :

~~~text
/Game/Localization/StringTables/ST_UI
~~~

et retourne un FText référencé par table/key.

Une clé manquante est une erreur de données : le helper journalise le problème
et retourne un texte vide. Il n'invente pas de prose fallback en C++.

## Services gameplay

Les erreurs techniques de FRPGCustomRecruitService et de la création initiale
ne sont plus recopiées telles quelles dans l'UI.

Le détail technique reste journalisé. Le joueur reçoit une clé localisable
générique après les validations UI spécifiques.

Cela évite notamment l'affichage direct des diagnostics anglais des services.

## CSV

UI.csv reçoit les domaines :

~~~text
CharacterCreation.Gender.*
CharacterCreation.Configuration.*
CharacterCreation.Portrait.*
CharacterCreation.Identity.*
CharacterCreation.Wizard.*
CharacterCreation.Validation.*
CharacterCreation.Submit.*
CharacterCreation.Summary.*
CharacterCreation.Attribute.*
~~~

Les formats utilisent des placeholders nommés.

## Cook

/Game/Localization/StringTables est ajouté à DirectoriesToAlwaysCook car
FUILocalizationText charge ST_UI par chemin runtime.

## Validation

Nouveau test :

~~~text
Game.UI.Localization.LOCALIZATION01.CharacterCreationCsvCoverage
~~~

Il vérifie :

- que UI.csv est valide et non vide ;
- que toutes les clés CharacterCreation.* référencées par les trois sources
  existent dans UI.csv ;
- que les principaux littéraux P0 identifiés par l'audit 01.2 ont disparu des
  trois sources migrées.

## Matérialisation 01.5B

01.5A ne modifie pas le binaire ST_UI.uasset.

Après build et validation source, lancer le commandlet existant :

~~~powershell
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
~~~

Le premier lancement après ce ticket doit indiquer Changed=true.

Le second doit indiquer Changed=false.

Le seul changement binaire attendu pour 01.5B est :

~~~text
Content/Localization/StringTables/ST_UI.uasset
~~~
