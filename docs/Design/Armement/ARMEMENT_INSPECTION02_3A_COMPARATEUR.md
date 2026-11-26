# ARMEMENT-INSPECTION02.3A — Comparateur et contextes

## 02.3A.1 — Comparateur A/B

Socle accepté : `3b3b71676420ee04878de85242c3c495eb777ba9`, deux statuts Windows SUCCESS. Aucun changement des gestes validés : clic gauche libre 360°, droit/central pan, molette zoom de détail.

- Vue simple par défaut. « Comparer A/B » seulement pour une définition avec niveaux ; B est initialisé à N20. Les deux niveaux sont indépendants, résolus exactement dans le même registre transitoire (aucune instance physique fictive). N0 désactive et ferme B. Fermer B libère sa scène.
- Même présentateur, deux scènes et composants distincts, finitions/MID indépendants. Réutilisation des commandes et de la ligne de niveau pour chaque panneau. Synchro par défaut, désactivable ; à la réactivation A est la référence. Les gestes dans A ou B se propagent sans boucle, sans modifier les équations de navigation. Les boutons d'axes agissent sur A et sur B si lié. Un nouveau niveau conserve le cadrage ; si les géométries diffèrent, chaque vue garde son propre pivot et les limites physiques de sa boîte englobante. Le cadrage peut donc différer pour des bounds différents. Même éclairage studio et exposition fixe ; pas de variation inventée.
- L'inspecteur conserve le détail A et ajoute B en dessous, valeurs réellement résolues. Les liens d'assets restent ceux de A à ce stade. Aucune sauvegarde ni édition implicite. Pas de variation artistique nouvelle.

Test `Grimrock.Editor.Items.Inspection.Compare` : B N1–20, identité, indépendance de A, erreurs, N0, angle/pan/FOV et synchronisation bidirectionnelle, bounds distincts. Le socle PresentationParity couvre les MID indépendants ; pas de preuve de rendu GPU par NullRHI.

## 02.3A.2 — Contextes

Lot suivant, après validation technique de 02.3A.1 : Examen noir existant, sol/décor technique isolé et éclairages communs Studio/Torche/Donjon ; main/vol uniquement sur représentations canoniques effectivement disponibles. Ne pas fabriquer un mesh équipé absent ni présenter une pose arbitraire comme celle du runtime. Captures comparables sous mêmes conditions. Pas de planche artistique inventée.

## Recette native (à effectuer après les deux lots)

Ouvrir l'atelier hors PIE, sélectionner ARM014, comparer N1/N20 puis N1/N2. Manipuler A et B en rotation 360°, pan, zoom de détail ; vérifier synchro et mode libre ; resynchroniser, recadrer, changer les niveaux et objets ; fermer B et tester N0. Contrôler les contextes réellement disponibles, le fond noir, les lumières et les matériaux. Aucune validation esthétique implicite des vingt niveaux.
