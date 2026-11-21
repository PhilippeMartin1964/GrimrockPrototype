# ARM-CLEAN06.5H.8 — Lecture des définitions historiques limitée aux fixtures sans GameInstance

**Parent :** `b7e5f2d30278fc7dc97c0bacf3ab937c7172fd51` (double SUCCESS Windows exigé avant publication).

`FGridLegacyItemLookup::Resolve` retournait une définition historique d'inventaire pour un ItemId non enregistré, même dans un world dont la `GameInstance` portait un registre canonique valide. Cela permettait aux anciens chemins de curseur, dépôt et projectile d'obtenir un `UGridItemDefinitionAsset*` en amont de leurs gardes locaux.

**Après correction :** le helper ne consulte plus la table historique que si `WITH_DEV_AUTOMATION_TESTS` est compilé, que `GIsAutomationTesting` est vrai et que le monde du PartyPawn **n'a aucune GameInstance**. Dans les builds jeu (y compris Shipping), la fonction renvoie immédiatement `nullptr` sans parcourir les définitions historiques. Les ID canoniques ne changent pas : les chemins de jeu prennent déjà l'autorité de `DA_ItemSystem`.

La régression `Grimrock.RPG.ARM.CLEAN06_5E4D.PhysicalTransferLegacyLookup` est réalignée : elle vérifie maintenant le rejet d'une identité inconnue en world configuré, la caractérisation explicite de l'ancienne table en worldless Automation, puis le rejet immédiat lorsque la GameInstance est réattachée. Le comportement de `IsRegistered` utilisé pour le diagnostic de rejet reste inchangé.

Aucun asset, niveau, SaveGame, shader, kit ou donnée gameplay canonique ne change. Un Cook/Shipping reste une validation complémentaire, distincte de la suite `Grimrock` en Editor.
