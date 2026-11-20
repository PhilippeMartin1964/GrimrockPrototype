# ARM-CLEAN06.5G.69 — Prévisualisation native des catégories d’énigmes

Parent : eb22e645fd122de1a20700156f6068f15bd9f708. Preuve native : [issue #208](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/208).

Le contrat choisi conserve la catégorie des 22 ARM depuis leur Kind et ajoute un unique tag réservé PuzzleCategory.<Type> à chacun des 26 non-ARM N0. Aucun champ ItemType ajouté au modèle canonique. Le manifeste est uniquement une entrée Editor de migration, pas une table de fallback runtime.

Le plan refuse toute divergence du rapport natif, tout tag réservé préexistant, tout lien de catégorie historique ou lien par tag réservé, tout package sale et toute cible inattendue. Duplique uniquement les non-ARM dans un registre de preview, conserve les catalogues et l'ordre du registre. Le test compare les 466 résolutions et toutes les propriétés réfléchies sauf Tags, dont la différence est contrôlée exactement.

Aucun package, SaveGame, catalogue, niveau ou runtime de réceptacle n'est modifié. Avant matérialisation, adapter les tests de parité historiques et vérifier l'isolation des tags puzzle vis-à-vis des modificateurs de combat. La transaction native reste un lot distinct avec sauvegardes et réouverture.
