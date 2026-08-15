# MON15.3 — Validation Checklist

Statut : **à valider sous Unreal Engine 5.5.4**.

Ne pas commencer MON15.4 avant validation de cette checklist.

---

## 1. Compilation

- [ ] Compiler `GrimrockPrototype` sous UE5.5.4 / Visual Studio.
- [ ] Vérifier `RPGLevelUpService.h/.cpp`.
- [ ] Vérifier l'intégration dans `RPGExperienceRewardService.cpp`.
- [ ] Aucun `.uasset`, `.umap` ou WBP requis.

---

## 2. Tests MON15.3

Exécuter :

```text
Grimrock.RPG.MON15.3.SingleLevelResourcePolicy
Grimrock.RPG.MON15.3.MultiLevelTransaction
Grimrock.RPG.MON15.3.DeadCharacterRemainsDead
Grimrock.RPG.MON15.3.AtomicFailure
Grimrock.RPG.MON15.3.ExperienceAwardIntegration
Grimrock.RPG.MON15.3.PersistenceState
```

Attendus :

- [ ] `SingleLevelResourcePolicy` — Success
- [ ] `MultiLevelTransaction` — Success
- [ ] `DeadCharacterRemainsDead` — Success
- [ ] `AtomicFailure` — Success
- [ ] `ExperienceAwardIntegration` — Success
- [ ] `PersistenceState` — Success

---

## 3. Calcul du niveau

- [ ] Le niveau cible vient uniquement de `GetLevelForExperience()`.
- [ ] `Experience` reste cumulative et inchangée par le level-up.
- [ ] Un seul seuil franchi applique un seul nouveau niveau.
- [ ] Plusieurs seuils franchis sont appliqués en une seule transaction.
- [ ] Aucun level-up n'est rejoué si `TargetLevel == Level`.
- [ ] Aucun downgrade n'est effectué si `TargetLevel < Level`.

---

## 4. Classe et atomicité

- [ ] `ClassDefinition.Get()` est accepté lorsqu'elle est déjà résolue.
- [ ] Une soft reference peut être chargée si nécessaire.
- [ ] `ClassDefinition` doit être valide.
- [ ] `ClassId` doit correspondre.
- [ ] Un échec ne modifie ni `Level` ni `DerivedStats`.
- [ ] L'XP reste acquise en cas d'échec du level-up.

---

## 5. DerivedStats

- [ ] Recalcul via `URPGCharacterRulesLibrary::CalculateDerivedStats()`.
- [ ] `HealthPerLevel` est appliqué.
- [ ] `ManaPerLevel` est appliqué.
- [ ] Armures de base recalculées depuis la classe.
- [ ] Initiative/précision/esquive suivent les règles existantes.
- [ ] Les bonus d'équipement ne sont pas écrits dans `Character.DerivedStats`.

---

## 6. CurrentHealth

- [ ] Aucun full-heal implicite.
- [ ] Le déficit absolu de PV est conservé.
- [ ] Un personnage full HP reste full HP après croissance du maximum.
- [ ] Un personnage blessé garde le même nombre de dégâts.
- [ ] Un personnage à `0 PV` reste à `0 PV`.
- [ ] Le level-up ne ressuscite jamais.

---

## 7. CurrentMana

- [ ] Le déficit absolu de mana consommé est conservé.
- [ ] Un personnage full mana reste full mana.
- [ ] Un personnage ayant dépensé du mana garde le même déficit absolu.
- [ ] Les valeurs restent clampées dans `[0, MaxMana]`.

---

## 8. Événement et notification

- [ ] `OnCharacterLevelUpApplied()` est diffusé une fois par transaction personnage.
- [ ] Il fournit `CharacterIndex`, `PreviousLevel`, `NewLevel`, `LevelsGained`.
- [ ] Un gain multi-niveau n'émet pas plusieurs événements intermédiaires.
- [ ] Lors d'une attribution XP, le level-up est résolu avant `NotifyPartyInventoryChanged`/événement XP.

---

## 9. Non-régression état personnage

- [ ] Inventaire inchangé.
- [ ] Equipment inchangé.
- [ ] Hotbar inchangée.
- [ ] Attributes inchangés.
- [ ] Race/Class identity inchangées.
- [ ] Carry weight inchangé par MON15.3.

---

## 10. SaveGame

- [ ] `CurrentSaveVersion == 3`.
- [ ] Aucun nouveau champ SaveGame.
- [ ] `Level` restauré exactement.
- [ ] `Experience` restaurée exactement.
- [ ] `DerivedStats` restaurées exactement.
- [ ] Aucun deuxième level-up après restauration cohérente.

---

## 11. Régressions recommandées

Exécuter au minimum :

```text
Grimrock.RPG.MON15.1
Grimrock.RPG.MON15.2
Grimrock.CharacterCreation.CC2
Grimrock.CharacterCreation.CC5
Grimrock.Monsters.MON8.VictoryOnLastDeath
Grimrock.Monsters.MON9
```

- [ ] MON15.1 reste vert.
- [ ] MON15.2 reste vert.
- [ ] CC2 reste vert.
- [ ] CC5 reste vert.
- [ ] Combat/victoire reste vert.
- [ ] Persistance monstres reste verte.

---

## 12. PIE réel

Pour éviter de tuer 100 Rats Géants à 10 XP, utiliser une sauvegarde/fixture de test ou une valeur XP proche du seuil, sans modifier durablement l'équilibrage de production.

Scénario recommandé :

1. personnage niveau 1 avec XP proche de `1000` ;
2. relever `Level`, `Experience`, PV et mana avant le combat ;
3. tuer un Rat Géant ;
4. vérifier `[GridExperience]` ;
5. vérifier exactement un `[GridLevelUp]` si le seuil est franchi ;
6. relever niveau/XP/PV/mana après ;
7. sauvegarder ;
8. quitter puis `Continue` ;
9. vérifier les mêmes valeurs ;
10. vérifier qu'aucun second `[GridLevelUp]` n'est produit au chargement.

Logs attendus, exemple :

```text
[GridLevelUp] Character=0 PreviousLevel=1 NewLevel=2 LevelsGained=1 Experience=1000 ...
[GridExperience] ... New=1000 LevelStored=2
```

---

## 13. Critère de clôture

```text
Compilation UE5.5.4                    OK
6 tests Grimrock.RPG.MON15.3.*         Success
Régressions MON15.1/MON15.2            Success
Régressions CC/save/combat pertinentes Success
PIE seuil XP -> level-up unique         OK
Save + Continue sans second level-up    OK
```

Lorsque ces éléments sont confirmés, MON15.3 peut être marqué **VALIDÉ ET CLOS**.
