# HOTBAR01 — Simplified General Action Bar

Date : 31.08.2026

## Objectif

HOTBAR01 simplifie la barre d'actions générale afin que la rangée `1–9,0` soit l'unique surface de raccourcis visible. L'ancienne palette d'actions intermédiaire située au-dessus de la barre n'est plus créée ni affichée par le runtime.

## Slot 1 : PrimaryAttack

Le premier slot est réservé au binding système `PrimaryAttack`. Il ne peut pas être effacé, remplacé ni déplacé.

Sa présentation et son exécution sont résolues dynamiquement :

1. si la MainHand fournit une attaque d'équipement, cette attaque et l'icône de l'arme sont utilisées ;
2. sinon, le slot retombe automatiquement sur `Attack_Unarmed` et son icône de poing.

La sauvegarde persiste donc l'intention `PrimaryAttack`, pas l'identité runtime d'une arme particulière. Lors du chargement d'une ancienne sauvegarde, un ancien raccourci occupant le slot 1 est déplacé vers le premier slot libre parmi `2–9,0` lorsque c'est possible.

## Slots 2–9,0 : configuration directe

Les autres slots restent configurables par glisser-déposer depuis les sources métier : inventaire, spellbook, puis futures pages de capacités/compétences. La palette intermédiaire n'est plus nécessaire.

## Projectiles physiques depuis l'inventaire

Un objet physiquement lançable qui ne fournit pas d'action de combat peut être déposé directement depuis l'inventaire vers un slot configurable. Le binding persistant prend la forme `ThrowItem_<ItemDefinitionId>`.

À l'exécution, le stock, le poids, la Force et `HandUsage` sont réévalués, puis le mode `Cursor_Aim` commun est ouvert. Une unité n'est retirée de l'inventaire qu'après création réussie de `AGridThrownItemActor`.

Quand la quantité tombe à zéro, le raccourci est supprimé automatiquement de la barre. Si le personnage récupère ensuite un nouvel exemplaire, le joueur doit le réaffecter explicitement par glisser-déposer depuis l'inventaire.

## Compatibilité

Les anciens bindings `ThrowMainHand` restent compris afin de ne pas invalider les sauvegardes existantes. Une attaque de combat de jet et un lancer physique utilitaire restent deux concepts distincts.
