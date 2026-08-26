# GrimrockPrototype — Registre autoritaire de dette technique

Date de référence : **26 août 2026**  
Baseline fonctionnelle validée : `0564d296003fec7da8ca1dc99b791e46ed579861` — TD05.3 RuntimeActor Diagnostics extrait et validé sous UE5.5.4  
Baseline documentaire auditée : `0564d296003fec7da8ca1dc99b791e46ed579861`  
Statut : **ACTIF — TD05 RUNTIMEACTOR CIBLÉ**

Ce document est la source autoritaire pour la dette technique du projet. Les documents de jalon datés restent valides pour leur époque ; l’état courant, les priorités et la prochaine tranche de dette sont définis ici.

TD04 a atteint sa stop condition locale. MON21.2 n’est plus bloqué par TD04, mais TD05 traite d’abord la concentration résiduelle de `AGridLevelRuntimeActor` tant qu’une frontière claire et à faible risque reste disponible.

---

## 1. Définition

Une entrée est une dette technique lorsqu’un choix historique, un contrat incomplet ou une concentration de responsabilités augmente aujourd’hui au moins un des risques suivants :

- régression fonctionnelle ;
- incohérence Save / Continue ;
- comportement implicite ou non testable ;
- difficulté à modifier une zone sans effet de bord ;
- duplication d’autorité ;
- diagnostic ou maintenance inutilement difficiles ;
- dépendance manuelle évitable dans le workflow de validation.

Ne sont pas comptés comme dette technique : contenu de production manquant, bestiaire/sorts limités, Quests/Journal/Map/Codex, Recipes, futur éditeur joueur, publication ou campagne complète.

---

## 2. Priorités et compteurs

```text
P0 — bloque le projet / corruption / perte de données connue
P1 — risque fonctionnel, persistance ou contrat incohérent
P2 — maintenabilité / architecture / tooling à réduire de manière ciblée
P3 — nettoyage opportuniste ou intégration non bloquante
```

État au 26 août 2026 :

```text
P0 : aucun blocage connu
P1 : 0 dette active
P2 : 8 dettes actives ou surveillées
P3 : 2 dettes actives
```

`TD-ARCH-001` est la seule dette P2 actuellement en **priorité ciblée immédiate**. Les autres P2 restent surveillées ou différées.

---

# 3. Registre actif

## TD-ARCH-001 — `AGridLevelRuntimeActor` trop centralisé

**Priorité : P2 — PRIORITÉ CIBLÉE ACTUELLE**

TD05.1 a rebaseliné la concentration initiale :

```text
GridLevelRuntimeActor.cpp
    3 359 lignes
    107 095 octets (~104,6 KiB)

GridLevelRuntimeActor.h
    22 161 octets
```

Extractions déjà réalisées :

```text
TD-ARCH-001.1 — Persistence   RÉALISÉ
    GridLevelRuntimeActorPersistence.cpp

TD-ARCH-001.2 — World Items   RÉALISÉ
    GridLevelRuntimeActorWorldItems.cpp

TD-ARCH-001.3 — Diagnostics   RÉALISÉ / VALIDÉ UE5.5.4
    GridLevelRuntimeActorDiagnostics.cpp
```

### État après TD05.3

Mesure locale sur l’état validé puis publié :

```text
GridLevelRuntimeActor.cpp
    2 951 lignes

GridLevelRuntimeActorDiagnostics.cpp
    480 lignes après clang-format 19.1.5
```

Le volume n’est pas le seul signal : le header expose encore dungeon/level state, geometry/rebuild, grid queries, portes, interactions, items, monstres/encounters, transitions, façade Event -> Command, feedback UI et preview éditeur.

`AGridLevelRuntimeActor` doit néanmoins rester la façade/orchestrateur du niveau. Aucune extraction ne doit créer une seconde autorité.

### TD05 — tranche actuelle

```text
TD05.1 — Documentation debt audit / RuntimeActor re-baseline    RÉALISÉ
TD05.2 — RuntimeActor Diagnostics characterization              RÉALISÉ / VALIDÉ
TD05.3 — Extract RuntimeActor Diagnostics                       RÉALISÉ / VALIDÉ
TD05.4 — RuntimeActor re-audit                                  RÉALISÉ
TD05.5 — Feedback UI characterization                           PROCHAIN
TD05.6 — Extract RuntimeActor Feedback UI                       après baseline verte
TD05.7 — RuntimeActor final stop-condition audit                après validation
```

TD05.2 a verrouillé :

```text
Grimrock.TechnicalDebt.TD05_2.RuntimeActorDiagnostics.Contract
```

TD05.3 a conservé l’API publique et déplacé uniquement l’implémentation Diagnostics. Validation post-extraction : 1 Success / 0 Failed / 0 warning.

### Frontière suivante retenue : Feedback UI

Méthodes :

```text
ShowReadableMessage
HasActiveReadableMessage
DismissReadableMessage
HideReadableMessage
ShowInteractionFeedback
HideInteractionFeedback
ShowCombatFeedback
HideCombatFeedback
```

État associé déjà groupé dans le RuntimeActor : classes de widget Readable/Interaction/Combat, trois widgets transient, trois timers, `bReadableMessageAutoHide` et `ReadableMessageDuration`.

Cette responsabilité ne possède aucune donnée gameplay autoritaire et peut être déplacée vers :

```text
GridLevelRuntimeActorFeedbackUI.cpp
```

sans changer le header public ni créer une nouvelle classe d’état.

### Frontières différées

```text
Monster spawn/lifecycle
    -> fortement couplé persistance + occupancy + encounters + combat + Event->Command

Geometry/rebuild
    -> fortement couplé ISM + transforms + preview + spawning runtime

Doors/interactions/transitions
    -> cohérent mais partagé avec conventions directionnelles et composants runtime
```

Décision TD05.4 : **stop condition non atteinte**, uniquement parce que Feedback UI constitue encore une frontière nette et à faible risque. Aucun split Monster/Geometry n’est autorisé par défaut.

---

## TD-ARCH-002 — `UGridPartyInventoryComponent` très volumineux

**Priorité : P2 — surveillée**

Le composant reste l’unique autorité d’état du groupe/inventaire.

Réalisé :

```text
TD-ARCH-002.1 — Equipment World Transfer
TD-ARCH-002.2 — Inventory Diagnostics
```

Les extractions futures doivent rester stateless ou transactionnelles et ne jamais fragmenter l’autorité.

**État : aucune tranche immédiate.**

---

## TD-ARCH-003 — `AGrimrockPartyPawn` historiquement trop chargé

**Priorité : P2 — surveillée / stop condition atteinte**

Réalisé :

```text
TD02.4 — Input Buffer
TD02.5 — Held Item Presentation
TD02.6 — Save / Load Façade
TD02.7 — Item Transfer Façade
TD02.8 — UI Façade
TD02.9 — Movement & Rotation Façade
```

Unités dédiées :

```text
GrimrockPartyPawnInputBuffer.cpp
GrimrockPartyPawnHeldItem.cpp
GrimrockPartyPawnSave.cpp
GrimrockPartyPawnItemTransfer.cpp
GrimrockPartyPawnUI.cpp
GrimrockPartyPawnMovement.cpp
```

Décision : **arrêter ici la décomposition de `AGrimrockPartyPawn`** tant qu’une nouvelle douleur concrète n’apparaît pas.

---

## TD-ARCH-004 — `AGrimrockPlayerController` volumineux

**Priorité : P2 — surveillée**

L’audit TD02.9 n’a identifié aucune duplication d’autorité justifiant un refactor immédiat. Le Controller reste responsable du pointeur/curseur, de l’intention souris, du hover/ciblage et de la délégation.

Décision : **ne pas découper le PlayerController isolément**.

---

## TD-ARCH-005 — `UGridActivationComponent` concentré

**Priorité : P2 — surveillée**

Le bus Event -> Command reste l’architecture correcte. `TD-EVENT-001` est résolu : Gameplay / StateOnly / Unsupported sont explicites et protégés par tests.

La dette porte uniquement sur l’organisation interne de `UGridActivationComponent`. Toute extraction future doit conserver un bus et une autorité uniques.

---

## TD-EDITOR-001 — Complexité Slate / Grid Editor

**Priorité : P2 — surveillée**

TD03 a nettoyé la duplication prouvée entre Grid Editor Mode canonique et anciens boutons `CallInEditor` :

```text
TD03.1 — SetStartFromSelection / ValidateCurrentLevel     VALIDÉ
TD03.2 — Move To Current Cell                             VALIDÉ
TD03.3 — Item/Monster Definition Sync                     VALIDÉ
TD03.4 — ApplyCurrentDungeonLevel                         VALIDÉ
```

Stop condition : pas de TD03.5 cosmétique. Les actions avancées/debug restantes restent tant qu’aucun équivalent Slate canonique n’est prouvé.

---

## TD-LOG-001 — Taxonomie de logs partiellement `LogTemp`

**Priorité : P2 — surveillée / opportuniste**

TD01.4 a stabilisé plusieurs domaines. Continuer uniquement dans les domaines réellement touchés ; aucun remplacement global de `LogTemp`.

---

## TD-TOOL-001 — CI distante UE non autoritaire

**Priorité : P2 — différée**

### TD04.1 — contrat CI/Shipping — RÉALISÉ

À la baseline TD04.1, aucune `.github/workflows` ni CI UE autoritaire n’existait.

### TD04.2 — Local UE Validation Harness — RÉALISÉ / VALIDÉ

```text
Scripts/ValidateUE.ps1
GrimrockPrototypeEditor Win64 Development
Automation explicite
index.json contrôlé
```

Le harness a depuis été réutilisé avec succès pour TD05.2/TD05.3.

### TD04.3 — Shipping Package Harness — RÉALISÉ / VALIDÉ

```text
Scripts/ValidatePackage.ps1
Win64 Shipping
Build + Cook + Stage + Package + Pak + Archive
Pak files     : 1
Archive files : 41
Archive bytes : 905582948
```

### TD04.4 — vraie CI UE — CONDITIONNEL / DIFFÉRÉ

Pas de workflow UE tant qu’un vrai runner Windows + UE5.5.4 + toolchain n’est pas provisionné. Lorsqu’il existe, il doit réutiliser les deux harness locaux plutôt que dupliquer leurs commandes.

---

## TD-UI-001 — Nommage historique `Inventory` pour le menu global

**Priorité : P3 — opportuniste**

Exemples : `EInventoryTopTab`, `ToggleInventoryWidget()`. Le contrat fonctionne et peut être sensible aux Blueprint/valeurs sérialisées. Aucun renommage transversal dédié.

---

## TD-RPG-001 — Lancer manuel non encore relié aux Skills

**Priorité : P3 — intégration fonctionnelle**

Le TODO de scaling du lancer reste à relier au design Skills lorsque vitesse/précision/dégâts seront définis. Ne pas traiter comme refactor structurel urgent.

---

# 4. Dettes résolues

## TD-PERSIST-001 — Permission de retrait des réceptacles — RÉSOLU

- SaveGame v8 -> v9 ;
- `bCanRemoveItem` capturé/restauré ;
- migration v1-v8 explicite ;
- Automation round-trip/migration et MON20.9 validées.

## TD-PARTY-001 — Selection / Held Visual notification — RÉSOLU

- `UGridPartyInventoryComponent` reste autorité du personnage sélectionné ;
- notification autoritaire ;
- Pawn resynchronise le held visual ;
- tests `SelectionChange` et `SelectedCharacterFilter` validés.

## TD-EVENT-001 — Event -> Command contract — RÉSOLU

- Gameplay / StateOnly / Unsupported explicites ;
- faux succès state-only supprimés ;
- aucune seconde autorité Event -> Command.

## TD-STYLE-001 — Formatting baseline — RÉSOLU

- `.clang-format`, `.editorconfig`, `.gitattributes` ;
- `Scripts/FormatCpp.ps1` ;
- `Scripts/CheckCppFormat.ps1` ;
- clang-format 19.1.5 figé.

---

# 5. Campagne réalisée et suite

```text
TD01.1–TD01.4   persistance / notification / Event->Command / logs    RÉALISÉ
TD02.1–TD02.9   extractions structurelles ciblées                     RÉALISÉ
TD03.1–TD03.4   Grid Editor Details cleanup                           RÉALISÉ
TD04.1–TD04.3   validation Editor/Automation/Shipping                 RÉALISÉ
TD04.4           CI UE réelle                                         DIFFÉRÉ
TD05.1           doc debt audit / RuntimeActor re-baseline            RÉALISÉ
TD05.2           RuntimeActor Diagnostics characterization            RÉALISÉ / VALIDÉ
TD05.3           RuntimeActor Diagnostics extraction                  RÉALISÉ / VALIDÉ
TD05.4           RuntimeActor re-audit                                RÉALISÉ
TD05.5           Feedback UI characterization                         PROCHAIN
```

---

# 6. Règles autoritaires de réduction de dette

1. Pas de refactor massif.
2. Caractériser avant extraction/changement.
3. Une seule autorité d’état.
4. Préserver les API Blueprint sauf migration explicitement auditée.
5. Toute évolution SaveGame inclut compatibilité/migration et tests.
6. Event -> Command reste un bus unique.
7. Un sous-jalon = un commit logique autant que possible.
8. Après caractérisation séparée, production + adaptation du test = même commit logique.
9. Validation UE réelle dès qu’un contrat moteur/asset/binding est impliqué.
10. Une extraction doit réduire un risque ou clarifier une frontière, pas seulement déplacer des lignes.
11. Helpers locaux de nouveaux `.cpp` nommés de façon Unity-safe.
12. Stop condition : si le changement ajoute de la complexité sans supprimer un risque observé, arrêter.
13. `Scripts/FormatCpp.ps1` formate tout le périmètre first-party : pour une micro-extraction, vérifier le diff et restaurer toute modification historique hors périmètre avant commit.

---

# 7. Contrat de validation actuel

```text
Editor/C++ : Scripts/ValidateUE.ps1 + Automation ciblée
UI/assets  : Automation disponible + PIE ciblé
Save       : capture/restore + migration + PIE lorsque nécessaire
Shipping   : Scripts/ValidatePackage.ps1
CI distante: non autoritaire tant qu’aucun runner UE5.5.4 réel n’est provisionné
```

---

# 8. Références courantes

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
docs/Architecture/TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md
docs/Architecture/ARCHITECTURE_INDEX.md
docs/Architecture/SAVE_PERSISTENCE_FOUNDATION.md
docs/Architecture/UI_GAME_FLOW_FOUNDATION.md
docs/Architecture/COMBAT_MONSTER_AI_FOUNDATION.md
docs/Architecture/TEST_AUTOMATION_FOUNDATION.md
docs/Design/UI_ARCHITECTURE_CURRENT.md
docs/Design/TD04_2_LOCAL_UE_VALIDATION_HARNESS.md
docs/Design/TD04_3_COOK_PACKAGE_VALIDATION.md
docs/Design/TD05_4_RUNTIMEACTOR_REAUDIT.md
```

`ARCHITECTURE_CONSISTENCY_AUDIT.md`, `Maps/GRIMROCK_PROJECT_MAP.md` et les documents MON/TD/STYLE datés restent des snapshots historiques jusqu’à rafraîchissement explicite.

---

# 9. Prochain travail recommandé

```text
TD05.5 — Feedback UI characterization
```

Objectif : figer le contrat public et les comportements sûrs de Readable/Interaction/Combat feedback avant extraction vers `GridLevelRuntimeActorFeedbackUI.cpp`. MON21.2 peut reprendre après la tranche TD05 jugée utile ; il n’est plus bloqué par TD04.
