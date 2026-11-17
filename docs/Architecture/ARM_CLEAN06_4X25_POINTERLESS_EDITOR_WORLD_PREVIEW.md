# ARM-CLEAN06.4X.25 — Prévisualiser les objets canoniques sans ancienne définition

**Date : 17 novembre 2026. Parent : `cafd5850a5e471dfc323f3002ccc7e1e98a251ff`.** Périmètre C++ du composant d'aperçu et du modèle de lecture, un test Unreal transitoire, documentation. **Aucun niveau, .uasset, WBP, preset ni SaveGame modifié.**

## Anomalie structurelle

Le monde runtime sait créer un ARM depuis `CanonicalItemId` + `ItemLevel` depuis X.20, mais `UGridEditorPreviewComponent::IsPreviewableLooseItem` et `AddLooseItemPreview` écartaient les placements si `ItemDefinition` était nul **avant** d'interroger `FGridEditorLooseItemPreviewReadModel`. Ce modèle lisait à son tour seulement `Definition->ItemDefinitionId`. Un objet nouvellement authoré canoniquement pouvait donc être valide en jeu mais invisible dans l'éditeur.

## Correction limitée, fail-closed

- `FGridEditorLooseItemPreviewReadModel` donne désormais priorité à `Placement.CanonicalItemId` explicitement sérialisé. Pour cette provenance : registre requis, GUID et quantité valides, cohérence des identités si un ancien pointeur subsiste, niveau physique strict par `ResolvePersistedInstance`. ARM N0, non-ARM N1, ID absent, définition contradictoire ou registre indisponible donnent `bCanonicalIdentity=true` avec `bResolved=false` : **aucun fallback vers l'ancien mesh**.
- Pour les fixtures non migrées qui possèdent un ancien pointeur **sans** `CanonicalItemId`, le modèle conserve `ResolveAuthoredPlacement` et la possibilité d'hériter une seule fois du niveau authoré. Les identités réellement hors registre gardent leur chemin legacy transitoire.
- `UGridEditorPreviewComponent` ne refuse plus une définition absente si un `CanonicalItemId` est renseigné ; les acteurs d'aperçu restent entièrement transitoires, sans write-back dans `LevelAsset`. Mesh et transform proviennent du registre.

## Preuve

Test `Grimrock.RPG.ARM.CLEAN06_4X25.PointerlessCanonicalEditorPreview` : monde transitoire avec vrai `DA_ItemSystem`, acteur `AGridLevelRuntimeActor`, véritable `UGridEditorPreviewComponent`, puis `RebuildPreviewObjects` et inspection d'un acteur réel de preview créé pour `Weapon_Dagger_01` N20 sans ancien asset. Compare mesh/transform. Contrôles négatifs : ARM N0, nonARM N1, registre absent, ancien pointeur contradictoire, ID inconnu. Tests historiques X.3Z de prévisualisation des fixtures non migrées conservés.

## Suites

La **validation des niveaux et le Selected Object** de `Source/GrimrockPrototypeEditor` exigent encore les anciennes définitions ; ce ticket ne peut pas les supprimer sans revoir séparément les outils d'authoring, dont la liste et les champs Selected Object. La palette est toujours à migrer par ID. Les paramètres persistants des 22 ARM dans les catalogues sont **présents en schéma C++ mais pas encore matérialisés dans Unreal**, ce qui bloque leur retrait. 06.4 partielle ; **06.5 BLOQUÉE**.
