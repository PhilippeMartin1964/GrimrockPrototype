# MON15.2 — Validation Checklist

Statut : **à valider sous Unreal Engine 5.5.4**.

MON15.2 ne doit pas être marqué `Validé` avant réception des résultats UE5 et du scénario PIE demandé.

---

## 1. Compilation

- [ ] Compiler `GrimrockPrototype` sous Unreal Engine 5.5.4 / Visual Studio.
- [ ] Vérifier qu'aucune erreur UHT/C++ n'est introduite par `RPGExperienceRewardService`.
- [ ] Vérifier que `GridMonsterDeathComponent.cpp` compile avec le raccord MON15.2.
- [ ] Vérifier qu'aucun `.uasset`, `.umap` ou WBP n'est nécessaire au patch C++.

---

## 2. Automation Tests MON15.2

Exécuter :

```text
Grimrock.RPG.MON15.2.ActivePartyDistribution
Grimrock.RPG.MON15.2.ProgressionBoundaries
Grimrock.RPG.MON15.2.MonsterDeathExactlyOnce
Grimrock.RPG.MON15.2.LootIndependence
Grimrock.RPG.MON15.2.PersistenceState
```

Résultats attendus :

- [ ] `ActivePartyDistribution` — Success
- [ ] `ProgressionBoundaries` — Success
- [ ] `MonsterDeathExactlyOnce` — Success
- [ ] `LootIndependence` — Success
- [ ] `PersistenceState` — Success

---

## 3. Contrats de distribution

- [ ] `ExperienceReward <= 0` ne modifie rien.
- [ ] La récompense est partagée uniquement entre `ActiveCharacters` éligibles.
- [ ] `CharacterPool` ne reçoit rien.
- [ ] Le quotient est partagé également.
- [ ] Le reste est distribué selon l'ordre stable de `ActiveCharacters`.
- [ ] Un personnage actif à 0 PV n'est pas exclu par une règle de santé.
- [ ] Un personnage déjà à `190000 XP` est exclu du nombre de bénéficiaires.
- [ ] Une XP persistante invalide n'est pas migrée silencieusement par MON15.2.

---

## 4. Contrats MON15.1 préservés

- [ ] `Experience` reste cumulative.
- [ ] Le plafond de calcul reste `190000 XP`.
- [ ] `Level` n'est jamais modifié par MON15.2.
- [ ] Un seuil peut être franchi et reconstruit via `GetLevelForExperience()`.
- [ ] `DerivedStats` ne sont pas recalculées.
- [ ] L'inventaire n'est pas modifié par l'attribution XP.
- [ ] La hotbar n'est pas modifiée par l'attribution XP.

---

## 5. Exactly-once MonsterDeath

- [ ] Le premier `MarkDead()` distribue `ExperienceReward`.
- [ ] Un second `MarkDead()` ne distribue rien.
- [ ] `bDeathCommitted` reste l'unique garde logique existante.
- [ ] `RestoreCommittedDeathState()` ne distribue rien.
- [ ] Un monstre restauré mort puis ciblé par un nouveau `MarkDead()` ne distribue rien.
- [ ] Aucun registre persistant supplémentaire des récompenses n'est créé.

---

## 6. Indépendance du loot

- [ ] Une table de loot vide n'empêche pas le gain XP.
- [ ] Un drop réussi n'ajoute aucune XP supplémentaire.
- [ ] Un échec de placement du loot n'empêche pas l'XP.
- [ ] Le calcul XP ne lit pas `PlacedLootCount` ou `FailedLootCount`.
- [ ] Le loot ne lit pas l'XP du groupe.

---

## 7. Événements et notifications

- [ ] Un gain réel diffuse `FGridCharacterExperienceAwardedNativeSignature`.
- [ ] L'événement fournit `CharacterIndex`, `AwardedExperience`, `PreviousExperience`, `NewExperience`.
- [ ] Aucun événement XP n'est émis pour une part nulle.
- [ ] `NotifyPartyInventoryChanged(CharacterIndex)` est diffusé après mutation du personnage.

---

## 8. SaveGame

- [ ] `Experience` utilise toujours le champ existant de `FGridCharacterInventoryState`.
- [ ] `CurrentSaveVersion` reste `3`.
- [ ] Aucun nouveau champ SaveGame n'est ajouté.
- [ ] Une valeur XP gagnée est restaurée exactement depuis `PartyInventoryState`.
- [ ] Le `Level` stocké reste inchangé avant MON15.3.

---

## 9. Régressions recommandées

Après les cinq tests MON15.2, lancer au minimum :

```text
Grimrock.RPG.MON15.1
Grimrock.Monsters.MON8.DeathExactlyOnce
Grimrock.Monsters.MON8.MultipleIndependentDrops
Grimrock.Monsters.MON8.VictoryOnLastDeath
Grimrock.Monsters.MON9
Grimrock.CharacterCreation.CC2
```

Si votre filtre habituel est plus pratique, exécuter les suites MON8/MON9 complètes.

- [ ] MON15.1 reste vert.
- [ ] MON8 death/loot reste vert.
- [ ] MON9 persistence reste vert.
- [ ] CC2 reste vert.

---

## 10. Validation PIE avec Rat Géant

Ne modifier aucun asset automatiquement pour cette validation.

1. Ouvrir `DA_MON_RatGiant`.
2. Relever la valeur réellement configurée de `ExperienceReward`.
3. Démarrer une **New Game** ou une sauvegarde de test dont l'état est connu.
4. Relever `Level` et `Experience` des personnages actifs avant le combat.
5. Tuer exactement un Rat Géant.
6. Relever les lignes `[GridExperience]`.
7. Vérifier la répartition selon le nombre de personnages actifs éligibles.
8. Vérifier qu'un second appel de debug sur le Rat mort n'ajoute aucune XP.
9. Sauvegarder après la mort.
10. Quitter le PIE.
11. Relancer avec **Continue**.
12. Vérifier que les valeurs `Experience` sont conservées.
13. Vérifier que le Rat reste mort.
14. Vérifier qu'aucune nouvelle ligne de gain `[GridExperience]` n'est produite par la restauration du mort.

Résultat à fournir :

```text
DA_MON_RatGiant ExperienceReward = ...
XP avant = ...
XP après mort = ...
XP après second MarkDead/debug = ...
XP après Continue = ...
logs [GridExperience] = ...
```

---

## 11. Critère de validation

MON15.2 pourra être marqué **VALIDÉ ET CLOS** après confirmation de :

```text
Compilation UE5.5.4                 OK
5 tests Grimrock.RPG.MON15.2.*       Success
Régressions MON8/MON9 pertinentes    Success
PIE Rat Géant gain unique            OK
Save + Continue sans second gain     OK
```

Ne pas commencer MON15.3 avant cette validation.
