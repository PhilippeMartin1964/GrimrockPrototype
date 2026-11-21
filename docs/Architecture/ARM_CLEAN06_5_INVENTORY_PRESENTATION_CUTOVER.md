# ARM-CLEAN06 — Cutover groupé : équipement, raccourcis, présentation et interaction

## Objet du lot

Après validation Windows des deux statuts du cutover d'exécution de combat, ce lot traite **ensemble** cinq consommateurs du système d'objets. Il remplace les chemins de consultation d'anciennes définitions encore accessibles dans l'exécutable par un seul comportement de production: requêter le `UGridItemSubsystem` et refuser toute identité inconnue.

## Modifications

- `GridPartyInventoryComponentHotbar.cpp`: liens d'actions équipées et QuickItem résolus uniquement dans le catalogue canonique. `CombatActions`, `CompatibleEquipmentSlots`, `BuildInventoryCombatActionDefinition` et `IsPhysicallyThrowable` historiques sont sous `WITH_DEV_AUTOMATION_TESTS`, sans registre et pendant `GIsAutomationTesting`.
- `GridPlayerAttackPresentationComponent.cpp`: profils historiques de mouvements, audio et multi-frappes hors Shipping; les profils canoniques conservent leur priorité absolue, et les attaques à mains nues ne changent pas.
- `GridEquippedGuardResolver.cpp`: parades et boucliers utilisent le registre, sans charge d'ancienne définition dans la production.
- `GridPartyInventoryComponentEquipment.cpp`: calcul d'armure, bonus, résistances, identification de source et traces de consommation d'armure n'accèdent à l'ancien registre qu'en Automation Editor isolée; les ItemId canoniques n'ont aucune seconde autorité.
- `GridItemContextActionLibrary.cpp`: le menu contextuel ne transporte plus de vieux pointeur issu du sac; l'éventuelle définition fournie par une fixture et la construction d'une ancienne action de consommation sont strictement conditionnées à l'Automation sans registre.

## Non-régressions et limites

Les raccourcis et objets canoniques, la main équipée, le poids, les résistances, les interactions, le rendu de l'attaque et les capacités de consommables doivent rester inchangés. Les anciennes fixtures de tests sans `GameInstance` continuent d'exercer le comportement historique sans l'introduire dans Shipping. Aucun ajout de texte WBP, modification de `ST_UI`, d'asset, de mesh, de sauvegarde, de Niagara ou de niveau.

Validation requise sur le SHA final: `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` success; recette native de retrait des 53 packages protégés et 7 sauvegardes à conduire séparément dans UE5.5.4. `-NullRHI` ne vaut pas essai visuel.
