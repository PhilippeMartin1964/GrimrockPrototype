# ARM-CLEAN06.5H.5 — Audit privé complet, toujours non destructif

**Parent :** `0f686044f537b13111c3eb6c4e42ea37d7db3214` (double validation Windows obligatoire avant H.5).

L'audit natif exécuté par `Scripts/AuditARM_CLEAN06_5_Retirement.ps1` demandait auparavant uniquement `-ReadOnly`. Le rapport opérateur du 21.11 avait par conséquent les modes spécialisés désactivés : les `0` de cache/pointeurs résultaient du schéma C++ déjà retiré, non d'une preuve complète de parité sur les archives.

## Renforcement

Le script passe désormais les quatre **modes déjà implémentés** dans le commandlet, toujours `-ReadOnly` :
`-WorldPointerPreflight`, `-InstanceCacheEvidence`, `-SemanticCacheEvidence`, `-InstanceSchemaProjection`. `-WeightCacheEvidence` demeure **désactivé** (ancien mode incompatible avec le schéma retiré).

En plus des empreintes SHA256 et de la vérification octet pour octet des sauvegardes réelles et de leurs copies privées, le script refuse un rapport s'il manque un mode demandé, si un des **54 packages** ne correspond pas aux groupes natifs classés H.4 ou si la moindre sauvegarde ne valide pas `canonical_world_valid`, `memory_roundtrip_equal`, `cache_normalized_roundtrip_equal`, `cache_invalid_identities=0` et `cache_projection_failures=0`.

Les octets privés, leurs projections, leurs noms et les rapports restent hors dépôt dans le répertoire de sauvegarde externe. Les modes supplémentaires ne modifient aucune sauvegarde : toutes les écritures sont limitées au rapport et aux copies externes, avec vérification du contenu de l'inventaire live inchangé.

**Cette modification ne relance pas et ne prétend pas avoir relancé l'audit sur les fichiers privés de l'opérateur.** Elle conditionne uniquement la prochaine exécution native. `deletion_authorized=false` reste invariant. Aucun asset `.uasset`, `.umap`, WBP ou SaveGame retiré.

**Tests :** `Scripts/AuditARM_CLEAN06_5_Retirement.ps1 -SelfTest` sur fixtures, et les tests Editor natifs `Grimrock.Editor.ARM.CLEAN06_5A` et `Grimrock.Editor.ARM.CLEAN06.5G85`/5G88 ; `SilentTask` + `LocalAuto` sur SHA exact. Un audit de fichiers privés exigera toujours l'environnement Unreal Windows et la fermeture du jeu.
