# ARM-CLEAN06.4X.56 — Audits et authoring compatibles avec la palette sans pointeurs

**Parent :** `859fa3cc79f04d906c6c6b235a64d28bf55e49ac` (X.55 : les deux validations Windows ont réussi).

## Changements

Les entrées directes de `DA_ObjectPalette_Default` conservent les mêmes EntryId, CanonicalItemId, niveaux et catégories. Les tests historiques 4W, X.2, X.4, X.11–14, X.48 et ARMEMENT/LOT02 sont adaptés au passage physique de 33 à 0 pointeurs, tout en contrôlant l'identité du pointeur lorsqu'il existe encore. X.13/X.14 comparent toujours la présentation au registre canonique ; les tests de conflit utilisent un ancien asset synthétique explicite, et ne dépendent plus de la présence résiduelle d'un pointeur réel. X.48 exige désormais les quatre règles X.54B sans pointeur, 33 lignes de palette et un total de 59 (avant) ou 26 (après), sans état mixte. X.55 demeure obligatoire et contrôle la parité des champs et l'intégrité de la projection.

Les inspecteurs historiques ARMEMENT reconnaissent les références déjà enregistrées par ItemId, refusent une identité contradictoire ou un alias et ne recréent pas un pointeur sur les entrées existantes. Ceci ne réautorise pas l'ancien pipeline de création de nouvelles armes, gelé par le contrat normatif. Les fixtures synthétiques historiques de ce pipeline restent testées jusqu'à leur suppression finale ; aucun nouveau pipeline de création historique n'est introduit.

Dans les Details du DataAsset de palette, `DefaultItemDefinition` reste sérialisé mais est en lecture seule. Le champ Icon est conditionné à une définition de mécanisme/objet monde ; aucune icône locale pour un collectible canonique. L'ID canonique reste l'autorité d'authoring. Le mécanisme de normalisation et les tests de réflexion couvrent l'entrée pointerless. Vérification visuelle Details à la prochaine recette PIE/éditeur.

**Aucun asset natif, aucune sauvegarde, aucune UPROPERTY supprimée.** Aucun changement de texte WBP, UI.csv, ST_UI ou Game/fr/en. Ne pas relancer les anciens commandlets de création/migration. Il reste à préparer et valider la transaction native pour le package unique de palette. 06.4 partielle ; 06.5 bloquée.
