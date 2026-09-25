# MON13.6 — Encounter Start Reliability

## Objectif

Rendre le contrat `Trigger.Activated -> StartEncounter` déterministe :

```text
Trigger.Activated
    -> StartEncounter
    -> spawn atomique de la vague
    -> EncounterWaveStarted
    -> demande d'engagement différée
    -> prochain point runtime sûr
    -> checkpoint pré-combat
    -> combat avec les groupes explicitement déclenchés
```

Un `StartEncounter` n'est plus conditionné par le Facing ou la LOS du monstre. Ces règles restent exclusivement celles de l'engagement automatique d'exploration MON14.1.

## Autorités

- `UGridMonsterEncounterComponent` reste l'autorité de spawn, vagues et persistance.
- `UGridAutomaticPerceptionEngagementSubsystem` reste le pont différé vers le combat.
- `UGridTurnManagerComponent` reste l'autorité d'admission des participants et de démarrage effectif.
- `FGridCombatSavePolicy` reste obligatoire avant le démarrage d'un combat de rencontre.
- aucun Tick, aucun nouveau type de placement et aucun appel synchrone direct Encounter -> TurnManager n'est ajouté.

## Fiabilité

Une demande de rencontre conserve son intention si elle tombe pendant une action ou un mouvement momentanément non sûr. Elle est rejouée au tick sûr suivant.

Répéter `StartEncounter` pendant une vague active :

- ne respawn aucun Actor ;
- ne réémet pas `EncounterWaveStarted` ;
- réémet uniquement la demande d'engagement, ce qui permet de récupérer d'un démarrage précédemment bloqué.

Plusieurs groupes déclenchés avant la même évaluation sont conservés dans un ensemble et démarrent un seul combat commun. Un monstre appartenant à un autre groupe n'est jamais ajouté par ce chemin.

## Diagnostics

Logs attendus :

```text
[MON13.6] Encounter engagement queued ...
[MON13.6] Encounter engagement deferred ... Cause=UnsafeActionOrMotion
[MON13.6] Encounter combat candidates Groups=... Living=... Selected=...
[MON13.6] Encounter combat started ...
```

Un échec de checkpoint ou l'absence de participant combat-ready produit un warning explicite.

## Automation

Filtre principal :

```text
Grimrock.Monsters.MON13.6
```

Couverture :

- rencontre sans LOS/Facing utile -> combat ;
- monstre hors groupe exclu ;
- `StartEncounter` répété idempotent mais réémet l'engagement ;
- demande non perdue pendant un état temporairement non sûr ;
- plusieurs groupes coalescés sans perte.

La suite `Grimrock.Monsters.MON14.1` reste la régression de l'engagement visuel ordinaire.
