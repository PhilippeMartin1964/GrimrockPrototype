# UI-LOCALIZATION01.4A — UI String Table CSV importer + integrity tests

Date : **13 octobre 2026**  
Projet : **UE 5.5.4 dungeon crawler prototype**

## Scope

01.4A matérialise le contrat de 01.3 côté source :

- autorité CSV : Localization/Source/UI.csv ;
- parseur/validateur Editor-only ;
- projection exacte vers UStringTable ;
- commandlet de matérialisation ;
- tests d'intégrité et d'idempotence ;
- aucun texte joueur n'est encore migré ;
- aucun .uasset n'est créé hors Unreal.

Le CSV de production est volontairement limité à son en-tête dans 01.4A.
Cela évite de créer une seconde autorité avant la première migration fonctionnelle.

## Identifiants

Les nouveaux identifiants du pipeline restent neutres :

~~~text
Localization/Source/UI.csv
/Game/Localization/StringTables/ST_UI
Game.UI.Localization.LOCALIZATION01.*
~~~

Le validateur refuse case-insensitivement le token protégé défini par 01.3
dans les clés CSV. Les tests vérifient aussi que le chemin source, le package
et le nom de la String Table générée restent neutres.

## Validation

Filtres ajoutés :

~~~text
Game.UI.Localization.LOCALIZATION01.CsvSourceIntegrity
Game.UI.Localization.LOCALIZATION01.CsvValidationContract
Game.UI.Localization.LOCALIZATION01.StringTableProjection
~~~

Le filtre parent :

~~~text
Game.UI.Localization.LOCALIZATION01
~~~

couvre aussi TalentTextSetIntegrity.

## Matérialisation locale — 01.4B

Le binaire ST_UI.uasset doit être produit par Unreal 5.5.4 avec le commandlet
ajouté par 01.4A. Il ne doit pas être synthétisé ou modifié à l'aveugle.

Commande depuis PowerShell :

~~~powershell
& "D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "D:\Development\GrimrockPrototype\GrimrockPrototype.uproject" -run=UIStringTableAuthoring -unattended -nop4 -nosplash -nullrhi
~~~

Résultat attendu :

~~~text
Content/Localization/StringTables/ST_UI.uasset
~~~

Un second lancement sur le même CSV doit indiquer Changed=false.

01.4B sera le commit atomique de matérialisation du .uasset, après build
et validation locale de 01.4A.
