# ARM-CLEAN06.4X.40 — Retrait du pointeur legacy des entrées monde canoniques

**Parent :** `06effc11ee81d6de76230007a8c0b01f3417ae7b` (commit de politique Git distinct). **Périmètre :** runtime C++, tests, documentation uniquement.

## Motivation

X.39 a basculé les 44 objets libres des quatre vrais niveaux vers
`AGridItemActor::InitializeFromCanonicalItem` pour les ItemId enregistrés
dans `DA_ItemSystem`, y compris les armes avec un `DA_Weapon_*`
historique encore sérialisé. Le nouvel acteur physique ne conserve plus
ce pointeur. Néanmoins, `AddPlacedItemActor` le copiait encore dans
`FGridSpawnedItemRuntimeEntry::ItemDefinitionAsset`, bien que ce champ
ne soit pas une autorité pour les objets canoniques.

## Correction ciblée

- `AddPlacedItemActor` stocke désormais `nullptr` dans l'entrée
  transitoire si `CanonicalItemId` est présent et validé. Les
  placements vraiment hors registre gardent leur source historique ;
  aucun `UPROPERTY` n'est supprimé.
- `GetWorldItemWeightAtCell` ignore explicitement le pointeur
  de l'entrée pour tout ID connu du registre, même si une ancienne
  instance en mémoire le possédait encore. Le poids reste résolu
  uniquement par `FGridItemWeightReadModel`.
- Le transfert PIT02 ignorait déjà `Entry.ItemDefinitionAsset` pour
  un ID canonique ; les nouvelles chutes et les sauvegardes continuent
  de résoudre ItemId, niveau physique et GUID plutôt que ce pointeur.
- Les objets Legacy vraiment hors registre restent inchangés ; le champ
  de données persistantes `FGridLooseItemInstance::ItemDefinition`
  n'est **pas converti** dans ce jalon.

## Tests

Le test `Grimrock.RPG.ARM.CLEAN06_4X8.RealLooseItemCanonicalNonArmCutover`
dispose d'un accès **lecture seule réservé à l'automation** aux
`SpawnedItemEntries`, sans élargir l'API gameplay. Sur les sources
réelles, il vérifie maintenant l'absence d'UObject ancien dans l'entrée
du non-ARM N0, de l'ARM réel N1/N20 et d'un ancien pointeur fabriqué de
même ID mais propriétés visuelles contradictoires. L'ID et le niveau
sont conservés. Les suites existantes `Grimrock.Pit`, tests de poids,
ramassage et Save/Continue sont incluses dans la validation Windows
`Grimrock`.

**Garde-fous :** pas de modification de `.uasset`, `.umap`, palette,
WBP, sauvegarde réelle ou nouveau schéma. Pas de suppression des 107
références historiques audit X.19. `-NullRHI` ne valide pas à lui
seul la présentation visuelle PIE/Chaos.

**ARM-CLEAN06.4 PARTIELLE ; ARM-CLEAN06.5 BLOQUÉE.**
