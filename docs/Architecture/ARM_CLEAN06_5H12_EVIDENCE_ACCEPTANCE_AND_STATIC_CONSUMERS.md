# ARM-CLEAN06.5H.12 — Réception du préflight complet et plan de retrait non destructif

**Parent :** `01f82ebe9b8d803b7cf5a44e3cecbfe85a1fdf12` (double validation Windows SUCCESS).

## Preuves réellement reçues

Le fichier privé `retirement-evidence(1).json`, transmis le 21 novembre 2026, a été inspecté **en entier**. Il n'est pas commité et aucun contenu de sauvegarde ou identifiant de personnage n'est publié.

- Quatre modes activés : `world_pointer_preflight`, `instance_cache_evidence`, `semantic_cache_evidence`, `instance_schema_projection`. `weight_cache_evidence` est volontairement `false` pour le schéma retiré.
- 7 sauvegardes v27, `decode_failures=0` ; **7/7** `canonical_world_valid`, `memory_roundtrip_equal` et `cache_normalized_roundtrip_equal` à `true`. Les 145 lignes inventaire/monde et les six instances effectivement inspectées par le contrôle des caches ne produisent aucun `cache_invalid_identities` ni `cache_projection_failures` ; aucun ItemId inconnu.
- 48 identités canoniques dont 22 armements, 466 résolutions exactes, 48 définitions historiques avec correspondance, 14 actions détaillées, zéro arête externe AssetRegistry au groupe audité, quatre niveaux parcourus et zéro lien puzzle de type relevé.
- 54 packages classés en **48 définitions anciennes**, **2 catalogues historiques sources** (distincts des autorités `CanonicalStage/`), **2 Readables de sauvegarde**, **1 redirecteur de couteau**, **1 palette d'actions historique**. Aucun candidat non classé.
- Deux `ReadableContentAsset` effectivement résolus dans une sauvegarde, accompagnés de deux chemins sérialisés lisibles : `DA_BookPage_Test` et `DA_Scroll_Test` doivent être préservés.

Le rapport conserve explicitement **`deletion_authorized=false`**. Les compteurs de noms/poids et pointeurs historiques à zéro restent en partie des champs de compatibilité du schéma retiré, non une preuve autonome de l'absence d'archives plus anciennes. Le nouveau rapport fournit, lui, les preuves spécialisées de validation canonique et de parité sur ses **sept fichiers contrôlés** ; il ne prouve ni Cook/Shipping, ni les comportements GPU/PIE, ni l'absence de références C++ littérales.

## Nouvelle commande de préparation, sans purge

`Scripts/PlanARM_CLEAN06_5H12_Retirement.ps1` accepte un rapport JSON privé complet, `-ExpectedHead` égal à un `master` local et distant propre et `-OutputPath` **hors dépôt**, nouveau fichier qui n'existe pas encore.

Il vérifie les modes de preuve, le classement et les identités des 54 packages, la parité de chacune des sept sauvegardes, les deux Readables protégés, les objets sans ID inconnu et l'absence d'arêtes externes déclarées. Il produit un plan privé avec :

- `candidate_definitions_not_approved_for_deletion` : exactement les 48 anciens packages à examiner ultérieurement **en Editor natif**, pas un jeu d'instructions de suppression ;
- `protected_or_deferred_packages` : les six autres packages (catalogues sources, Readables, palette, redirecteur) ;
- `source_text_mentions_not_an_exhaustive_dependency_graph` : chemins relatifs et numéros de lignes des mentions statiques de `LoadObject<UGridItemDefinitionAsset>`, anciens catalogues, palette et redirecteur dans `Source/` et `Scripts/` ;
- `deletion_authorized=false` sans condition d'activation.

Cette recherche par texte peut produire des faux positifs (commentaires, tests) et **des faux négatifs** (chargements dynamiques, références sérialisées, soft paths reconstitués). Elle n'est pas une analyse exhaustive de dépendances. Les sources legacy des tests Editor demeurent en place ; ne pas supprimer les 48 anciens DataAssets avant conversion des consommateurs et vérification native des binaires.

L'option `-SelfTest` est exécutée automatiquement en préflight `ValidateUE.ps1` avec des fixtures purement synthétiques, y compris deux refus (save modifiée et arête externe). Elle n'accède pas aux sauvegardes de l'utilisateur. Une véritable exécution du plan nécessite le fichier privé sur le poste Windows et ne fait que lire le JSON et le code source.

## Réserve technique

Le JSON `retirement-evidence/v1` ne contient **pas de SHA Git du producteur** : le contrôle du HEAD lors de la création du plan ne prouve donc pas à lui seul que le rapport a été produit sur ce même SHA. Avant tout retrait, régénérer l'audit natif sur le SHA de la transaction, protéger les sources et destinations, prévoir rollback, migrer tous les consommateurs tests/Editor, refaire Save/Continue + PIE + Cook/Shipping, puis retirer les `.uasset` au moyen d'Unreal Editor **uniquement**.

**Aucun retrait, correctif des sauvegardes, patch des `.uasset` ou modification de niveau dans H.12.**
