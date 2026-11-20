# ARM-CLEAN06.5G.110 — Équipement canonique

Parent `e8030b4dc57bbc16e47f3969ff39b526fbb71ad4`. TD06.6 utilise le registre transitoire pour les emplacements, bonus et résistances non-ARM, consommation de piles équipées et refus de déséquipement en inventaire plein. La source inconnue est injectée directement comme état sauvegardé invalide afin de conserver la vérification du garde d'équipement ; l'entrée normale refuse désormais cette source. La politique de main est explicitement authorée (plus d'Auto historique), et les deux emplacements restent testés. Aucun changement de runtime.
