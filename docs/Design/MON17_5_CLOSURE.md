# MON17.5 — Patrol / Perception / Alarm Integration — CLOSURE

Statut : **VALIDÉ ET CLOS sous UE5.5.4**

## Objectif

Prouver que la seconde famille de monstres, `MON_GoblinThrower` avec `PrimaryAIProfile=RangedKeeper`, s'intègre aux systèmes d'exploration MON14 sans logique parallèle spécifique au Gobelin.

## Contrat final

```text
Patrouille MON14.3
→ perception directionnelle / ouïe MON14.2
→ investigation / recherche MON14.3
→ alarme locale MON14.4
→ engagement visuel automatique MON14.1
→ TurnManager
→ tactique RangedKeeper MON17.4
```

Aucun nouveau moteur d'IA n'a été créé pour cette étape.

## Valeurs de production

```text
DA_MON_GoblinThrower
bSharesAggroWithGroup = true
AggroPropagationRange = 5
```

Les `MonsterSpawn` restent responsables de :

```text
EncounterGroupId
InitialMonsterState
PatrolMode
PatrolWaypoints
```

## Validation automatisée

```text
Grimrock.Monsters.MON17.5.1
    PatrolRangedKeeper          Success
    DirectionalPerception      Success
    HearingAlarm               Success
    VisionEngagementHandoff    Success

MON17.5.1                      4/4 Success
MON14 présent dans le run      19/19 Success
Campagne complète             198/198 Success
```

Le test `HearingAlarm` confirme explicitement que l'ouïe/alarme seule peut réveiller un allié et lancer l'investigation sans démarrer directement le combat.

## Validation PIE de production

Deux vrais Gobelins lanceurs ont été placés dans `Into_The_Dark` :

```text
MonsterId        = MON_GoblinThrower
EncounterGroupId = Encounter_GoblinThrowers_01
```

Le log confirme :

```text
[MON14.4] ExplorationAlert
Source=BP_MON_GoblinThrower_C_2
Group=Encounter_GoblinThrowers_01
Cell=(29,22)
Range=5
Alerted=1
Reason=PerceptionHearing
```

Puis :

```text
[MON14.1] Automatic combat started
Reason=PatrolVision
```

Les deux Gobelins rejoignent l'initiative. Le premier exécute `Attack_ThrowKnife` avec projectile depuis `ProjectileSource`, puis le second exécute lui aussi son attaque à distance dans le même combat.

La phase « ouïe seule » n'est pas longtemps observable dans ce PIE car une LOS valide est résolue dans la même évaluation. Ce comportement est cohérent : le contrat de non-engagement par ouïe seule est couvert par Automation, tandis que le PIE valide la chaîne de production complète.

## Régressions / architecture

MON17.5 ne modifie pas :

- le planner `RangedKeeper` ;
- le pipeline projectile MON17.3 ;
- le pathfinding MON14 ;
- le contrat de perception directionnelle ;
- le TurnManager ;
- les règles de propagation same `MonsterId` / same `EncounterGroupId`.

## Conclusion

`MON17.5 — Patrol / Perception / Alarm Integration` est clos.

La prochaine étape autoritaire est :

```text
MON17.6 — Encounter / Loot / XP Integration
```

Cette étape doit démontrer l'intégration du Gobelin avec les groupes/vagues MON13, la mort exactly-once, le loot data-driven MON8, la récompense `ExperienceReward=125` MON15 et la persistance sans duplication.
