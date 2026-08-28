# Core cleanup - Donjon / Niveau / Grille

## Nettoyage effectué

- diagnostics de `UGridDungeonAsset` complétés pour les `LevelId` vides, positions logiques dupliquées et fallback de niveau par défaut ;
- validation éditeur complétée pour le donjon, les dimensions, `Cells.Num()`, le départ et les murs partagés ;
- avertissements de murs agrégés afin de ne pas saturer le panneau de validation ;
- règle directionnelle des murs commentée dans la peinture, le rendu et le déplacement ;
- branches de nettoyage `Full` / `GeometryOnly` regroupées sans changer leur comportement ;
- création de niveau sécurisée contre une application partielle et reconstruction d'aperçu redondante supprimée.

## Conservé volontairement

- toutes les signatures publiques et expositions `BlueprintCallable` ;
- stockage indépendant des quatre murs de chaque cellule ;
- absence de synchronisation automatique avec le bord opposé ;
- suppression manuelle des niveaux et assets ;
- tous les widgets du mode éditeur, qui sont actuellement référencés par le toolkit.

## Points à traiter plus tard

- TD07.3.6 a supprimé `EGridRuntimeRebuildMode::ObjectsOnly`; les modes courants sont `Full` et `GeometryOnly` ;
- une API structurée commune de diagnostics pourrait remplacer à terme les synthèses textuelles et messages éditeur, mais cette passe évite une refonte publique ;
- les fonctions `BlueprintCallable` potentiellement internes doivent être auditées avec les Blueprints du projet avant toute réduction de visibilité.
