# MON10.1 — Journal de combat et fondation du feedback UI

## Objectif

MON10.1 fournit un journal de combat runtime structuré, lisible depuis C++,
Blueprint et un futur widget UMG. Il décrit les événements déjà résolus par le
système de combat : il ne relance aucun jet, ne recalcule aucun dégât et
n'applique aucun dommage.

Le jalon couvre le début du combat, les manches, les phases, les tours des
monstres, les attaques, les mises hors combat, la mort des monstres, la victoire
et la défaite. Aucun asset sous `Content/` n'est nécessaire.

## Architecture

Les fichiers principaux sont :

- `GridCombatLog.h` : types Blueprint et interface du formatteur pur ;
- `GridCombatLog.cpp` : messages localisables et catégorie `LogGridCombat` ;
- `GridTurnManagerComponent` : buffer, émission des entrées et API Blueprint ;
- `GridTurnManagerActions.cpp` : entrées produites par un impact résolu ;
- `GridTurnManagerPhases.cpp` : manches, tours et fin de rencontre.

Le flux reste unidirectionnel :

```text
Résolveur de combat
    → FGridAttackResult
    → TurnManager
    → FGridCombatLogEntry
    → OnCombatLogEntryAdded
    → futur widget UMG
```

L'animation déclenche éventuellement le moment de l'impact, mais elle ne produit
jamais le texte. `FGridAttackResult` demeure la source de vérité des jets, de la
défense, des dégâts d'armure et des points de vie.

## Structures

`FGridCombatLogEntry` est une structure `BlueprintType` transitoire. Elle expose :

- une séquence monotone ;
- la manche et la phase ;
- le type et le message ;
- les identifiants et noms source/cible ;
- l'index du personnage cible ;
- l'identifiant d'attaque ;
- le `FGridAttackResult` original ;
- l'indication de mise hors combat.

`FGridCombatLogFormatter` est une classe C++ pure. Ses fonctions ne lisent aucun
Actor ou `UWorld`, ne modifient aucun état et utilisent `LOCTEXT` et
`FText::Format`.

## Types d'entrées

Les dix types disponibles sont :

1. `CombatStarted` ;
2. `RoundStarted` ;
3. `PhaseChanged` ;
4. `MonsterTurnStarted` ;
5. `AttackHit` ;
6. `AttackMiss` ;
7. `CharacterDefeated` ;
8. `MonsterDefeated` ;
9. `Victory` ;
10. `Defeat`.

## Source de chaque entrée

| Entrée | Source runtime |
|---|---|
| `CombatStarted` | démarrage validé dans `StartCombatInternal()` |
| `RoundStarted` | vrai début de manche, au voisinage de `OnRoundStarted` |
| `PhaseChanged` | `SetPhase()`, uniquement si la phase change |
| `MonsterTurnStarted` | au moment de `OnMonsterTurnStarted` |
| `AttackHit` / `AttackMiss` | résultat déjà appliqué par `ResolveAndApplyPartyAttack()` |
| `CharacterDefeated` | transition de PV strictement positifs vers zéro |
| `MonsterDefeated` | callback MON8 `OnMonsterDied` |
| `Victory` / `Defeat` | fin validée par `FinishCombat()` |

Un démarrage refusé ne vide pas l'historique. `AbortCombat()` conserve les
entrées et peut ajouter le retour vers `Exploration`. Une phase finale utilise
son entrée spécifique `Victory` ou `Defeat`, sans doublon `PhaseChanged`.

## Ordre des événements

Une rencontre normale commence par `CombatStarted`, puis les changements de
phase et `RoundStarted`. Chaque tour ennemi valide produit
`MonsterTurnStarted`.

Une attaque réussie qui met la cible hors combat suit cet ordre :

```text
AttackHit
CharacterDefeated
Defeat (si aucun personnage ne reste vivant)
```

La mort du dernier monstre suit cet ordre :

```text
MonsterDefeated
Victory
```

Le garde existant `bActiveAttackImpactCommitted` reste l'autorité : deux appels
de l'impact n'appliquent les dégâts qu'une fois, ne diffusent
`OnAttackResolved` qu'une fois et ne créent qu'une entrée d'attaque.

## Noms affichés

Le nom d'un monstre provient dans l'ordre de `DisplayName`, `MonsterId`, puis du
nom de l'Actor. Le nom d'un personnage provient de
`ActiveCharacters[Index].DisplayName`, avec `Personnage {Index}` en repli.
L'attaque utilise actuellement son `AttackId` converti en texte ; un libellé
localisé d'attaque reste une extension future.

## API Blueprint

`UGridTurnManagerComponent` expose :

- `GetCombatLogEntries()` pour lire l'historique ;
- `GetLatestCombatLogEntry()` pour lire la dernière entrée ;
- `ClearCombatLog()` pour vider explicitement le journal ;
- `LogCombatHistory()` pour un diagnostic manuel ;
- `OnCombatLogEntryAdded` pour alimenter une interface au fil de l'eau ;
- `MaxCombatLogEntries` pour régler la capacité.

Le TurnManager ne crée aucun widget, ne recherche aucun élément dans le
viewport et n'impose ni couleur, ni police, ni mise en page.

## Ring buffer

La capacité par défaut est de 128 entrées et peut être réglée de 1 à 512. En cas
de dépassement, les entrées les plus anciennes sont supprimées en premier.
Les numéros de séquence ne sont pas réutilisés pendant une rencontre. Le début
d'une nouvelle rencontre validée vide le journal précédent et recommence à 1.

Chaque ajout émet exactement une ligne dans `LogGridCombat` :

```text
[GridCombat] Seq=7 Round=2 Phase=EnemyPhase Type=AttackHit Message="Rat géant attaque Elias..."
```

Aucune entrée n'est produite par Tick.

## Exemples de messages

```text
Le combat commence.
Manche 3.
Phase des ennemis.
Tour de Rat géant.
Rat géant attaque Elias avec Attack_Bite : échec (jet 8 contre défense 12).
Rat géant attaque Elias avec Attack_Bite : 4 dégâts, 8 → 4 PV.
Rat géant attaque Elias avec Attack_Bite : coup critique, 7 dégâts, 8 → 1 PV.
Rat géant attaque Elias avec Attack_Bite : 4 dégâts absorbés par l'armure.
Elias est hors combat.
Rat géant est vaincu.
Victoire.
Défaite.
```

Tous les nombres des messages d'attaque proviennent du
`FGridAttackResult` effectivement appliqué.

## Absence de persistance

Le journal décrit uniquement la rencontre courante. `FGridCombatLogEntry` ne
porte aucune propriété `SaveGame` et `CombatLogEntries` est `Transient`.

Le journal n'est ajouté ni à `FGridRuntimeMonsterState`, ni à
`FGridLevelRuntimeState`, ni à `UGridDungeonRuntimeState`. Une sauvegarde MON9
continue de restaurer séparément monstres, morts et items. Après chargement, le
journal peut être vide sans perte de gameplay.

## Tests

La suite `Grimrock.Monsters.MON10` vérifie :

- le formatteur (manches, phases, échec, réussite, critique, armure et PV) ;
- le ring buffer et ses diffusions ;
- l'unicité d'un impact et de `OnAttackResolved` ;
- l'ordre `AttackHit`, `CharacterDefeated`, `Defeat` ;
- l'ordre `MonsterDefeated`, `Victory` ;
- la conservation de l'historique après un démarrage refusé ;
- l'absence de propriétés de sauvegarde.

La régression complète reste `Grimrock.Monsters.MON`, complétée par les deux
tests CC5 de persistance des personnages.

## Procédure PIE

1. Lancer une nouvelle partie.
2. Terminer la création du personnage.
3. Démarrer un combat.
4. Utiliser `EndPlayerPhase`.
5. Laisser un Rat attaquer.
6. Appeler `LogCombatHistory`.
7. Vérifier `CombatStarted`.
8. Vérifier `RoundStarted`.
9. Vérifier `EnemyPhase`.
10. Vérifier `MonsterTurnStarted`.
11. Vérifier `AttackHit` ou `AttackMiss`.
12. Vérifier le nom du Rat.
13. Vérifier le nom du personnage.
14. En cas d'échec, vérifier le jet et la défense.
15. En cas de réussite, vérifier les dégâts et les PV.
16. Provoquer une défaite.
17. Vérifier `CharacterDefeated`, puis `Defeat`.
18. Recommencer une rencontre.
19. Tuer le dernier Rat.
20. Vérifier `MonsterDefeated`, puis `Victory`.
21. Vérifier l'absence d'entrée dupliquée.
22. Sauvegarder et recharger.
23. Vérifier que MON9 restaure toujours le gameplay.
24. Vérifier que le journal de la rencontre n'a pas été sérialisé.

## Limites du jalon

MON10.1 ne crée pas le widget UMG final. Il n'ajoute ni nombres flottants,
audio, Niagara, flash de dégâts, variations d'Idle, attaques du joueur,
équilibrage, optimisation, graine aléatoire par rencontre ou sauvegarde du
journal. `EncounterRandomSeed` reste inchangé. La localisation complète des
identifiants d'attaques sera traitée dans une extension.

Les suites préparées sont :

- MON10.2 : audio ;
- MON10.3 : VFX ;
- MON10.4 : variations d'Idle ;
- MON10.5 : équilibrage et optimisation.
