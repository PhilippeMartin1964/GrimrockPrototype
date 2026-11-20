# ARM-CLEAN06.5G.75 — Isolation des projections historiques d'inventaire

Parent 0db81454ad818900c29d99f7ed8f5bef4524f257. Publication après double validation Windows du parent.

Les trois projections partagées de matérialisation d'instance, capacité de pile et slots d'équipement ne compilent plus la lecture d'une ancienne définition dans les builds sans Automation. Même en Editor/Development, elle est conditionnée par GIsAutomationTesting. Les tests historiques explicitement enregistrés dans un inventaire Automation restent caractérisés, y compris ceux adossés à un registre réel ; ce n'est pas une autorisation de legacy en PIE.

Une définition ancienne portant un autre ID ne peut plus empêcher la matérialisation d'une identité canonique ; seules les fixtures historiques exigent l'ID correspondant. Masse de fixture négative/non finie refusée sans modifier l'instance. Les niveaux, quantités, noms sauvegardés, propriété et dette d'armure restent inchangés ; aucun changement de schéma ou d'asset. Tests 3AG, 3AS et 3AQ conservent leur couverture ; 3AG ajoute le pointeur contradictoire et le rejet de masse invalide. Le retrait des fixtures et des champs sérialisés devra suivre les preuves de compatibilité native et SaveGame.
