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

- Une vue valide et dans `SightRangeCells` peut déclencher automatiquement le combat.
- Un mur ou une porte fermée bloque la vue via `AGridLevelRuntimeActor::CanMove()`.
- L'ouïe continue à utiliser la distance de Manhattan.
- Une perception uniquement auditive peut faire `Dormant/Idle -> Alert` et mémoriser `LastKnownPartyCell`, mais elle n'est pas une source directe d'engagement automatique.
- Le filtre visuel n'existe que dans le scope synchrone de MON14.1 ; le démarrage manuel conserve le comportement historique vue/ouïe.

MON14.2 complète désormais ce contrat : la géométrie orthogonale MON4 est filtrée par le `Facing` cardinal courant du monstre. La source automatique doit donc se trouver à la fois sur une ligne de vue ouverte **et devant le monstre**. Le helper géométrique MON4 reste disponible indépendamment du Facing pour les tests et diagnostics purs.

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

MON14.2 ajoute désormais `InitialMonsterState` au `MonsterSpawn`, limité à `Idle` ou `Dormant`. Un playtest frais applique cet état au nouvel Actor ; une restauration MON9/MON13 reste ensuite autoritaire et remplace l'état initial par l'état sauvegardé.

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

Depuis MON14.2, les fixtures de vue MON14.1 orientent explicitement leurs monstres vers le groupe afin que ces tests continuent à vérifier le raccord automatique, tandis que la directionnalité elle-même est couverte par `Grimrock.Monsters.MON14.2`.

Les scénarios `StartEncounter` chargent les vrais assets de présentation et la vraie classe Blueprint du Rat géant, comme les fixtures MON13. Ils restent des tests runtime automatisés ; une validation PIE manuelle complète demeure distincte.

## Checklist PIE manuelle

1. Compiler `GrimrockPrototypeEditor` en `Win64 Development`.
2. Ouvrir la carte runtime de référence et lancer un playtest frais.
3. Approcher un Rat géant dans son axe avant sans appuyer sur F5 : vérifier `Idle/Dormant -> Alert`, puis l'entrée automatique en combat et l'initiative.
4. Refaire dans son axe arrière ou latéral : aucun engagement visuel tant qu'il ne fait pas face au groupe.
5. Refaire derrière un mur : aucun combat tant que la ligne de vue reste bloquée.
6. Refaire derrière une porte fermée puis ouvrir la porte : le combat doit démarrer après l'ouverture si le groupe se trouve dans l'axe avant, jamais avant.
7. Placer le groupe en diagonale dans la portée auditive seulement : vérifier l'alerte logique sans entrée automatique en combat.
8. Déclencher une rencontre dont la vague apparaît hors vue : vérifier le spawn sans combat.
9. Déclencher une rencontre dont au moins un membre apparaît en vue et regarde le groupe : vérifier un unique démarrage après la vague complète.
10. Tuer la première vague et vérifier que la vague suivante n'existe pas dans l'initiative avant son apparition.
11. Sauvegarder dans un slot de test distinct de `GrimrockParty`, faire Continue et vérifier qu'aucun combat ne démarre pendant la restauration, puis qu'une perception visuelle stable peut engager ensuite.
12. Vérifier que F5 reste utilisable comme diagnostic et n'est plus nécessaire au gameplay normal.
13. Contrôler les logs : une seule ligne de réussite MON14.1 par engagement et aucun participant dupliqué.

## Suite MON14.3

MON14.2 fournit désormais le champ de vision directionnel, l'état initial `Idle/Dormant` et les données `None/Loop/PingPong` avec waypoints.

MON14.3 peut donc se concentrer sur l'exécution hors combat : déplacements entre waypoints, orientations d'arrivée, attentes, abandon immédiat de la patrouille lors d'une perception et transitions vers une éventuelle investigation.
