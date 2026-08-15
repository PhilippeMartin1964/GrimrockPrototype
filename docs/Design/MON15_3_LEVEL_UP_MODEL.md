# MON15.3 — Montée de niveau et recalcul des statistiques

Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**.

Date de validation : **15 août 2026**.

MON15.3 transforme l'XP cumulative MON15.1/MON15.2 en montée de niveau effective. Il ne crée aucun nouveau champ persistant et ne traite encore ni talents, ni compétences, ni sorts, ni UI de level-up.

---

## 1. Autorité de progression

Le niveau cible est toujours reconstruit depuis :

```text
URPGCharacterRulesLibrary::GetLevelForExperience(Experience)
```

`FGridCharacterInventoryState::Level` reste le niveau appliqué et sérialisé. MON15.3 ne consomme jamais l'XP et ne remet jamais `Experience` à zéro.

Une transaction n'est appliquée que lorsque :

```text
TargetLevel > StoredLevel
```

Si `TargetLevel == StoredLevel`, il n'y a rien à faire. Si `TargetLevel < StoredLevel`, MON15.3 refuse de rétrograder le personnage : les incohérences anciennes restent du ressort de MON15.6.

---

## 2. Service runtime

`FRPGLevelUpService` est un service sans état persistant.

API principale :

```cpp
ApplyPendingLevelUp(PartyInventoryComponent, CharacterIndex)
ApplyPendingLevelUps(PartyInventoryComponent)
OnCharacterLevelUpApplied()
```

La transaction d'un personnage peut gagner plusieurs niveaux d'un coup. Elle calcule directement les statistiques du niveau cible au lieu d'enchaîner des mutations intermédiaires.

---

## 3. Résolution de la classe

La classe persistante existante reste :

```text
FGridCharacterInventoryState::ClassDefinition
```

qui est un `TSoftObjectPtr<URPGClassAsset>`.

La transaction tente d'abord `Get()`, puis `LoadSynchronous()` si nécessaire. Elle exige :

- une définition présente ;
- `IsValidDefinition() == true` ;
- un `ClassId` cohérent avec le personnage lorsqu'il est renseigné.

En cas d'échec, `Level` et `DerivedStats` ne sont pas modifiés. L'XP reste acquise : le personnage conserve un level-up en attente pouvant être résolu après correction de la définition ou migration.

---

## 4. Recalcul autoritaire

Les statistiques de base sont recalculées via le système existant :

```cpp
URPGCharacterRulesLibrary::CalculateDerivedStats(
    Character.Attributes,
    ClassDefinition,
    TargetLevel)
```

Cela applique notamment :

```text
HealthAtLevelOne
HealthPerLevel
ManaAtLevelOne
ManaPerLevel
BasePhysicalArmor
BaseMagicalArmor
```

ainsi que les modificateurs d'attributs déjà définis dans les règles RPG.

MON15.3 ne duplique aucune formule de progression de PV/mana.

---

## 5. Équipement

`Character.DerivedStats` reste l'état de base persistant.

Les bonus d'équipement continuent d'être projetés séparément par `GetCharacterSummary()`. Le level-up ne copie donc jamais dans `DerivedStats` :

- `MaxHealthBonus` ;
- `MaxManaBonus` ;
- `ArmorBonus` ;
- bonus d'attributs ;
- résistances.

L'inventaire, l'équipement et la hotbar ne sont pas modifiés par la transaction.

---

## 6. Politique CurrentHealth

MON15.3 ne fait pas de full-heal implicite.

Il conserve le **déficit absolu de PV** :

```text
DamageTaken = OldMaxHealth - OldCurrentHealth
NewCurrentHealth = NewMaxHealth - DamageTaken
```

avec clamp dans `[0, NewMaxHealth]`.

Exemple :

```text
avant : 15 / 20 PV
nouveau maximum : 25 PV
après : 20 / 25 PV
```

Le personnage porte toujours 5 points de dégâts. Il ne devient pas artificiellement blessé parce que son maximum a augmenté.

Cas spécial :

```text
OldCurrentHealth == 0
```

reste exactement `0`. Une montée de niveau ne ressuscite jamais un personnage mort.

---

## 7. Politique CurrentMana

Le mana conserve également le déficit absolu consommé :

```text
ManaSpent = OldMaxMana - OldCurrentMana
NewCurrentMana = NewMaxMana - ManaSpent
```

avec clamp dans `[0, NewMaxMana]`.

Un personnage à mana plein reste à mana plein lorsque son maximum augmente. Un personnage ayant dépensé 6 points de mana reste déficitaire de 6 points après la montée de niveau.

---

## 8. Plusieurs niveaux en une transaction

Exemple avec la courbe MON15.1 :

```text
Level stocké = 1
Experience = 6000
TargetLevel = 4
```

La transaction applique directement :

```text
1 -> 4
LevelsGained = 3
```

et calcule une seule fois les statistiques du niveau 4.

Aucun événement intermédiaire artificiel `1->2`, `2->3`, `3->4` n'est diffusé.

---

## 9. Événement Level Up

Après commit réussi :

```text
FGridCharacterLevelUpAppliedNativeSignature
```

fournit :

```text
CharacterIndex
PreviousLevel
NewLevel
LevelsGained
```

Les futurs systèmes MON15.4/MON15.5 pourront lire l'état final depuis `UGridPartyInventoryComponent`.

---

## 10. Intégration à l'attribution XP

`FRPGExperienceRewardService::AwardToActiveParty()` conserve sa transaction de partage MON15.2, puis :

1. écrit toutes les nouvelles valeurs `Experience` ;
2. tente les level-ups des personnages réellement récompensés ;
3. diffuse ensuite `NotifyPartyInventoryChanged()` et l'événement XP.

Un observateur du gain XP voit donc déjà le niveau et les statistiques finales lorsqu'une définition de classe valide permet la montée de niveau.

Si la définition de classe est absente/invalide, l'XP est conservée mais le niveau reste en attente.

---

## 11. SaveGame

MON15.3 n'ajoute aucun champ :

- `Level` existait déjà ;
- `Experience` existait déjà ;
- `DerivedStats` existaient déjà ;
- `ClassDefinition` existait déjà.

`UGrimrockPartySaveGame::CurrentSaveVersion` reste `3`.

Après une transaction réussie, sauvegarder/restaurer reproduit exactement `Level`, `Experience` et les stats recalculées. Une restauration cohérente ne produit pas un second level-up.

---

## 12. Hors périmètre

MON15.3 ne :

- n'accorde aucun point de compétence ;
- n'accorde aucun talent/don ;
- ne débloque aucun sort ;
- ne crée aucune spécialisation ;
- ne modifie aucun WBP ;
- ne crée aucune modal Level Up ;
- ne modifie aucun `.uasset` ou `.umap` ;
- ne migre pas les anciennes incohérences `Level <-> Experience` ;
- ne modifie pas la courbe XP ;
- ne modifie pas les récompenses des monstres.

Ces sujets restent réservés à MON15.4–MON15.6.

---

## 13. Automation Tests — validation finale

Suite dédiée MON15.3 exécutée sous UE5.5.4 le 15 août 2026 :

```text
Grimrock.RPG.MON15.3.SingleLevelResourcePolicy   Success
Grimrock.RPG.MON15.3.MultiLevelTransaction      Success
Grimrock.RPG.MON15.3.DeadCharacterRemainsDead  Success
Grimrock.RPG.MON15.3.AtomicFailure              Success
Grimrock.RPG.MON15.3.ExperienceAwardIntegration Success
Grimrock.RPG.MON15.3.PersistenceState           Success
```

Résultats observés significatifs :

```text
Single level :  Level 1 -> 2, XP=1000, HP=21/27, Mana=7/13
Dead character: Level 1 -> 2, HP=0/27
XP integration : 999 + 1 -> 1000, LevelStored=2
Multi-level :    Level 1 -> 4, LevelsGained=3, XP=6000
Persistence :    Level 4 / XP=6000 restaurés par le test dédié
```

Les suites MON15.1 et MON15.2 ainsi que les régressions monstres MON8/MON9 exécutées avec la même campagne restent `Success`.

Les warnings `InvalidClassDefinition` visibles dans certains fixtures MON15.2 sont attendus : ces fixtures historiques n'ont pas de `ClassDefinition`. L'XP y reste acquise et aucun level-up invalide n'est appliqué.

---

## 14. Régressions Character Creation

La campagne complémentaire du 15 août 2026 confirme :

```text
Grimrock.CharacterCreation.CC2.CreateInitialCharacter           Success
Grimrock.CharacterCreation.CC2.RejectInvalidRequestAtomically   Success
Grimrock.CharacterCreation.CC2.RejectSecondCreation              Success
Grimrock.CharacterCreation.CC5.RejectInvalidSnapshotAtomically  Success
Grimrock.CharacterCreation.CC5.SaveMemoryRoundTrip               Success
```

Les autres tests Character Creation présents dans cette campagne (CC0, CC1, CC4 et CC6) sont également restés `Success`.

---

## 15. PIE réel

Le scénario PIE MON15.3 a été **validé manuellement par l'utilisateur le 15 août 2026**, selon la procédure de la checklist : franchissement d'un seuil XP, level-up unique, cohérence PV/mana, sauvegarde puis Continue sans répétition de la montée de niveau.

Aucun log PIE détaillé n'est archivé dans ce document ; cette partie de la validation repose sur la confirmation manuelle explicite de l'utilisateur.

---

## 16. Clôture

MON15.3 satisfait sa porte de sortie :

- 6/6 tests dédiés MON15.3 `Success` ;
- MON15.1 et MON15.2 sans régression ;
- CC2 3/3 `Success` ;
- CC5 2/2 `Success` ;
- combat/victoire et persistance monstres sans régression dans la campagne fournie ;
- PIE réel validé manuellement ;
- aucun nouveau champ SaveGame ;
- aucun `.uasset`, `.umap` ou WBP requis.

L'exécution réussie des Automation Tests sous UE5.5.4 confirme que le module C++ testé est chargé et exécutable. Aucun transcript UBT/Visual Studio séparé n'a été archivé pour cette clôture.

**MON15.3 — VALIDÉ ET CLOS.**
