# ARM-CLEAN06 — Clôture du lecteur d'exécution de combat

## Objectif du lot

Rassembler le correctif du test MON12.8.5 de H.72 et les dernières lectures de définition historique **dans l'exécuteur** de combat (attaque, arbalète, projectile, profil d'arme). Aucun asset, niveau, SaveGame, texte WBP ou ressource d'interface ne change.

## Correctif de la transaction magique

L'ancien précontrôle considérait toute action `RangedAttack` comme un lanceur d'objets, même les sorts `Spell` sans objet source. Depuis H.72, un lanceur d'objet inconnu est correctement rejeté, mais le sort direct `Spell_MON1285_ArcaneBolt` subissait cette même règle malgré son absence de munition. L'appel `ResolveForAttack` reçoit maintenant `bResolvedRangedAttack && bItemSourceAttack`: les attaques magiques sans objet ne réservent aucune flèche; les vrais lanceurs équipés et QuickItem conservent le contrôle strict du registre. Le test existant `Grimrock.Monsters.MON12.8.5.DirectSpellManaTransaction` couvre le rejet d'une cible absente sans dépense, puis l'attaque autorisée avec exactement trois mana et deux PA.

## Cutover du combat

- Les trois lecteurs historiques de profil de mains et le décodage des tags/statuts/effets d'armure n'existent plus dans le Shipping, et sont accessibles uniquement en Automation Editor **sans** `UGridItemSubsystem`, avec `GIsAutomationTesting`.
- Source de jet physique et vitesse d'ancienne munition sont pareillement isolées à ces fixtures. Une identité inconnue du registre est refusée et ne profite pas de `bCombatThrowWeapon`.
- La résolution canonique, les actions de classe empruntant une arme enregistrée, le combat mains nues, les effets et l'intégrité des projectiles canoniques ne changent pas.
- Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalCombatPhysicalThrow.LegacyCompatibility` vérifie désormais le rejet d'une arme inconnue dans un registre actif et caractérise l'ancien scénario seulement en Automation sans registre.

## Validation et limites

Garde exigée sur le SHA exact: `UE5.5.4/SilentTask=SUCCESS`, `UE5.5.4/LocalAuto=SUCCESS` (Editor, Shipping, Automation). L'agent Windows n'exécute pas de Cook ou de PIE GPU. Les 53 packages historiques, les deux Readables et les sept sauvegardes restent protégés jusqu'au retrait natif puis à la recette ARM-CLEAN06.6. Un test qui échouerait ne justifie jamais de rouvrir un fallback en production.
