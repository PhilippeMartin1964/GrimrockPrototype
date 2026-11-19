# ARM-CLEAN06.5E.2 — Contributions et profils d'attaque sans ancienne définition

Parent `83f3643af6fb85e78ae70e919367e4698e5f6516` : deux statuts Windows SUCCESS.

## Périmètre
- Le profil offensif canonique de l'attaque directe et empruntée provient de FGridCanonicalEquippedAttackReadModel, déjà utilisé par les actions équipées et le dual wield. Pas de seconde reconstruction des dés.
- Les tags de l'arme inscrite proviennent de sa vraie instance équipée et du registre, même si la liste canonique est vide. Un QuickItem inscrit conserve les tags de l'action projetée. Seules les identités non inscrites peuvent encore lire une ancienne définition exacte.
- Les effets de statut et d'armure de l'attaque directe inscrite proviennent de cette action standard. Les effets explicites des actions de classe ou autres overrides restent intacts. Aucun effet compact avancé non implémenté ne peut être silencieusement omis : le modèle standard refuse Effects non vide ou coefficient WeaponDamagePercent non nul.
- Le test natif des 22 armes vérifie les vrais chemins, les trois passifs, les 19 attaques sans effet de statut/armure historique ni action supplémentaire, les tags préservés et les actions compactes sans effet non projeté. Le test de dual wield vérifie la parité de la projection autonome. Ces preuves sont exigées avant toute suppression de fichier ancien.

## Hors périmètre
La commande historique de diagnostic directe conserve son calcul de coût/quantité existant ; les chemins HUD restent autoritaires pour les actions jouables. Pas de modification du vol des munitions ni des transactions de projectile. Le repli de vitesse de munition et les autres lecteurs historiques restent à examiner avant retrait global de RehydrateOwnedItemDefinitions. Aucune écriture d'asset, SaveGame v27, WBP ou localisation. Deux statuts exact-SHA nécessaires, recette PIE et Shipping non attestées.
