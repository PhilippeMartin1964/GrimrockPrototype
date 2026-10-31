# TRAP01.4.1 — Récompense d'expérience après désamorçage

31 octobre 2026 — Implémentation publiée, validation UE 5.5.4 locale en attente.

## Contrat validé

- Seule une réussite **committée** de l'action `TryDisarmTriggerTrap` attribue de l'XP.
- Un piège détecté, déclenché, désactivé à distance, refusé ou dont le désamorçage échoue ne donne aucune XP de désamorçage.
- `XPGroup = 2 * DisarmDifficulty` ; la difficulté légale est 1..40. Hors bornes, la récompense est nulle.
- La récompense représente un **pool de groupe**, pas un bonus exclusif au saboteur.
- L'attribution utilise exclusivement `FRPGExperienceRewardService::AwardToActiveParty`. Cela conserve la répartition MON15.2, les personnages actifs à 0 PV, le plafonnement, les level-up MON15.3 et les notifications natives existantes.
- La protection **exactly once** réutilise la garde persistante `DisarmedTriggerTrapIds`. Elle est écrite avant l'appel au service XP et avant les liens logiques `Disarmed`. Aucune table d'XP parallèle ni nouvelle donnée SaveGame.
- Une récompense effectivement appliquée de 0 XP (groupe au niveau maximal) ne doit produire aucune notification XP, mais ne réarme pas le piège.
- Les tests vérifient la copie/restauration des états, sans prétendre tester un cycle complet de fichier disque.
- Le flux visuel vert est traité exclusivement dans le second commit `EVENTHUD01.XP01`, et sera **centralisé** dans le service de récompense commun pour les monstres comme pour l'exploration.

## Barème

| DC | XP groupe |
|---:|---:|
| 5 | 10 |
| 10 | 20 |
| 15 | 30 |
| 20 | 40 |
| 30 | 60 |
| 40 | 80 |

## Code

- `Public/Runtime/GridDungeonTrapRewardRules.h` : formule unique, invalides rejetés.
- `Private/Runtime/GridLevelRuntimeActorTraps.cpp` : application transactionnelle après succès et GUID durable.
- `Private/Tests/GridTRAP0141DisarmExperienceTests.cpp` : progression des récompenses et bornes.
- `Private/Tests/GridTRAP014PartyDisarmTests.cpp` : suite existante étendue (répartition XP, échec, rejets, snapshot Save/Continue, unicité).

## Validation UE locale

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TRAP01.4.1"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.TRAP01.4"
```

Prévoir en fin de livraison une régression `Grimrock.RPG.MON15.2`, `Grimrock.TRAP01.3` et `Grimrock.RPG.ARMEMENT01.16.Trap`.
