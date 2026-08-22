# MON19.2.1B — Connecteurs conditionnels et mutations exactes

**Statut :** implémenté — validation UE5.5.4 requise  
**Date :** 22 août 2026

## Objectif

MON19.2.1A a défini le contrat des connecteurs et leur identité persistante exacte. MON19.2.1B applique ce contrat aux mutations réelles de l’éditeur et rend les conditions déjà présentes dans `FGridObjectLink` accessibles directement dans le panneau **CONNECTORS**.

Cette étape ne change pas la sémantique du runtime : les conditions existantes continuent d’être évaluées par le dispatcher Event → Command actuel et, pour cette génération de conditions, elles portent sur une cible de type `Receptacle`.

## 1. Service unique de mutation des liens

Le nouveau `GridEditorLinkService` centralise les opérations suivantes :

- normalisation des champs conditionnels ;
- validation de la configuration d’une condition ;
- validation du triplet source/event et cible/commande/condition selon `GridEditorLinkPolicy` ;
- détection d’un doublon exact ;
- ajout d’un lien exact ;
- suppression d’une seule variante exacte ;
- mutation de `UGridLevelAsset::Links` depuis l’éditeur.

`GridEditorLinkPolicy` reste responsable de la question « qu’est-ce qui est autorisé ? ». `GridEditorLinkService` répond à la question « comment le lien est-il créé ou supprimé ? ».

## 2. Identité persistante exacte

Deux liens sont distincts dès qu’un des champs persistants suivants diffère :

- `SourceObjectId` ;
- `TargetObjectId` ;
- `SourceEvent` ;
- `Command` ;
- `Condition` ;
- `ConditionItemDefinitionId` ;
- `ConditionItemTag` ;
- `ConditionItemType` ;
- `ConditionCount` ;
- `ConditionWeight` ;
- `bInvertCondition`.

Il est donc désormais possible d’avoir, par exemple, deux liens partageant exactement le même source, le même événement, la même cible et la même commande, mais utilisant deux conditions différentes.

La suppression d’un lien depuis CONNECTORS transmet le `FGridObjectLink` complet et ne supprime plus toutes les variantes partageant le quadruplet historique.

## 3. Normalisation des paramètres

Avant stockage, le service efface les paramètres qui ne sont pas utilisés par la condition sélectionnée. Cela évite qu’une ancienne valeur invisible crée une variante fantôme du lien.

Exemples :

- `None` remet les paramètres conditionnels à leur valeur canonique et force `bInvertCondition = false` ;
- `ReceptacleContainsItemDefinition` ne conserve que `ConditionItemDefinitionId` ;
- `ReceptacleContainsItemTag` ne conserve que `ConditionItemTag` ;
- `ReceptacleContainsItemType` ne conserve que `ConditionItemType` ;
- `ReceptacleItemCountAtLeast` ne conserve que `ConditionCount` ;
- `ReceptacleWeightAtLeast` ne conserve que `ConditionWeight`.

## 4. Panneau CONNECTORS

Le formulaire de création affiche maintenant une ligne `Condition` déterminée par le type de cible.

Pour une cible `Receptacle`, les choix sont :

- `None` ;
- `ReceptacleIsEmpty` ;
- `ReceptacleHasAnyItem` ;
- `ReceptacleContainsItemDefinition` ;
- `ReceptacleContainsItemTag` ;
- `ReceptacleContainsItemType` ;
- `ReceptacleItemCountAtLeast` ;
- `ReceptacleWeightAtLeast`.

Le panneau n’affiche ensuite que le paramètre pertinent :

| Condition | Champ affiché |
|---|---|
| `ReceptacleContainsItemDefinition` | `Item Definition Id` |
| `ReceptacleContainsItemTag` | `Item Tag` |
| `ReceptacleContainsItemType` | `Item Type` |
| `ReceptacleItemCountAtLeast` | `Minimum Count` |
| `ReceptacleWeightAtLeast` | `Minimum Weight` |

Pour toute condition autre que `None`, l’option `Invert` est également disponible.

Un lien conditionnel déjà créé affiche un résumé de sa condition dans les listes de connecteurs entrants et sortants.

## 5. Compatibilité avec les chemins existants

Les fonctions Blueprint historiques de `AGridLevelEditorActor` :

- `CreateLink(SourceObjectId, TargetObjectId, SourceEvent, Command)` ;
- `RemoveExactLink(SourceObjectId, TargetObjectId, SourceEvent, Command)` ;

restent disponibles. Elles représentent désormais explicitement le cas sans condition (`Condition=None`) et délèguent au service centralisé.

L’outil `Link` du viewport crée lui aussi un lien `Condition=None` via le même service. Il ne possède donc plus sa propre définition du doublon.

## 6. Tests automatisés

Deux tests sont ajoutés au préfixe `Grimrock.MON19.2.Editor` :

- `Grimrock.MON19.2.Editor.ExactLinkMutations` ;
- `Grimrock.MON19.2.Editor.ConditionConfiguration`.

Ils complètent les tests déjà validés :

- `Grimrock.MON19.2.Editor.ConditionalLinkIdentity` ;
- `Grimrock.MON19.2.Editor.LinkPolicyMatrix`.

La suite MON19.2 éditeur doit donc comporter quatre tests après cette étape.

## 7. Hors périmètre

MON19.2.1B n’ajoute pas :

- de nouvelle condition runtime ;
- de variables logiques de niveau ;
- de primitive de compteur ou de relais ;
- de Lua ;
- de nouvelle sémantique de ciblage des conditions ;
- de modification `.uasset` ou `.umap`.

Les variables persistantes et primitives logiques appartiennent aux étapes MON19.2 suivantes.

## Validation attendue

Après compilation UE5.5.4, exécuter :

```text
Grimrock.MON19.2.Editor
```

Résultat attendu : quatre tests `Success`.

Une vérification manuelle du panneau CONNECTORS est également recommandée : sélectionner une cible `Receptacle`, constater l’apparition des conditions, créer deux variantes partageant le même quadruplet avec des conditions différentes, puis supprimer une variante et vérifier que l’autre reste présente.
