# ARM-CLEAN06.5G.40 — Kits de départ : registre obligatoire

Parent fbf63c22e28bdeee397ab5c69118ae1ea3b53675. Publication conditionnée aux deux statuts SUCCESS.

- `TryBuildKit` requiert un ItemSystem configuré, refuse les identités non inscrites avant toute projection et conserve le staging atomique. Exception limitée à une Automation active sans registre fourni.
- `CreateInitialCharacter` refuse l’absence de registre dans une vraie GameInstance (y compris PIE) et hors Automation active. Les anciens fixtures sans GameInstance conservent temporairement leur comportement ; les écritures et rollback du cache de départ sont compilés uniquement en Automation.
- Protection explicite contre tout retour inattendu de définitions historiques dans le chemin canonique ; réutilisation des clés de localisation existantes, aucun authoring UMG.
- X24 couvre l’ID historique hors registre ajouté après des entrées valides, puis le registre présent mais non configuré, avec absence de mutations partielles. Les 18 kits et STARTUPFLOW01 restent couverts par les tests existants.

Ce lot ferme l’alimentation du cache par les kits en jeu, sans supprimer les fixtures, les lecteurs historiques ou la déclaration du cache. Aucun asset, SaveGame, schéma ou règle d’équipement modifié. Validation Windows et Shipping sans Automation à confirmer.
