# Architecture UI courante

Le C++ orchestre, calcule et projette. Les WBP authored en Designer fixent l’apparence. Les contrats [UMG](../UI_UMG_PRESENTATION_CONTRACT.md) et [localisation](../UI_WBP_LOCALIZATION_WORKFLOW.md) s’appliquent. L’inventaire historique des jalons et de leurs validations reste [consultable au SHA CLEANUP06](https://github.com/PhilippeMartin1964/GrimrockPrototype/blob/2941d211ded3ffca873065d93eb05a9a2c51c7b3/docs/Design/UI_ARCHITECTURE_CURRENT.md).

## Surfaces et responsabilités

| Surface | Autorité de données et interaction |
| --- | --- |
| HUD persistant | `UGridPersistentHudWidget` porte navigation, portraits, barre d’actions et flux. Les portraits sélectionnent par `CharacterId` puis via l’index résolu de l’inventaire. `PartyPortraitOrder` est un ordre de présentation, indépendant de l’ordre du roster. |
| HUD de combat | `UGridCombatHudWidget` projette initiative, actions et états du combat. `UGridTurnManagerComponent` conserve les décisions et mutations. La production de vue reste immédiate. Le HUD persistant regroupe ses demandes de rafraîchissement visibles au tick suivant. |
| Inventaire, feuille et équipement | Fenêtres autonomes dans le viewport. Un sac pour le personnage sélectionné. Les filtres et tris conservent les indices physiques. Poids et surcharge proviennent des lecteurs métier. Le glisser-déposer délègue au transfert et peut cibler un portrait. |
| Infobulles | Textes, poids, icônes, statistiques et comparaisons sont des projections d’instance et du registre. `WBP_ItemTooltip` délègue à son presenter natif. Les slots d’équipement gardent leur identité physique. |
| Barre rapide | Les bindings référencent l’action et sa source. Les pierres N0 enregistrées peuvent être affectées au lancer générique. La Force est vérifiée à l’activation/exécution, le débit suit le spawn réussi. La dernière unité épuisée supprime le binding. Les consommables QuickItem gardent leurs actions propres. |
| Skills, Talents et progression | Les surfaces RPG utilisent les états et services natifs avec read models dédiés. La sélection de personnage et l’ordre des portraits restent distincts. Les badges de progression et de poids lisent les mêmes autorités que les autres surfaces. |
| Map | `FGridMapReadModelBuilder` dérive la vue de la géométrie et de l’exploration, y compris secrets, étages et dalles. Le thème et le Paint rendent cette projection. |
| Shell résiduel | `WBP_GrimrockMenu` accueille les surfaces encore dépendantes du shell. Le Journal et le Codex restent sur la roadmap. |

## Contrats de présentation

- Textes statiques liés à `ST_UI` depuis le Designer. FText de contenu porté par les DataAssets. Formattage et messages dynamiques natifs avec les clés de localisation. Ordre obligatoire `UI.csv → ST_UI → WBP → Game/fr/en`.
- Les listes dynamiques instancient des WBP complets. Le Designer maîtrise géométrie, espacement, style et animation. Le fallback reste borné pour une surface non matérialisée.
- Les catégories de filtre et les libellés localisés restent des projections. Les tags de présentation Torch, Book et Jewelry ne deviennent pas des propriétés d’instance. Les icônes d’action et d’objet gardent des sources distinctes.
- Les événements de conséquence du flux se regroupent selon leur contexte. Le flux ne doit pas publier plusieurs conséquences techniques comme autant de nouvelles actions joueur.

## Validation

La recette Shipping D.16 couvre la création et la reprise, les fenêtres et infobulles, les filtres, l’équipement, le HUD de combat, les raccourcis et la visée générique. L’anomalie visuelle de ciblage reportée reste ouverte. La compilation des Blueprints et les tests de projection constituent une preuve technique, et le rendu d’une hiérarchie WBP modifiée exige une vérification visuelle propre au changement.

[Barre rapide contextuelle](HOTBAR_CONTEXT01_INVENTORY_CONTEXT_ACTION.md) · [Portraits et badges](UI_PARTYHUD02_PROGRESSION_WEIGHT_BADGES.md) · [Rendu de carte](MAP_THEME01_TEXTURED_MAP_RENDERING.md) · [Modèle des objets](../Architecture/DataModel.md).
