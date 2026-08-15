# MON15.4 — Validation Checklist

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**.

---

## 1. Compilation / chargement UE

- [x] Le module MON15.4 est compilé et chargé dans Unreal Engine 5.5.4.
- [x] `RPGClassAsset.h/.cpp` chargé par les Automation Tests.
- [x] `RPGClassProgressionService.h/.cpp` chargé par les Automation Tests.
- [x] Aucun `.uasset`, `.umap` ou WBP requis ou modifié.

Aucun transcript UBT/Visual Studio séparé n'est archivé dans cette validation ; les campagnes Automation ont exécuté le code compilé sous UE5.5.4.

---

## 2. Tests MON15.4

- [x] `DefinitionValidation` — Success
- [x] `ChoicePointAccounting` — Success
- [x] `SelectionRules` — Success
- [x] `RequirementProjection` — Success
- [x] `CombatActionUnlockProjection` — Success
- [x] `LevelUpIntegration` — Success
- [x] `LegacyCompatibility` — Success

Le premier passage de `DefinitionValidation` a déclenché `TArray::CheckAddress` dans le fixture de test. Cause : insertion dans un `TArray` à partir d'une référence vers un élément du même tableau. Correctif appliqué dans `64f0571b90c30d311edfd49a25366804aada4b9f` par copie locale avant `Add()`. Nouvelle campagne : aucun assert, aucune exception, tous les tests Success.

---

## 3. Validation des données de classe

- [x] Une classe sans progression MON15.4 reste valide.
- [x] Les niveaux de grant sont bornés par les niveaux RPG.
- [x] Deux grants au même niveau sont refusés.
- [x] Un grant vide est refusé.
- [x] Les points négatifs sont refusés.
- [x] Les requirements invalides/dupliqués sont refusés.
- [x] Un `ChoiceId` vide ou dupliqué est refusé.
- [x] Un coût de choix invalide est refusé.
- [x] Un prérequis inconnu est refusé.
- [x] Un auto-prérequis est refusé.
- [x] Un cycle entre choix est refusé.

---

## 4. Points et règles de choix

Fixture validée :

```text
Level 1 -> 0 point
Level 2 -> 1 point cumulé
Level 3 -> 2 points cumulés
Level 4 -> 4 points cumulés
```

- [x] Grants cumulatifs.
- [x] Service pur sans mutation du personnage.
- [x] `Spent` = somme des coûts des choix valides.
- [x] `Remaining = Granted - Spent`.
- [x] Dépassement de budget refusé.
- [x] `MinimumLevel` appliqué.
- [x] `PrerequisiteChoiceIds` appliqué.
- [x] Choix déjà sélectionné signalé.
- [x] Choix inconnu signalé.
- [x] Budget insuffisant signalé.

---

## 5. Projection des requirements

- [x] `ClassId` projeté pour une classe valide.
- [x] Grant niveau 2 absent au niveau 1.
- [x] Grant niveau 2 présent au niveau 2+.
- [x] `ChoiceId` sélectionné projeté comme requirement.
- [x] `GrantedRequirementIds` projetés.
- [x] État hypothétique invalide rejeté.

---

## 6. Catalogue MON12

Le contrat unique reste :

```cpp
FGridCombatActionDefinition::Requirements
```

Validé par MON15.4 :

```text
Action Requirements=[Feature_Level2]
Level 1 -> MissingRequirement
Level 2 -> Enabled
```

et :

```text
Action Requirements=[Choice_A]
Choice_A absent -> locked
Choice_A présent dans une sélection valide -> enabled
```

- [x] Aucun second mécanisme d'activation d'action créé.
- [x] `Requirements` reste le contrat unique.

---

## 7. Raccord MON15.3

- [x] Personnage niveau 1 / XP 1000.
- [x] `FRPGLevelUpService` applique `1 -> 2`.
- [x] MON15.4 dérive ensuite 1 point disponible.
- [x] `Feature_Level2` est projeté automatiquement.
- [x] Aucun champ persistant supplémentaire n'est muté pendant le level-up.

---

## 8. SaveGame / compatibilité

- [x] Aucun nouveau champ dans `FGridCharacterInventoryState`.
- [x] Aucun nouveau champ dans `UGrimrockPartySaveGame`.
- [x] `CurrentSaveVersion == 3`.
- [x] Une classe historique sans données MON15.4 reste valide.

---

## 9. Régressions MON15 / CharacterCreation

Campagne validée :

- [x] MON15.1 — Success.
- [x] MON15.2 — Success.
- [x] MON15.3 — Success.
- [x] CharacterCreation CC1 — Success.
- [x] CharacterCreation CC2 — Success.
- [x] CharacterCreation CC6 — Success.
- [x] CC0 / CC4 / CC5 également verts dans la campagne fournie.

---

## 10. Régressions ActionCatalog / Hotbar

Campagne complémentaire validée :

```text
Grimrock.Monsters.MON12.ActionCatalog.Contributions             Success
Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle    Success
Grimrock.Monsters.MON12.8.*                                     Success
```

La famille MON12.8 exécutée couvre notamment :

- hotbar vide par défaut ;
- persistance / migration ;
- bindings par personnage ;
- rejet atomique des bindings invalides ;
- move/swap et drag/drop ;
- exécution clic et clavier ;
- mains nues ;
- quick items et suppression de binding à consommation ;
- parchemins ;
- capacités et sorts de classe avec mana ;
- ciblage cellule et zone ;
- arme de jet depuis inventaire ;
- sanitation des bindings historiques.

- [x] Catalogue d'actions sans régression.
- [x] Hotbar sans régression.
- [x] Transactions PA/mana/items sans régression observée.

---

## 11. PIE

Aucun PIE spécifique n'était requis pour MON15.4, car aucun choix réel n'est encore committé et aucun DataAsset de classe de production n'a été migré.

Le premier PIE de sélection réelle appartient à MON15.5.

---

## 12. Critère de clôture

```text
Compilation/chargement UE5.5.4                 OK
7 tests Grimrock.RPG.MON15.4.*                  Success
Régressions MON15.1 / MON15.2 / MON15.3         Success
Régressions CharacterCreation pertinentes       Success
Régressions ActionCatalog / MON12.8              Success
SaveVersion                                      3
Aucun asset/WBP modifié                          OK
Assert TArray du fixture                         Corrigé et non reproduit
```

**MON15.4 est VALIDÉ ET CLOS.**
