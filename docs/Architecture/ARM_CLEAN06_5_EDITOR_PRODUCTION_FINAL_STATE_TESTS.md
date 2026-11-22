# ARM-CLEAN06.5 — Tests de production, guerrier et preview canonique

## Objectif

Trois groupes de tests Editor d'ancienne création d'ARM utilisent maintenant
la seule autorité installée : DA_ItemSystem / UGridItemSubsystem.
Leurs noms Automation ne changent pas pour préserver le contrôle des régressions.

### 06.2Y

L'ancien CompleteProductionPreview23 ne reconstruit plus de candidats à partir
des 82 profils historiques et ne recharge plus les 23 anciens DataAssets.
Il valide le binaire unique, ses quatre catalogues, les 48 objets distincts,
les 22 armes N1-N20 (440 projections), les 26 items N0, les trois projectiles
spécialisés et leurs visuels, 19 actions ARM et 14 actions N0 détaillées.
La preuve de parité d'authoring original reste dans les rapports antérieurs ;
le test d'état final est désormais indépendant des candidats à supprimer.

### ARMEMENT01.10.3

Les cinq suites Warrior conservent les quatre armes concrètes, le donjon
et la palette, le placement transitoire et son mesh de preview, les transactions
équipement/déséquipement, la réservation de la seconde main de GreatAxe,
la conservation de GUID ainsi que la résolution des actions N1 et N20.
Les instances de test utilisent un monde Game avec ItemSystem configuré.

### LOT02.13

La matrice vérifie les onze armes restantes, leurs 220 tooltips réels, les
règles de main, d'attaque et d'ammo, le parcours bag -> equipment -> bag
et le CSV privé sous Saved. Le PIE manuel reste une recette distincte.

## Sécurité

Pas de modification native, de sauvegarde, de contenu ou de WBP.
Les deux Readables utilisés par les sauvegardes v27 restent protégés.
Le rapport de retraite garde deletion_authorized=false. Tout effacement
des 53 paquets exige un contre-audit sur le SHA exact, puis une procédure
Unreal Editor avec correction des redirectors et validation Cook/Shipping.
