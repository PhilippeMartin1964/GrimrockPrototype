# MON15.1 — Validation Checklist

Statut : **VALIDÉ sous Unreal Engine 5.5.4 — Automation Tests fournis le 15 août 2026**.

Les résultats ci-dessous proviennent des logs UE5.5.4 fournis après exécution des tests. Aucun log séparé de commande de compilation n'a été fourni ; les cases de compilation distincte ne sont donc pas cochées par déduction.

---

## 1. Compilation

- [ ] Compilation distincte `GrimrockPrototype` avec Unreal Engine 5.5.4 / Visual Studio documentée par un log de build séparé.
- [ ] Vérification UHT/C++ documentée par ce même log de build séparé.
- [x] Aucun `.uasset`, `.umap` ou WBP n'est requis par le patch MON15.1.

Note : les Automation Tests MON15.1 ont bien été chargés et exécutés par Unreal Engine 5.5.4, mais cette checklist ne transforme pas ce fait en affirmation d'une commande de compilation séparée non fournie.

---

## 2. Automation Tests dédiés

Tests exécutés :

```text
Grimrock.RPG.MON15.1.ProgressionCurve
Grimrock.RPG.MON15.1.LevelFromExperience
Grimrock.RPG.MON15.1.ProgressionBoundaries
Grimrock.RPG.MON15.1.ExistingCharacterState
```

Résultats fournis :

- [x] `ProgressionCurve` — Success
- [x] `LevelFromExperience` — Success
- [x] `ProgressionBoundaries` — Success
- [x] `ExistingCharacterState` — Success

---

## 3. Contrats couverts par les tests

### Courbe

- [x] Niveau 1 = 0 XP.
- [x] Seuil exact niveau 2 = 1 000 XP.
- [x] 999 XP reste niveau 1.
- [x] 1 001 XP reste niveau 2.
- [x] Plusieurs seuils successifs sont vérifiés.
- [x] Niveau maximum = 20.
- [x] Seuil niveau 20 = 190 000 XP.
- [x] Tous les seuils 1 → 20 sont strictement croissants.

### Bornes

- [x] XP négative normalisée à 0 pour les calculs.
- [x] XP supérieure à 190 000 normalisée à 190 000 pour les calculs.
- [x] XP supérieure au plafond reconstruit toujours le niveau 20.
- [x] Au niveau maximum, XP restante vers le niveau suivant = 0.
- [x] Les niveaux invalides demandés à la fonction de seuil sont bornés à 1..20.

### Reconstruction et cohérence

- [x] `Level <- Experience` fonctionne aux seuils exacts.
- [x] Un `Level` périmé/incohérent est détecté.
- [x] Une XP brute négative est considérée incohérente.
- [x] Une XP brute supérieure au plafond est considérée incohérente.
- [x] Un `FGridCharacterInventoryState` existant peut être validé sans migration ni mutation.

### Pureté

- [x] Les helpers ne modifient pas `Level`.
- [x] Les helpers ne modifient pas `Experience`.
- [x] Les helpers ne modifient pas `Attributes`.
- [x] Les helpers ne modifient pas `DerivedStats`.
- [x] Les helpers ne modifient pas l'inventaire.
- [x] Les helpers ne modifient pas la hotbar.

---

## 4. Non-régression exécutée

La suite suivante a été exécutée dans la même validation UE5.5.4 :

```text
Grimrock.CharacterCreation.CC2.CreateInitialCharacter
Grimrock.CharacterCreation.CC2.RejectInvalidRequestAtomically
Grimrock.CharacterCreation.CC2.RejectSecondCreation
```

Résultats :

- [x] `CreateInitialCharacter` — Success
- [x] `RejectInvalidRequestAtomically` — Success
- [x] `RejectSecondCreation` — Success

Contrats directement couverts par cette suite :

- [x] un nouveau personnage commence toujours niveau 1 / 0 XP ;
- [x] les statistiques initiales couvertes par CC2 restent identiques ;
- [x] la création invalide reste atomique ;
- [x] une seconde création initiale reste refusée sans remplacer le personnage existant.

Non couverts par les logs fournis lors de cette validation spécifique :

- [ ] suite complète inventaire ;
- [ ] suite complète équipement ;
- [ ] suite complète SaveGame/Continue ;
- [ ] suite complète combat/hotbar.

Ces suites n'étaient pas requises pour valider le modèle pur MON15.1, qui ne modifie aucun de ces systèmes.

---

## 5. Validation manuelle

Aucune validation PIE spécifique n'est requise par MON15.1 car :

- aucun comportement runtime d'attribution XP n'est ajouté ;
- aucun level-up automatique n'est ajouté ;
- aucun widget n'est modifié ;
- aucun asset/map n'est modifié.

Aucun résultat PIE n'est donc revendiqué pour MON15.1.

---

## 6. Résultat final

Validation UE5.5.4 fournie le 15 août 2026 :

```text
Grimrock.RPG.MON15.1.ProgressionCurve          Success
Grimrock.RPG.MON15.1.LevelFromExperience       Success
Grimrock.RPG.MON15.1.ProgressionBoundaries     Success
Grimrock.RPG.MON15.1.ExistingCharacterState    Success

Grimrock.CharacterCreation.CC2.CreateInitialCharacter          Success
Grimrock.CharacterCreation.CC2.RejectInvalidRequestAtomically  Success
Grimrock.CharacterCreation.CC2.RejectSecondCreation             Success
```

Conclusion : **MON15.1 — VALIDÉ ET CLOS**.

Prochain sous-jalon autorisé :

```text
MON15.2 — Attribution XP après combat
```
