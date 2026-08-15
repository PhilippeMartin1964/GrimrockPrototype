# MON15.4 — Progression propre aux classes

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**.

MON15.4 définit le modèle data-driven des points, grants et choix de progression propres à chaque classe. Il prépare la transaction/interface de sélection de MON15.5 et la persistance/migration de MON15.6 sans introduire prématurément un second état persistant dans le personnage.

---

## 1. Principe

La progression reste attachée à `URPGClassAsset` au moyen de deux structures :

```text
FRPGClassProgressionLevelGrant
FRPGClassProgressionChoiceDefinition
```

Une classe qui ne renseigne aucun de ces tableaux conserve le comportement antérieur. Aucun `.uasset`, `.umap` ou WBP de production n'a été modifié par MON15.4.

---

## 2. Grants automatiques par niveau

`FRPGClassProgressionLevelGrant` contient :

```text
Level
ChoicePointsGranted
GrantedRequirementIds[]
```

Les grants sont cumulatifs. Les points disponibles ne sont pas stockés comme un compteur mutable : ils sont reconstruits depuis la classe et le niveau appliqué, ce qui conserve `Level` comme source de vérité.

Exemple :

```text
Niveau 2 : +1 point, Feature_WeaponTraining
Niveau 3 : +1 point
Niveau 4 : +2 points, Feature_ExtraAttack
```

Au niveau 4, le personnage dispose donc de 4 points accordés au total et satisfait les requirements automatiques des niveaux atteints.

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

`ChoiceId` est une identité stable. Un choix peut coûter un ou plusieurs points, imposer un niveau minimum, dépendre d'autres choix et accorder des requirement tags supplémentaires.

MON15.4 évalue les choix comme candidats mais ne les committe pas encore dans le personnage.

---

## 4. Validation de `URPGClassAsset`

`URPGClassAsset::IsValidDefinition()` valide notamment :

- niveau de grant dans la plage RPG autorisée ;
- aucun niveau de grant dupliqué ;
- aucun grant vide ;
- aucun point négatif ;
- aucun requirement vide ou dupliqué ;
- `ChoiceId` obligatoire et unique ;
- niveau minimum valide ;
- coût strictement positif ;
- prérequis connus ;
- pas d'auto-prérequis ;
- pas de cycle entre choix.

La validation existante des `CombatActions` reste le contrat de référence pour les actions de classe.

---

## 5. `FRPGClassProgressionService`

Le service MON15.4 est pur et sans état persistant. Son API principale est :

```cpp
GetTotalChoicePointsGranted(...)
TryGetChoicePointBalance(...)
GetChoiceAvailability(...)
CollectSatisfiedRequirements(...)
CollectAutomaticSatisfiedRequirements(...)
```

Il ne modifie jamais le personnage.

---

## 6. Comptabilité des points

Les points accordés sont la somme des grants dont :

```text
Grant.Level <= CharacterLevel
```

Pour un ensemble hypothétique de choix :

```text
Spent = somme des PointCost
Remaining = Granted - Spent
```

L'ensemble est rejeté si un choix est inconnu, si son niveau minimum n'est pas atteint, si un prérequis manque ou si le budget est dépassé.

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
None
```

`None` signifie que le choix est sélectionnable dans l'état hypothétique fourni.

---

## 8. Requirements et capacités

MON15.4 réutilise le contrat existant de MON12 :

```cpp
FGridCombatActionDefinition::Requirements
```

Aucun second mécanisme d'activation d'action n'est introduit.

`CollectSatisfiedRequirements()` projette :

- `ClassId` ;
- les requirements automatiques accordés par le niveau ;
- les `ChoiceId` hypothétiquement sélectionnés ;
- les requirements supplémentaires accordés par ces choix.

Ainsi une action déclarant :

```text
Requirements=[Feature_Level2]
```

reste `MissingRequirement` au niveau 1 et devient disponible dès que le niveau 2 projette `Feature_Level2`.

---

## 9. Intégration à MON15.3

Les points et grants sont dérivés du `Level` déjà appliqué par `FRPGLevelUpService` :

```text
XP franchit un seuil
    -> MON15.3 applique Level
    -> MON15.4 dérive immédiatement points et requirements du nouveau niveau
```

Un saut multi-niveau récupère naturellement tous les grants intermédiaires.

---

## 10. Limite volontaire avant MON15.5

MON15.4 ne committe pas encore un choix dans `FGridCharacterInventoryState`.

Découpage retenu :

```text
MON15.4 = données et règles pures
MON15.5 = transaction atomique de sélection + interface Level Up
MON15.6 = sauvegarde, restauration et migration des sélections
```

Cette séparation évite d'introduire un état persistant sans migration complète.

---

## 11. SaveGame

MON15.4 n'ajoute aucun champ à :

```text
FGridCharacterInventoryState
UGrimrockPartySaveGame
```

`CurrentSaveVersion` reste `3`. Aucune migration n'est nécessaire pour ce sous-jalon.

---

## 12. Tests dédiés validés

Les sept tests MON15.4 sont validés sous UE5.5.4 :

```text
Grimrock.RPG.MON15.4.DefinitionValidation            Success
Grimrock.RPG.MON15.4.ChoicePointAccounting           Success
Grimrock.RPG.MON15.4.SelectionRules                  Success
Grimrock.RPG.MON15.4.RequirementProjection           Success
Grimrock.RPG.MON15.4.CombatActionUnlockProjection    Success
Grimrock.RPG.MON15.4.LevelUpIntegration              Success
Grimrock.RPG.MON15.4.LegacyCompatibility             Success
```

Le premier passage de `DefinitionValidation` a révélé un aliasing de fixture dans `TArray::Add()` ; le test a été corrigé par copie locale avant insertion dans le commit `64f0571b90c30d311edfd49a25366804aada4b9f`. La campagne suivante est passée sans assert ni exception.

---

## 13. Régressions validées

La campagne de validation a confirmé :

- MON15.1 — Success ;
- MON15.2 — Success ;
- MON15.3 — Success ;
- CharacterCreation CC1 / CC2 / CC6 — Success, avec CC0 / CC4 / CC5 également verts dans la campagne ;
- `Grimrock.Monsters.MON12.ActionCatalog.Contributions` — Success ;
- `Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle` — Success ;
- toute la famille `Grimrock.Monsters.MON12.8.*` exécutée — Success.

Les tests MON12.8 ont couvert notamment la persistance et migration de hotbar, les bindings par personnage, le drag/drop, l'exécution clic/clavier, mains nues, objets rapides, consommation de parchemin, capacités/sorts de classe, ciblage cellule/zone, projectile d'inventaire et nettoyage des bindings après consommation.

Aucun PIE spécifique n'était requis pour MON15.4 : aucune sélection réelle n'est encore committée et aucun asset de classe de production n'a été migré.

---

## 14. Conclusion

MON15.4 est **VALIDÉ ET CLOS**.

Les invariants retenus sont :

- progression de classe data-driven ;
- points dérivés du niveau, pas de compteur mutable redondant ;
- prérequis et cycles validés dans les DataAssets ;
- `FGridCombatActionDefinition::Requirements` reste le contrat unique d'accès aux actions ;
- aucune modification SaveGame ni SaveVersion ;
- anciennes classes compatibles ;
- MON15.5 peut maintenant ajouter la transaction réelle de choix et son interface.
