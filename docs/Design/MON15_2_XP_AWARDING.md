# MON15.2 — Attribution XP après combat

Statut : **implémenté en C++ — validation UE5.5.4 en attente**.

MON15.2 raccorde `UGridMonsterDefinitionAsset::ExperienceReward` au champ persistant existant `FGridCharacterInventoryState::Experience`. Le jalon n'effectue pas encore la montée de niveau : `Level`, les PV, le mana et les statistiques dérivées restent inchangés jusqu'à MON15.3.

---

## 1. Audit du pipeline de mort

MON8 fournit déjà l'autorité logique nécessaire :

```text
AGridMonsterActor::MarkDead()
    -> UGridMonsterDeathComponent::CommitDeath()
```

`CommitDeath()` place `bDeathCommitted = true` avant les hooks externes. Une seconde mort du même Actor est donc rejetée.

La restauration d'un monstre déjà mort utilise :

```text
RestoreCommittedDeathState()
```

Elle restaure `bDeathCommitted=true` sans rappeler `CommitDeath()`. Le chargement, un rebuild ou un second `MarkDead()` ne doivent donc pas rejouer la récompense XP.

MON15.2 réutilise directement ce contrat au lieu d'ajouter un second registre de monstres récompensés.

---

## 2. Autorité de la récompense

La valeur data-driven reste :

```cpp
UGridMonsterDefinitionAsset::ExperienceReward
```

Aucune valeur XP n'est dupliquée dans le TurnManager, le HUD, le loot ou un Widget Blueprint.

Une récompense négative est déjà interdite par le contrat du DataAsset ; le runtime traite néanmoins toute valeur `<= 0` comme un no-op sûr.

---

## 3. Transaction de groupe

`FRPGExperienceRewardService` est un helper runtime sans état persistant. Il travaille directement sur :

```text
UGridPartyInventoryComponent
    -> PartyInventoryState
        -> ActiveCharacters[]
            -> Experience
```

Il ne crée aucun second modèle de personnage et ne possède aucun tableau d'XP.

La courbe et le plafond restent fournis par `URPGCharacterRulesLibrary` MON15.1.

---

## 4. Politique de partage

`ExperienceReward` représente un **pool de groupe**.

Les bénéficiaires sont les personnages actuellement présents dans :

```text
FGridPartyInventoryState::ActiveCharacters
```

Les personnages de `CharacterPool` ne reçoivent rien.

Un personnage actif à 0 PV reste un membre du groupe et conserve donc son droit à la récompense. MON15.2 ne crée pas une politique parallèle fondée sur les PV courants.

Les personnages déjà au plafond MON15.1, soit `190000 XP`, sont exclus du nombre de bénéficiaires afin qu'ils ne consomment pas une part qui peut revenir à un personnage encore progressable.

Un état d'XP brut invalide (`< 0` ou `> 190000`) n'est pas migré silencieusement par MON15.2 : le personnage concerné est ignoré et un warning est journalisé. Les migrations restent un sujet de MON15.6.

---

## 5. Répartition déterministe

Pour `R` points d'XP et `N` bénéficiaires :

```text
BaseShare = R / N
Remainder = R % N
```

Chaque bénéficiaire reçoit `BaseShare`. Les `Remainder` premiers bénéficiaires dans l'ordre stable de `ActiveCharacters` reçoivent un point supplémentaire.

Exemple :

```text
Reward = 10
Active eligible characters = 4

Character 0 -> 3 XP
Character 1 -> 3 XP
Character 2 -> 2 XP
Character 3 -> 2 XP
```

Cette règle ne dépend ni de l'initiative, ni du dernier attaquant, ni de la position du personnage dans le combat.

Si un bénéficiaire est très proche du plafond et que sa part le dépasse, sa valeur est clampée à `190000`. La portion qui dépasse le plafond n'est pas redistribuée dans MON15.2 ; le retour de la transaction expose le total réellement appliqué et les logs indiquent la part non appliquée.

---

## 6. Niveau en attente

MON15.2 modifie uniquement `Experience`.

Exemple :

```text
avant : Level=1 Experience=999
reward : +2 XP
après : Level=1 Experience=1001
```

`URPGCharacterRulesLibrary::GetLevelForExperience(1001)` reconstruit bien le niveau attendu `2`, mais le champ persistant `Level` reste `1` jusqu'au traitement MON15.3.

Cette incohérence temporaire est volontaire : MON15.2 ne doit pas déclencher un level-up partiel ni recalculer les statistiques avant que la politique complète de montée de niveau soit définie.

---

## 7. Événement XP

Après chaque modification réelle d'un personnage actif, le service diffuse :

```cpp
FGridCharacterExperienceAwardedNativeSignature
```

avec :

```text
CharacterIndex
AwardedExperience
PreviousExperience
NewExperience
```

Le composant d'inventaire diffuse également son événement de changement existant via `NotifyPartyInventoryChanged(CharacterIndex)` afin que les projections déjà branchées sur l'état du groupe puissent se rafraîchir.

L'événement XP est un événement runtime natif ; MON15.2 ne crée aucun widget de présentation.

---

## 8. Point d'intégration MonsterDeath

Après la garde irréversible de mort et le traitement du loot, `CommitDeath()` lit `ExperienceReward`, retrouve le `PartyInventoryComponent` du `AGrimrockPartyPawn` courant puis appelle la transaction de groupe.

Le résultat du loot ne conditionne pas l'XP :

```text
mort validée
    -> tentative loot
    -> attribution XP
    -> liens MonsterDied
    -> OnMonsterDied
    -> rencontre / victoire
```

Une erreur de placement de loot n'annule pas la récompense XP. Réciproquement, une absence de récompense XP ne bloque pas la mort, les liens ou la victoire.

Le TurnManager ne distribue aucune XP.

---

## 9. Exactly-once

Le contrat est obtenu sans nouveau champ SaveGame :

```text
premier CommitDeath()
    bDeathCommitted false -> true
    XP distribuée

second MarkDead()
    CommitDeath() retourne false
    aucune XP

Continue / rebuild d'un mort
    RestoreCommittedDeathState()
    bDeathCommitted = true
    aucune XP
```

La même garde protège déjà le loot et `OnMonsterDied` depuis MON8/MON9.

---

## 10. SaveGame

`Experience` est déjà membre de `FGridCharacterInventoryState`, lui-même contenu dans le `PartyInventoryState` sauvegardé.

MON15.2 :

- n'ajoute aucun champ persistant ;
- ne modifie pas `UGrimrockPartySaveGame` ;
- ne change pas `CurrentSaveVersion` ;
- conserve `CurrentSaveVersion = 3` ;
- ne sérialise aucun registre de morts récompensés supplémentaire.

La persistance du monstre mort MON9 et la persistance de l'XP du groupe suffisent ensemble à empêcher un nouveau gain après Continue.

---

## 11. Hors périmètre

MON15.2 ne :

- change pas `Level` ;
- recalcule pas les PV ou le mana ;
- modifie pas `CalculateDerivedStats()` ;
- n'affiche pas de Level Up ;
- n'accorde aucun point de compétence, don ou talent ;
- ne modifie pas le loot ;
- ne modifie pas le TurnManager ;
- ne modifie aucun WBP ;
- ne modifie aucun `.uasset` ou `.umap` ;
- ne change pas automatiquement `DA_MON_RatGiant`.

MON15.3 prendra en charge le franchissement des seuils et le recalcul des statistiques.

---

## 12. Automation Tests

Suite dédiée :

```text
Grimrock.RPG.MON15.2.ActivePartyDistribution
Grimrock.RPG.MON15.2.ProgressionBoundaries
Grimrock.RPG.MON15.2.MonsterDeathExactlyOnce
Grimrock.RPG.MON15.2.LootIndependence
Grimrock.RPG.MON15.2.PersistenceState
```

Ils couvrent :

- partage égal et reste déterministe ;
- exclusion du `CharacterPool` ;
- exclusion d'un personnage déjà au plafond ;
- franchissement d'un seuil XP sans modification de `Level` ;
- absence de recalcul des statistiques ;
- événement par gain réel ;
- premier `MarkDead()` puis répétition ;
- restauration d'un mort déjà committé ;
- XP indépendante d'un échec de placement de loot ;
- persistance du champ `Experience` via le SaveGame existant ;
- absence de bump de version SaveGame.

---

## 13. Validation manuelle recommandée

Après validation des Automation Tests, un scénario PIE minimal doit confirmer l'asset de production :

1. ouvrir `DA_MON_RatGiant` ;
2. relever sa valeur réelle `ExperienceReward` sans la modifier pour les besoins du test ;
3. démarrer une partie fraîche ;
4. relever l'XP des personnages actifs avant le combat ;
5. tuer un Rat Géant ;
6. vérifier les logs `[GridExperience]` ;
7. vérifier que la somme des gains appliqués correspond à la récompense attendue, sauf clamp de plafond ;
8. sauvegarder ;
9. faire Continue ;
10. vérifier que le Rat reste mort et que l'XP est conservée ;
11. vérifier qu'aucun second gain n'est produit au chargement.

Aucun résultat PIE n'est revendiqué dans ce document avant retour d'un log réel.

---

## 14. Porte de sortie

MON15.2 pourra être marqué `Validé` lorsque :

- les cinq tests dédiés réussissent sous UE5.5.4 ;
- les régressions MON8/MON9 pertinentes restent vertes ;
- le scénario PIE avec un vrai Rat Géant confirme la récompense une seule fois et sa conservation après Continue ;
- aucun comportement MON15.3 n'a été introduit prématurément.
