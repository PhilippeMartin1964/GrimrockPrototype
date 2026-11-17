# ARM-CLEAN06.4X.36 — Présentation complète des projectiles sans ItemDefinition historique

**Date :** 17 novembre 2026. **Parent :** `871f26100e50fbebed254062dc3c54dd1d7075df`.
**Périmètre :** `AGridThrownItemActor`, porte de spawn, deux
régressions de projectiles et documentation ; aucun asset UE ni SaveGame.

## Problème restant

X.20 a rendu possibles les visuels canoniques des ARM sans ancien
asset ; X.34–X.35 refusent les niveaux physiques incohérents.
Mais `InitializeThrownItem` initialisait encore toute instance
**sans** `UGridItemDefinitionAsset` par
`InitializeFromItemDefinitionId`. Cette méthode n'installe
que l'identité, pas le profil entier de mesh/lumière/Niagara.
Le lecteur de projectile remplaçait ensuite le mesh sans
rematérialiser `LightEmitter` et `VisualEffects`.

## Correction

Si l'ancienne définition est absente **et** l'ItemId figure
dans `DA_ItemSystem`, appeler
`InitializeFromCanonicalItem(Instance, Registry)` avant la
configuration de vol. Le résultat complet contient mesh monde,
lumière, Niagara persistant, tags et niveau physique réel ;
l'orientation et le mesh de projectile ARM restent ensuite
déterminés par `WeaponVisualCatalogue`, les flèches N0
restent sur leur `WorldMesh`.

Si cette initialisation échoue, détruire le projectile incomplet et
faire refuser le résultat au lanceur avant d'attacher des ignorances
de collision ou de poursuivre le vol. La branche avec **ancien
ItemDefinition non nul reste inchangée** jusqu'à parité visuelle
des 22 profils ARM ; un ItemId vraiment hors registre conserve
son chemin historique. Ce jalon ne crée aucun nouveau lancer
autorisé : la porte de spawn et ses règles de main/munition
demeurent l'autorité.

## Tests Automation

- `Grimrock.RPG.ARM.CLEAN06_3.CanonicalSpecializedThrowWithoutLegacy.Projectile` :
  vrai projectile ARM N20 sans ancien asset, GUID, niveau, profil
  canonique complet, mesh de vol dédié, lumière et Niagara depuis
  ItemSystem.
- `Grimrock.RPG.ARM.CLEAN06_3.CanonicalRangedAmmo.Projectile` :
  vraie flèche N0, aucune ancienne définition, profil complet,
  mesh monde, lumière et Niagara sans débit prématuré d'inventaire.
- `Grimrock` complet obligatoire, SilentTask et LocalAuto
  SUCCESS sur le SHA publié.

**Ne prouve pas** la luminosité GPU/PIE, les collisions Chaos,
l'ouverture de fosses, la conversion des 22 assets ni Save/Continue.
Les anciens assets sont conservés, aucun `.uasset` ou `.sav`
n'est écrit. **06.4 partielle ; 06.5 BLOQUÉE.**
