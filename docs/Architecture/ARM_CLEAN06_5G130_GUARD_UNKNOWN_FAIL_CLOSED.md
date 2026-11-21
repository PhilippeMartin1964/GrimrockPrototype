# ARM-CLEAN06.5G.130 — Rejet du fallback de garde historique en production

**Parent requis :** `9d924786a08aca7e0da25cc77c78eb57279929f1` (5G.129.1), avec `SilentTask/LocalAuto` SUCCESS.

## Contrat fail-closed

`FGridEquippedGuardItemReadModel::Resolve` pouvait autoriser, depuis une définition historique non enregistrée, une parade de main-gauche ARM013 ou la qualité de bouclier même en présence d'un registre `DA_ItemSystem`. Ce chemin contredit l'autorité unique de la paire `ItemId` / `ItemLevel` canonique.

La résolution des identités enregistrées ne change pas. Une identité inconnue donne une vue `bResolved=false` en partie normale, qu'un vieux pointeur de définition soit présent ou non. Le lecteur historique reste exécutable **uniquement** pour les fixtures Automation sans registre : garde explicite `#if WITH_DEV_AUTOMATION_TESTS`, `!Registry`, `GIsAutomationTesting` et validation de l'identité de la définition.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalGuardItemIdentity` garde les preuves N1/N20, main-gauche ARM013, boucliers, arme en mauvaise main et N0 invalides. Il vérifie en plus le rejet de l'inconnu avec registre configuré et la compatibilité sans registre pour les tests autonomes de garde. Les autres tests historiques (`LOT02.19.3`, `MON11`) utilisent des fixtures sans GameInstance/registre et conservent le même comportement sous Automation.

Pas de WBP, `.uasset`, `.umap`, localisation, sauvegarde ni propriété C++ d'instance modifiée. Gate obligatoire : `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success` sur le nouveau SHA.
