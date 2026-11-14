# ARM-CLEAN06.3X — Métadonnées de butin de monstres canoniques

**Base de départ :** 06.3W.1, classification corrigée des instances de munition invalides, exigeant `LocalAuto = success` sur son SHA.

## Contrat migré

`UGridMonsterDeathComponent::GenerateAndPlaceLoot` conserve son tirage, son ordre, les probabilités, le choix historique déterministe d'un niveau fixe, le GUID, le nombre d'objets et leur placement dans le donjon. **Une fois ItemId et ItemLevel choisis**, les copies transitoires `FGridItemInstance.DisplayName` et `Weight` sont désormais initialisées depuis `UGridItemSubsystem::ResolveItem` pour les identités enregistrées. Le `WeightKg` n'est jamais lu du `FGridWeaponLevelProfile` historique ou de `DA_Weapon_*` dans ce cas. Les objets encore hors registre conservent leurs valeurs anciennes et leur source de niveau.

`FGridLootStaticMetadataReadModel` est un résolveur privé, en lecture seule, qui interdit le fallback pour un item canonique connu à niveau invalide, sans remplir une valeur d'instance arbitraire. La production d'un loot non résolu est refusée sans être placée. Les snapshots statiques ne sont **pas retirés** maintenant, car leur sérialisation et leur conversion appartiennent à 06.4. La gestion de la lumière et des tags du loot est inchangée.

## Preuves

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalMonsterLootMetadata` s'appuie sur le `DA_ItemSystem` réel, quatre ARM N1/N20, les flèches N0, des copies volontairement contradictoires, un N0 invalide pour ARM et un objet historique hors registre. Le test établit la résolution transitoire ; l'ensemble `Grimrock` protège les scénarios existants de mort, génération et reprise.

Aucun `.uasset`, `.umap`, SaveGame, widget ou chaîne localisée n'est modifié ; **UI.csv → ST_UI → WBP → Game/fr/en : inchangé**. La validation exige `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` verts sur le commit unique de 06.3X. La matérialisation visuelle d'un loot réel reste un contrôle PIE distinct.
