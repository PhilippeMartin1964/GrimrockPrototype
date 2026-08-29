# GEUI05 — Espace de travail Selected Object

**Date :** 28 août 2026  
**Statut :** implémenté sur master — compilation UE5.5.4 et validation visuelle en attente

## 1. Objectif

GEUI05 transforme la fenêtre dockable `Selected Object` en espace de travail focalisé sur l’authoring des objets.

Avant GEUI05, la fenêtre détachée empilait verticalement deux grands panneaux :

~~~text
PROPERTIES
CONNECTORS
~~~

Cela recréait une partie du problème du Toolkit monolithique d’origine à l’intérieur de la nouvelle fenêtre.

GEUI05 remplace cette pile par deux onglets d’espace de travail mutuellement exclusifs :

~~~text
Properties | Connectors
~~~

Seule la page active est affichée.

## 2. Page Properties

La page `Properties` embarque le widget existant :

~~~text
SGridEditorObjectInspectorPanel
~~~

Aucune logique d’édition d’objet n’est copiée ni modifiée.

L’inspecteur existant reste l’autorité pour :

- résumé de l’objet sélectionné ;
- placement et orientation ;
- état initially enabled / active ;
- classification dérivée de l’archétype ;
- champs de composants contextuels ;
- comportement des portes, boutons, leviers et plaques de pression ;
- triggers et receptacles ;
- teleporters et transitions ;
- contenu readable ;
- définitions d’items ;
- propriétés de MonsterSpawn ;
- propriétés de lumière ;
- champs avancés/debug ;
- actions focus / move / reset / apply déjà exposées par l’inspecteur.

## 3. Page Connectors

La page `Connectors` embarque le widget existant :

~~~text
SGridEditorLinksPanel
~~~

Aucune logique Event -> Command ou de condition n’est dupliquée.

La policy/le service de connecteurs existants restent l’autorité pour :

- connecteurs sortants ;
- connecteurs entrants ;
- création de connecteur ;
- événement source ;
- commande cible ;
- conditions ;
- suppression et effacement ;
- sélection d’objet lié ;
- présentation des liens cassés.

Les objets qui ne prennent pas en charge les connecteurs continuent d’afficher le message explicatif existant.

## 4. Comportement des onglets de l’espace de travail

La page sélectionnée est un état de présentation uniquement, stocké par :

~~~text
SGridEditorWorkspaceTab
~~~

avec :

~~~text
EGridEditorSelectedObjectPage::Properties
EGridEditorSelectedObjectPage::Connectors
~~~

La page reste sélectionnée lorsque :

- un autre objet est sélectionné ;
- la cellule sélectionnée change ;
- le contexte de niveau se rafraîchit ;
- l’espace de travail est reconstruit à cause de l’observation de contexte GEUI01 existante.

La fermeture puis recréation de l’onglet Nomad réinitialise encore sur `Properties`, ce qui convient à l’état courant non persistant de l’espace de travail éditeur.

## 5. Organisation visuelle

Le haut de la fenêtre fournit maintenant une barre d’onglets compacte utilisant le même langage visuel d’onglet sélectionné introduit pour la palette GEUI04 :

- fond d’onglet sélectionné ;
- libellé mis en évidence ;
- soulignement cyan ;
- onglets inactifs sombres.

Le contenu en dessous occupe la zone verticale restante et possède son propre défilement.

Cela supprime les en-têtes de section externes dupliqués et évite de forcer Properties et Connectors à se partager l’espace vertical.

## 6. Toolkit historique

Le Toolkit inline historique reste inchangé pour GEUI05 :

~~~text
SELECTED OBJECT
CONNECTORS
~~~

Ces sections sont conservées comme fallback de compatibilité jusqu’à :

~~~text
GEUI06 — Slim Main Toolkit
~~~

Le fallback et l’espace de travail dockable utilisent les mêmes widgets inspecteur/liens existants ; il n’existe donc pas de seconde implémentation d’édition d’objet.

## 7. Fichiers modifiés

Modifiés :

~~~text
Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorWorkspaceTab.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorWorkspaceTab.cpp
~~~

Nouveau :

~~~text
docs/Design/GEUI05_SELECTED_OBJECT_WORKSPACE.md
~~~

Aucune source runtime, aucun `.uasset` ou `.umap` n’est modifié.

## 8. Validation UE5.5.4 requise

Compilation :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipAutomation
~~~

Validation visuelle :

1. Ouvrir `L_GrimrockEditor`.
2. Activer `Grimrock Grid Editor`.
3. Ouvrir `Window > Selected Object`.
4. Confirmer que la rangée supérieure contient exactement :
   - Properties
   - Connectors
5. Confirmer qu’une seule page est visible à la fois.
6. Sélectionner plusieurs types d’objet et vérifier que la page Properties suit la sélection.
7. Passer à Connectors et sélectionner plusieurs objets :
   - les objets compatibles connecteurs affichent l’UI de connecteur existante ;
   - les objets non compatibles affichent le message existant indiquant l’absence de connecteur.
8. Créer/annuler un connecteur via le formulaire existant.
9. Revenir à Properties et vérifier que l’édition d’objet fonctionne toujours.
10. Redimensionner la fenêtre étroite/haute/large et vérifier que la page active défile sans dupliquer la seconde page.
11. Confirmer que les panneaux inline historiques SELECTED OBJECT et CONNECTORS fonctionnent toujours.

## 9. Hors périmètre explicite

GEUI05 ne :

- découpe pas les internes de l’inspecteur en nouveaux modèles de données ;
- modifie pas les champs de comportement des objets ;
- modifie pas la sémantique des connecteurs ;
- modifie pas Event -> Command ;
- ajoute pas de visualisation de graphe de connecteurs ;
- ajoute pas de préférences d’onglets persistantes par utilisateur ;
- supprime pas les panneaux inline historiques ;
- crée pas de plugin ;
- modifie pas le comportement runtime ;
- modifie pas de fichiers .uasset ou .umap ;
- n’ouvre pas MON21.4.

## 10. Étape suivante

Après compilation et validation visuelle :

~~~text
GEUI06 — Slim Main Toolkit
~~~

GEUI06 supprimera enfin du Toolkit principal les sections migrées dupliquées et le réduira au dashboard/en-tête compact de l’éditeur qui ouvre ou résume les fenêtres d’espace de travail dédiées.
