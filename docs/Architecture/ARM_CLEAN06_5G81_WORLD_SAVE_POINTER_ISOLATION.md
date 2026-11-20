# ARM-CLEAN06.5G.81 — Isolation des pointeurs v27 de monde

Parent a0c856aa958f02cc611aa8c48041d3fd43a78d97. Après double validation Windows du parent.

Les pointeurs ProjectileDefinitionAsset et PendingItem.ItemDefinitionAsset ne sont plus lus ni écrits explicitement par les routes de gameplay hors Automation. Les fixtures historiques sans GameInstance restent temporairement supportées uniquement sous GIsAutomationTesting. Les gates de capture, restauration, drop et passage PIT02 ne peuvent plus être désactivés hors Automation même en Development Editor. Une arrivée appelée directement vérifie identité, niveau, quantité et registre avant de toucher la file. Les tests BC gardent les GUID et les vrais cas de flèche en vol ou attachée avec un cache v27 contradictoire ; PIT02 ajoute le refus direct de l'inconnu avec conservation de sa file.

Aucune modification du schéma v27 ou des archives réelles. Le codec et les audits Editor peuvent encore lire les champs sérialisés pour la preuve de conversion ; le retrait définitif demande notamment la migration des fixtures sans registre et une vérification des archives sur copies. Aucun asset ni comportement valide modifié. PIE/Chaos et Save/Continue utilisateur restent distincts.
