# TD05.4 — RuntimeActor re-audit after Diagnostics extraction

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Baseline auditée : `0564d296003fec7da8ca1dc99b791e46ed579861`  
Statut : **RÉALISÉ — stop condition NON atteinte**

## 1. Contexte

TD05.2 a caractérisé les diagnostics de `AGridLevelRuntimeActor`. TD05.3 a ensuite déplacé leur implémentation vers :

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorDiagnostics.cpp
```

La validation locale UE5.5.4 post-extraction a donné :

```text
Grimrock.TechnicalDebt.TD05_2.RuntimeActorDiagnostics.Contract
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
```

Le commit publié est :

```text
0564d296003fec7da8ca1dc99b791e46ed579861
Extract TD05.3 RuntimeActor diagnostics
```

## 2. Mesure post-TD05.3

Mesure locale validée avant publication :

```text
GridLevelRuntimeActor.cpp
    avant TD05.3 : 3 359 lignes
    après TD05.3 : 2 951 lignes

GridLevelRuntimeActorDiagnostics.cpp
    après formatage : 480 lignes
```

L'extraction réduit donc nettement la taille du fichier principal, mais la taille n'est pas le critère de stop. Le header public reste important et le RuntimeActor conserve plusieurs responsabilités distinctes.

## 3. Responsabilités encore présentes dans le fichier principal

Les domaines visibles après TD05.3 restent notamment :

```text
Construction / BeginPlay / EndPlay
Geometry / rebuild / transforms
Grid queries / movement / walls
Doors / edge interaction
Dungeon transitions
Runtime object placement / spawning
Monster spawn / lifecycle / encounter / teleport
Item definition / placement support
Event -> Command façade
Editor preview façade
Readable / interaction / combat feedback UI
```

`AGridLevelRuntimeActor` doit rester la façade/orchestrateur du niveau. L'objectif n'est donc pas d'éliminer toutes ces méthodes, mais de déplacer uniquement les responsabilités autonomes qui réduisent réellement le risque de maintenance.

## 4. Candidats examinés

### 4.1 Feedback UI — RETENU

Le RuntimeActor porte actuellement trois familles de présentation :

```text
Readable message
    ShowReadableMessage
    HasActiveReadableMessage
    DismissReadableMessage
    HideReadableMessage

Interaction feedback
    ShowInteractionFeedback
    HideInteractionFeedback

Combat feedback
    ShowCombatFeedback
    HideCombatFeedback
```

Leur état est également groupé dans le RuntimeActor :

```text
ReadableMessageWidgetClass
InteractionFeedbackWidgetClass
CombatFeedbackWidgetClass
bReadableMessageAutoHide
ReadableMessageDuration

ActiveReadableMessageWidget
ActiveInteractionFeedbackWidget
ActiveCombatFeedbackWidget

ReadableMessageTimerHandle
InteractionFeedbackTimerHandle
CombatFeedbackTimerHandle
```

Cette responsabilité :

- ne possède aucune donnée gameplay autoritaire ;
- ne modifie ni `UGridLevelAsset`, ni `FGridDungeonRuntimeState` ;
- ne touche ni occupancy, ni Event -> Command, ni monster state ;
- dépend principalement du `UWorld`, du `APlayerController`, des classes de widget et des timers ;
- expose déjà une API cohérente et regroupée ;
- peut être déplacée dans un nouveau translation unit sans modifier le header public.

Cible :

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorFeedbackUI.cpp
```

Le RuntimeActor reste propriétaire des widgets et timers ; il n'y a donc aucune nouvelle autorité et aucune nouvelle classe d'état.

### 4.2 Monster spawn / lifecycle — DIFFÉRÉ

Le domaine Monster est plus volumineux mais fortement couplé à :

```text
FGridDungeonRuntimeState
monster placement persistence
UGridMonsterOccupancySubsystem
UGridMonsterEncounterComponent
combat abort / presentation
movement component
monster definition validation
Event -> Command
```

Une extraction peut être justifiée plus tard, mais son rayon de régression est nettement supérieur. TD05.4 ne la retient pas comme prochaine tranche.

### 4.3 Geometry / rebuild — DIFFÉRÉ

Le bloc Geometry/Rebuild reste fortement couplé aux ISM, aux transforms, au preview éditeur et au spawning des objets runtime. Le déplacer maintenant clarifierait moins l'architecture qu'il n'augmenterait le coût de navigation.

### 4.4 Doors / interaction / transitions — SURVEILLÉ

Le domaine est fonctionnellement cohérent mais partage les conventions directionnelles, les helpers de grille, `DoorSystemComponent`, `ActivationComponent` et le contrat de transition. Il pourra être réévalué après Feedback UI, mais n'est pas retenu maintenant.

## 5. Verdict de stop condition

**Stop condition NON atteinte après TD05.3.**

Motif : il reste une frontière Feedback UI très nette, à faible risque, sans nouvelle autorité et avec un bénéfice concret :

- retrait d'une responsabilité de présentation du gros orchestrateur runtime ;
- réduction des dépendances UI de `GridLevelRuntimeActor.cpp` ;
- regroupement des timers/widgets dans un translation unit dédié ;
- meilleure lisibilité de la façade de niveau.

En revanche, TD05.4 ne justifie pas une décomposition mécanique des domaines Monster ou Geometry.

## 6. Suite décidée

```text
TD05.1 — Documentation debt audit / RuntimeActor re-baseline    RÉALISÉ
TD05.2 — RuntimeActor Diagnostics characterization              RÉALISÉ / VALIDÉ
TD05.3 — Extract RuntimeActor Diagnostics                       RÉALISÉ / VALIDÉ
TD05.4 — RuntimeActor re-audit                                  RÉALISÉ
TD05.5 — Feedback UI characterization                           PROCHAIN
TD05.6 — Extract RuntimeActor Feedback UI                       après baseline verte
TD05.7 — RuntimeActor final stop-condition audit                après validation
```

TD05.7 devra décider explicitement entre :

```text
STOP — concentration résiduelle acceptable pour une façade/orchestrateur
```

ou une nouvelle tranche uniquement si une frontière supplémentaire est justifiée par un risque concret.
