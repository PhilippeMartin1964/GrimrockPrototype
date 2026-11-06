# EFFECTS-ORG01.2A — Dépendances confirmées et ordre de migration

**Base :** rapport Asset Registry Unreal 5.5.4, [issue #50](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/50), généré automatiquement après validation réussie du commit `a2254618`.

**Statut de cette étape :** protection des dépendances et préparation de migration. Aucun `.uasset` déplacé, aucun renommage effectué.

## Résultats établis (UE Asset Registry, pas inférence à partir des noms)

| Chaîne réelle | Décision |
|---|---|
| `DA_Item_Potion_Healing_01 → NS_LiquideSparkle → M_Teleporter_Spark_02` | Renommer uniquement le **matériau** en `M_Potion_Spark`, **dans Effects/Alchimie**, en conservant les deux liens. |
| `DA_Teleporter → NS_Teleporter_Blue → M_Teleporter_Spark, M_Teleporter_Volume, M_Teleporter_Wisp` | **Ne rien déplacer** : groupe déjà cohérent. |
| `DA_Teleporter → MI_Teleporter_Active / MI_Teleporter_Inactive` et `SM_Teleporter_01 → MI_Teleporter_Inactive` | Ne rien déplacer. Les ressources PBR communes aux matériaux restent dans `Art/Textures`. |
| `DA_Item_Torch → NS_Flame_8_Torch → MI_Flame_8, MI_Distortion_1` | Déplacement de la torche vers `Effects/Fire/Torch` à réaliser via l'éditeur Unreal, après le renommage de potion. |
| `MI_Flame_8 → M_Master_Flame, T_Flame_8` ; `MI_Distortion_1 → M_Master_Distortion, T_RefractFlowmap_1, T_RefractMask_1, T_RefractNormal_1` | **Conserver les matériaux maîtres/textures dans Art**, tant que leur exclusivité n'est pas démontrée. Les instances de flamme sont des candidates à migration, pas les masters. |
| `MI_Flame_4 → M_Master_Flame, T_Flame_4` avec aucun référent remonté par le Registry | Candidat à audit d'inutilisation **mais ne pas supprimer** avant recherche de chargements dynamiques, variantes de torches et authoring. |
| `M_Surface_Oil → MI_Surface_Oil` | Ne pas déplacer avant adaptation du runtime, du commandlet, des scripts de réparation et de l'agent Windows. L'absence de référent sérialisé du `MI` ne prouve rien : il est chargé par chemin C++. |
| `M_SurfaceCatalyst_Ember`, `M_SurfaceCatalyst_Flask`, `M_SurfaceCatalyst_RunePulse` sans référents Registry | Conserver. Le code d'authoring/test et les profils C++ utilisent ces noms, indépendamment des référents sérialisés. |
| `MI_Potion_LiquidFlow → M_Potion_LiquidFlow`, avec `SM_Flasque → MI_Potion_LiquidFlow` | **Conserver dans Art/Materials/Items** : matériau du mesh de flacon, non un système Niagara autonome. |
| `M_Item_WorldSparkle` référencé par `DA_Item_CopperKey` et `DA_Item_IronKey` | Conserver dans Art tant que son partage par plusieurs objets est confirmé. |

## Migration ciblée 01.2B : les bulles de la potion

Ancien objet Unreal :
`/Game/GrimrockPrototype/Effects/Alchimie/M_Teleporter_Spark_02.M_Teleporter_Spark_02`

Nouvel objet :
`/Game/GrimrockPrototype/Effects/Alchimie/M_Potion_Spark.M_Potion_Spark`

Référent à resauvegarder au minimum :
`/Game/GrimrockPrototype/Effects/Alchimie/NS_LiquideSparkle.NS_LiquideSparkle`.

Consommateur indirect :
`/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01`.
Il ne doit pas changer de système Niagara.

**Invariants de migration :**
1. Un seul matériau de potion existe après opération, sous le nom cible.
2. `NS_LiquideSparkle` dépend du **nouveau** matériau et non de l'ancien.
3. `DA_Item_Potion_Healing_01` dépend toujours de `NS_LiquideSparkle`.
4. Aucun `UObjectRedirector` orphelin ni paquet à l'ancien chemin ne subsiste après la correction finale des redirectors.
5. Aucun autre `.uasset`, aucun contenu commercial `FireFX` et aucune carte `L_Dungeon` ne change sans explication de dépendance.
6. Validation intégrale UE5.5.4 et contrôle visuel PIE des bulles.

**Règle technique :** toute opération réelle de renommage/migration doit passer par les mécanismes d'AssetTools/Unreal Editor qui mettent à jour les références internes. Un `git mv` de `.uasset` est interdit : il ne corrige pas les chemins d'objets sérialisés. Une migration unattended ne doit pas lancer de boîte de dialogue ni laisser un worktree de validation modifié sur échec.

## Tests conservés

- `Grimrock.Editor.EffectsOrg01.Inventory` : inventaire et liens réels (lecture seule).
- `Grimrock.Editor.EffectsOrg01.DependencyContract` : contrôle explicite de la chaîne potion, de la chaîne téléporteur et des matériaux de la torche ; accepte provisoirement un seul des deux noms de matériau jusqu'à la migration.
- La suite entière `Grimrock` reste la condition de publication d'un jalon.

L'issue #50 ne contient volontairement **que des chemins du projet** ; les noms et références internes du pack sous `/Game/FireFX` et les chemins locaux Windows restent privés.

**Décision :** la première vraie migration sera le matériau des bulles de potion. Les autres déplacements sont reportés à leurs sous-jalons respectifs pour conserver une régression détectable, un commit par étape et aucun code de compatibilité.
