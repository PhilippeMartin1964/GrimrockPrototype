# ARMEMENT-INSPECTION02.2 — Atelier d'objets V1

**État :** candidat de lot C++/Slate, validation native UE5.5.4 obligatoire avant publication/acceptation. Le rendu GPU n'est pas prouvé par le code ni par NullRHI. La durée de vie de la scène est liée à celle du client de viewport et l'inspecteur est recalculé lors des changements, pas à chaque lecture Slate.

Références : [plan](ARMEMENT_INSPECTION02_PLAN_IMPLEMENTATION.md), [socle](ARMEMENT_INSPECTION02_1_SOCLE_COMMUN.md), [addendum](ARMEMENT_INSPECTION01A_ADDENDUM.md).

## Contenu

- Onglet autonome « Atelier d'objets » via Fenêtre/Window d'Unreal Editor, utilisant le mécanisme de tabs existant ; pas de mode Grimrock Grid Editor requis. L'intitulé et l'emplacement suivent le menu natif existant plutôt que d'introduire un deuxième système de menus. Fermer et désenregistrer l'onglet au shutdown.
- Session transitoire avec le même DA_ItemSystem : liste de toutes les définitions concrètes, filtres Tous/ARM/Équipement non ARM/Autres, recherche par ItemId, nom, ArchetypeId. Sélection initiale ARM014 si présente, niveau entier, N0 masqué pour les objets sans progression. Actualisation explicite et notifications des modifications/transactions de packages du projet. Aucune écriture implicite. La sélection est conservée si encore valide.
- SEditorViewport + FAdvancedPreviewScene, acteur AGridItemPreviewActor dans le monde de preview. Sol et environnement masqués directement sans modifier le profil enregistré, couleur de fond noire, exposition fixe candidate (EV100=0, à ajuster après preuve GPU), contrôle d'orbite natif, molette, recentrage et vues suivant les axes. Pas de décor proche. Cadrage initial par bounds ; recette de zoom et d'aspect encore obligatoire.
- Inspecteur en lecture seule : identité, poids, caractéristiques, niveau, finition effective, mesh, transform, dimensions AABB, slots et paramètres PBR existants, effets persistants et liens vers les sources natives (ouvrir un asset permet une édition réelle et n'est pas un essai). Une erreur de résolution/mesh n'affiche pas l'ancien objet. Pas de prétention à des ornements non raccordés ni à une progression fine déjà authorée.
- Tests de session et sélection via le registre réel. Le rendu, les lumières et la navigation ne peuvent être certifiés par ces tests. Les données de jeu, assets et sauvegardes ne sont pas modifiés.

## Recette

1. Compilation Development Editor + Shipping et Automation sur le SHA exact ; deux statuts Windows SUCCESS.
2. Démarrer UE sans PIE, menu Fenêtre > Atelier d'objets. ARM014 sélectionnée. Vérifier fond noir, métal visible, rotation orbitale, zoom, pan natif, cadrage et boutons d'axes.
3. Parcourir N1–N20 ; les niveaux résolus doivent être exacts. Les finitions actuelles peuvent être identiques dans une bande, limitation explicitement signalée ; INSPECTION02.3B reste ouvert.
4. Rechercher ARM075, clé, pierre, livre/parchemin disponibles ; absence de contrôle de niveau pour N0. Vérifier le mesh réel et les données, relever les meshes manquants. Ouvrir une source native sans sauvegarder.
5. Actualiser, changer de sélection et fermer/réouvrir l'onglet. Aucun asset modifié par consultation, aucune interaction avec le donjon ou sauvegarde.

Pas de comparateur, de simulateur ponctuel ni d'écriture transactionnelle dans cette V1. Ces fonctions restent dans les lots suivants. Les titres d'éditeur Slate sont propres au module éditeur ; aucun WBP, ST_UI ou texte runtime n'est modifié. Le chemin de rendu est fondé sur les APIs natives UE5.5 FAdvancedPreviewScene::SetFloorVisibility / SetEnvironmentVisibility et FEditorViewportClient ; les signatures et l'éclairage sont à confirmer par compilation et recette sur l'installation réelle.

## Correction de compilation — 26.11.2026

Le diagnostic Windows [#242](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/242) sur `7f5edd09` indique C1083, ligne 9 de `SGridItemWorkshop.cpp`. Les delegates CoreUObject sont déclarés dans `UObject/UObjectGlobals.h` sous UE5.5, et les événements de transaction dans `Misc/TransactionObjectEvent.h`, comme dans le bridge Undo/Redo existant. Remplacement ciblé de l'en-tête inexistant ; aucune modification des fonctionnalités, des assets ou de la couverture Automation. Le lot corrigé doit encore obtenir les deux statuts Windows sur son SHA exact.

## INSPECTION02.2A — Orbite centrée sur l'objet

Le propriétaire a accepté la V1 et signalé que la navigation native faisait pivoter la caméra sur elle-même. Le hotfix retire l'orbite native comme autorité de navigation. Une surface Slate transparente et capturante reçoit le clic gauche, le déplacement et la molette au-dessus du viewport ; le clic droit n'active aucun mode libre. Le mesh et son transform ne changent pas. `FGridItemWorkshopOrbit` maintient la cible sur `Bounds.Origin`, conserve angle et distance lors du changement de niveau du même objet, et recalcule la position de la caméra sur la sphère orbitale. Recentrer et les vues d'axes réinitialisent le cadrage. Les limites de zoom tiennent compte du rayon et d'une marge de proximité ; le pitch est borné à ±89°. Aucun offset de pivot authoré ni pan n'est ajouté dans ce lot.

Test `Grimrock.Editor.Items.Inspection.Orbit` : cible excentrée, rayon constant, visée, conservation au changement de niveau, limites de zoom/pitch et recadrage d'un petit objet. La capture réelle de la souris, le cadrage et la fluidité restent à contrôler dans l'onglet natif sur ARM014, bouclier et objet N0. Pas de modification de catalogue, WBP, asset ou sauvegarde. La compilation Windows sur le SHA du hotfix est nécessaire avant la recette.

## INSPECTION02.2B — Rotation libre 360° et sens naturel de la souris

La recette du hotfix 02.2A a révélé les deux sens de déplacement inversés et la limitation verticale à ±89°. L'orientation orbitale est maintenant un quaternion unitaire, composé à chaque déplacement autour des axes locaux haut et droite de la caméra. Le sens est celui d'un objet saisi : sa face visible suit la souris à droite/gauche et haut/bas. Les axes restent relatifs à l'écran même lorsque l'objet apparaît à l'envers. Aucun clamp de tangage ni verrouillage du roulis ; la rotation passe les deux pôles, le dos et 360° sans réinitialisation. Le roulis résultant d'une succession de rotations libres est normal ; Recentrer restaure la vue initiale. Le quaternion est converti en rotateur uniquement pour transmettre la pose au viewport. Le clic gauche utilise le mouvement souris haute précision Slate pendant la capture afin de ne pas limiter l'orbite au bord du viewport. Clic droit, zoom, pivot géométrique et conservation du cadrage par niveau inchangés.

Le test Orbit couvre maintenant les quatre directions par projection dans le repère écran, deux tours complets avec passages de pôles (contrôle de continuité, conversion rotateur et conservation du centre), et le sens horizontal lorsque la vue est retournée. Le test natif souris/GPU reste obligatoire. Aucun asset, catalogue, runtime de jeu ou sauvegarde modifié.
