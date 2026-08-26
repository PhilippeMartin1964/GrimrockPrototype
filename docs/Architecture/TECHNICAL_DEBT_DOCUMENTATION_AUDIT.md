# TD05.1 — Audit des rubriques de dette technique et re-baseline RuntimeActor

Date : **26 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Baseline auditée : `7b6daf18fbc1a9f589fa3f1ad85668428cf8c71f`  
Statut : **AUDIT TERMINÉ**

## Objet

Cet audit répond à deux constats : plusieurs rubriques locales `Dette technique` décrivent encore l’état antérieur à TD01–TD04, et `AGridLevelRuntimeActor` reste une concentration structurelle importante malgré les extractions Persistence et World Items.

La source autoritaire de l’état courant reste `docs/Architecture/TECHNICAL_DEBT_REGISTER.md`. Les documents MON/TD/STYLE datés restent des archives de jalon et ne sont pas réécrits pour effacer leur contexte historique. Les documents `CURRENT`, `FOUNDATION` et l’index d’architecture doivent en revanche refléter le code actuel.

## Inventaire des rubriques `Dette technique`

| Document | Nature | Verdict TD05.1 |
|---|---|---|
| `docs/Design/UI_ARCHITECTURE_CURRENT.md` | courant | à corriger : `TD-PARTY-001` résolu, SaveGame v9 |
| `docs/Architecture/ARCHITECTURE_INDEX.md` | courant | à corriger : SaveGame v9, phase et références courantes |
| `docs/Architecture/UI_GAME_FLOW_FOUNDATION.md` | courant | à corriger : `TD-PARTY-001` résolu, SaveGame v9, TD04 local validé |
| `docs/Architecture/COMBAT_MONSTER_AI_FOUNDATION.md` | courant | correction mineure : le contrat Event -> Command est résolu ; seule la concentration interne reste surveillée |
| `docs/Architecture/SAVE_PERSISTENCE_FOUNDATION.md` | courant | correction critique : annonce encore SaveGame v8 et `TD-PERSIST-001` actif |
| `docs/Architecture/TECHNICAL_DEBT_REGISTER.md` | autoritaire | `TD-ARCH-001` doit redevenir une priorité P2 ciblée |
| `docs/Design/STYLE01_NEXT_THREAD_PROMPT.md` | historique | ne pas réécrire |
| `docs/Design/STYLE01_CPP_FORMATTING_BASELINE.md` | historique clos | ne pas réécrire |
| `docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md` | snapshot du 23 août | utile, mais plus autoritaire pour le statut courant tant qu’il n’est pas rafraîchi |
| `docs/Design/TD02_9_PARTY_MOVEMENT_AND_STOP_CONDITION.md` | clôture historique | ne pas réécrire ; stop condition PartyPawn toujours valide |

Deux documents courants sans rubrique exacte `Dette technique` restent à rafraîchir globalement lors de la prochaine passe de synthèse : `PROJECT_SYNTHESIS.md` et la carte détaillée. Le présent audit et `TECHNICAL_DEBT_REGISTER.md` prévalent entre-temps.

## Corrections factuelles autoritaires

### SaveGame

Le code courant déclare :

```cpp
UGrimrockPartySaveGame::CurrentSaveVersion = 9;
UGrimrockPartySaveGame::MinimumCompatibleSaveVersion = 1;
```

La version 9 a été introduite par TD01.1 pour persister `bCanRemoveItem`. `TD-PERSIST-001` est donc **RÉSOLU**. Les sauvegardes v1-v8 migrent avec la politique legacy explicite `bCanRemoveItem = true`.

### Party selection / held visual

`TD-PARTY-001` est **RÉSOLU** depuis TD01.2. `UGridPartyInventoryComponent::SelectedCharacterIndex` reste l’autorité ; le Pawn reçoit la notification autoritaire et resynchronise le held visual. Les contrats `SelectionChange` et `SelectedCharacterFilter` ont été validés sous UE5.5.4.

### Event -> Command

`TD-EVENT-001` est **RÉSOLU**. La dette `TD-ARCH-005` porte encore sur la concentration interne de `UGridActivationComponent`, pas sur la sémantique Gameplay / StateOnly / Unsupported.

### Validation UE / Shipping

TD04 a établi deux harness réellement exécutés :

```text
Scripts/ValidateUE.ps1       -> Editor Win64 Development + Automation + index.json
Scripts/ValidatePackage.ps1  -> Win64 Shipping + Build/Cook/Stage/Package/Pak/Archive
```

Validation Shipping réelle du 26 août 2026 :

```text
Pak files     : 1
Archive files : 41
Archive bytes : 905582948
```

La CI distante reste conditionnelle à un vrai runner UE5.5.4, mais la validation locale Editor/Automation/Shipping n’est plus une dette manuelle non outillée.

## Re-baseline de `AGridLevelRuntimeActor`

À la baseline auditée :

```text
GridLevelRuntimeActor.cpp
    3 359 lignes
    107 095 octets (~104,6 KiB)

GridLevelRuntimeActor.h
    22 161 octets

Extractions déjà réalisées :
    GridLevelRuntimeActorPersistence.cpp  22 463 octets
    GridLevelRuntimeActorWorldItems.cpp   17 129 octets
```

La taille seule ne justifie pas un refactor. Ici, elle s’accompagne d’une concentration vérifiable d’API et de responsabilités : dungeon/level state, geometry/rebuild, grid queries, portes, interactions, items, monstres, encounters, transitions, façade Event -> Command, diagnostics, feedback UI et preview éditeur.

`TD-ARCH-001` redevient donc une **priorité P2 ciblée**. `AGridLevelRuntimeActor` doit toutefois rester la façade/orchestrateur ; l’objectif n’est pas de créer une nouvelle autorité.

## Première frontière recommandée : Diagnostics

La zone de diagnostics forme déjà une responsabilité cohérente et majoritairement en lecture seule :

```text
GetRuntimeDebugSummary
LogRuntimeDebugSummary
ShowRuntimeDebugSummary
GetLevelAssetDiagnostics
LogLevelAssetDiagnostics
GetPIEReadinessDiagnostics
LogPIEReadinessDiagnostics
```

Cible envisagée :

```text
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActorDiagnostics.cpp
```

Contraintes : aucun nouvel état, aucune nouvelle classe propriétaire, aucun changement du header public, façade conservée, helpers locaux préfixés `GridLevelRuntimeDiagnostics...` pour être Unity-safe, mêmes chaînes et mêmes comportements avant/après.

Geometry/rebuild et Monster spawn/lifecycle sont plus couplés et ne constituent pas une bonne première tranche. Le feedback UI est une autre frontière nette, mais sa caractérisation dépend davantage d’UMG/timers.

## Séquence TD05

```text
TD05.1 — Documentation debt audit / RuntimeActor re-baseline    RÉALISÉ
TD05.2 — RuntimeActor Diagnostics characterization              PROCHAIN
TD05.3 — Extract RuntimeActor Diagnostics                       après baseline verte
TD05.4 — Re-audit RuntimeActor stop condition                   après validation
```

TD05.4 n’impose pas une nouvelle extraction : la stop condition reste applicable dès qu’une extraction supplémentaire n’apporte plus une réduction de risque tangible.
