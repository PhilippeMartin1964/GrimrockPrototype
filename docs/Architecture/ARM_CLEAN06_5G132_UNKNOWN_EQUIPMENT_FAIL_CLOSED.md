# ARM-CLEAN06.5G.132 — Rejet des emplacements d'équipement hérités hors registre

**Parent** : `24791da73c2a392c652f581dd50592c140653006` (PIE création de personnage validée par le propriétaire, Windows SUCCESS/SUCCESS).

## Diagnostic

Deux chemins d'exécution `UGridPartyInventoryComponent::IsTwoHandedItem` et `CanEquipItemToSlot` retombaient sur `UGridItemDefinitionAsset` dès que le `ItemId` n'était pas présent dans le `UGridItemSubsystem` courant. La projection `FGridCanonicalEquipmentSlotsReadModel` autorisait le même repli dans un monde de test même si un registre canonique était configuré. Résultat potentiel : un objet inconnu reçoit une permission de main ou de slot à partir d'une définition historique.

## Correction

- L'identité connue conserve le lecteur canonique, ses niveaux N0 et N1–N20 et sa politique de main.
- Une identité inconnue refuse toute attribution de slots et toute classification à deux mains dans un monde avec `UGridItemSubsystem`.
- Un test Automation **sans registre** conserve la caractérisation de l'ancien modèle, uniquement derrière `WITH_DEV_AUTOMATION_TESTS` ; aucune autorité legacy en gameplay.
- Menu, tooltip et exécuteur refusent désormais l'ID non enregistré. Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalContextAndTooltipEquipmentSlots` compare les refus avec la fixture isolée et vérifie aussi l'exécuteur.

## Portes de validation

`UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` doivent être SUCCESS sur le SHA exact avant lot suivant. Aucun DataAsset, schéma, SaveGame, WBP, fichier de localisation ou niveau n'est modifié. Le scan complet de l'ancien stockage et la recette 06.6 restent à faire. PIE des armes déjà validées demeure acquis, mais pas pour un binaire nouveau avant confirmation.
