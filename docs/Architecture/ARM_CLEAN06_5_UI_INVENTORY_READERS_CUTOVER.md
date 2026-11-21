# ARM-CLEAN06 — Cutover groupé des lecteurs d'inventaire et de prévisualisation

## Périmètre

Ce regroupement ferme les chemins historiques de six consommateurs: filtre/tri du sac, consultation et consommation des objets, raccourcis, slots et tooltips, comparaisons d'équipement, prévisualisation, diagnostics et transferts au curseur.

Dans le runtime normal, ces modules passent par `UGridItemSubsystem` et les read models canoniques. Les anciennes définitions ne peuvent être recherchées que sous `WITH_DEV_AUTOMATION_TESTS`, si `GIsAutomationTesting` et si aucun registre n'est présent. L'ancien `BuildQuickItemCombatActionDefinition` est exclu du Shipping. Les identités manquantes n'obtiennent aucun fallback de données statiques.

## Validation

Condition obligatoire avant toute publication ultérieure: succès des deux statuts Windows `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` au SHA exact (Editor, Shipping et Automation). Sans mutation de WBP, localisation, assets, niveaux ou SaveGame. Les 53 packages historiques identifiés, en particulier les deux Readables des sauvegardes v27, restent protégés. Retrait natif, audit sur HEAD et recette PIE/Shipping/Save-Continue restent dus.
