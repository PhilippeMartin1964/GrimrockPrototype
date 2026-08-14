# MON14.1 — Engagement automatique par perception visuelle

## Problème initial

Depuis MON5, `UGridTurnManagerComponent::StartCombatFromPerception()` sait construire une rencontre à partir de la perception MON4 et de la propagation d'aggro MON7. Le chemin normal restait toutefois déclenché manuellement par la commande de diagnostic F5.

MON14.1 raccorde l'exploration au combat sans changer l'autorité de perception : c'est toujours le monstre qui perçoit logiquement le groupe sur la grille. La visibilité de la caméra joueur n'intervient jamais.

## Flux automatique

```text
événement runtime sûr
    -> RequestEvaluation(Runtime, Reason)
    -> requête différée + coalescée au prochain tick sûr
    -> RefreshPerception() sur les candidats MON5
       - vue : peut devenir source directe
       - ouïe : met à jour Alert / LastKnownPartyCell seulement
    -> sources visuelles directes
    -> propagation MON7 existante
    -> StartCombatFromPerception()
    -> StartCombatInternal()
    -> initiative globale et comportement de poursuite existants
```

Le pont autoritaire est `UGridAutomaticPerceptionEngagementSubsystem`, un `UWorldSubsystem` runtime sans `Tick`. Il ne remplace ni MON4, ni MON5, ni MON7 : il centralise uniquement la décision *quand demander* une évaluation automatique et impose, pendant cette évaluation, qu'une source directe possède `bCanSeeParty=true`.

`StartCombatFromPerception()` reste l'API historique. Hors du scope MON14.1, son contrat reste inchangé : une perception par la vue **ou par l'ouïe** peut toujours servir au démarrage manuel/diagnostic.

## Déclencheurs autorisés

MON14.1 demande une évaluation après :

- la fin logique d'un changement de cellule du groupe ;
- une notification de cellule stable utilisée par l'initialisation/restauration ;
- le `BeginPlay` initialisé d'un comportement de monstre ;
- un rebuild des index runtime de portes ;
- l'ouverture logique d'une porte ;
- une commande runtime réussie sur un `MonsterSpawn` ;
- la fin réussie de l'activation atomique d'une vague MON13.4.

Les producteurs ne connaissent jamais le TurnManager. Ils appellent seulement `GridAutomaticPerceptionEngagement::Request(Runtime, Reason)`.

## Vue et ouïe

`UGridMonsterBehaviorComponent::RefreshPerception()` continue toujours à calculer les deux sens.

- Une vue orthogonale valide et dans `SightRangeCells` peut déclencher automatiquement le combat.
- Un mur ou une porte fermée bloque la vue via `AGridLevelRuntimeActor::CanMove()`.
- L'ouïe continue à utiliser la distance de Manhattan.
- Une perception uniquement auditive peut faire `Dormant/Idle -> Alert` et mémoriser `LastKnownPartyCell`, mais elle n'est pas une source directe d'engagement automatique.
- Le filtre visuel n'existe que dans le scope synchrone de MON14.1 ; le démarrage manuel conserve le comportement historique vue/ouïe.

Le champ de vision directionnel n'est pas ajouté ici. La vue reste orthogonale et omnidirectionnelle dans sa portée, conformément à MON4.

## Participants

La sélection reste celle du TurnManager MON5/MON7 :

1. candidats vivants, activés et runtime-actifs avec identité persistante valide ;
2. sources dont la perception directe satisfait le filtre visuel MON14.1 ;
3. membres ajoutés par la propagation d'aggro MON7 ;
4. déduplication avant l'initiative.

Les Actors absents d'une vague future ne peuvent donc pas être participants. Les monstres morts, désactivés, despawnés ou inactifs dans le niveau courant restent exclus par le pipeline existant.

## Protection contre les démarrages prématurés

Les demandes sont événementielles, différées avec `SetTimerForNextTick()` et coalescées. Plusieurs notifications dans la même action produisent une seule évaluation pending.

Une évaluation automatique n'est pas exécutée si :

- un combat est déjà actif ;
- le TurnManager n'est pas en `Exploration` ;
- une action du TurnManager est en cours ;
- un mouvement de groupe suivi par le TurnManager est encore en cours ;
- `bIsExecutingDungeonTransition` protège une restauration/changement de niveau.

Une requête rencontrant le garde de transition est replanifiée jusqu'au prochain point runtime sûr. Le compteur de réussite et le log `Automatic combat started` ne sont émis qu'après un véritable `StartCombatFromPerception()` réussi.

Aucune logique de perception ou de décision IA n'est ajoutée au `Tick`.

## Interaction avec MON13

`StartEncounter` reste une commande de rencontre et de spawn. Elle n'appelle pas `StartCombat()`.

`UGridMonsterEncounterComponent::ActivateWave()` termine d'abord :

1. le spawn atomique de toute la vague ;
2. l'enregistrement de son état ;
3. les événements `MonsterSpawned` ;
4. l'événement `EncounterWaveStarted`.

Ensuite seulement, il demande une évaluation MON14.1 différée. Une vague cachée ou hors ligne de vue est donc créée normalement sans démarrer le combat. Une vague visible engage au prochain point sûr.

Cette séparation préserve `MonsterPlacements`, `MonsterEncounters`, `CommitDeath`, les vagues atomiques et le chemin Continue.

## Dormance

`Dormant` reste un état d'un monstre **présent**. `RefreshPerception()` sait déjà le faire passer à `Alert` lorsqu'il voit ou entend le groupe. MON14.1 couvre explicitement le réveil par la vue puis l'engagement.

`bInitiallyEnabled=false` conserve une autre signification : le MonsterSpawn est absent. Il ne doit jamais servir à simuler la dormance.

### Limite reportée

Le placement `MonsterSpawn` ne possède pas encore de champ sérialisé permettant de choisir proprement `Dormant` comme état initial. `InitializeMonster()` initialise actuellement l'état à `Idle`. Ajouter un `InitialMonsterState` limité aux états d'exploration est reporté à MON14.2 ; MON14.1 ne détourne pas `bInitiallyEnabled`.

## Tests automatisés

La suite `Grimrock.Monsters.MON14.1` couvre :

- vue directe et portée ;
- hors portée ;
- mur ;
- porte fermée puis passage ouvert ;
- ouïe seule avec `Alert` et dernière cellule connue ;
- réveil `Dormant` ;
- monstres morts/désactivés ;
- placement de vague future sans Actor ;
- coalescence de plusieurs notifications ;
- propagation MON7 et déduplication ;
- `StartEncounter` sans vue ;
- `StartEncounter` avec vue ;
- garde de restauration/Continue ;
- requête après rebuild ;
- maintien du démarrage manuel historique par l'ouïe.

Les scénarios `StartEncounter` chargent les vrais assets de présentation et la vraie classe Blueprint du Rat géant, comme les fixtures MON13. Ils restent des tests runtime automatisés ; une validation PIE manuelle complète demeure distincte.

## Checklist PIE manuelle

1. Compiler `GrimrockPrototypeEditor` en `Win64 Development`.
2. Ouvrir la carte runtime de référence et lancer un playtest frais.
3. Approcher un Rat géant en ligne droite sans appuyer sur F5 : vérifier `Idle/Dormant -> Alert`, puis l'entrée automatique en combat et l'initiative.
4. Refaire derrière un mur : aucun combat tant que la ligne de vue reste bloquée.
5. Refaire derrière une porte fermée puis ouvrir la porte : le combat doit démarrer après l'ouverture, jamais avant.
6. Placer le groupe en diagonale dans la portée auditive seulement : vérifier l'alerte logique sans entrée automatique en combat.
7. Déclencher une rencontre dont la vague apparaît hors vue : vérifier le spawn sans combat.
8. Déclencher une rencontre dont au moins un membre apparaît en vue : vérifier un unique démarrage après la vague complète.
9. Tuer la première vague et vérifier que la vague suivante n'existe pas dans l'initiative avant son apparition.
10. Sauvegarder dans un slot de test distinct de `GrimrockParty`, faire Continue et vérifier qu'aucun combat ne démarre pendant la restauration, puis qu'une perception visuelle stable peut engager ensuite.
11. Vérifier que F5 reste utilisable comme diagnostic et n'est plus nécessaire au gameplay normal.
12. Contrôler les logs : une seule ligne de réussite MON14.1 par engagement et aucun participant dupliqué.

## Limites MON14.2 / MON14.3

MON14.2 doit traiter le champ de vision directionnel, l'état initial `Dormant` sérialisé et les premières données de patrouille sans transformer MON14.1 en système d'IA généraliste.

MON14.3 pourra ensuite traiter l'investigation hors combat, les routes `Loop/PingPong`, les attentes aux points de passage et les transitions de comportement associées.
