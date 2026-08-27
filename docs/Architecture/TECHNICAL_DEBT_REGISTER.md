# GrimrockPrototype — Registre autoritaire de dette technique

Date de référence : **27 août 2026**  
Baseline GitHub auditée pour TD06.1 : `51f9e300cfcc1039412bc8951ac7d64cdece73f0`  
Baseline RuntimeActor : **TD05.9 — stop condition atteinte**  
Baseline PartyInventory : **TD06.9 — stop condition atteinte et validée**  
Statut : **TD07 FUTURE-PROOFING ACTIF — TD07.3.4 AUTHORING IDENTITY CHARACTERIZATION ACTIVE**

Ce document est la source autoritaire pour la dette technique du projet. Les documents de jalon datés restent valides pour leur époque ; l’état courant, les priorités et la prochaine tranche de dette sont définis ici.

TD04 a atteint sa stop condition locale. TD05 et TD06 ont atteint leurs stop conditions respectives. La revue post-TD06 du 27 août 2026 ouvre TD07 sur les risques de future-proofing : reproductibilité des dépendances, dépréciations UE, dette de migration Save, concentration d’ActivationComponent et infrastructures de test suspendues.

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

État au 27 août 2026, après ouverture du Prototype Data Model Reset :

```text
P0 : aucun blocage connu
P1 : 1 dette active — TD-DATA-001
P2 : 9 dettes actives, surveillées ou différées
P3 : 2 dettes actives
```

TD05 et TD06 restent clos. TD07 traite uniquement des risques concrets identifiés après leur clôture ; il ne réouvre pas leurs refactors sans nouveau signal.

---

# 3. Registre actif


## TD-BUILD-001 — Dépendance Meshy / clone non reproductible

**Priorité : RÉSOLUE — TD07.1 VALIDÉ**

### Constat

Avant TD07.1 :

```text
GrimrockPrototype.uproject
    meshy Enabled=true

.gitignore
    /Plugins/

master
    aucun Plugins/meshy versionné
```

La machine de développement compilait uniquement parce qu'une copie locale de Meshy existait. Cette copie produisait en plus plusieurs warnings de chemins Editor UE inexistants.

Aucune dépendance C++, module ou API first-party à Meshy n'a été trouvée dans le repository.

### Décision TD07.1

Meshy est un **outil local optionnel de production d'assets**, pas une dépendance du jeu.

Le contrat versionné devient :

```text
meshy
    Enabled=false
    Optional=true
```

`.gitignore` ignore uniquement `/Plugins/meshy/`, et non plus tout `/Plugins/`.

Un futur développeur peut installer Meshy localement sous `Plugins/meshy/`, l'activer ponctuellement, produire/importer ses assets puis restaurer le `.uproject` avant commit.

### Contrôle

```text
Scripts/CheckProjectDependencies.ps1
docs/Design/DEVELOPMENT_ENVIRONMENT_SETUP.md
docs/Design/TD07_1_BUILD_DEPENDENCY_REPRODUCIBILITY.md
```

Validation TD07.1 du 27 août 2026 :

```text
Dependency check     [OK]
TD06_8 Automation    1 Success / 0 warning / 0 Failed
Win64 Shipping       BUILD SUCCESSFUL
Meshy Build.cs       aucun warning / plugin non chargé
```

**TD-BUILD-001 est RÉSOLU.**

---

## TD-BUILD-002 — Toolchain Visual Studio / MSVC non figée

**Priorité : P2 — surveillée**

En août 2026, UBT utilise avec succès :

```text
Visual Studio 2022
MSVC 14.44.35227
Windows SDK 10.0.26100.0
```

mais UE5.5.4 avertit que `14.44.35227` n'est pas sa version préférée et cite `14.38.33130`.

Décision TD07.1 : ne pas forcer un downgrade ou un pinning tant que les harness Editor et Shipping restent verts. Documenter la baseline et réouvrir uniquement si :

- un nouvel environnement ne compile plus ;
- UBT transforme le warning en incompatibilité ;
- deux toolchains produisent des résultats divergents ;
- la future CI exige un environnement strictement reproductible.

Référence : `docs/Design/DEVELOPMENT_ENVIRONMENT_SETUP.md`.

---

## TD-COMPAT-001 — API Skeleton Editor dépréciée

**Priorité : RÉSOLUE — TD07.2 VALIDÉ**

Deux tests MON17 utilisaient encore `USkeleton::IsCompatible()`, signalé C4996 par UE5.5.4 avec demande explicite de migration vers `IsCompatibleForEditor()`.

TD07.2 remplace uniquement ces deux appels dans les tests `EditorContext`. Aucun runtime monster n'est modifié.

Validation du 27 août 2026 :

```text
TD07_2  1 Success / 0 warning / 0 Failed
MON17.2 2 Success / 0 warning / 0 Failed
MON17.8 8 Success / 0 warning / 0 Failed
```

**TD-COMPAT-001 est RÉSOLU.**

---

## TD-COMPAT-002 — Collision Python ItemTransfer

**Priorité : RÉSOLUE — TD07.2 VALIDÉ**

Le cook Shipping du 27 août 2026 expose un conflit de nom Python entre :

```text
EGridItemTransferResult
FGridItemTransferResult
```

TD07.2 conserve les noms C++/Blueprint et donne au `UENUM` :

```text
ScriptName = GridItemTransferResultCode
```

Validation Shipping du 27 août 2026 :

```text
package validated
GridItemTransferResult search in AutomationTool logs -> no match
```

**TD-COMPAT-002 est RÉSOLU.**

---

## TD-DATA-001 — Compatibilité historique et modèles legacy conservés pendant le prototype

**Priorité : P1 — ACTIVE / TD07.3**

### Constat

Le projet contient encore plusieurs générations de schémas simultanées :

```text
SaveGame v1-v9 + migrations
champs DeprecatedProperty
flags / enums legacy
Asset + Id représentant la même définition
fallbacks ancien -> nouveau système
snapshots runtime/save dupliqués
données calculables persistées
```

Cette complexité n'apporte aucune valeur tant que GrimrockPrototype reste un prototype jetable.

### Décision autoritaire du 27 août 2026

**Aucune compatibilité arrière n'est requise pendant la phase prototype.**

Git conserve l'historique. Une ancienne SaveGame, map, Blueprint ou DataAsset incompatible peut être supprimée ou recréée.

Le projet ne recommencera à garantir des migrations et une compatibilité durable qu'après validation définitive du prototype et entrée en phase de développement produit.

### Cible

```text
Authoring     : références de définitions, une seule autorité
Runtime/Save  : identités stables + état réellement mutable
Derived       : recalculé, jamais conservé pour compatibilité
Schema change : ancien état rejeté, aucune migration arrière
```

TD07.3 porte le nettoyage complet correspondant.

Baseline TD07.3.1 validée le 27 août 2026 :

```text
86 DataAssets scannés
41 findings

Conflict           2
DuplicateAuthority 23
LegacyField        11
LegacyOnly         3
SchemaRename       2
```

Les deux conflits réels sont des incohérences Shuriken/Stone. Les autres findings caractérisent les suppressions de schéma à venir.

Référence : `docs/Design/TD07_3_1_PROTOTYPE_DATA_MODEL_POLICY_AND_ASSET_AUDIT.md`.

TD07.3.2 est validé le 27 août 2026 : SaveGame v10 exact-match, migrations historiques supprimées, validations ciblées vertes et Shipping Win64 validé. Le correctif de validation `25e59f4a516dbc7dfde043c0a0dc0d0c66113c29` a remplacé une assertion TD01.1 encore figée sur v9 par `CurrentSaveVersion`.

---

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

`AGridLevelRuntimeActor` reste volontairement la façade/orchestrateur du niveau. Le corps principal conserve essentiellement : lifecycle de niveau, geometry/rebuild, requêtes de grille, placement/runtime objects génériques, portes/interactions via composants spécialisés, transitions de dungeon et orchestration transverse `RebuildRuntimeObjects()`.

Les splits Geometry, Doors ou Generic Objects sont techniquement possibles, mais aucun risque observé ne les justifie aujourd’hui.

**Décision : arrêter ici la décomposition de `AGridLevelRuntimeActor`.**

Réouvrir uniquement en présence d’une douleur concrète : duplication d’état, difficulté de test, régression récurrente, dépendance de compilation problématique ou nouvelle responsabilité métier importante.

Référence : `docs/Design/TD05_9_RUNTIMEACTOR_STOP_CONDITION.md`.

---

## TD-ARCH-002 — `UGridPartyInventoryComponent` historiquement très volumineux

**Priorité : P2 — surveillée / stop condition atteinte**

### Baseline TD06.1

```text
GridPartyInventoryComponent.cpp            2 337 lignes
GridPartyInventoryComponent.h                293 lignes
GridPartyInventoryComponentWorldTransfer.cpp ~104 lignes
GridPartyInventoryComponentDiagnostics.cpp    465 lignes
```

Le composant reste l'unique autorité d'état du groupe/inventaire via `FGridPartyInventoryState`. TD06 n'a créé ni second composant propriétaire, ni copie autoritaire de l'inventaire ; les extractions répartissent l'implémentation de la même classe entre plusieurs `.cpp`.

### Tranches TD06

```text
TD06.2 / TD06.3   Hotbar characterization + extraction
TD06.4 / TD06.5   Cursor Transfer characterization + extraction
TD06.6 / TD06.7   Equipment Core characterization + extraction
TD06.8            Registry / Rehydration audit + characterization
TD06.9            final cleanup / Diagnostics relocation / stop condition
```

### Distribution après TD06.9

```text
GridPartyInventoryComponent.cpp                 1 228 lignes
GridPartyInventoryComponentHotbar.cpp             237 lignes
GridPartyInventoryComponentCursorTransfer.cpp     513 lignes
GridPartyInventoryComponentEquipment.cpp          397 lignes
GridPartyInventoryComponentWorldTransfer.cpp      105 lignes
GridPartyInventoryComponentDiagnostics.cpp        488 lignes
GridPartyInventoryComponent.h                     294 lignes
```

Le fichier principal diminue de **1 109 lignes / ~47,5 %** depuis la re-baseline TD06.1.

### Cœur restant

```text
Party lifecycle / reset / restore
character creation / selection / summary
inventory add / stack / remove / count / consume
Item Definition registry / rehydration / instance application
weight recalculation
ownership validation
default initialization / hotbar restore-sanitation-validation
equipment-count synchronization
```

Ces responsabilités constituent le cœur d'autorité/orchestration de `FGridPartyInventoryState`.

### Registry / Rehydration

TD06.8 protège explicitement : atomicité du rehydrate, liste des sources possédées, déduplication, remplacement du registre transient après succès et exclusion des bindings Spell/Ability.

**Décision : conserver Registry/Rehydration dans le cœur.** Son extraction isolée ne supprimerait aujourd'hui aucun risque observé.

### Nettoyage TD06.9

TD06.9 supprime huit helpers Diagnostics morts confirmés et déplace `GetEquipmentDiagnosticsForCharacter()` dans `GridPartyInventoryComponentDiagnostics.cpp`, où son contrat TD02.3 existe déjà.

### Stop condition

**Décision : arrêter ici la décomposition de `UGridPartyInventoryComponent`.**

Réouvrir uniquement si une douleur concrète apparaît : duplication d'autorité, régression récurrente, difficulté de test, dépendance de compilation problématique ou nouvelle responsabilité autonome importante.

Référence : `docs/Design/TD06_9_PARTY_INVENTORY_STOP_CONDITION.md
docs/Design/TD07_3_1_PROTOTYPE_DATA_MODEL_POLICY_AND_ASSET_AUDIT.md
docs/Design/TD07_1_BUILD_DEPENDENCY_REPRODUCIBILITY.md
docs/Design/DEVELOPMENT_ENVIRONMENT_SETUP.md`.

Validation finale TD06.9 :

```text
Grimrock.TechnicalDebt.TD06_8   1 Success / 0 warning / 0 Failed
Grimrock.TechnicalDebt.TD02_3   1 Success / 0 warning / 0 Failed
Grimrock.CharacterCreation.CC5  2 Success / 0 warning / 0 Failed
```
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

- `bCanRemoveItem` est capturé/restauré dans le schéma courant ;
- la migration historique v8 -> v9 a été supprimée par TD07.3.2 ;
- les tests round-trip courants restent l'autorité.

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

**Note de suivi :** le contrôle global a révélé le 26 août 2026 une dérive historique de formatage dans du code first-party, notamment `GridPartyInventoryComponentDiagnostics.cpp`. Cette anomalie est distincte de TD06 et doit être auditée séparément avant de considérer la baseline de format globale comme entièrement verte.

---

# 5. Campagne réalisée et suite

```text
TD01.1–TD01.4   persistance / notification / Event->Command / logs    RÉALISÉ
TD02.1–TD02.9   extractions structurelles ciblées                     RÉALISÉ
TD03.1–TD03.4   Grid Editor Details cleanup                           RÉALISÉ
TD04.1–TD04.3   validation Editor/Automation/Shipping                 RÉALISÉ
TD04.4          CI UE réelle                                          DIFFÉRÉ
TD05.1          doc debt audit / RuntimeActor re-baseline             RÉALISÉ
TD05.2          RuntimeActor Diagnostics characterization             VALIDÉ
TD05.3          RuntimeActor Diagnostics extraction                   VALIDÉ
TD05.4          RuntimeActor re-audit                                 RÉALISÉ
TD05.5          Feedback UI characterization                          VALIDÉ
TD05.6          RuntimeActor Feedback UI extraction                   VALIDÉ
TD05.7          RuntimeActor post-feedback re-audit                   RÉALISÉ
TD05.8          RuntimeActor Monster extraction                       VALIDÉ
TD05.9          RuntimeActor final stop condition                     ATTEINTE
TD06.1          PartyInventory re-baseline / documentation audit      RÉALISÉ
TD06.2          PartyInventory Hotbar characterization                VALIDÉ
TD06.3          PartyInventory Hotbar extraction                      VALIDÉ
TD06.4          PartyInventory Cursor Transfer characterization       VALIDÉ
TD06.5          PartyInventory Cursor Transfer extraction             VALIDÉ
TD06.6          PartyInventory Equipment Core characterization        VALIDÉ
TD06.7          PartyInventory Equipment Core extraction              VALIDÉ
TD06.8          Item Definition Registry / Rehydration audit          VALIDÉ
TD06.9          PartyInventory final re-audit / stop condition        VALIDÉ — ATTEINTE
TD07.1          Build / dependency reproducibility                     VALIDÉ — TD-BUILD-001 RÉSOLU
TD07.2          UE deprecation cleanup / compiler warning audit        VALIDÉ
TD07.3          Prototype Data Model Reset                             ACTIF
TD07.3.1        Policy + Current Schema Asset Audit                    VALIDÉ
TD07.3.2        SaveGame Reset / no backward migration                VALIDÉ
TD07.3.3        Character State Normalization                         ACTIF
TD07.3.3.1      Character State Authority Audit                       VALIDÉ
TD07.3.3.2      Remove Legacy Attribute Bridge                        VALIDÉ
TD07.3.3.3      Normalize Derived Stats / Mutable Resources            VALIDÉ
TD07.3.3.4      Normalize Weight State                                 VALIDÉ
TD07.3.3.5      Normalize XP / Level / Class Progression                VALIDÉ
TD07.3.3.6      Normalize Skills                                       VALIDÉ — CLOS
TD07.3.3.7      Normalize Spellbook                                    VALIDÉ — CLOS
TD07.3.3.8      Normalize Status Effects                               VALIDÉ — CLOS
TD07.3.3.9      Normalize Level-Up Notification State                  VALIDÉ — CLOS
TD07.3.3.10     Current Save Schema / Regressions / Closure            VALIDÉ — CLOS
TD07.3.4        Authoring Identity Normalization                      CHARACTERIZATION ACTIVE
TD07.3.5        Combat Data Schema Reset                              À FAIRE
TD07.3.6        Remaining Legacy API/Data Purge                       À FAIRE
TD07.3.7        Current Asset Repair / Recreation                     À FAIRE
TD07.3.8        Strict Current-Schema Validation / stop condition     À FAIRE
TD07.4          ActivationComponent characterization                   À FAIRE
TD07.5          Suspended test infrastructure / branch recovery        À FAIRE
TD07.6          Legacy asset/API cleanup audit                          ABSORBÉ PAR TD07.3
TD07.7          Targeted log / formatting hygiene                      À FAIRE
TD07.8          Future-proofing re-audit / stop condition              À FAIRE
```

**TD05 reste clos pour `AGridLevelRuntimeActor`.** Aucun split Geometry/Doors/Generic Objects n’est recommandé sans nouveau signal concret.

TD06 est clos : la stop condition PartyInventory est validée. Aucune nouvelle tranche PartyInventory ne doit être ouverte sans signal concret ; les autres dettes du registre restent surveillées, opportunistes ou différées selon leur statut.

---

# 6. Règles autoritaires de réduction de dette

1. Pas de refactor massif.
2. Caractériser avant extraction/changement.
3. Une seule autorité d’état.
4. Pendant TD07.3, les ruptures C++ / Blueprint / DataAsset sont autorisées lorsqu'elles sont caractérisées et que le contenu courant est réparé ou recréé ; ne pas ajouter de shim de compatibilité.
5. Pendant le prototype, toute évolution SaveGame utilise un schéma courant exact-match ; aucune migration arrière n'est requise.
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
Save       : capture/restore du schéma courant + rejet strict des anciennes versions ; aucune migration prototype
Shipping   : Scripts/ValidatePackage.ps1
CI distante: non autoritaire tant qu’aucun runner UE5.5.4 réel n’est provisionné
Format     : clang-format 19.1.5 ; baseline first-party globale à réauditer séparément
```

TD06.1 ne modifie aucun contrat C++ ni asset : sa validation est documentaire/statique. À partir de TD06.2, la caractérisation puis toute extraction C++ doivent passer le harness UE et les Automations ciblées.

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
docs/Design/TD06_1_PARTY_INVENTORY_REBASELINE.md
docs/Design/TD06_8_PARTY_INVENTORY_ITEM_DEFINITION_REGISTRY_AUDIT.md
docs/Design/TD06_9_PARTY_INVENTORY_STOP_CONDITION.md
```

`TECHNICAL_DEBT_DOCUMENTATION_AUDIT.md` reste le snapshot historique de TD05.1. `ARCHITECTURE_CONSISTENCY_AUDIT.md`, `Maps/GRIMROCK_PROJECT_MAP.md` et les documents MON/TD/STYLE datés restent des snapshots historiques jusqu’à rafraîchissement explicite.

---

# 9. Prochain travail recommandé

**Valider TD07.3.3.9 — Normalize Level-Up Notification State.**

Gate local validé le 27 août 2026 :

```text
Grimrock.TechnicalDebt.TD07_3_3_9.Normalization
4 Success / 0 warning / 0 Failed / 0 Not run
Report : TD04-20260827-224647
```

Nouveau contrat :

```text
FGridCharacterInventoryState::LastAcknowledgedLevel
    autorité durable unique

LastAcknowledgedLevel < Level
    Level-Up non acquitté
```

Supprimés :

```text
FRPGPendingLevelUpSaveState
PendingLevelUpNotifications
PersistentNotificationMirror
PendingPersistentRestoreStates
CapturePersistentState / RestorePersistentState
restore retries
```

Le subsystem conserve uniquement sa queue transient et la reconstruit depuis le PartyInventory.

```text
CurrentSaveVersion = 20
v18 et antérieures -> rejet sans migration
```

Filtres prioritaires :

```text
Grimrock.TechnicalDebt.TD07_3_3_9.Normalization
Grimrock.TechnicalDebt.TD07_3_3_9.Characterization
Grimrock.TechnicalDebt.TD07_3_2.SaveContract
Grimrock.RPG.MON15.5
```

Référence :

```text
docs/Design/TD07_3_3_9_LEVEL_UP_NOTIFICATION_STATE_NORMALIZATION.md
```


TD07.3.3.10 gates locaux validés le 27 août 2026 :

```text
Normalization     4/4 — TD04-20260827-231740
Characterization  4/4 — TD04-20260827-231753
Warnings            0
Failures            0
```

Reste : campagne de régression finale TD07.3.3 puis Win64 Shipping.


## TD07.3.3 — clôture Character State Normalization

TD07.3.3 clôturé le 27 août 2026.

```text
SaveGame schema         v20 exact-match
Character-state tests   71/71
Campagne finale        314/314
Warnings                  0
Failures                  0
Shipping Win64            OK
```

Référence Shipping : `TD04-Shipping-20260827-232723`.

Résultat architectural :

```text
Durable character authority
    Attributes
    Experience
    Resources
    SelectedClassProgressionChoiceIds
    SkillRanks
    KnownSpellIds
    StatusEffects
    LastAcknowledgedLevel
    inventory / hotbar / identity

Transient / reconstructed
    Level
    DerivedStats
    StatusEffect DefinitionAsset
    runtime projections / read models

Removed parallel Save snapshots
    ClassProgressionStates
    CharacterSkillStates
    CharacterSpellbookStates
    CharacterStatusEffectStates
    PendingLevelUpNotifications
```

Prochaine tranche : `TD07.3.4 — Authoring Identity Normalization`.
