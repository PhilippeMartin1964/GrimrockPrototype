# Tests Automation et validation — Fondation d’architecture

## Niveaux de validation

1. **Tests unitaires/contractuels C++** pour données et services purs.
2. **Tests runtime avec UWorld léger** pour intégrations gameplay.
3. **Tests Editor-only** pour authoring et validation du Grid Editor.
4. **PIE manuel/automatisé** lorsque le comportement dépend de Blueprints, meshes ou DataAssets binaires.
5. **Compilation UE5.5.4** fournie par l’environnement utilisateur avant de déclarer une tranche compilée.

## Organisation

Les suites portent le jalon dans leur chemin (`Grimrock.MONxx...`). Les tests de non-régression s’appuient sur les mêmes contrats data-driven que la production.

Les helpers de tests doivent utiliser des namespaces nommés, car Unreal peut regrouper plusieurs `.cpp` dans une unité de Unity Build. Des helpers identiques placés dans des namespaces anonymes de deux fichiers peuvent alors entrer en collision.

## Validations récentes de référence

```text
Grimrock.MON19.8                   4/4 Success
Grimrock.MON19                    55/55 Success
PIE MON19                           VALIDÉ
Grimrock.MON20.2.Recruitment       6/6 Success
Grimrock.MON20.3.StoryCompanion    6/6 Success
```

## Warnings intentionnels

Certains tests négatifs provoquent volontairement des warnings (cycle Logic, budget partagé, Lua invalide, données rejetées) puis terminent en `Success`. Le résultat Automation, pas la seule présence d’un warning, décide du verdict.

## Règles de projet

- ne pas annoncer compilation/test UE comme validé sans résultat utilisateur ;
- ne pas modifier un `.uasset/.umap` pour un test C++ si une fixture transiente suffit ;
- un sous-jalon = un commit logique ;
- la documentation de validation est mise à jour avec le même jalon logique ;
- après une évolution transversale, lancer la suite ciblée puis la régression du domaine.

## Manques

- CI build/test automatique ;
- validation Shipping/package ;
- scénarios de campagne longs ;
- validation automatisée exhaustive des assets binaires.
