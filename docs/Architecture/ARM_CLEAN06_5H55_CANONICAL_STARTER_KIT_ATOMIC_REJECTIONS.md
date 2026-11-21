# ARM-CLEAN06.5H.55 — Rejets atomiques des kits canoniques

## Migration ARMEMENT01.9

`Grimrock.RPG.ARMEMENT01.9.StartingKits.RejectionIsAtomic` est converti aux identités physiques explicites `Weapon_ShortBow_01` N9, `Weapon_Dagger_01` N1 et `Weapon_RoundShield_01` N1, résolues depuis `ItemSystem` dans un monde transitoire. Aucun des trois packages `DA_Weapon_*` n'est chargé par le fichier de tests `GridARMEMENT0191StartingEquipmentTests.cpp`.

Le premier échec contrôle le conflit entre arc deux mains en MainHand et bouclier en OffHand. Le second échec contrôle une dague et un bouclier demandés dans le sac alors qu'un seul emplacement est autorisé. Les deux doivent refuser la création du personnage sans modifier son identité préalable, sa classe, sa propriété, sa progression de création ni introduire des items partiels. Les pointeurs historiques ne sont jamais enregistrés.

Les tests de succès Archer N9+N1 (H.53) et dague N4/bouclier N1 (H.54) conservent leurs assertions. `LegacyCreationUnchanged` reste un contrôle d'une classe sans kit, sans dépendance aux anciens packages.

## Périmètre

Retrait des trois chemins directs vers anciens DataAssets, de l'aide `AddEntry` à pointeurs historiques et de l'include devenu inutilisé, dans ce seul fichier de tests. Aucun changement au gameplay, aux véritables classes de production, `.uasset`, `.umap`, `.sav`, Niagara ou HUD. Aucun des 53 packages protégés H.23 n'est supprimé.

H.56 bloqué jusqu'à `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS pour H.55 (Editor, Shipping et Automation `Grimrock.RPG.ARMEMENT01.9.StartingKits`). Sept SaveGames privés à réauditer nativement avant tout retrait destructeur.
