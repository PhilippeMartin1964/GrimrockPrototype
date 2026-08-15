# MON15.4 — Progression propre aux classes

Statut : **implémenté en C++ — validation UE5.5.4 en attente**.

MON15.4 définit le modèle data-driven des points et choix de progression propres à chaque classe. Il prépare les sélections de MON15.5 et leur persistance/migration de MON15.6 sans introduire maintenant un second état personnage incomplet.

---

## 1. Principe

La progression reste attachée à `URPGClassAsset`.

Deux types de données sont ajoutés :

```text
FRPGClassProgressionLevelGrant
FRPGClassProgressionChoiceDefinition
```

Une classe qui ne renseigne aucun de ces tableaux conserve exactement le comportement antérieur.

Aucun `.uasset` de production n'est modifié par MON15.4.

---

## 2. Grants automatiques par niveau

`FRPGClassProgressionLevelGrant` contient :

```text
Level
ChoicePointsGranted
GrantedRequirementIds[]
```

Les grants sont cumulatifs.

Exemple :

```text
Niveau 2 : +1 point, Feature_WeaponTraining
Niveau 3 : +1 point
Niveau 4 : +2 points, Feature_ExtraAttack
```

Un personnage niveau 4 possède donc 4 points accordés au total et satisfait les requirements automatiques des niveaux 2 et 4.

Les points ne sont jamais stockés comme un compteur mutable : ils sont reconstruits depuis la classe et le niveau appliqué. Cela évite une deuxième source de vérité à côté de `Level`.

---

## 3. Choix de classe

`FRPGClassProgressionChoiceDefinition` contient :

```text
ChoiceId
DisplayName
Description
MinimumLevel
PointCost
PrerequisiteChoiceIds[]
GrantedRequirementIds[]
```

`ChoiceId` est une identité stable. Lorsqu'un choix sera effectivement sélectionné dans MON15.5, son `ChoiceId` lui-même deviendra aussi un requirement satisfait.

Un choix peut donc :

- coûter un ou plusieurs points ;
- exiger un niveau minimum ;
- exiger d'autres choix ;
- accorder des tags génériques supplémentaires.

---

## 4. Validation de `URPGClassAsset`

`IsValidDefinition()` valide désormais aussi la progression :

- niveaux de grant compris entre le niveau minimum et le niveau maximum RPG ;
- aucun niveau de grant dupliqué ;
- aucun grant vide ;
- aucun point négatif ;
- aucun requirement vide ou dupliqué dans un même tableau ;
- aucun requirement automatique accordé plusieurs fois ;
- `ChoiceId` obligatoire et unique ;
- niveau minimum valide ;
- coût strictement positif ;
- prérequis connus ;
- pas d'auto-prérequis ;
- pas de cycle entre choix.

La validation existante des `CombatActions` reste inchangée.

---

## 5. `FRPGClassProgressionService`

Le nouveau service est pur et sans état persistant.

API principale :

```cpp
GetTotalChoicePointsGranted(...)
TryGetChoicePointBalance(...)
GetChoiceAvailability(...)
CollectSatisfiedRequirements(...)
CollectAutomaticSatisfiedRequirements(...)
```

Il ne modifie jamais un personnage.

---

## 6. Comptabilité des points

Les points accordés sont calculés par somme des grants dont :

```text
Grant.Level <= CharacterLevel
```

Pour un ensemble hypothétique de choix sélectionnés :

```text
Spent = somme des PointCost
Remaining = Granted - Spent
```

La sélection hypothétique est rejetée si :

- un choix n'existe pas ;
- son niveau minimum n'est pas atteint ;
- un prérequis manque ;
- le total dépensé dépasse les points accordés.

MON15.4 permet donc de tester complètement une transaction avant que MON15.5 ne la committe.

---

## 7. Raisons d'indisponibilité

`ERPGClassProgressionChoiceAvailabilityReason` distingue :

```text
InvalidClassDefinition
InvalidLevel
InvalidSelectionState
UnknownChoice
AlreadySelected
LevelTooLow
MissingPrerequisite
InsufficientChoicePoints
```

`None` signifie que le choix est sélectionnable dans l'état hypothétique fourni.

---

## 8. Requirements et capacités

MON12 possède déjà :

```cpp
FGridCombatActionDefinition::Requirements
```

MON15.4 réutilise ce contrat au lieu d'introduire un second système de conditions.

`CollectSatisfiedRequirements()` produit :

- `ClassId` ;
- les requirements automatiques accordés par le niveau ;
- les `ChoiceId` sélectionnés dans l'état hypothétique ;
- les requirements supplémentaires accordés par ces choix.

Exemple :

```text
Level grant 2 -> Feature_PowerStrike
Combat action Requirements=[Feature_PowerStrike]
```

Avant le niveau 2, le catalogue MON12 renvoie `MissingRequirement`.
Après projection du niveau 2, le même catalogue peut rendre l'action disponible.

Pour un choix :

```text
ChoiceId=Talent_Cleave
Combat action Requirements=[Talent_Cleave]
```

la capacité devient disponible dès que l'ensemble de choix validé contient `Talent_Cleave`.

---

## 9. Intégration à MON15.3

Les points et grants sont dérivés du `Level` déjà appliqué par `FRPGLevelUpService`.

Il n'existe donc aucune transaction supplémentaire lors du level-up :

```text
XP franchit un seuil
    -> MON15.3 applique Level
    -> MON15.4 dérive immédiatement les nouveaux points/grants depuis ce Level
```

Un saut multi-niveau récupère naturellement tous les grants intermédiaires.

---

## 10. Limite volontaire avant MON15.5

MON15.4 **ne committe pas encore un choix dans `FGridCharacterInventoryState`**.

C'est intentionnel :

- MON15.4 = règles pures et données ;
- MON15.5 = notification/interface et transaction atomique de sélection ;
- MON15.6 = sauvegarde, restauration et migration des sélections.

Le runtime actuel peut donc consommer les grants automatiques par niveau via le service, tandis que les choix restent évalués comme candidats jusqu'à MON15.5.

Cette séparation évite d'introduire en MON15.4 un champ persistant sans migration complète.

---

## 11. SaveGame

MON15.4 n'ajoute aucun champ à :

```text
FGridCharacterInventoryState
UGrimrockPartySaveGame
```

`CurrentSaveVersion` reste `3`.

Aucune migration n'est nécessaire dans ce sous-jalon.

---

## 12. Préparation des jalons futurs

Le système de requirement tags est volontairement générique.

Il pourra servir à :

- MON15.5 : choix et interface de level-up ;
- MON16 : accès à certains effets/statuts ;
- MON18 : écoles de magie, rangs de sorts, capacités de classe ;
- MON20 : talents et compétences plus avancés.

Il ne dépend d'aucun Widget Blueprint.

---

## 13. Automation Tests

Suite dédiée :

```text
Grimrock.RPG.MON15.4.DefinitionValidation
Grimrock.RPG.MON15.4.ChoicePointAccounting
Grimrock.RPG.MON15.4.SelectionRules
Grimrock.RPG.MON15.4.RequirementProjection
Grimrock.RPG.MON15.4.CombatActionUnlockProjection
Grimrock.RPG.MON15.4.LevelUpIntegration
Grimrock.RPG.MON15.4.LegacyCompatibility
```

Elle couvre :

- validation des données ;
- cumul des points ;
- niveau minimum ;
- prérequis ;
- budget insuffisant ;
- projection des requirements ;
- raccord avec le catalogue MON12 ;
- raccord avec le level-up MON15.3 ;
- compatibilité des classes historiques ;
- absence de changement SaveVersion.

---

## 14. Porte de sortie

MON15.4 pourra être marqué **VALIDÉ ET CLOS** lorsque :

- le projet compile sous UE5.5.4 ;
- les sept tests MON15.4 réussissent ;
- MON15.1, MON15.2 et MON15.3 restent verts ;
- les tests de classe/création pertinents restent verts ;
- les tests du catalogue d'actions MON12 pertinents restent verts ;
- aucune régression SaveGame n'est observée.
