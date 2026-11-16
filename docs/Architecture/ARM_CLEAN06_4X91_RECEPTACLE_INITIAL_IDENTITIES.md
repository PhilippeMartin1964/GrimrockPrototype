# ARM-CLEAN06.4X.9.1 — identité canonique des contenus initiaux

**Périmètre** : bascule des identités et niveaux physiques
des contenus initiaux des réceptacles. Aucun `.uasset`,
`.sav`, `.umap`, Blueprint, catalogue ni règle d'énigme
n'est modifié. Le montage physique des items dans le
réceptacle reste inchangé pendant cette étape.

**Base** : ARM-CLEAN06.4X.8, commit final
`688f54c45d9838132a18c7cba08c3ba24e16e178`
validé par `SilentTask=SUCCESS` et `LocalAuto=SUCCESS`.
Les 18 occurrences réelles ont déjà `CanonicalItemId`
depuis 06.4W, et leur `ItemLevel` définitif
depuis 06.4X.5.

## Changement runtime

Le modèle de lecture sans mutation
`FGridReceptacleInitialCanonicalReadModel::TryResolve`
reçoit une `FGridReceptacleInitialItemConfig` et le
seul `UGridItemSubsystem`.

Si `CanonicalItemId` est renseigné, il exige que l'ID
existe au registre, que l'ancienne définition éventuelle
ne contredise pas cet ID, que la quantité soit positive
et que `ResolvePersistedInstance` valide exactement le
niveau physique. **Pas d'héritage N0 pour une arme déjà
enregistrée, pas de non-ARM N1, pas de repli sur un
ancien nom ou un vieux pointeur**.

`AGridReceptacleActor::InitializeRuntimeWorldObject`
utilise ce modèle pour construire `InitialContainedItems`.
Le pointeur historique est conservé dans l'entrée
runtime afin de ne pas changer simultanément l'affichage,
l'acceptation des objets et leur persistance.

Pour les fixtures et contenus par défaut *non encore*
munis de `CanonicalItemId`, le parcours historique
reste inchangé avec clamp de quantité. Les items
réellement convertis ne peuvent pas emprunter ce parcours.

## Tests

`Grimrock.RPG.ARM.CLEAN06_4X91.RealInitialReceptacleIdentities`
charge les quatre niveaux réels et contrôle :

- les **18 contenus initiaux**, leurs IDs, niveaux,
  quantités et sources transitoires ;
- la résolution stricte dans le registre unique ;
- le refus d'un registre manquant, d'un ARM N0,
  d'une flèche N1 et d'un `ItemId` inconnu ;
- l'absence de mutation des packages ;
- la compatibilité d'une configuration de test
  historique non encore convertie.

Les filtres d'acceptation, clés, triggers et
liens logiques ne changent pas dans ce jalon.
La compilation UE5.5.4 et la suite complète
`Grimrock` doivent réussir sur son SHA.

## Étapes suivantes

Basculer le montage des items non-ARM dans les
réceptacles sur `InitializeFromCanonicalItem`,
puis traiter les règles d'acceptation, ARM, spawns
et palette. Vérifier en PIE la présence physique,
la quantité, la lumière et le déclenchement
des énigmes. Les SaveGames seront migrés sur copies
avant suppression des champs historiques.
