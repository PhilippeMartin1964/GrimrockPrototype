# MON15.3 — Validation Checklist

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**.

Date : **15 août 2026**.

---

## 1. Compilation / chargement UE

- [x] `RPGLevelUpService.h/.cpp` chargé et exécuté par les Automation Tests UE5.5.4.
- [x] Intégration `RPGExperienceRewardService.cpp` chargée et exécutée.
- [x] Aucun `.uasset`, `.umap` ou WBP requis.
- [x] Aucun échec UHT/C++ observé dans la campagne UE fournie.

Note : aucun transcript UBT/Visual Studio séparé n'a été archivé ; la validation repose ici sur l'exécution réussie du module et des tests dans UE5.5.4.

---

## 2. Tests MON15.3

Résultats :

```text
Grimrock.RPG.MON15.3.SingleLevelResourcePolicy    Success
Grimrock.RPG.MON15.3.MultiLevelTransaction       Success
Grimrock.RPG.MON15.3.DeadCharacterRemainsDead   Success
Grimrock.RPG.MON15.3.AtomicFailure               Success
Grimrock.RPG.MON15.3.ExperienceAwardIntegration Success
Grimrock.RPG.MON15.3.PersistenceState            Success
```

- [x] `SingleLevelResourcePolicy` — Success
- [x] `MultiLevelTransaction` — Success
- [x] `DeadCharacterRemainsDead` — Success
- [x] `AtomicFailure` — Success
- [x] `ExperienceAwardIntegration` — Success
- [x] `PersistenceState` — Success

---

## 3. Calcul du niveau

- [x] Le niveau cible vient uniquement de `GetLevelForExperience()`.
- [x] `Experience` reste cumulative et inchangée par le level-up.
- [x] Un seul seuil franchi applique un seul nouveau niveau.
- [x] Plusieurs seuils franchis sont appliqués en une seule transaction.
- [x] Aucun level-up n'est rejoué si `TargetLevel == Level`.
- [x] Aucun downgrade n'est effectué si `TargetLevel < Level`.

Preuves principales :

```text
ExperienceAwardIntegration : 999 + 1 -> 1000, LevelStored=2
MultiLevelTransaction      : PreviousLevel=1 NewLevel=4 LevelsGained=3 Experience=6000
AtomicFailure              : Level=3 Target=2 -> Rejected Reason=WouldDemote
```

---

## 4. Classe et atomicité

- [x] `ClassDefinition.Get()` accepté lorsqu'elle est déjà résolue.
- [x] Une soft reference peut être chargée si nécessaire par le service.
- [x] `ClassDefinition` doit être valide.
- [x] `ClassId` doit correspondre.
- [x] Un échec ne modifie ni `Level` ni `DerivedStats`.
- [x] L'XP reste acquise en cas d'échec du level-up.

Le test `AtomicFailure` valide les rejets de classe incohérente et de downgrade.

Les warnings `InvalidClassDefinition` dans MON15.2 sont attendus pour les anciennes fixtures sans classe et ne provoquent aucune mutation de niveau invalide.

---

## 5. DerivedStats

- [x] Recalcul via `URPGCharacterRulesLibrary::CalculateDerivedStats()`.
- [x] `HealthPerLevel` appliqué.
- [x] `ManaPerLevel` appliqué.
- [x] Armures de base recalculées depuis la classe.
- [x] Initiative/précision/esquive suivent les règles existantes.
- [x] Les bonus d'équipement ne sont pas écrits dans `Character.DerivedStats`.

---

## 6. CurrentHealth

- [x] Aucun full-heal implicite pour un personnage blessé.
- [x] Le déficit absolu de PV est conservé.
- [x] Un personnage full HP reste full HP après croissance du maximum.
- [x] Un personnage blessé garde le même nombre de dégâts.
- [x] Un personnage à `0 PV` reste à `0 PV`.
- [x] Le level-up ne ressuscite jamais.

Résultats observés :

```text
SingleLevelResourcePolicy : HP=21/27 après level-up, déficit conservé
DeadCharacterRemainsDead  : HP=0/27 après level-up
```

---

## 7. CurrentMana

- [x] Le déficit absolu de mana consommé est conservé.
- [x] Un personnage full mana reste full mana.
- [x] Un personnage ayant dépensé du mana garde le même déficit absolu.
- [x] Les valeurs restent clampées dans `[0, MaxMana]`.

Résultat observé :

```text
SingleLevelResourcePolicy : Mana=7/13 après level-up
```

---

## 8. Événement et notification

- [x] `OnCharacterLevelUpApplied()` est diffusé une fois par transaction personnage.
- [x] Il fournit `CharacterIndex`, `PreviousLevel`, `NewLevel`, `LevelsGained`.
- [x] Un gain multi-niveau n'émet pas plusieurs transactions intermédiaires.
- [x] Lors d'une attribution XP, le level-up est résolu avant la notification XP/inventaire.

Le log d'intégration confirme :

```text
[GridLevelUp] ... PreviousLevel=1 NewLevel=2 ... Experience=1000
[GridExperience] ... Previous=999 New=1000 LevelStored=2
```

---

## 9. Non-régression état personnage

- [x] Inventaire inchangé.
- [x] Equipment inchangé.
- [x] Hotbar inchangée.
- [x] Attributes inchangés.
- [x] Race/Class identity inchangées.
- [x] Carry weight inchangé par MON15.3.

---

## 10. SaveGame

- [x] `CurrentSaveVersion == 3`.
- [x] Aucun nouveau champ SaveGame.
- [x] `Level` restauré exactement par le test dédié.
- [x] `Experience` restaurée exactement par le test dédié.
- [x] `DerivedStats` restaurées exactement par le test dédié.
- [x] Aucun deuxième level-up après restauration cohérente dans le scénario validé.

`Grimrock.RPG.MON15.3.PersistenceState` est `Success` avec un état niveau 4 / XP 6000.

---

## 11. Régressions

### MON15

- [x] MON15.1 reste vert.
- [x] MON15.2 reste vert.

### Character Creation

```text
Grimrock.CharacterCreation.CC2.CreateInitialCharacter           Success
Grimrock.CharacterCreation.CC2.RejectInvalidRequestAtomically   Success
Grimrock.CharacterCreation.CC2.RejectSecondCreation              Success
Grimrock.CharacterCreation.CC5.RejectInvalidSnapshotAtomically  Success
Grimrock.CharacterCreation.CC5.SaveMemoryRoundTrip               Success
```

- [x] CC2 — 3/3 Success.
- [x] CC5 — 2/2 Success.

La campagne complémentaire contient également des tests CC0, CC1, CC4 et CC6, tous `Success`.

### Combat / monstres / persistance

- [x] `Grimrock.Monsters.MON8.VictoryOnLastDeath` — Success.
- [x] autres tests MON8 observés — Success.
- [x] suite MON9 observée — Success.

---

## 12. PIE réel

Scénario demandé : franchissement réel d'un seuil XP, vérification du level-up unique, PV/mana, sauvegarde, sortie PIE, Continue, absence de second level-up.

- [x] PIE MON15.3 validé manuellement par l'utilisateur le 15 août 2026.
- [x] Franchissement de seuil XP validé.
- [x] Level-up unique validé.
- [x] Cohérence PV/mana validée.
- [x] Save + Continue validé.
- [x] Pas de second level-up au chargement validé.

Aucun log PIE détaillé n'a été fourni avec cette confirmation ; ces cases reposent sur la validation manuelle explicite de l'utilisateur selon le scénario précédemment demandé.

---

## 13. Critère de clôture

```text
Chargement/exécution UE5.5.4             OK
6 tests Grimrock.RPG.MON15.3.*           Success
Régressions MON15.1/MON15.2              Success
CC2                                       3/3 Success
CC5                                       2/2 Success
Combat/victoire et MON9                   Success
PIE seuil XP -> level-up unique           OK (validation manuelle)
Save + Continue sans second level-up      OK (validation manuelle)
```

**MON15.3 — VALIDÉ ET CLOS.**
