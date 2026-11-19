# ARM-CLEAN06.5G.16 — Retrait du schéma des règles de réceptacles

Précondition : parent 4c702e46bc85301f651072771e169a8ca49e7f67 validé par les deux statuts. Les règles de réception sont déjà canoniques dans les assets natifs.

- Retirer l'ancienne propriété réfléchie `FGridReceptacleAcceptedItemConfig.ItemDefinition`. Conserver le seul `CanonicalItemId`, le contrôle d'identité inscrite, les flags/capacités et la priorité des overrides.
- Retirer le helper de conversion historique devenu sans objet. Les tests et audits de lecture des quatre niveaux, 44 définitions, 6 listes, 2 règles (torche/gemme) exigent maintenant exclusivement l'état final. Les trois owners dormants doivent rester vides et l'activation de règles explicites reste testée.
- X.161/X.162/X.164 vérifient les règles inconnues/vides, copie atomique, ordre/doublons, absence de registre, override et absence de propriété réfléchie. X.51 vérifie la copie des quatre règles natives avec les autres propriétés inchangées ; X.48 vérifie l'absence de la propriété et les identités exactes.
- Aucun fichier de contenu UE, SaveGame ou localisation modifié. Le contenu initial des réceptacles reste un modèle physique séparé à traiter ensuite. Validation Windows exacte, puis contrôle visuel en 06.6.
