# ARM-CLEAN06.5H.10 — Enregistrement des anciennes définitions exclusivement sous Automation

**Parent** : `0cf3a932cf91fb7eb2ccefdd32cbb04b9f7df5c9`. À publier seulement après SUCCESS/SUCCESS exact du H.9 corrigé.

## Correction

`RegisterItemDefinition` dans `UGridPartyInventoryComponent` était déjà absent des builds sans `WITH_DEV_AUTOMATION_TESTS`, mais les builds Editor peuvent compiler cette fonction alors que les tests ne sont **pas** en cours. Un appel d'éditeur ordinaire pouvait encore remplir la table `RuntimeItemDefinitionsById`. Le garde `FGridLegacyInventoryRegistrationGate::Allows(GIsAutomationTesting)` refuse désormais l'enregistrement d'un `UGridItemDefinitionAsset` hors Automation.

La lecture `FindItemDefinition` utilise le même garde central au lieu d'un contrôle dupliqué, sans modification de son comportement. Les tests historiques sous Automation, y compris ceux qui construisent volontairement une `GameInstance`, restent autorisés à caractériser l'ancienne table ; cela ne rend pas ses objets utilisables par les résolveurs canoniques du jeu, protégés dans les jalons H.7, H.8 et H.9.

Le test `Grimrock.RPG.ARM.CLEAN06_5H.LegacyInventoryRegistrationGate` vérifie les deux états. La non-régression des appels natifs réels demeure exercée par `Grimrock.TechnicalDebt.TD06_8.PartyInventoryItemDefinitionRegistry.Contract` et la suite `Grimrock`.

**Non compris :** migration/suppression de `.uasset`, fichiers `.sav`, Cook/Shipping, modification du catalogue canonique, changement de représentation UMG. Aucun fichier binaire n'est modifié.
