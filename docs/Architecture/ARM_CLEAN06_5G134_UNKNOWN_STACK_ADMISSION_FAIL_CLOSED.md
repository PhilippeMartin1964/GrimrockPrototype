# ARM-CLEAN06.5G.134 — Refus des objets inconnus en inventaire canonique

**Parent:** `3f25386d82eda1995b656b7357667242c29a50be` ; publication après succès `SilentTask` et `LocalAuto` du SHA parent.

## Diagnostic

Une exemption `WITH_DEV_AUTOMATION_TESTS` de `UGridPartyInventoryComponent::RequiresResolvedInventoryDefinition` admettait des identités physiques absentes de `DA_ItemSystem` même lorsque le monde Automation avait un `GameInstance` et son registre configuré. `FGridCanonicalInventoryStackReadModel` pouvait alors emprunter `MaxStackSize` au DataAsset historique. Le build de jeu hors Automation exigeait déjà un objet canonique : la faille était dans la couverture de test et les permissions contradictoires en environnement Automation.

## Correction et vérification

- Avec un `UGridItemSubsystem` présent, la matérialisation canonique reste obligatoire en Automation aussi : un ID inconnu ne peut être accordé via une définition historique.
- Le lecteur de pile n'utilise plus d'ancien `MaxStack` si le registre est présent. La caractérisation `Registry=nullptr` subsiste uniquement derrière `WITH_DEV_AUTOMATION_TESTS`.
- Dans `Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryMaterializationWithoutLegacyDefinition`, l'ID inconnu en quantité **13** et en quantité **1** est refusé par `CanAdd` et `Add`, sans mutation des slots. La fixture sans registre confirme l'ancien `MaxStack=10` ainsi que le nom et le poids hérités, sans les réintroduire en jeu.
- Les items canoniques, leurs quantités et leur progression N0/N1-N20 ne changent pas.

Aucune modification de `.uasset`, `.umap`, SaveGame, WBP, localisation ou schéma. Attendre SUCCESS/SUCCESS Windows sur le SHA exact ; la recette PIE/Chaos/Cook/Shipping 06.6 reste distincte.
