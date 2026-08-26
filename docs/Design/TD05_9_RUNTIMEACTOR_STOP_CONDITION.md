# TD05.9 — RuntimeActor final stop condition

Date : 26 août 2026  
Baseline : `2e5c00f64265603033d86b36869e84b1b8311179` — `Extract TD05.8 RuntimeActor monsters`

## 1. Objet

TD05.9 clôt la campagne ciblée de réduction de dette sur `AGridLevelRuntimeActor`.

La question n’est pas de savoir si `GridLevelRuntimeActor.cpp` peut encore être découpé, mais si une nouvelle extraction supprimerait aujourd’hui un risque concret sans fragmenter l’autorité du niveau ni compliquer artificiellement l’orchestration.

## 2. Évolution mesurée

Baseline TD05.1 :

```text
GridLevelRuntimeActor.cpp
    3 359 lignes
    107 095 octets (~104,6 KiB)
```

Après TD05.3 Diagnostics :

```text
GridLevelRuntimeActor.cpp
    2 951 lignes
```

Après TD05.6 Feedback UI :

```text
GridLevelRuntimeActor.cpp
    2 768 lignes
```

Après TD05.8 Monsters :

```text
GridLevelRuntimeActor.cpp
    ~1 882 lignes

GridLevelRuntimeActorMonsters.cpp
    ~958 lignes
```

TD05.8 a déplacé 886 lignes hors du fichier principal. Sur l’ensemble de TD05, le fichier principal passe de 3 359 à environ 1 882 lignes :

```text
réduction : 1 477 lignes
réduction relative : ~44 %
```

Le nombre de lignes reste un indicateur secondaire ; la décision repose surtout sur la séparation effective des responsabilités et le risque des frontières restantes.

## 3. Responsabilités désormais isolées

Les responsabilités suivantes ne sont plus mêlées au corps principal :

```text
Persistence
    GridLevelRuntimeActorPersistence.cpp

World Items
    GridLevelRuntimeActorWorldItems.cpp

Diagnostics
    GridLevelRuntimeActorDiagnostics.cpp

Feedback UI
    GridLevelRuntimeActorFeedbackUI.cpp

Monster spawn / lifecycle / encounter façade
    GridLevelRuntimeActorMonsters.cpp
```

Le RuntimeActor reste l’unique façade/orchestrateur du niveau. Aucun nouvel owner d’état n’a été introduit.

## 4. Validation TD05.8

La suite MON13 existante a servi de caractérisation avant/après extraction.

Baseline pré-extraction après remise à niveau des fixtures :

```text
Filter                 : Grimrock.Monsters.MON13
Succeeded              : 13
Succeeded with warnings: 4
Failed                 : 0
Not run                : 0
```

Validation post-extraction :

```text
Filter                 : Grimrock.Monsters.MON13
Succeeded              : 13
Succeeded with warnings: 4
Failed                 : 0
Not run                : 0
```

Soit 17/17 tests exécutés avec succès avant et après TD05.8.

Les corrections de fixtures MON13 ont été séparées de l’extraction de production :

```text
74e7a184465e56c3b8537d9414563f6d6f481aa7
    Fix MON13 event marker fixtures

be15574eda445f5ee9c6da20a79896161c5d2a67
    Fix MON13 marker mesh fixtures

2e5c00f64265603033d86b36869e84b1b8311179
    Extract TD05.8 RuntimeActor monsters
```

## 5. Ce qui reste dans le fichier principal

Le corps principal conserve surtout les responsabilités structurelles qui définissent précisément la façade d’un niveau runtime :

- construction / `BeginPlay` / `EndPlay` ;
- sélection du `LevelAsset` courant et état de dungeon ;
- rebuild et nettoyage transversal ;
- géométrie Floor / Wall / Ceiling et transforms de placement ;
- requêtes de grille et mouvement ;
- orchestration des objets runtime génériques ;
- portes et interaction d’arêtes via les composants dédiés ;
- transitions de dungeon ;
- glue vers Activation, DoorSystem, EditorPreview et PlayerAttackPresentation ;
- `RebuildRuntimeObjects()` comme orchestrateur cross-domain.

Ces responsabilités restent nombreuses, mais elles sont désormais cohérentes avec le rôle de façade du RuntimeActor.

## 6. Frontières réévaluées

### Geometry / rebuild

Une extraction est techniquement possible, mais elle déplacerait un noyau fortement connecté à :

- `FloorISM`, `WallISM`, `CeilingISM` ;
- transforms de placement ;
- `LevelAsset` ;
- génération d’objets ;
- preview éditeur ;
- rebuild transversal.

Aucun défaut actuel n’exige ce split.

Verdict : **ne pas extraire maintenant**.

### Doors / interactions

Le comportement spécialisé est déjà délégué à `UGridDoorSystemComponent` et `UGridActivationComponent`. Les méthodes restantes dans le RuntimeActor sont essentiellement façade/adaptation de coordonnées et orchestration.

Verdict : **ne pas créer un fichier supplémentaire uniquement pour déplacer ces wrappers**.

### Generic runtime objects / placement

Items complexes et World Items possèdent déjà leur frontière dédiée. Le code restant assemble les archétypes, transforms et actors génériques dans le rebuild commun.

Verdict : **conserver dans le noyau tant qu’une douleur fonctionnelle ou de test ne justifie pas une nouvelle frontière**.

## 7. Stop condition

La règle autoritaire du registre est :

> si le changement ajoute de la complexité sans supprimer un risque observé, arrêter.

Après TD05.8, une nouvelle extraction Geometry/Doors/Generic Objects serait principalement motivée par la taille résiduelle du fichier, et non par :

- une duplication d’autorité ;
- une régression connue ;
- un contrat non testable ;
- un problème de persistance ;
- une dépendance circulaire concrète ;
- une difficulté actuelle bloquant le développement fonctionnel.

**La stop condition TD05 est donc atteinte.**

## 8. Décision architecturale

`TD-ARCH-001` n’est plus une priorité ciblée immédiate.

Il reste classé **P2 — surveillée / stop condition atteinte**, de la même manière que les autres gros objets dont la responsabilité résiduelle est cohérente avec leur rôle.

Réouvrir cette dette uniquement si une douleur concrète apparaît, par exemple :

- modification Geometry impossible sans toucher plusieurs domaines non liés ;
- nouvelle duplication d’état ;
- tests difficiles à isoler ;
- dépendances de compilation problématiques ;
- nouvelle responsabilité métier importante ajoutée directement au fichier principal.

La taille seule ne constitue plus un motif suffisant.

## 9. Suite recommandée

La campagne TD05 est close :

```text
TD05.1  audit documentaire / re-baseline              RÉALISÉ
TD05.2  Diagnostics characterization                   VALIDÉ
TD05.3  Diagnostics extraction                         VALIDÉ
TD05.4  re-audit                                       RÉALISÉ
TD05.5  Feedback UI characterization                   VALIDÉ
TD05.6  Feedback UI extraction                         VALIDÉ
TD05.7  post-feedback re-audit                         RÉALISÉ
TD05.8  Monster extraction                             VALIDÉ
TD05.9  final stop condition                           ATTEINTE
```

Aucune nouvelle tranche de dette technique n’est prioritaire immédiatement.

Le développement fonctionnel peut reprendre. **MON21.2** redevient la suite recommandée, sauf priorité produit plus récente explicitement décidée.
