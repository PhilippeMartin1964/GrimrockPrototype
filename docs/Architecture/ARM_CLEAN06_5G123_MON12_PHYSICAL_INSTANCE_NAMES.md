# ARM-CLEAN06.5G.123 — Instances de combat MON12 sans copie de nom

Parent `a810b7a347aa79ec2961d5f66c9af3a775c8b23a`. Supprimer 17 affectations `FGridItemInstance::DisplayName` dans les tests MON12 CombatHud (armes, potions, parchemins, shurikens, flasques, catalyseurs, boucliers et équipement de combat), ainsi qu'une dans le constructeur de fixtures MON12 CombatActionPanel. Ne pas supprimer ni réécrire les `FGridCombatActionDefinition::DisplayName`, les `UGridItemDefinitionAsset::DisplayName`, les identités des personnages ou les textes utilisateurs. Les scénarios de cooldown, usages, ressources, main secondaire, projectile, surface et raccourcis conservent leurs contrôles. Aucun changement runtime, asset, SaveGame ou gameplay.

La définition synthétique `MON12_7_Sword` doit conserver le texte visible `Épée de test` : ne pas reprendre le champ de l'instance, désormais vide. L'action utilise le nom de la définition, et les assertions de présentation restent inchangées.
