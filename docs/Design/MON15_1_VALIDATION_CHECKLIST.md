# MON15.1 — Validation Checklist

Statut : **à exécuter sous Unreal Engine 5.5.4**.

MON15.1 ne doit pas être marqué `Validé` avant réception des résultats UE5.

---

## 1. Compilation

- [ ] Ouvrir/compiler `GrimrockPrototype` avec Unreal Engine 5.5.4 / Visual Studio.
- [ ] Vérifier qu'aucune erreur UHT/C++ n'est introduite dans `RPGCharacterRulesLibrary`.
- [ ] Vérifier qu'aucun `.uasset`, `.umap` ou WBP n'est requis pour compiler MON15.1.

---

## 2. Automation Tests dédiés

Exécuter :

```text
Grimrock.RPG.MON15.1.ProgressionCurve
Grimrock.RPG.MON15.1.LevelFromExperience
Grimrock.RPG.MON15.1.ProgressionBoundaries
Grimrock.RPG.MON15.1.ExistingCharacterState
```

Résultats attendus :

- [ ] `ProgressionCurve` — Success
- [ ] `LevelFromExperience` — Success
- [ ] `ProgressionBoundaries` — Success
- [ ] `ExistingCharacterState` — Success

---

## 3. Contrats couverts par les tests

### Courbe

- [ ] Niveau 1 = 0 XP.
- [ ] Seuil exact niveau 2 = 1 000 XP.
- [ ] 999 XP reste niveau 1.
- [ ] 1 001 XP reste niveau 2.
- [ ] Plusieurs seuils successifs sont vérifiés.
- [ ] Niveau maximum = 20.
- [ ] Seuil niveau 20 = 190 000 XP.
- [ ] Tous les seuils 1 → 20 sont strictement croissants.

### Bornes

- [ ] XP négative normalisée à 0 pour les calculs.
- [ ] XP supérieure à 190 000 normalisée à 190 000 pour les calculs.
- [ ] XP supérieure au plafond reconstruit toujours le niveau 20.
- [ ] Au niveau maximum, XP restante vers le niveau suivant = 0.
- [ ] Les niveaux invalides demandés à la fonction de seuil sont bornés à 1..20.

### Reconstruction et cohérence

- [ ] `Level <- Experience` fonctionne aux seuils exacts.
- [ ] Un `Level` périmé/incohérent est détecté.
- [ ] Une XP brute négative est considérée incohérente.
- [ ] Une XP brute supérieure au plafond est considérée incohérente.
- [ ] Un `FGridCharacterInventoryState` existant peut être validé sans migration ni mutation.

### Pureté

- [ ] Les helpers ne modifient pas `Level`.
- [ ] Les helpers ne modifient pas `Experience`.
- [ ] Les helpers ne modifient pas `Attributes`.
- [ ] Les helpers ne modifient pas `DerivedStats`.
- [ ] Les helpers ne modifient pas l'inventaire.
- [ ] Les helpers ne modifient pas la hotbar.

---

## 4. Non-régression recommandée

MON15.1 ne touche pas directement aux systèmes ci-dessous, mais ces tests sont utiles si une validation plus large est souhaitée :

```text
Grimrock.CharacterCreation.CC2
```

Puis les suites existantes de création de personnage, inventaire et sauvegarde habituellement utilisées dans le projet.

Points à observer :

- [ ] un nouveau personnage commence toujours niveau 1 / 0 XP ;
- [ ] les statistiques initiales restent identiques ;
- [ ] l'inventaire et l'équipement restent inchangés ;
- [ ] la hotbar existante reste inchangée ;
- [ ] une sauvegarde v1-v3 compatible continue d'utiliser le format existant.

---

## 5. Validation manuelle

Aucune validation PIE spécifique n'est requise par MON15.1 car :

- aucun comportement runtime d'attribution XP n'est ajouté ;
- aucun level-up automatique n'est ajouté ;
- aucun widget n'est modifié ;
- aucun asset/map n'est modifié.

Une ouverture du projet et une vérification de création de personnage peuvent néanmoins servir de smoke test de non-régression.

---

## 6. Critère de validation

MON15.1 pourra être marqué `Validé` après confirmation au minimum de :

```text
Compilation UE5.5.4 : OK
Grimrock.RPG.MON15.1.ProgressionCurve : Success
Grimrock.RPG.MON15.1.LevelFromExperience : Success
Grimrock.RPG.MON15.1.ProgressionBoundaries : Success
Grimrock.RPG.MON15.1.ExistingCharacterState : Success
```

En cas d'échec, fournir le log complet de compilation ou des Automation Tests avant de poursuivre MON15.2.
