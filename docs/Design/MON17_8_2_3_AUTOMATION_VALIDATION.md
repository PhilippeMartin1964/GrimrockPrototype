# MON17.8.2 / MON17.8.3 — Automation Validation

Date : 2026-08-22

Statut : **VALIDATION AUTOMATISEE UE5.5.4 REUSSIE — 51/51 tests Success**

Ce document consigne le retour d'execution fourni depuis l'environnement local UE5.5.4 apres :

- MON17.8.2 — Generic Monster Walk Synchronization ;
- MON17.8.3 — Generic Monster Presentation State Integration.

## Résultat global

```text
Tests exécutés : 51
Success         : 51
Failed          : 0
```

Répartition :

```text
Grimrock.Monsters.MON17.8 :  2 /  2 Success
Grimrock.Monsters.MON17.2 :  2 /  2 Success
Grimrock.Monsters.MON17.3 : 10 / 10 Success
Grimrock.Monsters.MON10   : 37 / 37 Success
---------------------------------------------
Total                      : 51 / 51 Success
```

## MON17.8

```text
Grimrock.Monsters.MON17.8.AnimationStateBridgeContract  Success
Grimrock.Monsters.MON17.8.BestiaryPresentationBridge    Success
```

Ces résultats valident côté automation :

- le pont générique `UGridMonsterAnimInstance` ;
- les propriétés/états de présentation génériques ;
- le chargement et la compatibilité Skeleton/AnimBP du RatGiant ;
- le chargement et la compatibilité Skeleton/AnimBP du GoblinThrower ;
- l'absence de régression du contrat de présentation générique couvert par ces tests.

## Régressions MON17.2

```text
PresentationBridgeContract       Success
VisualRotationOffsetContract     Success
```

Le contrat historique Mesh / Skeleton / AnimBP et le `VisualRotationOffset` restent protégés.

## Régressions MON17.3

Les dix tests MON17.3 fournis passent, couvrant notamment :

- LineOfSight ;
- planner melee ;
- planner ranged stationnaire ;
- timing projectile ;
- trajectoire projectile ;
- optionalité visuelle projectile ;
- source projectile ;
- isolation/lifecycle cooldown ;
- contrat zéro cooldown Goblin.

Le pipeline `ThrowKnife` n'est donc pas régressé par les changements de locomotion selon la couverture automatisée existante.

## Régressions MON10

Les 37 tests MON10 fournis passent, couvrant notamment :

- audio ;
- Hurt/Death exclusivity ;
- idle variations ;
- restauration silencieuse ;
- optimisation/runtime metrics ;
- seeds déterministes ;
- VFX ;
- combat log.

## Warnings observés

Deux catégories de warnings apparaissent dans le log fourni :

```text
LogRendererCore: Warning: FlushRenderingCommands called recursively!
```

pendant le démarrage des tests MON17.8, et :

```text
LogGridMonsterCombat: Warning: Initialization failed ... Party=None
```

pendant un fixture `IdleVariationSchedulingLifecycle` MON10.

Dans les deux cas, les tests concernés se terminent explicitement avec `Result={Success}`. Aucun échec automatisé n'est associé à ces warnings dans le retour fourni.

## Validation PIE GoblinThrower acquise séparément

La locomotion GoblinThrower a été validée visuellement avant ce run avec :

```text
MoveDuration             = 1.00 s
Idle -> Walk Blend       = 0.20 s
Walk -> Idle Blend       = 0.20 s
Walk Sequence            = A_GoblinThrower_Walk_Fwd
ExplicitTime             = 0.895 + MoveAlpha * 0.905
```

Le résultat a été accepté comme référence visuelle MON17.8.2.

## Éléments non attestés par ce log

Ce fichier de test ne constitue pas un log de compilation Visual Studio / UnrealBuildTool distinct. Il ne faut donc pas transformer ce retour en affirmation séparée de build C++ validé.

De même, la non-régression PIE manuelle complète du RatGiant n'est pas explicitement consignée dans ce log ; `BestiaryPresentationBridge` couvre son pont Mesh/Skeleton/AnimBP côté automation.

## Conclusion

La validation automatisée demandée pour MON17.8.2 / MON17.8.3 est **réussie : 51/51 Success**.

La suite fonctionnelle est :

```text
MON17.8.4 — Generic Monster Death Animation
```

Le GoblinThrower reste le cas pilote, mais le runtime doit continuer à utiliser le contrat générique existant :

```text
UGridMonsterDefinitionAsset.DeathMontage
DeathExpectedDuration
UGridMonsterDeathComponent
```

La logique de mort, loot, XP et `MonsterDied` reste indépendante de la présentation animée.
