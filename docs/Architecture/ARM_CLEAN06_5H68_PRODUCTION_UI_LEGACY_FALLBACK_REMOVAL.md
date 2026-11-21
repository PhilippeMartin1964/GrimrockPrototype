# ARM-CLEAN06.5H.68 — Retrait du repli de présentation historique en jeu

## Coupe du lecteur historique

Dans `FGridWeaponVisualUiResolver::ResolveInventoryItemIcon`, un `ItemId` que `UGridItemSubsystem` ne connaît pas ne peut plus obtenir une icône ou un `VisualId` depuis un ancien `UGridItemDefinitionAsset` lorsque le registre est configuré. L'absence de registre en production échoue également de manière fermée. Un repli vers les anciens objets subsiste **uniquement sous `WITH_DEV_AUTOMATION_TESTS` lorsque `GIsAutomationTesting` et que la fixture ne possède pas de registre**. Les APIs de projection visuelle historique `Resolve` et `ApplyCatalogueIcon` ne sont plus déclarées ni compilées en Shipping.

Dans `UGridCombatHudWidget`, le repli qui injectait nom, description et icône d'une vieille définition pour un `ItemId` inconnu est enfermé dans la même condition stricte de fixture Automation sans registre. Les résolutions canoniques du HUD, des actions d'équipement et des QuickItems ne sont pas modifiées. Aucun raccourci de gameplay ni chaîne de repli n'est introduit.

## Preuves et garde-fous

Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryItemIcons` continue à vérifier les profils canoniques et le refus N0 d'une arme. Il vérifie désormais qu'un vieux pointeur contradictoire ne peut plus imposer son icône quand le registre est présent, et que les fixtures sans registre gardent une caractérisation isolée sous Automation.

Un seul commit C++/test/documentation, aucune écriture `.uasset`, `.umap`, `.sav`, aucune nouvelle clé de localisation et aucun WBP modifié. `UI.csv → ST_UI → WBP → Game/fr/en` : **inchangé** ; les noms/descriptions canoniques existants demeurent des `FText` du DataAsset, pas de nouveaux libellés. Les cas inconnus échouent fermés plutôt que d'afficher des textes non autorisés ; aucune matérialisation `ST_UI` requise pour ce lot sans clé nouvelle. La présentation visuelle en PIE/Shipping reste à contrôler séparément.

H.69 exige `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur le SHA H.68. Le retrait natif des 53 packages historiques reste interdit avant réaudit des sept sauvegardes et examen AssetRegistry.
