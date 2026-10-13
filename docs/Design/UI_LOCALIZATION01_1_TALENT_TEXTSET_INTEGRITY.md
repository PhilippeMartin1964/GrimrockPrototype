# UI-LOCALIZATION01.1 — Talent TextSet integrity

Date : **13 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Statut : **CLOSED**

## Objet

Établir le premier garde-fou de la campagne de localisation en protégeant
l'intégrité du TextSet générique utilisé par la présentation des Talents.

L'objectif n'était pas encore de créer le pipeline de localisation complet,
mais de vérifier qu'une autorité de texte authorée ne contient pas
d'identifiants dupliqués ni d'ambiguïtés structurelles.

## Autorité concernée

Asset de production :

~~~text
/Game/GrimrockPrototype/Core/DataAssets/RPG/DA_RPGTalentDetailTextSet.DA_RPGTalentDetailTextSet
~~~

Type C++ :

~~~text
RPGTalentDetailTextSet.h
~~~

Les noms et descriptions des Talents restent authorés dans les Data Assets
RPG appropriés. Ce TextSet ne remplace pas les autorités de contenu : il porte
du vocabulaire générique de présentation Talent.

## Garde-fou

Test :

~~~text
Game.UI.Localization.LOCALIZATION01.TalentTextSetIntegrity
~~~

Le test protège notamment :

- unicité des Entry Id ;
- absence de doublons ;
- cohérence de l'asset de production attendu.

Deux Entry Id dupliqués ont été retirés de l'asset :

~~~text
Talent.Effect.PhysicalArmorReference
Talent.Screen.SkillPointsUnavailable
~~~

## Commits

Implémentation du garde-fou :

~~~text
d53e43108779bf3b92810e92f9ce045630e70b95
UI-LOCALIZATION01.1 guard Talent TextSet integrity
~~~

Correction de l'asset de production :

~~~text
e8d57f9c99c05bfd85c834ee02bc9003b10c9d41
Remove duplcatire Entry Id
~~~

## Décision de campagne

01.1 a servi de point de départ à la règle ensuite généralisée par 01.2 et
01.3 :

~~~text
contenu RPG authoré       -> Data Assets
vocabulaire UI générique  -> autorité localisable
mécanique                 -> C++
diagnostics techniques    -> C++ / logs
~~~

Le filtre a ensuite été aligné sur le namespace neutre
`Game.UI.Localization` lors de la définition du contrat 01.3.

## Fermeture

Le jalon est fermé. Aucun travail supplémentaire n'est attendu dans
`DA_RPGTalentDetailTextSet` au titre de UI-LOCALIZATION01, hors évolution
fonctionnelle future explicitement décidée.
