# ARM-CLEAN06.5H.1 — Décision conservatrice sur le rapport de retrait actualisé

**État : analyse en lecture seule — aucune suppression autorisée.**  
**Parent de référence :** `c6b1492da26422fc4dbc94f2234642a5240f2579` (SilentTask/LocalAuto SUCCESS).  
**Date de l'évidence opérateur :** 21 novembre 2026. Le JSON et les sauvegardes privées ne sont pas publiés.

## Résultat établi par le rapport privé 5A/v1

- Registre canonique : **48 identités**, **22 armements**, **466 identités/niveaux**, **14 actions détaillées** ; 48 définitions historiques correspondant à ces identités.
- AssetRegistry : **54 packages dans le périmètre très large de l'inventaire**, **0 arête externe** au groupe audité. **Zéro arête externe n'est pas zéro dépendance C++/Automation ni une preuve d'effacement sûr.**
- Sauvegardes : **7/7 v27 décodées**, **0 échec**, **145 occurrences objets/monde** dénombrées, **0 ItemId inconnu**. Les quatre MON188 historiques restent protégées.
- Une sauvegarde contient **deux références Readables résolues** vers `Tests/Readables/DA_Scroll_Test` et `Tests/Readables/DA_BookPage_Test`, avec deux chemins bruts correspondants. **Ne pas supprimer ces assets.**
- Quatre `UGridLevelAsset` examinés ; aucun lien `ReceptacleContainsItemType` trouvé dans ce périmètre. Les 48 sources de catégories historiques et leurs tags canoniques sont présents pour comparaison future.


> **Rectificatif 5H.4 :** les deux catalogues du répertoire `Weapons/` sont des **sources historiques de migration**, et non les autorités runtime. Les vrais catalogues canonisés sont `CanonicalStage/DA_CanonicalWeaponCatalogue` et `CanonicalStage/DA_CanonicalWeaponVisualCatalogue`, liés depuis `DA_ItemSystem`. Ils ne font pas partie des 54 candidats. Tant que les tests Editor utilisent les anciennes sources, leur conservation est indispensable.

## Classification obligatoire des 54 packages (pas un ensemble de suppressions)

| Groupe | Nombre | Disposition |
|---|---:|---|
| `UGridItemDefinitionAsset` historiques | **48** | Candidats à un retrait natif ultérieur uniquement après neutralisation de tous les consommateurs et preuve de parité. Ne pas supprimer maintenant. |
| `DA_WeaponCatalogue`, `DA_WeaponVisualCatalogue` | **2** | **À CONSERVER POUR L'INSTANT** : **catalogues historiques**, sources des outils/tests Editor, distincts des catalogues canoniques sous `CanonicalStage/`. Leur présence dans le rapport tient au filtre large `Weapons/`. |
| `UGridReadableContentAsset` de test | **2** | **À CONSERVER** : deux références effectivement décodées dans une sauvegarde v27. |
| Redirecteur `DA_Weapon_ThrowingKnife` | **1** | Examiner et traiter ses chemins/redirects via Unreal Editor avant toute suppression ; il référence encore le package historique `DA_Weapon_ThrowingKnife_01`. |
| `DA_EquippedWeaponActionIcons` | **1** | L'arête WBP externe précédente a disparu, mais des tests Editor chargent encore explicitement cette palette ; ne pas supprimer avant leur conversion et la validation native. |

## Limites de preuve importantes

`deletion_authorized=false` est explicite dans le rapport. `instance_cache_schema_retired=true`, mais les clés `instance_legacy_name_or_weight=0`, `projectile_definition_pointers=0` et `pending_definition_pointers=0` sont des **valeurs de compatibilité écrites par le collecteur actuel**, et non une nouvelle preuve de décodage exhaustif des caches présents dans les archives pré-cutover. Les modes `world_pointer_preflight`, `instance_cache_evidence`, `weight_cache_evidence`, `semantic_cache_evidence` et `instance_schema_projection` valent **false** pour cette exécution. Ne pas confondre ce rapport avec les preuves spécialisées 5G85 / 5G88 / suivantes.

L'analyse AssetRegistry ne garantit pas l'absence de chemins C++ explicites, de fixtures de test ou de vieux chemins sérialisés non connus. La disparition de l'ancienne arête `WBP_GridCombatHudAction → DA_EquippedWeaponActionIcons` est un progrès, pas une autorisation de retirer la palette.

## Suite utile et critères d'arrêt

1. **Préparer les tests et consumers historiques** qui chargent encore explicitement les anciens assets (notamment tests Editor de production 06.2 et anciens chemins d'authoring), sans créer de deuxième autorité runtime.
2. **Établir une liste blanche fermée de packages éligibles** par classes et identités, excluant impérativement les deux anciens catalogues sources à maintenir pour les outils Editor ainsi que les deux Readables sauvegardés ; traiter la palette et le redirecteur par lots séparés.
3. **Retrait éventuel : Unreal Editor natif seulement**, sauvegarde privée des packages, preuve de graphes de références complets, contrôle des chemins SaveGame v27, validation en second processus, rollback et un seul commit atomique de binaires autorisés. Les fichiers privés `.sav` ne sont ni modifiés ni publiés.
4. Exiger `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur chaque SHA modifiant du code ou des assets ; valider PIE, Save/Continue et Cook/Shipping avant fermeture ARM-CLEAN06.6.

**Ce lot est documentaire uniquement : aucun code, UAsset, niveau, SaveGame ou texte joueur modifié.**
