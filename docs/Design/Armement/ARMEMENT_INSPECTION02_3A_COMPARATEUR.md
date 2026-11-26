# ARMEMENT-INSPECTION02.3A — Comparateur et contextes

## 02.3A.1 — Comparateur A/B

Socle accepté : `3b3b71676420ee04878de85242c3c495eb777ba9`, deux statuts Windows SUCCESS. Aucun changement des gestes validés : clic gauche libre 360°, droit/central pan, molette zoom de détail.

- Vue simple par défaut. « Comparer A/B » seulement pour une définition avec niveaux ; B est initialisé à N20. Les deux niveaux sont indépendants, résolus exactement dans le même registre transitoire (aucune instance physique fictive). N0 désactive et ferme B. Fermer B libère sa scène.
- Même présentateur, deux scènes et composants distincts, finitions/MID indépendants. Réutilisation des commandes et de la ligne de niveau pour chaque panneau. Synchro par défaut, désactivable ; à la réactivation A est la référence. Les gestes dans A ou B se propagent sans boucle, sans modifier les équations de navigation. Les boutons d'axes agissent sur A et sur B si lié. Un nouveau niveau conserve le cadrage ; si les géométries diffèrent, chaque vue garde son propre pivot et les limites physiques de sa boîte englobante. Le cadrage peut donc différer pour des bounds différents. Même éclairage studio et exposition fixe ; pas de variation inventée.
- L'inspecteur conserve le détail A et ajoute B en dessous, valeurs réellement résolues. Les liens d'assets restent ceux de A à ce stade. Aucune sauvegarde ni édition implicite. Pas de variation artistique nouvelle.

Test `Grimrock.Editor.Items.Inspection.Compare` : B N1–20, identité, indépendance de A, erreurs, N0, angle/pan/FOV et synchronisation bidirectionnelle, bounds distincts. Le socle PresentationParity couvre les MID indépendants ; pas de preuve de rendu GPU par NullRHI.

## 02.3A.2 — Contextes

Base validée : `8f59bf5417ad2c7b4e64aef7b02a5d84610a536f` (Editor, Shipping, Automation). Le contexte et l'éclairage sont communs à A et B, indépendamment du lien de caméra. Examen reste noir, sans support. Sol et Décor ajoutent le vrai sol `SM_Floor_Stone_01` et, pour Décor, le mur `SM_Wall_Stone_05`, en composants transitoires sans collision ni changement d'échelle. Le support rejoint le bas de la boîte englobante, sans prétendre reproduire le repos physique Chaos. Le mur technique est placé derrière l'objet à 100 cm de sa boîte, hors du calcul de cadrage. Aucune modification de niveau réel. Les trois éclairages utilisent les lumières du preview, avec retour aux valeurs studio initiales ; Torche et Donjon sont des approximations fixes et non une reproduction photométrique du jeu, à apprécier en GPU. Aucune modification des profils Asset Viewer ni des matériaux.

Main (mesh) n'est disponible que pour le mesh équipé d'un émetteur, conformément à `SyncHeldVisualFromSelectedCharacterEquipment` : pas de main/personnage ni de transform d'attache Blueprint supposé. Vol (pose) est limité aux ARM dont le profil projectile est disponible ou le lancer physique générique authoré : mesh, rotation initiale, échelle et axe stable reprennent `GridProjectileVisuals.h` et `GridThrownItemActor.cpp`. Pas de trajectoire, animation, son, effets de vol ou permission d'attaque simulée ; les flèches N0 et le lancer générique non ARM restent hors de ce mode statique. Le mesh affiché est identifié dans l'inspecteur, sans altérer les données canoniques. Si un contexte n'est plus applicable à A, retour à Examen ; B affiche son indisponibilité si sa définition ne permet pas la même projection. En cas de support manquant, message explicite.

Test `Grimrock.Editor.Items.Inspection.Context` : conservation du mesh/transform monde, conditions Main, pose stable et échelle Vol, non-mutation de la session. Pas de validation artistique implicite. Les captures GPU comparables et le rendu des trois éclairages restent une recette native ; pas d'export de planche automatisé dans ce lot, ni de grille de référence facultative. Les supports ne sont pas inclus dans les bounds orbitaux. API moteur : `FPreviewScene::AddComponent/RemoveComponent`, `UDirectionalLightComponent::SetIntensity/SetLightColor`, `USkyLightComponent::SetIntensity`.

## Recette native (à effectuer après les deux lots)

Ouvrir l'atelier hors PIE, sélectionner ARM014, comparer N1/N20 puis N1/N2. Manipuler A et B en rotation 360°, pan, zoom de détail ; vérifier synchro et mode libre ; resynchroniser, recadrer, changer les niveaux et objets ; fermer B et tester N0. Contrôler les contextes réellement disponibles, le fond noir, les lumières et les matériaux. Aucune validation esthétique implicite des vingt niveaux.
