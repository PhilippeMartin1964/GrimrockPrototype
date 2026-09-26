# HOTBAR01 — Simplified General Action Bar

Date : 31.08.2026

## Objectif

HOTBAR01 simplifie la barre d'actions générale afin que la rangée `1–9,0` soit l'unique surface de raccourcis visible. L'ancienne palette d'actions intermédiaire située au-dessus de la barre n'est plus créée ni affichée par le runtime.

## Slot 1 : PrimaryAttack

Le premier slot est réservé au binding système `PrimaryAttack`. Il ne peut pas être effacé, remplacé ni déplacé.

Sa présentation et son exécution sont résolues dynamiquement :

1. si la MainHand fournit une attaque d'équipement, cette attaque et l'icône de l'arme sont utilisées ;
2. sinon, le slot retombe automatiquement sur `Attack_Unarmed` et son icône de poing.

La sauvegarde courante persiste l'intention `PrimaryAttack`, pas l'identité runtime d'une arme particulière. Aucune migration arrière des anciens schémas de hotbar n'est assurée pendant le prototype ; une sauvegarde obsolète doit être supprimée.

## Slots 2–9,0 : configuration directe

Les autres slots restent configurables par glisser-déposer depuis les sources métier : inventaire, spellbook, puis futures pages de capacités/compétences. La palette intermédiaire n'est plus nécessaire.

## Projectiles physiques depuis l'inventaire

Un objet physiquement lançable qui ne fournit pas d'action de combat peut être déposé directement depuis l'inventaire vers un slot configurable. Le binding persistant prend la forme `ThrowItem_<ItemDefinitionId>`.

À l'exécution, le stock, le poids, la Force et `HandUsage` sont réévalués, puis le mode `Cursor_Aim` commun est ouvert. Une unité n'est retirée de l'inventaire qu'après création réussie de `AGridThrownItemActor`.

Quand la quantité tombe à zéro, le raccourci est supprimé automatiquement de la barre. Si le personnage récupère ensuite un nouvel exemplaire, le joueur doit le réaffecter explicitement par glisser-déposer depuis l'inventaire.

## Politique prototype

Aucune compatibilité arrière de sauvegarde n'est maintenue pour les anciens bindings de hotbar supprimés pendant la phase prototype. HOTBAR01.2.1 retire l'ancien binding synthétique de lancer MainHand.

Une attaque de combat de jet et un lancer physique utilitaire restent deux concepts distincts. Les raccourcis physiques actuels utilisent exclusivement `ThrowItem_<ItemDefinitionId>`.


## UI-GLOBALHUD01 — séparation du HUD de combat

La barre d'action générale est désormais une surface permanente de `WBP_GridPersistentHud`, et non un enfant conceptuel de `WBP_GridCombatHud`. Le stockage historique `CombatHotbarSlots` reste temporairement l'autorité unique afin d'éviter une migration de données parallèle.

Le nombre visible n'est plus fixe. Les 12 premiers slots affichent `1 2 3 4 5 6 7 8 9 0 ' ^` pour le profil clavier suisse ; tous les slots suivants sont utilisables à la souris sans label clavier. UI-GLOBALHUD01.1 calcule combien de slots de largeur fixe peuvent tenir dans la largeur restante. Ils sont jointifs en `Auto`, sans `Fill`; un éventuel reliquat reste vide à droite.

Référence : `docs/Design/UI_GLOBALHUD01_PERSISTENT_NAV_ACTION_BAR.md`.


## UI-GLOBALHUD01.1 — capacité dynamique

Le tableau persistant n'est plus ramené à une taille visuelle fixe. Il garantit au moins 12 slots, peut croître selon la largeur du viewport et n'est jamais réduit implicitement. Le nombre de slots affichés est calculé par la surface `UGridPersistentHudWidget`, pas par le système de combat.
