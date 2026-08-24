# MON20.9.4 — Automation Validation

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — 8/8 SUCCESS — 24/24 CUMULÉS MON20.9**

## Filtre ciblé

```text
Grimrock.MON20.9.RestoredConsumers
```

Résultat :

```text
EmptySnapshotClearsTransientConsumers    Success
InvalidRestorePreservesConsumers         Success
RestoreBelowThresholdKeepsActionLocked   Success
RestoreProjectsSkillId                   Success
RestoreProjectsThresholdRequirement      Success
RestoreUnlocksAction                     Success
RestoreUpdatesSelectedCharacterPage      Success
RestoreUpdatesSkillsPage                 Success

8 / 8 Success
0 Fail
0 Error
```

## Campagne cumulative MON20.9

Le même run a exécuté les trois familles MON20.9 :

```text
Grimrock.MON20.9.SkillPersistence        8/8 Success
Grimrock.MON20.9.ActivePoolPersistence   8/8 Success
Grimrock.MON20.9.RestoredConsumers       8/8 Success
---------------------------------------------------
TOTAL                                   24/24 Success
0 Fail
0 Error
```

## Contrats confirmés

La campagne confirme que :

- les rangs Skill persistants sont reconstruits par `CharacterId` ;
- le `SkillId` d'un rang entraîné est projeté immédiatement ;
- les `GrantedRequirementIds` sont recalculés selon les seuils ;
- un seuil non atteint reste absent ;
- `FGridCombatActionCatalog` déverrouille ou verrouille correctement les actions ;
- `MissingRequirements` reste cohérent avec l'état restauré ;
- `FGridSkillsPageService` affiche le rang restauré ;
- `SelectedCharacterIndex` reste l'autorité de la page sélectionnée ;
- un snapshot vide remet les données transient à zéro ;
- un restore invalide échoue atomiquement sans mutation partielle ;
- aucune donnée dérivée de requirement ou de page n'est persistée.

## Suite

```text
MON20.9.5 — Automation / PIE Regression & Closure
```

La logique MON20.9 est désormais couverte à **24/24 Automation**. Le dernier travail de MON20.9.5 est un smoke test PIE de la chaîne Save/Continue et la clôture documentaire du jalon.
