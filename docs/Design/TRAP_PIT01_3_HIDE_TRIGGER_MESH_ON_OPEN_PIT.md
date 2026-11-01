# TRAP-PIT01.3 — Masquer le mesh du piège au-dessus d'une fosse ouverte

**Statut : correctif C++ publié sur `master` ; compilation UE5.5.4 et recette PIE à effectuer.**

## Demande
Lorsqu'un piège au sol déclenche réellement l'ouverture de sa fosse associée (PIT03), le mesh du piège doit disparaître pour dégager la vue sur les volets et le trou. Les pièges ordinaires (feu, glace, pointes sans fosse) conservent leur mesh après déclenchement.

## Contrat
- Autorité du déclenchement : GUID et état `EGridDungeonTrapState::Triggered` existants.
- Autorité de la fosse : GUID PIT03 et état persistant `IsPitOpenForLevel`.
- Relation exigée : piège de surface `Floor`, option individuelle `InstanceConfig.DungeonTrap.bOpenPitOnTrigger`, fosse **unique sur la même case** et couverture PIT03 complète ; `IsPitConcealedByFloorTrap` valide la relation.
- **Visuel masqué uniquement si l'instance de piège est `Triggered` ET sa fosse associée est réellement ouverte.** Il ne suffit pas que l'option soit cochée.
- L'acteur de piège est conservé en runtime ; son état, son GUID, ses liens et le SaveGame sont intacts. Seuls `SetActorHiddenInGame`, la collision souris `NoCollision`, et les effets de surbrillance inutiles changent.
- La fermeture ultérieure de la fosse rend de nouveau le mesh du piège visible et interactif pour son statut ; la réouverture le masque.
- En cas de fosse absente, ambiguë, invalide, encore fermée ou dont l'ouverture échoue, le piège déclenché conserve son mesh.
- Les autres pièges ordinaires, même présents dans le même niveau, restent visibles après déclenchement.
- Aucune modification de dégâts, animations, chute, calculs RPG, DataAsset partagé, carte ou placement.
- La restauration après Save/Continue reconstruit la présentation depuis les états persistants existants.

## Implémentation
- `GridLevelRuntimeActorTraps.cpp / RefreshDungeonTrapRevealVisual` calcule `bCoveredByOpenedPit` et `bShowVisual` ; collision souris synchronisée.
- `GridLevelRuntimeActor.cpp / FinalizePitGameplayStateChange` rafraîchit la présentation du piège lors des ouvertures/fermetures réelles de la fosse.
- `GridTRAPPIT01Tests.cpp` étend les vérifications de déclenchement/SaveGame avec des assertions de visibilité, collision et retour après fermeture.

## Validation locale
```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP_PIT01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Pit.PIT03"
```

## Recette PIE
1. Placer un piège `Floor` avec `Ouvrir une fosse au déclenchement` et une unique fosse PIT03 fermée sur sa cellule.
2. Avant activation : le piège détecté est visible ; la fosse reste camouflée.
3. Au déclenchement : la fosse s'ouvre, **le mesh du piège et sa collision souris disparaissent**, les deux volets restent visibles.
4. Après Save/Continue : seule la fosse apparaît, sans superposition du piège.
5. Après fermeture par un mécanisme (si applicable) : le mesh de piège déclenché redevient visible. Après nouvelle ouverture : de nouveau masqué.
6. Piège au sol feu/glace/pointes sans option : son mesh reste visible après déclenchement.
7. Cas d'erreur : sans fosse compatible, pas de suppression silencieuse du mesh.

**Aucune modification de localisation :** `UI.csv` et `ST_UI.uasset` inchangés ; aucun nouveau WBP ou texte joueur ; `Game` fr/en inchangé.
