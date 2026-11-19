# ARM-CLEAN06.5G.25 — Retrait de l'identité historique du scratch d'authoring

Précondition : 5G.24 validé sur son SHA exact. FGridItemBehaviorParams ne contient plus ItemDefinitionAsset ni ItemDefinitionId. Le scratch conserve uniquement DefaultReadableContentAsset, DefaultReadableContentId, DefaultReadTitleOverride, DefaultReadTextOverride. Les placements d'objets, générateurs et leur identité canonique restent distincts. Plus de dépendance de l'authoring à ces deux champs ; retrait du picker Slate mort. La validation interdit toujours qu'une WorldObjectDefinition représente un collectible.

TD0731/TD0737/TD0738 : audit de la configuration lisible et des autres champs inchangé ; l'absence des deux propriétés est maintenant vérifiée dans le schéma. X.28 conserve les tests de refus de palette liée à l'ancien asset et de stabilité du GUID/N12. Les méthodes réfléchies historiques et DefaultItemDefinition de la palette sont encore présentes : retrait dans des lots séparés après vérification des références Blueprint. Aucun package, WBP ou SaveGame modifié.

## Correctif 5G.25.1

Issue #204 : GridWorldObjectMIG05CollectibleDefinitionTests conserve les tests palette, type Item et absence de lookup runtime, mais vérifie désormais l'absence des deux champs supprimés via réflexion plutôt que d'écrire dans des membres inexistants.
