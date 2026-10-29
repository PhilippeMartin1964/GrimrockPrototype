# ARMEMENT-LOT02.13.4 — Tir visible et récupération des munitions

Statut : code publié pour validation locale UE5.5.4 ; PIE et sauvegarde/rechargement encore à observer.

## Contrat
- Réutiliser `AGridThrownItemActor` et `Item_Arrows_01` : aucune nouvelle classe de projectile ni nouvel item de munition.
- Après validation de la cible et réservation des PA, préparer un projectile unitaire avec GUID neuf ; si la création échoue, refuser l'attaque et restituer les PA, sans prélever de flèche.
- Conserver une seule consommation par `FGridRangedAmmunitionPolicy` : `2 -> 1 -> 0`. Ne pas prélever à nouveau depuis la présentation.
- `FGridCombatResolver` reste l'autorité unique du hit et des dégâts ; le projectile suit la direction du monstre, sans gravité ni spin.
- Hit : conversion en pickup sur la cellule cible si l'impact est proche ; miss : passage au-delà de la cible et dépôt au point d'arrêt valide.
- Si aucun point de dépôt valide n'est trouvé, le fallback historique est la cellule source. Le logger doit permettre de diagnostiquer ce cas.
- `SM_Item_Arrows_01` est pris dans l'ItemDefinition canonique. Ajuster au besoin `ThrowVisualRelativeRotation` et `ThrowVisualRelativeScale` dans le DataAsset local pour l'alignement du mesh. Ne jamais activer `bCombatThrowWeapon` pour les munitions.

## Tests
1. Fermer Unreal Editor.
2. `./Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'` (inclut le nouveau test `RecoverableRangedAmmunitionProjectile`).
3. `./Scripts/ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.Monsters.MON11'` (si ce filtre existe sur votre installation ; sinon utiliser le filtre exact des tests d'armes de jet).
4. Recette PIE isolée : tirer deux fois sur un monstre ; voir une flèche en vol à chaque tir, vérifier une seule munition consommée, retrouver les flèches dans la cellule du monstre après la mort et les ramasser.
5. Vérifier l'échec sans flèche, sans PA consommés, le raté et le dépôt après impact, puis la sauvegarde et le chargement.
6. Ne jamais déclarer les 11 armes `Active` sans ces observations PIE.

## Périmètre du commit
C++ runtime et un test automatisé uniquement, avec cette note. Aucun `.uasset`, carte, sauvegarde ou catalogue modifié.
