# ARM-CLEAN06.5G.1 — Retrait du champ de palette du HUD

**Préconditions acquises :** e1790a0b531011eebf9a72a1e99b8b24ab508d76 (trois statuts SUCCESS), 2dfdd7fbc93203c5e6e6865b12f3a52880ae0f7c (deux statuts SUCCESS, un seul WBP modifié).

- Retirer la propriété sérialisée historique de la classe native du widget, désormais détachée du vrai WBP.
- Extraire le même UENUM, sans changement de nom réfléchi, de module, d'ordre ni de valeurs, dans un en-tête canonique indépendant de la classe de palette historique. Celle-ci reste requise pour les previews de conversion et certains tests historiques jusqu'au retrait global.
- Conserver le test permanent de chargement du WBP, d'absence de la propriété, de résolution des douze catégories et d'absence de référence AssetRegistry. Transformer le compteur 5A des arêtes externes en assertion zéro après la preuve native.
- Retirer le commandlet, le plan et le runner one-shot 5F ainsi que son hook, devenus inutiles après matérialisation ; leur source et leurs preuves restent dans l'historique Git.
- Aucun fichier binaire, SaveGame, clé UI.csv, ST_UI ou traduction n'est écrit par ce lot. Le test LocalAuto ne remplace pas PIE.
- **Ne pas encore effacer l'asset historique ni sa classe** : les outils et tests qui chargent les sources historiques restent à migrer dans un lot séparé. Aucun changement du fallback hors registre dans ce lot.

Le second classificateur historique et FindIcon sont également retirés, ainsi que les paramètres legacy du helper HUD. Le test LOT02.20 utilise désormais des FGridResolvedItem transitoires sur les 82 archétypes (mêmes familles, armes à lancer, défense liée/non liée, passifs, sources non-ARM) ; les tests réels 3AL/3AR conservent la parité native. Le fallback individuel hors registre reste temporairement disponible mais ne peut plus contaminer un sort/QuickItem au même ActionId. La classe de palette reste data-only jusqu'au retrait des tests/outils de projection dépendants.
