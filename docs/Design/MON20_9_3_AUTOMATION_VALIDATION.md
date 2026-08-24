# MON20.9.3 — Automation Validation

Date : **24 août 2026**  
Statut : **VALIDÉ UE5.5.4 — 8/8 SUCCESS**

Validation fournie après correction de la collision de helpers en build Unity.

Filtre :

```text
Grimrock.MON20.9.ActivePoolPersistence
```

Résultat :

```text
ActiveSnapshotRestoresAfterReserveMove  Success
MixedActivePoolIsolation                 Success
PoolReorderRestoresByIdentity            Success
PoolSnapshotRestoresAfterRecruitment     Success
RecruitmentPreservesPoolSkill            Success
RecruitmentRejectPreservesPoolSkill      Success
SelectedCharacterIndependent             Success
SnapshotValidAfterRecruitmentMove        Success

8 / 8 Success
0 Fail
0 Error
```

Cette campagne valide que les Skills restent attachés au `CharacterId` à travers recrutement, réserve, réordonnancement et changement de personnage sélectionné.
