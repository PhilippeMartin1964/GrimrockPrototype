# MON15.4 — Validation Checklist

Statut : **à valider sous Unreal Engine 5.5.4**.

Ne pas commencer MON15.5 avant validation de cette checklist.

---

## 1. Compilation

- [ ] Compiler `GrimrockPrototype` sous UE5.5.4 / Visual Studio.
- [ ] Vérifier `RPGClassAsset.h/.cpp`.
- [ ] Vérifier `RPGClassProgressionService.h/.cpp`.
- [ ] Aucun `.uasset`, `.umap` ou WBP requis.

---

## 2. Tests MON15.4

Exécuter :

```text
Grimrock.RPG.MON15.4.DefinitionValidation
Grimrock.RPG.MON15.4.ChoicePointAccounting
Grimrock.RPG.MON15.4.SelectionRules
Grimrock.RPG.MON15.4.RequirementProjection
Grimrock.RPG.MON15.4.CombatActionUnlockProjection
Grimrock.RPG.MON15.4.LevelUpIntegration
Grimrock.RPG.MON15.4.LegacyCompatibility
```

Attendus :

- [ ] `DefinitionValidation` — Success
- [ ] `ChoicePointAccounting` — Success
- [ ] `SelectionRules` — Success
- [ ] `RequirementProjection` — Success
- [ ] `CombatActionUnlockProjection` — Success
- [ ] `LevelUpIntegration` — Success
- [ ] `LegacyCompatibility` — Success

---

## 3. Validation des assets de classe

- [ ] Une classe sans progression MON15.4 reste valide.
- [ ] Les niveaux de grant sont dans `[1, 20]`.
- [ ] Deux grants au même niveau sont refusés.
- [ ] Un grant vide est refusé.
- [ ] Les points négatifs sont refusés.
- [ ] Les requirements vides/dupliqués sont refusés.
- [ ] Un `ChoiceId` vide ou dupliqué est refusé.
- [ ] Un coût de choix nul/négatif est refusé.
- [ ] Un prérequis inconnu est refusé.
- [ ] Un choix ne peut pas se requérir lui-même.
- [ ] Un cycle entre choix est refusé.

---

## 4. Points de progression

Avec la fixture MON15.4 :

```text
Level 1 -> 0 point
Level 2 -> 1 point cumulé
Level 3 -> 2 points cumulés
Level 4 -> 4 points cumulés
```

- [ ] Les grants sont cumulatifs.
- [ ] Les points ne sont pas consommés/modifiés par le service pur.
- [ ] `Spent` est la somme des coûts des choix hypothétiques valides.
- [ ] `Remaining = Granted - Spent`.
- [ ] Un ensemble qui dépasse le budget est invalide.

---

## 5. Règles de choix

- [ ] `MinimumLevel` est appliqué.
- [ ] `PrerequisiteChoiceIds` est appliqué.
- [ ] Un choix déjà sélectionné est signalé.
- [ ] Un choix inconnu est signalé.
- [ ] Un budget insuffisant est signalé.
- [ ] Aucun état personnage n'est muté par MON15.4.

---

## 6. Projection des requirements

- [ ] `ClassId` est satisfait pour une classe valide.
- [ ] Un grant niveau 2 n'apparaît pas au niveau 1.
- [ ] Il apparaît au niveau 2 et au-dessus.
- [ ] Un `ChoiceId` hypothétiquement sélectionné devient un requirement satisfait.
- [ ] `GrantedRequirementIds` d'un choix sont projetés.
- [ ] Un état de sélection hypothétique invalide ne produit aucun set fiable.

---

## 7. Catalogue MON12

Le test `CombatActionUnlockProjection` doit prouver :

```text
Action Requirements=[Feature_Level2]
Level 1 -> MissingRequirement
Level 2 -> Enabled
```

et :

```text
Action Requirements=[Choice_A]
Choice_A absent  -> locked
Choice_A validé  -> enabled dans la projection hypothétique
```

- [ ] Aucun second mécanisme d'activation d'action n'est créé.
- [ ] `FGridCombatActionDefinition::Requirements` reste le contrat unique.

---

## 8. Raccord MON15.3

- [ ] Personnage niveau 1 / XP 1000.
- [ ] `FRPGLevelUpService` applique `1 -> 2`.
- [ ] MON15.4 dérive ensuite 1 point disponible.
- [ ] `Feature_Level2` est projeté automatiquement.
- [ ] Aucun champ supplémentaire n'est muté pendant le level-up.

---

## 9. SaveGame / compatibilité

- [ ] Aucun nouveau champ dans `FGridCharacterInventoryState`.
- [ ] Aucun nouveau champ dans `UGrimrockPartySaveGame`.
- [ ] `CurrentSaveVersion == 3`.
- [ ] Une classe historique sans `ProgressionLevelGrants`/`ProgressionChoices` reste valide.

---

## 10. Régressions recommandées

Exécuter au minimum :

```text
Grimrock.RPG.MON15.1
Grimrock.RPG.MON15.2
Grimrock.RPG.MON15.3
Grimrock.CharacterCreation.CC1
Grimrock.CharacterCreation.CC2
Grimrock.CharacterCreation.CC6
Grimrock.Monsters.MON12.6
Grimrock.Monsters.MON12.8
```

Si les chemins MON12 sont subdivisés différemment dans Automation, exécuter les tests contenant `ActionCatalog`, `CombatActionPanel` et `CombatHud` pertinents.

- [ ] MON15.1 reste vert.
- [ ] MON15.2 reste vert.
- [ ] MON15.3 reste vert.
- [ ] Création de personnage/classe reste verte.
- [ ] Catalogue/actions/hotbar restent verts.

---

## 11. PIE

Aucun changement de `.uasset` de production n'est requis pour valider MON15.4.

Un PIE spécifique n'est pas une porte obligatoire à ce stade, car aucun choix n'est encore committé et aucun asset de classe de production n'est migré. La preuve runtime est assurée par le raccord au catalogue MON12 et par `LevelUpIntegration`.

Le premier test PIE de choix réel sera pertinent en MON15.5 après ajout de la transaction de sélection et de l'interface.

---

## 12. Critère de clôture

```text
Compilation UE5.5.4                         OK
7 tests Grimrock.RPG.MON15.4.*              Success
Régressions MON15.1 / MON15.2 / MON15.3     Success
Régressions CharacterCreation pertinentes   Success
Régressions action catalogue/hotbar          Success
SaveVersion                                  3
Aucun asset/WBP modifié                      OK
```

Lorsque ces éléments sont confirmés, MON15.4 peut être marqué **VALIDÉ ET CLOS**.
