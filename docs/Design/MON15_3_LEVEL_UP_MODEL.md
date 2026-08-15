# MON15.3 — Montée de niveau et recalcul des statistiques

Statut : **implémenté en C++ — validation UE5.5.4 en attente**.

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

Le personnage porte toujours 5 points de dégâts. Il ne devient pas artificiellement blessé de 10 points parce que son maximum a augmenté.

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

Après une transaction réussie, sauvegarder/restaurer doit reproduire exactement `Level`, `Experience` et les stats recalculées. Une restauration cohérente ne doit pas produire un second level-up.

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

## 13. Automation Tests

Suite dédiée :

```text
Grimrock.RPG.MON15.3.SingleLevelResourcePolicy
Grimrock.RPG.MON15.3.MultiLevelTransaction
Grimrock.RPG.MON15.3.DeadCharacterRemainsDead
Grimrock.RPG.MON15.3.AtomicFailure
Grimrock.RPG.MON15.3.ExperienceAwardIntegration
Grimrock.RPG.MON15.3.PersistenceState
```

Ils couvrent le recalcul PV/mana, les déficits courants, les morts, plusieurs niveaux, les échecs atomiques, l'intégration au gain XP et la persistance.

---

## 14. Porte de sortie

MON15.3 pourra être marqué **VALIDÉ ET CLOS** lorsque :

- les six tests dédiés réussissent sous UE5.5.4 ;
- MON15.1 et MON15.2 restent verts ;
- CC2/CC5 et les tests de combat pertinents restent verts ;
- un scénario PIE réel franchit un seuil XP et montre un unique log `[GridLevelUp]` cohérent ;
- les PV/mana courants respectent la politique décrite ;
- sauvegarder/Continue conserve le niveau appliqué sans seconde montée de niveau.
