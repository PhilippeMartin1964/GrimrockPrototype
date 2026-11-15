# ARM-CLEAN06.4M — Texte lisible et physique des objets non-ARM, parité en mémoire

**Source de vérité :** `ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`. Aucun nouvel `ItemType` runtime, aucune extension de `UGridArmamentDefinitionAsset` et aucun catalogue d'armes détourné. Le `DA_ItemSystem` reste la référence commune.

## Préconditions vérifiées

- 06.4K `4d5e7eb8b1406d29eeb9b0805f82507aa8a1bfc7` : 11 ItemId non-armement, 40 références monde, 50 occurrences de champs historiques actifs.
- 06.4L `a279761d52fba8d8a5280104e009bd336cb6c2a1` : trois statuts Windows SUCCESS, 12 occurrences lumière/VFX couvertes par 88 projections réfléchies sur le vrai UE ; rapport public [145](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/145).
- Après 06.4L, le propriétaire a confirmé un **PIE du runtime actuel sans régression perceptible**, y compris torches, effets persistants et Save/Continue. Il s'agit d'une référence de comportement pré-cutover, **pas** d'une preuve du runtime canonique.
- 06.4G : 84 binaires sources + 84 sauvegardes privées immuables ; leurs SHA-256 sont vérifiés à chaque audit.

## Propriétés et autorité cible

Le précédent inventaire avait exactement **1** champ `READING` (`ReadText`) et **2** champs `WORLD_CHAOS` (`bUseItemWeightAsWorldPhysicsMass`, `WorldPhysicsInitialTiltDegrees`).

La cible `UGridNonArmamentDefinitionAsset` conserve le noyau commun à cinq propriétés, plus les visuels non-armement de 06.4L. Elle possède désormais :

| Champ canonique | Règle conservée |
|---|---|
| `ReadText` | Texte par défaut propre à la définition, localisable en `FText` |
| `bReadable` | Capacité **explicite** pour un livre/parchemin lisible même avec texte vide ; dérivée uniquement de `ItemType::Book/Scroll` ou de tags effectivement authorés lors de la conversion Editor |
| `bUseItemWeightAsWorldPhysicsMass` | Opt-in pour la masse Chaos de l'objet au sol ; masse = `WeightKg` **unitaire**, jamais quantité × poids |
| `WorldPhysicsInitialTiltDegrees` | Inclinaison initiale déterministe, clamp 0–45° ; ne se réapplique jamais lors de Save/Continue |

Les vrais textes d'une **instance** (`ReadTextOverride`) et les contenus `UGridReadableContentAsset` existants restent des sources d'instance distinctes et prioritaires sur le texte par défaut. On ne réécrit pas les `FText` depuis des valeurs extraites du nom de l'asset. Aucune seconde masse ne se retrouve dans la définition.

## Contrôle UE5.5.4

Le commandlet Editor-only `GridArmClean064MNonArmReadingPhysics` est déclenché seulement sur ce sujet exact :

`feat(ARM-CLEAN06.4M): preserve nonarm reading and world physics policies in canonical data`

Le script `Scripts/AuditARM_CLEAN06_4M_NonArmReadingPhysics.ps1` :

1. vérifie les rapports privés 06.4K et 06.4L, leur SHA et les gates `migration_authorized=false`, avec sélection sécurisée des duplications historiques identiques de 06.4K ;
2. rehash les 84 originaux et les 84 copies du snapshot 06.4G, ne lit aucun `.sav` ;
3. recharge les 11 vrais assets historiques et le `DA_ItemSystem` réel (23 définitions) ;
4. recoupe tous les 50 champs actifs de 06.4K : noms, catégories et valeurs réfléchies actuelles exactes ;
5. instancie 11 `UGridNonArmamentDefinitionAsset` **transitoires**, projette les trois champs historiques ci-dessus et leurs défauts (**33** copies avec `FProperty::SameType`, `Identical`, `ExportText`) ;
6. définit seulement en mémoire `bReadable` depuis les **données sémantiques authorées** : type livre/parchemin et tags `Readable`/`Lisible` ;
7. compare le véritable `UGridItemContextActionLibrary::IsItemReadable` à la nouvelle politique pure `FGridCanonicalNonArmamentReadPolicies::CanRead` pour quatre combinaisons d'override et de contenu réutilisable sur chaque identité (**44** tests) ;
8. compare les règles purement numériques de masse unitaire (quantité 1 et 5) et inclinaison initiale (**33** tests) ;
9. interdit les changements de `Dirty` des packages sources et tout changement suivi dans le worktree ; sort un reçu privé, puis uniquement un bilan agrégé dans une issue GitHub.

Les vérifications d'instances sont **synthétiques et transitoires** ; elles ne chargent ni ne sauvegardent de SaveGame réel.

Le nouveau contrôle `UE5.5.4/ARM-CLEAN06.4M` s'ajoute à `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`. Ces trois statuts doivent être SUCCESS sur le SHA du **commit 06.4M** avant validation du jalon. Aucun statut d'un parent ne vaut pour ce commit.

## Limites et suite

- Les trois occurrences actives obtiennent une destination et une parité de données/politiques pures **mais aucune autorisation de cutover binaire**.
- Restent **35 occurrences actives non encore projetées** : lancer générique 3, actions 11, équipements non-ARM 15, empilement 6.
- Le runtime actuel, les `.uasset`, `.umap`, `.sav`, WBP et textes joueur ne sont pas modifiés ; aucun nouveau texte à matérialiser dans `ST_UI`.
- Les contacts, collisions et mouvements réels Chaos, le widget de lecture en PIE, les SaveGame versionnés et le chargement des objets convertis devront être vérifiés séparément lors du cutover 06.4 et de la recette 06.6.
- Prochaine revue recommandée : **06.4N**, politique de lancer générique non-armement (hors `WeaponCatalogue.ProjectilePolicies`) avec preuve d'identité N0 et de rendu/physique des jets.

**Gate de sortie :** `migration_authorized=false`, `allowed_binary_writes=[]`, `allowed_package_deletions=[]`, `allowed_savegame_writes=[]`.
