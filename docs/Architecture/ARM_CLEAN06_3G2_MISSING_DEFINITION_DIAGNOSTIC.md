# ARM-CLEAN06.3G.2 — Rétablissement du contrat TD06.6

## Diagnostic vérifié (14 novembre 2026)

Après `ARM-CLEAN06.3G`, le test `Grimrock.TechnicalDebt.TD06_6.PartyInventoryEquipmentCore.Contract` échouait : un objet `Unregistered_TD066` correctement refusé par `EquipItemFromInventorySlot` produisait `Reason=UnsupportedSlot` au lieu du contrat historique `Reason=MissingDefinition`. L'échec est corroboré par le journal Windows fourni par le propriétaire et par `UE5.5.4/LocalAuto = failure` sur `56a2cf9`.

## Réparation ciblée

Avant la validation de compatibilité de slot, la fonction distingue deux sources :

1. `UGridItemSubsystem::HasCanonicalItem(ItemDefinitionId)` : un ARM enregistré se valide uniquement par le registre canonique, sans exiger l'ancien `DA_Weapon_*` ;
2. l'identité hors registre : sa définition existante reste obligatoire tant que cette famille n'a pas migré. Si elle manque, le diagnostic `MissingDefinition` est conservé.

Pour un ARM connu mais invalide (par exemple N0), `CanEquipItemToSlot` conserve son refus strict sans retour vers les anciennes règles. Les objets non canoniques avec définition conservent leur logique temporaire. Aucune politique de mains ni règle transactionnelle n'est modifiée.

## Preuve / validation

Le test persistant `Grimrock.TechnicalDebt.TD06_6.PartyInventoryEquipmentCore.Contract` est la régression à vérifier : il attend exactement un avertissement `MissingDefinition` pour `Unregistered_TD066`, et vérifie que l'équipement refusé n'altère ni le sac ni `MainHand`. Ne pas affaiblir le test pour le rendre vert.

La validation requise est `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` (suite complète `Grimrock`) sur le SHA de ce correctif. La réussite locale de compilation/Automation n'est **pas** présumée à partir du patch GitHub.

Aucun `.uasset`, `.umap`, SaveGame, asset de combat, document hors ARM-CLEAN ou définition concrète n'est modifié.
