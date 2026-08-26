# GrimrockPrototype — Registre autoritaire de dette technique

Date de référence : **26 août 2026**  
Baseline fonctionnelle validée : `2e5c00f64265603033d86b36869e84b1b8311179` — TD05.8 RuntimeActor Monsters extrait et validé sous UE5.5.4  
Baseline documentaire : **TD05.9 — stop condition RuntimeActor**  
Statut : **ACTIF — AUCUN REFACTOR ARCHITECTURAL CIBLÉ IMMÉDIAT**

Ce document est la source autoritaire pour la dette technique du projet. Les documents de jalon datés restent valides pour leur époque ; l’état courant, les priorités et la prochaine tranche de dette sont définis ici.

TD04 a atteint sa stop condition locale. TD05 a atteint sa stop condition RuntimeActor. Le développement fonctionnel peut reprendre ; une dette surveillée ne doit être réouverte que lorsqu’une douleur concrète apparaît.

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

Aucune dette P2 n’est actuellement en priorité architecturale immédiate. Les entrées P2 restent surveillées, opportunistes ou différées jusqu’à apparition d’un risque observable.

---

# 3. Registre actif

## TD-ARCH-001 — `AGridLevelRuntimeActor` historiquement trop centralisé

**Priorité : P2 — surveillée / stop condition atteinte**

### Baseline TD05.1

```text
GridLevelRuntimeActor.cpp
    3 359 lignes
    107 095 octets (~104,6 KiB)

GridLevelRuntimeActor.h
    22 161 octets
```

### Frontières dédiées

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

### Mesures TD05

```text
TD05.1 baseline             3 359 lignes
TD05.3 après Diagnostics    2 951 lignes
TD05.6 après Feedback UI    2 768 lignes
TD05.8 après Monsters      ~1 882 lignes
```

Réduction du fichier principal pendant TD05 : environ **1 477 lignes / 44 %**.

TD05.8 a retiré 886 lignes du fichier principal et créé `GridLevelRuntimeActorMonsters.cpp` (~958 lignes). Le commit de production ne modifie pas le header public.

### Validation

Diagnostics :

```text
Grimrock.TechnicalDebt.TD05_2.RuntimeActorDiagnostics.Contract
    1 Success / 0 Failed / 0 warning
```

Feedback UI :

```text
Grimrock.TechnicalDebt.TD05_5.RuntimeActorFeedbackUI.Contract
    1 Success / 0 Failed / 0 warning
```

Monster :

```text
Grimrock.Monsters.MON13
    13 Success
    4 Success with warnings
    0 Failed
    0 Not run
```

La suite MON13 a été verte avant et après TD05.8.

### Décision TD05.9

`AGridLevelRuntimeActor` reste volontairement la façade/orchestrateur du niveau. Le corps principal conserve essentiellement :

- lifecycle de niveau ;
- geometry/rebuild ;
- requêtes de grille ;
- placement/runtime objects génériques ;
- portes/interactions via composants spécialisés ;
- transitions de dungeon ;
- orchestration transverse `RebuildRuntimeObjects()`.

Les splits Geometry, Doors ou Generic Objects sont techniquement possibles, mais aucun risque observé ne les justifie aujourd’hui. Les réaliser uniquement pour réduire le nombre de lignes contreviendrait à la règle de stop condition.

**Décision : arrêter ici la décomposition de `AGridLevelRuntimeActor`.**

Réouvrir uniquement en présence d’une douleur concrète : duplication d’état, difficulté de test, régression récurrente, dépendance de compilation problématique ou nouvelle responsabilité métier importante.

Référence : `docs/Design/TD05_9_RUNTIMEACTOR_STOP_CONDITION.md`.

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

Le harness a été réutilisé avec succès jusqu’à TD05.8.

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

## TD-STYLE-001 — Formatting tooling — RÉSOLU POUR LE CONTRAT D’OUTIL

- `.clang-format`, `.editorconfig`, `.gitattributes` ;
- `Scripts/FormatCpp.ps1` ;
- `Scripts/CheckCppFormat.ps1` ;
- clang-format 19.1.5 figé.

**Note de suivi :** le contrôle global a révélé le 26 août 2026 une dérive historique de formatage dans du code first-party, notamment `GridPartyInventoryComponentDiagnostics.cpp`. Cette anomalie est distincte de TD05 et doit être auditée séparément avant de considérer la baseline de format globale comme entièrement verte.

---

# 5. Campagne réalisée et suite

```text
TD01.1–TD01.4   persistance / notification / Event->Command / logs    RÉALISÉ
TD02.1–TD02.9   extractions structurelles ciblées                     RÉALISÉ
TD03.1–TD03.4   Grid Editor Details cleanup                           RÉALISÉ
TD04.1–TD04.3   validation Editor/Automation/Shipping                 RÉALISÉ
TD04.4           CI UE réelle                                         DIFFÉRÉ
TD05.1           doc debt audit / RuntimeActor re-baseline            RÉALISÉ
TD05.2           RuntimeActor Diagnostics characterization            VALIDÉ
TD05.3           RuntimeActor Diagnostics extraction                  VALIDÉ
TD05.4           RuntimeActor re-audit                                RÉALISÉ
TD05.5           Feedback UI characterization                         VALIDÉ
TD05.6           RuntimeActor Feedback UI extraction                  VALIDÉ
TD05.7           RuntimeActor post-feedback re-audit                  RÉALISÉ
TD05.8           RuntimeActor Monster extraction                      VALIDÉ
TD05.9           RuntimeActor final stop condition                    ATTEINTE
```

**TD05 est clos.** Aucun split Geometry/Doors/Generic Objects n’est recommandé sans nouveau signal concret.

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
Format     : clang-format 19.1.5 ; baseline first-party globale à réauditer séparément
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
docs/Design/TD05_9_RUNTIMEACTOR_STOP_CONDITION.md
```

`ARCHITECTURE_CONSISTENCY_AUDIT.md`, `Maps/GRIMROCK_PROJECT_MAP.md` et les documents MON/TD/STYLE datés restent des snapshots historiques jusqu’à rafraîchissement explicite.

---

# 9. Prochain travail recommandé

La campagne de décomposition RuntimeActor est terminée.

Deux suites sont légitimes et non concurrentes :

1. reprendre le développement fonctionnel avec **MON21.2**, qui n’est plus bloqué par TD04/TD05 ;
2. traiter séparément la dérive historique `clang-format` si l’on veut rendre `CheckCppFormat.ps1` globalement vert avant la prochaine campagne de code.

Ne pas rouvrir TD05 uniquement parce que `GridLevelRuntimeActor.cpp` reste un fichier important.
