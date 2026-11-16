# ARM-CLEAN06.4X.8 — premier cutover des loose items physiques

**Statut : bascule limitée au placement des objets libres non-ARM
en runtime.** Aucun `.uasset`, `.sav`, `.umap`, WBP, item historique
ou définition canonique n'est modifié.

Base validée : ARM-CLEAN06.4X.7,
`28c0426982d30dec239a3c6d5bdaf281e920c5b0`.
Les 95 `CanonicalItemId` des quatre niveaux et de la palette
sont matérialisés depuis 06.4W ; leurs niveaux physiques sont
enregistrés depuis X.5. X.6 garantit la projection non-ARM et X.7
l'initialisation canonique complète de `AGridItemActor`.

## Règle de transition

`AGridLevelRuntimeActor::AddPlacedItemActor` :

1. Pour une occurrence qui porte `CanonicalItemId`, vérifie
   le GUID, la quantité, le registre, l'identité persistée et la
   cohérence de l'ancien pointeur lorsqu'il existe.
2. Utilise **`ResolvePersistedInstance`** pour le vrai niveau
   N1–N20 d'une arme et N0 d'un non-ARM. Il n'appelle plus
   `ResolveAuthoredPlacement` pour un niveau **déjà matérialisé**
   et ne convertit jamais artificiellement N0 en N1.
3. Pour le seul non-ARM, crée l'acteur sans source historique,
   puis applique `InitializeFromCanonicalItem` avec la véritable
   instance (GUID, ItemId, niveau, quantité). La lumière initiale
   provient de `FGridResolvedItem.LightEmitter.bDefaultEnabled`.
   Mesh, Niagara, tags, sparkle, masse et inclinaison sont
   pilotés par la projection canonique. Les données lisibles
   et overrides propres au placement sont ensuite transmis
   par `InitializeReadableContent`, sans les effacer.
4. Les ARM continuent temporairement leur présentation antérieure,
   après validation stricte de leur `CanonicalItemId` et
   niveau déjà persisté. Leurs lumières/VFX propres ne sont
   pas effacés avant le cutover ARM dédié.
5. Les placements synthétiques historiques, sans
   `CanonicalItemId`, conservent leur ancien parcours déjà
   testé pour éviter une régression des tests et outils de
   création en cours de migration. **Ce n'est pas un nouveau
   fallback autorisé pour un ItemId connu invalide.**
6. L'entrée runtime conserve temporairement son pointeur
   `ItemDefinitionAsset` pour les consommateurs SaveGame
   non encore migrés. L'acteur non-ARM n'en dépend plus pour
   les effets, la lecture ou la physique.

## Test

`Grimrock.RPG.ARM.CLEAN06_4X8.RealLooseItemCanonicalNonArmCutover`
charge les **quatre niveaux réels** et contrôle les **44
loose items** déjà migrés. Dans un monde transitoire avec
`UGameInstance` et le registre natif, il sélectionne un
non-ARM et un ARM de ces vrais niveaux, les place dans
une grille miniature et vérifie la présence des acteurs,
l'identité, le niveau, l'absence de pointeur sur l'acteur
non-ARM, les paramètres lumière et les VFX. Il exige
que l'ARM conserve sa présentation pendant la transition
et que les contradictions d'identité ou ARM N0 soient
refusées **avant l'apparition d'un acteur**.

La compilation et la suite complète `Grimrock` sur le
commit de X.8 sont obligatoires ; aucun test `-NullRHI`
ne démontre la parité visible du rendu PIE, Chaos dynamique,
Niagara, Save/Continue ou Shipping.

## Suite

Basculer les règles des spawns (aucune occurrence réelle
actuelle) et les **18 contenus initiaux** des réceptacles,
avec préservation des règles d'acceptation, des signaux logiques
et des inventaires physiques, puis le cutover de la palette
et des anciens comportements ARM. Migrer les sauvegardes
seulement sur copies et retirer les pointeurs historiques
après vérification de toutes ces voies.
