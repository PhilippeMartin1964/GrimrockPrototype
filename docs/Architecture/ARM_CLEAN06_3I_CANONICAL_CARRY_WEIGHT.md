# ARM-CLEAN06.3I — Masse transportée à lecture canonique

**Baseline vérifiée :** `b362c55`, statuts `UE5.5.4/SilentTask=success` et `UE5.5.4/LocalAuto=success`.

## Périmètre

Les calculs de charge du sac (`CalculateCharacterCurrentWeight`) et des emplacements d'équipement (`CalculateEquipmentWeight`) lisent désormais `FGridItemWeightReadModel::GetTotalWeight`.

Pour les `ItemId` enregistrés dans `UGridItemSubsystem` (22 armes actives et objets déjà présents au registre), la masse provient uniquement de `ResolveItem(ItemDefinitionId, ItemLevel).WeightKg`, multipliée par `max(1, Quantity)`. Le niveau N1–N20 est celui de l'instance, jamais du personnage. Un `ItemId` connu mais invalide (par exemple `ItemLevel=0`) ne retombe jamais sur le snapshot `FGridItemInstance.Weight` ; la lecture échoue et la contribution est zéro, sans réécrire l'objet.

Les objets non enregistrés et les contextes de tests sans GameInstance configuré conservent temporairement la lecture `FGridItemInstance.Weight`. La présence du snapshot en SaveGame est maintenue jusqu'au jalon 06.4 ; aucune modification de schéma, sauvegarde, asset binaire ou WBP à cette étape. `ApplyItemDefinitionToInstance` conserve son rôle provisoire de normalisation des objets legacy ; **la lecture du poids porté n'utilise plus sa copie pour un ARM enregistré**.

Cela met à jour `GetCharacterSummary().CurrentWeight`, `IsCharacterOverloaded`, les avertissements de surcharge et les limites dérivées sans modifier les barèmes de force ou les bonus de capacité.

## Preuves

Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalCarryWeightReadModel` utilise le vrai `DA_ItemSystem` avec ARM014, ARM031, ARM075 et ARM081 en N1/N20 : poids d'instance volontairement faux, masse lue de l'asset canonique, quantité x3, rejet du niveau 0 sans fallback, conservation de la famille legacy, résultat réinitialisé pour objet invalide.

L'ensemble des tests `Grimrock` doit être vert sur le SHA exact grâce à l'agent Windows `VALIDATION-AUTO01`. Une validation PIE visuelle reste distincte pour vérifier l'affichage du poids porté, non nécessaire pour démarrer l'étape de conversion suivante tant que l'UI n'est pas modifiée.

**Limite explicite :** les autres lecteurs qui utilisent encore le champ `Weight` (lancers, monde, diagnostic d'objets, tooltip) seront migrés par étapes ultérieures ; 06.3I ne prétend pas supprimer définitivement tous les snapshots de poids.
