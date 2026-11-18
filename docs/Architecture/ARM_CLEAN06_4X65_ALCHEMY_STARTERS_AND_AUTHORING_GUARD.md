# ARM-CLEAN06.4X.65 — points d'entrée alchimie canoniques

Parent : `433c08fde6fe47fcdf5550c211bc521b7bc4281a`. Ce lot prépare la bascule native des dix définitions sans changer le gameplay. Registre natif encore 38 ; projection X.63 48. Aucun changement de sauvegarde ni de .uasset.

## Kits de départ et aperçu frontend

Le kit Grenadier contient une bombe de feu ; le kit Transmuter contient un flacon d'huile déjà canonique. La recette C++ conserve les mêmes ItemId, quantité 1, N0, destination sac, références de kits et autres armes.

`FRPGStartingEquipmentPresetCatalog::TryPopulateApprovedTransientClass` accepte le registre déjà configuré comme paramètre optionnel. Les deux points d'entrée de production (création réelle, aperçu de l'assistant) passent leur UGridItemSubsystem existant. Pour les deux identités précises FireBomb/Oil **enregistrées**, la recette génère `CanonicalItemId` et un niveau physique N0 ; pas de LoadObject de l'ancienne source, pas de pointeur, aucune action implicite. Vérifie N0, pile, action détaillée QuickItem exacte et résoluble, slot sac ; erreur si identité enregistrée invalide, sans fallback. Avant la migration FireBomb, cette identité seule peut encore passer par l'ancien chemin. Les anciennes recettes de caractérisation sans monde/registre et les autres armes de démarrage restent transitoires jusqu'au chantier de retrait. Ne pas interpréter ce lot comme retrait de tous les usages de ResolveKnownStartingItemDefinition.

`FGridStartingKitDisplayReadModel` reconnait une identité explicite sans pointeur, résout strictement le niveau physique, refuse pointeur contradictoire, registre absent, ID inconnu, ARM N0 et non-ARM N1. Les autres entrées autorisent toujours le mécanisme historique de défaut de niveau uniquement pour ARM. Le wizard appelle le lecteur pour les entrées par ID et ne les omet plus.

## Authoring

`FRPGAlchemistAuthoring::PlanUnregisteredItems` est une preuve read-only avant toute écriture du générateur historique. Configure un registre de prévisualisation depuis la même source de configuration que le jeu ; vérifie les onze identités uniques, Oil présent, chaque identité enregistrée est un non-ARM N0 à action détaillée QuickItem. Etats autorisés dix absentes + Oil ou zéro absente. Etat hybride refusé. AuthorProductionAssets parcourt uniquement les sources absentes, jamais une définition déjà canonique. Les autres écritures du générateur (classe, talents, statuts) restent distinctes et ne sont pas déclenchées par ce lot. ConfigureItem reste utilisé par tests historiques sur copies ; les autres anciens générateurs (palette FireBomb, SurfaceCatalyst, etc.) restent à neutraliser lors de la suppression finale. L'ancien authoring ne doit pas être exécuté après migration hors procédure approuvée.

## Tests

- Grimrock.RPG.ARM.CLEAN06_4X65.CanonicalAlchemyStartersAndAuthoringGuard : vraie classe Alchemist, registre 38 et preview 48, gardes 10/0 et refus 47, deux kits réels pointerless, vraie API TryBuildKit, une unité N0 en sac et absence de définition historique dans OutDefinitions.
- Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitDisplayByActualLevel : étendu au vrai Oil et au refus niveaux/identités explicites incohérents. La recette PIE visuelle du wizard et le chemin combat seront validés au cutover natif ; pas d'assertion de rendu par NullRHI.

UI : workflow relu ; aucun nouveau libellé ni clé. Texte dynamique du DataAsset et patterns ST_UI existants. Pas de modification UI.csv, ST_UI, WBP, Game/fr/en. La migration d'alchimie nécessite encore X.66 natif, la stratégie de copies X.64 compatible avec la bascule de noms d'action, l'adaptation des gates 38/48 et les tests d'exécution. Ne pas déclarer la migration globale terminée.
