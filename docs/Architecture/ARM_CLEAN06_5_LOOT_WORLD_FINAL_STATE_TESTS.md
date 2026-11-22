# ARM-CLEAN06.5 — Tests de loot et de présentation monde sur l'état final

Les suites X.37, X.60 et X.61 conservent leur nom Automation mais
ne chargent plus les historiques DA_Weapon_* et DA_Item_*.

X.37 valide sur le DA_ItemSystem définitif les 22 ARM N1/N20, les
22 associations visuelles, les propriétés de physique monde, torches,
Niagara et sparkle et l'absence de mutation du catalogue binaire.
L'ancien helper de conversion GridArmClean064X37ArmWorldProfileProjection
est retiré. Le contrôle de réception X.37.1 conserve séparément et
strictement son allowlist figé de 24 fichiers de sauvegarde historique :
ce garde-fou ne charge aucune définition d'arme.

X.60 et X.61 vérifient les vraies tables Loot Goblin/Rat,
les cinq ItemIds canoniques N0, les 48 définitions du registre,
les résolutions de niveaux et le générateur de loot déterministe.
Aucun test n'a besoin de recréer une définition candidate pour
obtenir une preuve sur les sources déjà migrées.

GridEditorNativeStoneLootPlan et GridEditorLootDefinitionProjection
restent provisoirement compilés : les anciens essais X.48,
X.63 et l'audit d'alchimie les incluent encore. Ils seront retirés
uniquement après remplacement de ces consommateurs.

Aucun fichier .uasset ni .sav n'est écrit. Deletion_authorized=false
pour les 53 candidats. L'audit des sept vraies sauvegardes et la
recette PIE/Cook/Shipping demeurent une étape locale distincte.
