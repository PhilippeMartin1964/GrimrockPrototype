# ARM-CLEAN06.5 — Retrait des auteurs d'objets RPG historiques

Les anciens modules FRPGStartingHealthPotionAuthoring,
FRPGStartingDisarmToolAuthoring et FRPGStartingSurfaceCatalystAuthoring
pouvaient charger, créer et sauvegarder les anciennes définitions
DA_Item_* alors que leurs équivalents N0 sont déjà actifs dans ItemSystem.
Leurs trois paires .h/.cpp sont retirées.

Le test HealthPotion.AuthoringAndQuickItemContract vérifie maintenant
l'action détaillée canonique, la restauration de 20 HP, le coût et
l'absence de consommation à pleine vie. Le test CatalystDefinition
résout le catalyseur N0 canonique, sa zone carrée Area1, sa portée,
ses coûts et sa réaction Fire sur l'huile ; les noms des suites sont gardés.

Les commandlets RPGAlchemistAuthoring et RPGRogueAuthoring restent
disponibles pour les classes, talents et statuts : les flags obsolètes
StartingHealthPotion, StartingSurfaceCatalyst et StartingDisarmTool
sont refusés explicitement. FRPGAlchemistAuthoring::AuthorProductionAssets
ne peut plus créer ni modifier de source DA_Item_* ; lorsque l'un des
onze items canoniques manque, il échoue avant toute sauvegarde.

Les wrappers AuthorRPGAlchemist.ps1 et AuthorRPGRogue.ps1 gardent le
chemin valide de classe/statuts mais refusent leurs anciens modes de
création d'objet. Le script AuthorStartingSurfaceCatalyst.ps1 est retiré.
UEValidationAgent ne peut plus lancer automatiquement cette écriture
sur un test Automation en échec : il publie seulement les diagnostics.

Ce changement ne modifie aucune donnée binaire, aucun SaveGame ni
les deux Readables v27 protégés. La suppression native des anciens
assets reste soumise au contre-audit H.23/5A sur le SHA de transaction.
