# ARM-CLEAN06.5H.4 — Classification native non destructive des 54 packages

**Parent :** `a1f6d236430ddbe9c56e58221c1b4234963338e9` (publication H.4 uniquement après `SilentTask` et `LocalAuto` SUCCESS du parent exact).

## Constat vérifié

Le préflight 5A recense 54 packages par filtre large, **pas 54 assets à supprimer**. Les deux assets `Weapons/DA_WeaponCatalogue` et `Weapons/DA_WeaponVisualCatalogue` sont de **vieilles sources de numéros et visuels**, encore utilisées par les tests/outils Editor. Les autorités canoniques sont deux autres assets sous `CanonicalStage/` : `DA_CanonicalWeaponCatalogue` et `DA_CanonicalWeaponVisualCatalogue`, et leurs références sont enregistrées dans `DA_ItemSystem`.

## Extension de l'audit en lecture seule

`GridEditorRetirementAudit::CandidateRetirementRole` affecte un libellé stable à chaque candidat, sans lui accorder le droit de supprimer. Le JSON `native.candidates[].retirement_role`, `native.candidate_role_counts` et `native.unclassified_candidate_count` donne une répartition explicite :

| Rôle de classification | Attendu |
|---|---:|
| `historical_item_definition` | 48 |
| `historical_catalogue_source` | 2 |
| `save_readable_fixture` | 2 |
| `historical_throwing_knife_redirector` | 1 |
| `historical_action_icon_palette` | 1 |
| `unclassified_keep_and_investigate` | 0 |

Le test `Grimrock.Editor.ARM.CLEAN06_5A.NativeRetirementEvidence` exige que les 54 candidats soient attribués à ces rôles. Un nouvel asset de catégorie inconnue doit être investigué, pas automatiquement éligible à une purge. La collecte AssetRegistry et la validation du contenu de chaque sauvegarde demeurent inchangées.

**Limites :** le rapport 5A/v1 du 21 novembre fourni par l'opérateur a été produit *avant* cette extension ; il n'inclut pas encore les champs de classification. Leur présence est une exigence des futures exécutions. Les compteurs `instance_legacy_name_or_weight=0` et de pointeurs historiques sont des compatibilités structurales, pas des preuves exhaustives sur les anciennes archives. `deletion_authorized=false` reste obligatoire.

**Interdits :** tout retrait d'asset via GitHub, suppression de binaires, réécriture de `.sav` ou tentative de migration silencieuse. Le retrait éventuel exige le programme natif Unreal Editor, une copie privée, des tests de Cook/PIE/Save/Continue et une validation SHA par SHA.
