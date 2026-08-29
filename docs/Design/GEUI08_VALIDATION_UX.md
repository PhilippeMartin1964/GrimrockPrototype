# GEUI08 — UX de validation

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI08 améliore l’espace de travail dédié `PlayTest & Validation` sans modifier aucune règle de validation.

La source de validation reste :

~~~text
GridEditorLuaService::ValidateCurrentLevelWithLua
~~~

et le modèle `FGridLevelValidationMessage` existant reste celui faisant autorité.

GEUI08 concerne uniquement la présentation, le filtrage et la navigation.

## 2. Bandeau d’état de validation

Après une exécution de validation, le panneau expose maintenant un bandeau d’état bien visible :

~~~text
INVALID
VALID WITH WARNINGS
VALID
~~~

Règles :

- une ou plusieurs erreurs -> `INVALID` ;
- zéro erreur et un ou plusieurs avertissements -> `VALID WITH WARNINGS` ;
- zéro erreur et zéro avertissement -> `VALID`.

Le bandeau conserve également le résumé numérique complet :

~~~text
Errors: N | Warnings: N | Infos: N
~~~

## 3. Recherche

Une zone de recherche de validation est ajoutée directement sous le bandeau d’état.

La recherche est insensible à la casse et correspond à :

- texte du message ;
- catégorie de validation ;
- GUID d’objet optionnel ;
- GUID d’objet source ;
- GUID d’objet cible ;
- coordonnées de cellule ;
- nom/nom affiché de l’arête.

Exemples :

~~~text
door
lua
12,7
north
A1B2C3D4
~~~

La recherche est combinée avec les filtres de sévérité.

## 4. Filtres de sévérité

Les filtres indépendants existants restent disponibles, mais incluent maintenant leurs nombres totaux :

~~~text
Errors (3)
Warnings (5)
Infos (12)
~~~

Cela conserve la possibilité utile d’afficher des combinaisons telles que :

~~~text
Errors + Warnings
Warnings only
Infos only
~~~

au lieu de les remplacer par des onglets mutuellement exclusifs.

Une nouvelle action :

~~~text
Clear Filters
~~~

restaure :

- Errors visible ;
- Warnings visible ;
- Infos visible ;
- recherche vide.

## 5. Nombre de résultats

Le panneau affiche maintenant :

~~~text
Showing X of Y validation messages
~~~

Cela rend explicite l’effet des filtres de recherche/sévérité.

S’il ne reste aucun résultat :

~~~text
No messages match the active filters or search.
~~~

## 6. Navigation existante conservée

GEUI08 réutilise volontairement toutes les actions de message existantes :

- Select Object ;
- Focus Object ;
- Select Source ;
- Focus Source ;
- Select Target ;
- Focus Target ;
- Select Cell.

Aucune seconde implémentation de sélection d’objet ou de focus viewport n’est introduite.

## 7. Ordre des messages

Les messages conservent l’ordre stable déjà établi :

1. Error ;
2. Warning ;
3. Info ;

puis la catégorie à l’intérieur d’une même sévérité.

La recherche/le filtrage ne modifie pas cet ordre déterministe.

## 8. Copy Summary

`Copy Summary` reste inchangé et copie l’exécution complète de validation, pas uniquement le sous-ensemble actuellement filtré.

C’est volontaire : le diagnostic copié dans le presse-papiers reste un rapport technique complet.

## 9. Durée de vie de l’état

La recherche et les filtres de sévérité vivent dans :

~~~text
FGridEditorValidationPanelState
~~~

qui est déjà possédé par l’hôte de l’espace de travail dockable.

Ils survivent donc aux reconstructions de l’espace de travail provoquées par les changements de sélection/contexte éditeur pendant la durée de vie de la fenêtre courante.

Aucun asset gameplay n’est rendu dirty.

## 10. Fichiers modifiés

Modifiés :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorValidationPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorValidationPanel.cpp
~~~

Nouveau :

~~~text
docs/Design/GEUI08_VALIDATION_UX.md
~~~

Aucune source runtime, aucun `.uasset` ni `.umap` n’est modifié.

## 11. Validation UE5.5.4 requise

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Validation visuelle :

1. Ouvrir `PlayTest & Validation`.
2. Exécuter `Refresh Validation`.
3. Confirmer que le bandeau d’état est visible et cohérent avec les compteurs.
4. Confirmer que les contrôles de sévérité affichent des nombres.
5. Désactiver/activer indépendamment Errors, Warnings et Infos.
6. Rechercher un terme connu de catégorie/message.
7. Rechercher une cellule connue sous la forme `X,Y`.
8. Confirmer que `Showing X of Y` se met à jour.
9. Utiliser `Clear Filters`.
10. Tester Select/Focus sur des messages liés à des objets.
11. Confirmer que `Copy Summary` copie toujours l’exécution complète.
12. Redimensionner la fenêtre et confirmer que la partie Validation reste lisible.

## 12. Hors périmètre explicite

GEUI08 ne :

- modifie pas les règles de validation ;
- ne corrige pas automatiquement les problèmes de validation ;
- modifie pas la sémantique de compilation/validation Lua ;
- ajoute pas de second modèle de données de validation ;
- ajoute pas de comportement runtime ;
- modifie pas de `.uasset` ou `.umap` ;
- n’ouvre pas MON21.4.

## 13. Étape suivante

Après validation visuelle/compilation :

~~~text
GEUI09 — Refresh / State cleanup
~~~

GEUI09 consolidera le mécanisme temporaire d’observation/reconstruction du contexte introduit pendant la migration des espaces de travail et réduira les reconstructions complètes inutiles de widgets.

## GEUI08.1 — Préserver le focus de recherche pendant le filtrage en direct

Le gestionnaire de recherche initial de GEUI08 appelait le `RequestRefresh()` du niveau espace de travail à chaque événement `OnTextChanged`.

Cela reconstruisait l’espace de travail complet `PlayTest & Validation` après chaque caractère saisi, détruisant puis recréant le `SSearchBox`. Le symptôme pratique était la perte du focus clavier après le premier caractère.

GEUI08.1 sépare les contrôles stables de validation de la zone dynamique des résultats.

Nouvelles références locales de widgets :

~~~text
ValidationSearchBox
ValidationResultsRoot
~~~

Le comportement de recherche devient :

~~~text
frappe clavier
  -> mise à jour de SearchText
  -> RebuildValidationResults()
  -> remplacement des cartes de résultats uniquement
  -> l’instance SSearchBox reste vivante
  -> le focus clavier reste intact
~~~

Les options de sévérité utilisent maintenant le même rafraîchissement local des résultats au lieu de reconstruire tout l’espace de travail.

`Clear Filters` efface explicitement l’état de recherche persistant et le texte du widget de recherche existant, puis ne rafraîchit que les résultats.

Le rafraîchissement complet de l’espace de travail reste réservé aux opérations qui changent réellement le contexte éditeur ou régénèrent l’exécution de validation.
