# MON15.5 — Clôture

Statut : **VALIDÉ ET CLOS**  
Date : **16 août 2026**

---

## Objectif du sous-jalon

MON15.5 devait rendre la progression de classe réellement utilisable en jeu après les fondations MON15.1 à MON15.4 : notification de level-up, interface avant/après, sélection staged, transaction atomique, annulation, projection immédiate vers le catalogue de combat et gestion des notifications multiples.

Cet objectif est atteint.

---

## Fonctionnalités validées

- transaction autoritaire de choix de progression par `CharacterId` ;
- validation atomique du niveau, budget, prérequis et doublons ;
- sélection staged sans mutation avant confirmation ;
- annulation non mutante ;
- confirmation de plusieurs choix en un seul commit ;
- comparaison niveau/statistiques avant et après ;
- modal native Slate utilisable sans WBP ;
- intégration au catalogue MON12 via `Requirements` ;
- grants automatiques et choix confirmés projetés dans les requirements ;
- isolation des personnages ;
- file de notifications de level-up ;
- déféré des notifications pendant un combat actif ;
- ouverture au point sûr `OnCombatEnded` ;
- pause réelle du jeu pendant la modal ;
- coalescence de plusieurs montées successives du même personnage ;
- restauration des contrôles après fermeture.

---

## Correctifs issus de la validation PIE

La validation réelle a permis de corriger quatre problèmes qui n'étaient pas visibles dans les tests unitaires initiaux :

1. **Binding Slate UObject**  
   Les boutons `Confirmer` / `Annuler` utilisaient une surcharge `CreateSP` incompatible avec `UUserWidget`. Ils utilisent désormais `FOnClicked::CreateUObject`.

2. **Modal ouverte pendant le combat**  
   Une montée de niveau gagnée à la mort d'un monstre pouvait ouvrir l'interface avant la fin de la résolution du combat. Les notifications sont maintenant différées tant que `bCombatActive` est vrai.

3. **Résolution du TurnManager**  
   Le coordinateur cherchait le `UGridTurnManagerComponent` sur le PartyPawn. Le TurnManager appartient au `AGridLevelRuntimeActor`. La résolution a été corrigée, avec fallback monde.

4. **Plusieurs level-ups différés du même personnage**  
   Les notifications `1->2` et `2->3` pouvaient produire deux modales alors que le personnage était déjà niveau 3. Elles sont maintenant fusionnées en une seule notification `1->3`, garantissant que les choix sont évalués avec le niveau final réel.

---

## Preuve PIE finale

Scénario Giant Rat avec XP temporairement augmenté :

```text
[GridLevelUpUI] Queued Character=0 Previous=1 New=2 ...
[GridLevelUpUI] Deferred ... Reason=CombatActive
...
[GridLevelUpUI] Coalesced Character=0 Previous=1 New=3 Gained=2 Pending=1
...
[GridLevelUpUI] CombatSafePoint Result=EGridCombatPhase::Victory Pending=1
[GridLevelUpUI] ModalGuard Applied Character=0 PausedByModal=true
[GridLevelUpUI] Opened Character=0 Previous=1 New=3
[GridClassProgression] ... Level=3 Committed=1 Granted=1 Spent=1 Remaining=0
[GridLevelUpUI] ModalGuard Restored Character=0
```

Le choix temporaire `Maîtrise martiale` a été acquis correctement.

---

## Preuve Automation finale

Log final après les derniers correctifs runtime :

```text
54 tests terminés
54 Success
0 échec
0 assert
0 ensure
0 CheckAddress
0 Fatal error
```

Détail :

- `Grimrock.RPG.MON15.5.*` : 8 tests uniques, exécutés 3 fois => **24/24 Success** ;
- `Grimrock.Monsters.MON12.ActionCatalog.*` : 2 tests uniques, exécutés 2 fois => **4/4 Success** ;
- `Grimrock.Monsters.MON12.8.*` : 26 tests uniques => **26/26 Success**.

Les campagnes précédentes avaient déjà validé MON15.1–15.4 et CharacterCreation pertinente.

---

## Frontière de persistance

MON15.5 conserve volontairement :

```text
UGrimrockPartySaveGame::CurrentSaveVersion = 3
```

Les choix de progression restent runtime/transitoires. Aucune prétention de persistance finale n'est faite dans ce sous-jalon.

Cette responsabilité passe à **MON15.6**.

---

## Commits MON15.5 principaux

```text
a14a1a79327daccd146e5e32f30966fd92991a8c  Implement MON15.5 level-up interface
66e93428a80d094a18c2121e80889ca3c71d8d6b  Fix MON15.5 Slate UObject button bindings
a77d7612085074b8cbbfc36c36e54f8bfb4e75e3  Fix MON15.5 level-up combat modal safety
8401939caa8e4529106437635694338d1416e5d9  Fix MON15.5 turn manager resolution
a86600571c787d2f2411af487c8d41cebad83c87  Coalesce deferred MON15.5 level-up notifications
```

---

## Décision de clôture

**MON15.5 est VALIDÉ ET CLOS.**

Le prochain sous-jalon est :

**MON15.6 — Save / migration**

Objectif attendu : faire entrer les choix de progression dans le contrat de sauvegarde, définir la migration des sauvegardes existantes, restaurer exactement les choix et la projection runtime, puis tester les anciennes versions et les round-trips.
