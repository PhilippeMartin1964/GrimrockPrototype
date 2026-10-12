# UI-RPG-TEXT01 — Politique texte, localisation et notation mécanique

Date : **12 octobre 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
État : **POLITIQUE ARCHITECTURALE ACTIVE**  
Périmètre : **UI Talent / read-model / localisation**

## 1. Décision

UI-RPG-TEXT01 externalise uniquement le **texte joueur dépendant de la langue**.

Le C++ reste l'autorité des **valeurs mécaniques** et de leur notation universelle :
nombres, signe positif/négatif et symbole de pourcentage.

Cette frontière est volontaire et doit être conservée dans les tickets futurs.

## 2. Ce qui appartient au C++

Le C++ porte la valeur mécanique et sa sémantique numérique.

Exemples de tokens complets rendus par le presenter C++ :

~~~text
10
+10
-25
10%
+10%
-25%
~~~

Le signe `+`, le signe `-` et le symbole `%` font partie de la notation
mécanique. Ils ne sont pas externalisés dans un Data Asset de traduction et ne
sont pas reconstruits par UMG.

`FGridTalentDetailLineView::NumericValue` conserve la valeur brute.
`EGridTalentDetailValueStyle` indique la notation à appliquer :

~~~text
Integer        -> 10
SignedInteger  -> +10 / -10
Percent        -> 10%
SignedPercent  -> +10% / -10%
~~~

`UGridTalentDetailLineWidget::ResolvedValueText` fournit ensuite le token
numérique complet prêt à afficher.

## 3. Ce qui doit être localisable

Tout libellé ou toute phrase qui dépend de la langue doit provenir d'une
autorité éditable/localisable, notamment `URPGTalentDetailTextSet` ou les
Data Assets gameplay qui possèdent déjà un `FText` authoré.

Exemples :

~~~text
Précision
Dégâts infligés
Chance de critique
Multiplicateur de dégâts critiques
Portée
Recharge
Niveau requis
Prérequis
Actions d'équipement
Armes tranchantes
Soi-même
Une fois par round
~~~

Ces textes ne doivent pas être recréés par concaténation de français dans
`GridSkillsPageService.cpp`.

## 4. Rôle du WBP

Le WBP reste responsable du rendu visuel uniquement :

- police ;
- couleur ;
- padding ;
- alignement ;
- wrapping ;
- disposition des colonnes ;
- icônes ;
- visibilité des éléments.

Le WBP ne calcule pas la valeur, n'ajoute pas `+`, `-` ou `%`, et ne
traduit pas un enum en texte joueur.

## 5. Exemple canonique

Pour **Maîtrise critique**, la projection correcte est :

~~~text
Chance de critique                    +10%
Multiplicateur de dégâts critiques    +25%
~~~

`Chance de critique` et `Multiplicateur de dégâts critiques` sont des
textes localisables.

`+10%` et `+25%` sont des tokens mécaniques complets fournis par le C++.

Il est interdit de transformer automatiquement `+10%` en une phrase telle
que « +10 points de pourcentage » au seul motif que la donnée interne s'appelle
`CriticalChancePercentModifier`. Une reformulation linguistique éventuelle
appartient au texte authoré/localisable, pas au formatter numérique.

## 6. Règle pour les futurs tickets TEXT01

Lorsqu'un texte est audité, poser une seule question :

**Est-ce du langage humain à traduire, ou une notation mécanique universelle ?**

- langage humain -> Data Asset / `FText` authoré / ressource localisable ;
- notation mécanique -> C++ / read-model numérique ;
- disposition visuelle -> WBP.

Aucun ticket ne doit déplacer un nombre, `+`, `-` ou `%` vers UMG dans le
but de localisation.
