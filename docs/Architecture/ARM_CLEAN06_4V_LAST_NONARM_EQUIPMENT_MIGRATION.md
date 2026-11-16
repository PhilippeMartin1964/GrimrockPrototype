# ARM-CLEAN06.4V — Migration des deux derniers non-ARM en vrais DataAssets

## Source et périmètre

Parent : 06.4U, commit `1fd53212b60b26363a874db2b561dcf2c92347d2`, trois checks UE5.5.4 SUCCESS, issue #154.
Le registre réel `DA_ItemSystem` contient 32 définitions. Les 11 identités non-ARM du rapport privé signé 06.4K comprennent neuf définitions déjà sauvegardées en 06.4T/U, et **deux manquantes**. 06.4O a recensé **une action équipée complète** et **une présentation complète**, en plus des trois QuickItems de 06.4U. Les 40 références monde ne sont pas touchées par cette opération.

## Runtime : aucune nouvelle route legacy

`UGridItemSubsystem::ResolveDetailedNonArmEquippedAction` résout un `ItemId`, `ItemLevel`, `ActionId` et la vraie `FGridItemInstance` équipée via le seul registre canonique. Il vérifie GUID, quantité, la **main physique réellement autorisée** et le `FGridItemActionBinding` explicite. Il renvoie le `FGridCombatActionDefinition` entier : ciblage, PA, coûts, effets, dégâts et cooldown ne sont jamais recalculés à partir d'une vieille arme.
Le TurnManager traite ce profil canonique pour les équipements non-ARM ; les armes ARM restent sur leur propre système d'archétypes. Aucun repli vers les champs `CombatActions` historiques lorsqu'un `ItemId` est enregistré.
`UGridOtherEquipmentDefinitionAsset` possède un seul `MaxStack` spécialisé pour conserver le nombre d'objets d'un équipement non-ARM également empilable ; sa valeur est 1 pour les objets non empilables. Les familles canoniques et le noyau à cinq champs restent inchangés.

## Outil Editor : cinq binaires réels, pas un audit

`GridArmClean064VNonArmEquipmentMaterialize` relit les onze vraies sources et leurs exports UPROPERTY privés 06.4K. Il autorise seulement les **deux ItemId absents** du registre de 32 et préserve les neuf déjà convertis. Il transpose l'identité, les icônes/meshes, les effets de lumière, la lecture, les particules persistantes, les règles Chaos et lancer, les emplacements/bonus/résistances, la politique de main effective et la capacité de pile.
La seule action équipée originale est normalisée suivant les règles d'exécution existantes, puis enregistrée **une seule fois** dans `DA_ActionCatalogue.DetailedActions`. La seule présentation complète originale est stockée dans `DA_ActionPresentationCatalogue.DetailedPresentations` ; l'asset concret ne contient que le binding `ActionId/PresentationId`. Tout doublon, nombre inattendu de profils ou accès à une main non autorisée provoque l'échec fermé.

Le passage `-Apply` peut modifier uniquement :
- deux nouvelles définitions `CanonicalStage/Definitions/DA_Canonical_*.uasset` ;
- `DA_ItemSystem.uasset` ;
- `DA_ActionCatalogue.uasset` ;
- `DA_ActionPresentationCatalogue.uasset`.

Le passage `-Verify` rouvre ces vrais binaires dans **un autre processus UE**, reconstruit les deux définitions depuis les sources originales et compare chaque FProperty, l'action et la présentation via `CompareScriptStruct`. Le manifeste de l'Apply détermine les deux identités pendant la vérification immédiate ; une seconde revalidation indépendante sur le commit Git final lit les deux dernières entrées du registre.

## Contrôles et publication Git

Le test `Grimrock.RPG.ARM.CLEAN06_4.NativeEquippedNonArm` vérifie l'action équipée, la correspondance intégrale, N0, le rejet de N21, des mains virtuelles et des actions non liées. Les 19 actions ARM restent identiques ; les trois QuickItems sont testés séparément. Le test ancien 06.2Y ne doit pas confondre la nouvelle collection `DetailedPresentations` avec la présentation ARM de base.

Sujet exact de commit : `feat(ARM-CLEAN06.4V): migrate last nonarm equipment and authored presentation`.

`Scripts/ApplyARM_CLEAN06_4V_NonArmEquipmentMigration.ps1` est déclenché par l'agent Windows après Build et Automation. Les **cinq binaires précis** doivent relever de Git LFS ; toute autre modification bloque la publication. L'agent exécute `-Verify`, puis la suite `Grimrock` complète **après sauvegarde**, amende seulement son propre commit V de code et pousse avec lease. Le commit final conserve exactement 06.4U comme parent. Le nouveau SHA est testé de nouveau.

**Clôture obligatoire :** `UE5.5.4/SilentTask`, `UE5.5.4/LocalAuto` et `UE5.5.4/ARM-CLEAN06.4V` tous SUCCESS **sur le SHA final qui contient les cinq véritables UAssets**.

## La suite, sans autre inventaire général

Il restera à convertir les 40 références historiques des niveaux/palettes/réceptacles vers `ItemId`, puis les SaveGames sur copies, et supprimer l'ancien système en 06.5. Tests PIE/Chaos/Save/Continue/Cook en 06.6. Aucune modification des fichiers `.sav`, `.umap`, WBP et localisation UI n'est autorisée par V.
