# TD07.4 — ActivationComponent Characterization

Date : 28 août 2026
Projet : GrimrockPrototype — Unreal Engine 5.5.4
Dette : TD-ARCH-005 — UGridActivationComponent concentré
Statut : CHARACTERIZATION PREPARED — À VALIDER

## 1. Objectif

Caractériser la concentration actuelle de `UGridActivationComponent` avant toute décision d'extraction.

Règles :

- pas de refactor massif ;
- Event -> Command reste un bus unique ;
- une seule autorité runtime ;
- une extraction n'est autorisée que si elle réduit un risque observé ;
- ne pas déplacer des lignes uniquement pour réduire la taille du fichier.

## 2. Baseline statique

Audit du HEAD au 28 août 2026 :

```text
GridActivationComponent.h
    135 lignes
    5 404 octets

GridActivationComponent.cpp
    1 542 lignes
    52 224 octets
    43 occurrences UGridActivationComponent::
    41 appels UE_LOG
    38 occurrences LogTemp
```

Le volume seul n'est pas une preuve suffisante pour extraire.

## 3. Responsabilités actuellement concentrées

```text
Interaction directe
    TryInteractAtEdge
    ActivateObject
    ActivateReadableObject
    ActivateReceptacle

Cell events
    Pressure Plates
    Triggers
    Enter / Exit

Event -> Command
    ExecuteLinksFromObjectForEvent
    ApplyLinkCommand
    condition evaluation
    Door / Receptacle / Stateful adapters

Quest
    RegisterCurrentLevelQuestDefinitions
    ApplyQuestLinkCommand

Lua
    ReloadLuaRuntime
    callback dispatch
    Lua-issued commands
    recursion guard
    shared action budget

Recruitment
    Story Companion offer suppression / decline memory

Indexes / runtime state
    ObjectIndexById
    LinkIndexesBySource
    edge/cell indexes
    ActiveObjectIds

Diagnostics
    link result logging
    debug summary
```

## 4. Autorité

Le contrat important est sain :

```text
AGridLevelRuntimeActor
    -> exactement un UGridActivationComponent

UGridActivationComponent
    -> ActiveObjectIds
    -> Event -> Command dispatch
    -> recursion/action-budget guard
    -> indexes
```

TD07.4 ne doit pas créer un second dispatcher ou une seconde autorité d'état.

## 5. Couverture de régression existante

La caractérisation vérifie la présence de tests autour de :

- Event -> Command hardening ;
- Logic primitive / variable conditions ;
- Lua bridge / LogicId / production puzzle ;
- Monster integration ;
- Story Companion Event -> Command et offer suppression ;
- Custom Recruit Event -> Command.

Cette couverture rend possible une extraction ciblée si un risque concret est confirmé.

## 6. Frontière avec TD07.7

La présence de `LogTemp` dans ce fichier est enregistrée mais **n'est pas nettoyée en TD07.4**.

La taxonomie de logs appartient à :

```text
TD07.7 — Targeted log / formatting hygiene
```

Cela évite de mélanger refactor architectural et hygiene.

## 7. Gate

```text
Grimrock.TechnicalDebt.TD07_4.Characterization
```

Sous-tests :

```text
SurfaceMetrics
ResponsibilityMap
AuthorityBoundary
RegressionCoverage
```

## 8. Décision après validation

Après caractérisation verte :

1. mesurer si une responsabilité crée aujourd'hui duplication d'autorité, fragilité de test ou couplage bloquant ;
2. si oui, extraire uniquement cette frontière ;
3. si non, **ne pas refactorer** et clôturer TD07.4 comme dette surveillée caractérisée.

La taille du fichier n'est pas, seule, une justification d'extraction.
