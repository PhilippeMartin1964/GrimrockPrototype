# ARM-CLEAN06.4N — Parité des lancers génériques non-ARM (sans migration binaire)

**Base vérifiée :** 06.4M `8d925f186e02f419c3b5087f7df8f8f848548552` : les trois statuts `SilentTask`, `LocalAuto` et `ARM-CLEAN06.4M` SUCCESS, preuve GitHub [146](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/146). Source inventoriée en 06.4K `4d5e7eb8b1406d29eeb9b0805f82507aa8a1bfc7`.

## Périmètre et motivation

L'inventaire privé 06.4K a identifié **3 propriétés historiques actives** du domaine `GENERIC_THROW` parmi 50 propriétés non-ARM. Leur nom et leurs valeurs exactes restent uniquement dans l'historique Windows privé.

Le système de lancer existant emprunte l'ancien `UGridItemDefinitionAsset` pour les objets non enregistrés dans `DA_ItemSystem`, alors que la politique `DA_WeaponCatalogue.ProjectilePolicies` est réservée aux véritables archétypes d'armes. Il serait erroné de déplacer les lancers de torches, pierres ou composants dans ce catalogue.

Le modèle cible est **une seule** propriété `GenericThrow` de type `FGridNonArmamentThrowPolicy`, dans `UGridNonArmamentDefinitionAsset`. Elle contient 12 réglages authorés sur l'objet non-ARM, indépendants des actions de combat :

| Ancien champ historique | Propriété de `GenericThrow` |
|---|---|
| `bCombatThrowWeapon` | `bCombatRecoverable` |
| `ThrowSpeed` | `InitialSpeed` |
| `ThrowArc` | `Arc` |
| `ThrowLifeSeconds` | `LifeSeconds` |
| `ThrowImpactDropOffset` | `ImpactDropOffset` |
| `ThrowVisualRelativeRotation` | `VisualRelativeRotation` |
| `ThrowVisualRelativeScale` | `VisualRelativeScale` |
| `ProjectileMeshForwardAxis` | `MeshForwardAxis` |
| `ThrowVisualMode` | `VisualMode` |
| `ThrowVisualTumbleAxis` | `VisualTumbleAxis` |
| `ThrowVisualTumbleDegreesPerSecond` | `VisualTumbleDegreesPerSecond` |
| `ThrowVisualSpinDegreesPerSecond` | `VisualSpinDegreesPerSecond` |

Le mode Stable/Tumble/Spin utilise une nouvelle enum `EGridNonArmamentThrowVisualMode` : le commandlet doit vérifier les **trois correspondances explicitement**. L'axe `EGridProjectileForwardAxis` reste un type géométrique commun, non une référence vers un catalogue d'armes. Les arcs signés restent autorisés jusqu'au clamp appliqué par l'exécuteur réel.

**Important :** `bCombatRecoverable` est la préservation de la valeur source, **pas** une action d'attaque automatiquement offerte par l'objet. Cette autorisation sera réconciliée avec `DA_ActionCatalogue`, les coûts et l'exécuteur au cours d'un jalon ultérieur.

## Test en mémoire dans le véritable Unreal Editor

`GridArmClean064NNonArmGenericThrow` est un commandlet Editor-only en lecture seule. Le déclenchement Windows requiert le sujet Git exact :

`feat(ARM-CLEAN06.4N): preserve generic nonarm throwing and visual physics data`

`Scripts/AuditARM_CLEAN06_4N_NonArmGenericThrow.ps1` :

1. vérifie le reçu privé de 06.4M, la baseline immuable 06.4K et les gates de migration strictement fermées ; refuse toute divergence ou duplication historique contradictoire ;
2. rehash les **84 sources binaires** et les **84 copies privées** du snapshot 06.4G ;
3. recharge `DA_ItemSystem` (23 définitions) et les **11 anciennes définitions non-ARM** représentant les **40 placements physiques** des niveaux, palettes et réceptacles ;
4. compare toutes les 50 propriétés actives des vraies sources avec leurs exports conservés en privé ;
5. construit 11 `UGridNonArmamentDefinitionAsset` transitoires, puis transfère les 12 valeurs de lancer de chaque source dans le `GenericThrow` transitoire (**132 correspondances**, 121 copies de même type vérifiées `SameType + Identical + ExportText` et 11 mappings de mode enum explicites) ;
6. compare la résolution numérique (vitesse, arc signé) pour `ItemLevel` 0, 1 et 21 (**33** sondes) ; le cas invalide 21 est refusé ;
7. compare les projections visuelles (scale clamp, vitesse angulaire Stable/Tumble/Spin, durée, impact) (**44** sondes) sur les valeurs authorées ;
8. vérifie l'absence de changement de `Dirty`, la propreté du worktree et **zéro écriture** de package UE et SaveGame ;
9. conserve les valeurs et identités seulement dans un rapport `generic-throw-private.json`, scellé par `throw-gate-private.json`, et publie sur GitHub **uniquement des compteurs agrégés**.

L'agent Windows ajoute `UE5.5.4/ARM-CLEAN06.4N`. Ne déclarer ce jalon validé qu'après ses **trois statuts** (`SilentTask`, `LocalAuto`, `ARM-CLEAN06.4N`) SUCCESS sur le SHA exact.

## Frontières obligatoires

- La politique de lancer est **non-ARM**, sans `WeaponCatalogue.ProjectilePolicies`, sans fallback et sans vitesse inventée ; les ARM existants conservent leurs règles N1–N20.
- Le préflight Force/poids/usage de main, le coût PA, la consommation d'objet, le projeté au sol, les collisions Chaos, les actions en combat, Save/Continue et le rendu GPU/PIE doivent encore être vérifiés sur les données réellement converties. Le présent audit prouve seulement la **parité en mémoire des données et fonctions pures**.
- `bCombatRecoverable` ne remplace pas un `ActionId` canonique et n'autorise pas l'utilisation du lanceur en combat.
- **32 propriétés historiques actives** non encore projetées : actions **11**, équipement non-ARM **15**, empilement **6**.
- Aucun `.uasset`, `.umap`, `.sav`, WBP ou texte joueur modifié. `migration_authorized=false` et listes d'écriture vides. Le snapshot protégé 06.4G reste la référence.

**Suite :** 06.4O — contrat de lien explicite entre actions non-ARM et `DA_ActionCatalogue`, puis équipement/empilement, authentification des SaveGames et cutover UE Editor seulement après toutes les preuves.
