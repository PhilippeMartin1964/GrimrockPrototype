# ARM-CLEAN06.5H.71 — Catalogue des actions du joueur : anciennes définitions exclues du Shipping

## Portée du changement

Le `GridTurnManagerPlayerActionCatalog.cpp` générait déjà les actions ARM et non-ARM enregistrées depuis `UGridItemSubsystem`. Il conservait cependant quatre secours `FindItemDefinition` pour des identités absentes du registre : les attaques d'équipement, QuickItems, prérequis/étiquettes de compétences et munitions. Le coup double pouvait également récupérer une ancienne icône depuis `FGridDualWieldSource::Definition`.

Ce lot compile les branches anciennes **uniquement** avec `WITH_DEV_AUTOMATION_TESTS` et les rend exécutables **uniquement** lorsque `GIsAutomationTesting` et qu'aucun registre canonique n'est présent. Dans le jeu normal, ni les actions, ni les prérequis, ni les icônes, ni les besoins de munition ne peuvent être reconstruits depuis une définition historique inconnue. Le pointeur `FGridDualWieldSource::Definition` n'existe plus dans la structure compilée en Shipping.

## Contrats inchangés

Les voies canoniques précèdent toujours les anciennes fixtures : action d'équipement depuis les catalogues uniques, actions non-ARM détaillées, QuickItems et munition N0, prérequis de classe et compétences. L'action universelle mains nues, les coûts PA, deux frappes à 3 PA, 50 % OffHand, nombre de coups, résolution audio et HUD demeurent inchangés. Les anciennes fixtures sans registre continuent à fonctionner sous Automation, mais ne deviennent jamais autorité d'un `ItemId` connu ou inconnu en jeu.

Aucun `.uasset`, `.umap`, `.sav`, WBP, effet sonore, effet visuel ou texte joueur n'est modifié. La chaîne `UI.csv → ST_UI → WBP → Game/fr/en` est inchangée ; les libellés déjà localisés `DualAttackName` et `DualAttackDescription` ne sont pas modifiés. Aucun retirage des 53 packages historiques de H.23.

## Validation

H.72 attend deux statuts `UE5.5.4/SilentTask = SUCCESS` et `UE5.5.4/LocalAuto = SUCCESS` sur le SHA exact H.71, y compris Editor/Shipping et la suite Automation `Grimrock`. Les tests `Grimrock.RPG.ARM.CLEAN06_3.CanonicalDualWieldIndependentLevels`, `Grimrock.RPG.ARMEMENT.LOT02.19.2` et les scénarios MON12 doivent rester positifs. Cela ne remplace ni le contrôle GPU ni l'audit des sept sauvegardes avant suppression.
