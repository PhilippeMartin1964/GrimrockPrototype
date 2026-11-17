# ARM-CLEAN06.4X.33 — initialisation physique d'acteur canonique à niveau strict

**Date :** 17 novembre 2026. **Parent :** `d4d329415b46ff33039adace55a6fdeac1909a26`.
**Périmètre :** `AGridItemActor::InitializeFromCanonicalItem`,
régression Automation existante et documentation, aucun `.uasset`,
`.umap`, asset d'équipement, WBP ni SaveGame.

## Problème

La projection générale `UGridItemSubsystem::ResolveItem` autorise
encore une lecture de **famille non-ARM N1–N20** par compatibilité
(contrat ARM-CLEAN06.3V). Elle n'est pas un certificat de validité
physique : les 12 objets non-ARM effectivement matérialisés dans
`DA_ItemSystem` ont un niveau physique N0. Les objets ARM sont N1–N20.

`InitializeFromCanonicalItem` vérifiait `ResolveItem` seulement.
Un appel direct avec une clé, flèche ou torche N1 aurait donc pu
construire un acteur canonique physiquement impossible, malgré
les protections plus strictes des chemins monde, receptacle et palette.

## Correction

Après résolution complète du registre mais **avant toute écriture**
sur les composants ou sur les champs de l'acteur, refuser
`!Projected.bIsArmament && Instance.ItemLevel != 0`.
ARM N0 continue d'échouer dans `ResolveItem`. Les armes N1–N20,
les non-ARM N0, les codes historiques hors registre et les
projections N1 non-ARM sans acteur ne changent pas.
Sur échec, ID, GUID, niveau et présentation déjà en place restent
inchangés : aucun fallback vers un ancien `ItemDefinition`.

## Régression

Renforcer `Grimrock.RPG.ARM.CLEAN06_4X7.CanonicalItemActorInitialization`
sur les **34 vraies définitions** déjà chargées : pour chaque
non-ARM, prouver que `ResolveItem(Id, 1)` reste disponible
tandis que l'initialisation physique N1 échoue sans mutation.
Conserver les vérifications existantes des 22 ARM N1, 12
non-ARM N0, lumière, Niagara, mesh, GUID, tags, ARM N0 et
identités inconnues.

Attendre `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`
SUCCESS sur le SHA final. Les tests ne démontrent pas un rendu PIE
ni la migration native des 22 profils ARM.

**ARM-CLEAN06.4 partielle ; 06.5 BLOQUÉE ; SaveGame v27 conservé.**
