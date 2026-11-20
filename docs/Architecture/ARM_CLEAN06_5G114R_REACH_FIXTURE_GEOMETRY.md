# ARM-CLEAN06.5G.114R — Réparation géométrique de la fixture de portée

Parent `76c5dafa34eeddad4af59f0ffd7bec4d58ed8d7e`, échec unique de `Grimrock.TechnicalDebt.TD02_1.WorldItemsContract`, issue #214. L'intégration canonique 5G114 avait remplacé le mesh vide de la fixture historique par `/Engine/BasicShapes/Cube` (100 cm). Le lecteur de portée `GetClosestWorldPickupPoint` calcule la distance jusqu'au point **le plus proche de la collision**, et non jusqu'au centre. Un cube modifie donc les seuils à 200 et 201 cm que cet ancien contrat cherchait précisément à caractériser.

La réparation retire le mesh visuel des **deux définitions canoniques transitoires de test uniquement**, de sorte que la distance de pickup utilise le centre de l'acteur comme auparavant. Les tests de poids, qualité d'identité, ramassage, plaque et lancer continuent d'exercer les chemins canoniques. Aucune modification du code de production, du placement visuel des véritables objets, de sauvegarde ou d'asset.

**Hypothèse de diagnostic à confirmer par le retour LocalAuto exact de ce commit**, sans suppression d'assertion ni changement de valeur de portée.
