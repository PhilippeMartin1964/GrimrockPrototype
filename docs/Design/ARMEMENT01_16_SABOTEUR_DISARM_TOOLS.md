# ARMEMENT01.16 — Outils de désamorçage du Voleur/Saboteur

Date : 30 octobre 2026
Statut : **ARMEMENT01.16.1 : définition physique autorée en C++ ; génération locale et tests en attente. Pas encore de désamorçage fonctionnel.**

## Source d'autorité et objectif

La recette approuvée dans `ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv`
prévoit pour `Kit_Rogue_Saboteur` l'arme ARM003 N1 (`DA_Weapon_Stiletto_01`)
et le complément `DisarmToolToAuthor`.

`ARMEMENT01_9_7_FINAL_KIT_APPROVAL.md` exige un outil **avec usage N1 testé**.
Ce document interdit d'attribuer un objet inutilisable sous prétexte de
compléter le kit. Les premiers talents du Saboteur sont acquis au niveau 2,
pas à la création.

## Audit de l'existant

- Les pièges temporaires de combat utilisent
  `FGridCombatTrapState` et `AGridLevelRuntimeActor::FindCombatTrapAtCell`,
  `ConsumeCombatTrapAtCell` dans `GridLevelRuntimeActorTraps.cpp`.
- **Ces pièges de combat ne constituent pas encore un système complet de
  pièges mécaniques de donjon** : pas d'identité d'objet de monde ni de
  difficulté de désamorçage que l'item de départ pourrait utiliser.
- `EGridItemActionType::UseOnTarget` existe dans l'interface mais ne possède
  pas de chemin d'exécution générique pour les outils et les pièges.
- `Skill_Traps` est défini dans le catalogue RPG, mais aucun pipeline
  de désamorçage d'un piège de donjon ne le relie actuellement à
  une cible, un test de compétence et un événement de niveau.
- Le document `GRIMROCK_LOCK_SYSTEM.md` est **prospectif** ; son
  contrat de liens `SourceObject+SourceEvent -> TargetObject+TargetCommand`
  doit être respecté par la future implémentation.

## ARMEMENT01.16.1 — Objet canonique non consommable

`UGridItemDefinitionAsset` :

| Champ | Valeur |
|---|---|
| Package | `/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01` |
| `ItemDefinitionId` | `Item_DisarmTools_01` |
| `ItemType` | `Misc` (outil, pas une potion) |
| `HandUsage` | `NotHandHeld` |
| Poids | 0,5 kg **provisoire** |
| Stack | Non, 1 exemplaire par unité |
| `ItemTags` | `Tool.Disarm` |
| QuickItem / action de combat | **Aucun** |
| Effets de talent/compétence | **Aucun bonus automatique** |
| Localization | `Item.StartingTool.Disarm01.Name/Description` dans ST_UI |
| Icône / mesh | à authorer et vérifier en PIE avant attribution au kit |

Il ne faut **pas** interpréter le tag `Tool.Disarm` comme un exécuteur :
il est seulement un identifiant pour la future logique dédiée.

### Production

Le mode isolé ajouté au script existant `Scripts/AuthorRPGRogue.ps1`
ne modifie ni les six classes, ni leurs kits, ni le catalogue d'armes :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\AuthorRPGRogue.ps1 `
    -EngineRoot D:\UE_5.5 `
    -StartingDisarmTool `
    -Apply
```

Cette commande compile UE, synchronise `ST_UI` avec `Localization/Source/UI.csv`,
crée le seul package `DA_Item_DisarmTools_01.uasset` si absent,
refuse tout overwrite incompatible, puis exécute les tests ciblés.
L'asset peut ensuite être enrichi d'une icône et d'un mesh sans réécrire
le contrat gameplay. La commande est idempotente si le DataAsset concorde.

Tests :
`Grimrock.RPG.ARMEMENT01.16.DisarmTool.AuthoringContract` et
`Grimrock.RPG.ARMEMENT01.16.DisarmTool.NoPrematureKitGrant`.

**Le nouvel asset binaire et ST_UI.uasset sont des modifications locales,
non poussées automatiquement par le commit C++ distant.**

## Suite obligatoire avant attribution au kit

**ARMEMENT01.16.2 — Désamorçage N1 fonctionnel**, à implémenter et tester :

1. Une cible de piège de donjon explicitement désamorçable dans le modèle
   de niveau, avec ID stable, difficulté et état actif persisté en SaveGame.
2. Un caller d'interaction unique qui cible ce piège, vérifie la proximité,
   l'outil détenu, l'acteur sélectionné et la disponibilité du Skill_Traps.
3. Résolution du test via `FRPGSkillCheckService` (sans « +2 »
   du talent ExpertDisarm, indisponible au niveau 1).
4. Résultat de succès/échec conforme à la règle décidée et émission
   d'événements via les liens logiques du niveau. Ne pas retirer l'outil
   à moins qu'une règle de durabilité/consommation soit explicitement approuvée.
5. Tests de cible absente, hors de portée, échec du test, réussite,
   absence d'outil, persistance, UI et PIE.
6. **Seulement après ces validations :** ajout de `DA_Item_DisarmTools_01`
   au `Kit_Rogue_Saboteur`, puis vérification du gameplay N1 et de Save/Continue.

Ce découpage protège le principe acté dans ARMEMENT01.9.7 :
**aucun objet décoratif ou action fictive dans un kit approuvé.**

## Prompt Meshy (pour une étape visuelle ultérieure)

```text
A single medieval fantasy trap-disarming toolkit for a dungeon crawler game:
a compact folded leather roll containing small brass lock probes, slender steel
needles, miniature screwdrivers and precise mechanical picks, secured by a
simple leather strap. One isolated object on a neutral gray studio background,
three-quarter view, all parts clearly visible, practical plausible medieval
craftsmanship, subdued dark brown leather and worn steel, game-ready
stylized realism, clean silhouette, symmetry where appropriate, no hands,
no character, no scene, no lettering, no magic effects, no extra props.
```

Le rendu 3D Meshy et l'icône doivent être approuvés dans Unreal, puis
committés ensemble depuis Windows. Ne pas attribuer ce kit prématurément.
