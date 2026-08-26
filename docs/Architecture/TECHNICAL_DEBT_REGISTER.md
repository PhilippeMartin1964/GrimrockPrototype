# GrimrockPrototype — Registre autoritaire de dette technique

Date de référence : **26 août 2026**  
Baseline fonctionnelle validée : `fbab179a7366cce9322b39fb4f70eabb5d618dc8` — post-TD03.4 Grid Editor Details cleanup  
Statut : **ACTIF — PHASE EXPLOITATION / STABILISATION**

Ce document est la source autoritaire pour la dette technique du projet. Les documents historiques de jalon restent valides pour leur époque, mais l'état courant, les priorités et le prochain travail sont définis ici.

MON21.2 reste suspendu pendant cette campagne de stabilisation.

---

## 1. Définition

Une entrée est une dette technique lorsqu'un choix historique, un contrat incomplet ou une concentration de responsabilités augmente aujourd'hui au moins un des risques suivants :

- régression fonctionnelle ;
- incohérence Save / Continue ;
- comportement implicite ou non testable ;
- difficulté à modifier une zone sans effet de bord ;
- duplication d'autorité ;
- diagnostic ou maintenance inutilement difficiles ;
- dépendance manuelle évitable dans le workflow de validation.

Ne sont pas comptés comme dette technique : contenu de production manquant, bestiaire/sorts limités, Quests/Journal/Map/Codex, Recipes, futur éditeur joueur, publication ou campagne complète.

La nécessité de vérifier certains `.uasset/.umap` dans Unreal est une contrainte normale du moteur ; la dette apparaît seulement lorsque le processus de validation est inutilement fragile ou non reproductible.

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
P2 : 8 dettes actives
P3 : 2 dettes actives
```

---

# 3. Registre actif

## TD-ARCH-001 — `AGridLevelRuntimeActor` trop centralisé

**Priorité : P2 — maintenabilité structurelle**

L'acteur reste la façade/orchestrateur du niveau et expose encore plusieurs domaines. La stratégie reste ciblée : extraire uniquement une frontière déjà cohérente lorsqu'une douleur concrète le justifie.

Sous-jalons réalisés :

```text
TD-ARCH-001.1 — Persistence                 RÉALISÉ
TD-ARCH-001.2 — World Items                 RÉALISÉ
```

Réalisations notables :

- `GridLevelRuntimeActorPersistence.cpp` ;
- `GridLevelRuntimeActorWorldItems.cpp` ;
- API publique et autorité conservées ;
- SaveGame inchangé par les extractions structurelles ;
- helpers locaux rendus Unity-safe ;
- Automation de caractérisation repassée après extraction.

**État : actif mais non prioritaire immédiatement.** Aucun refactor massif n'est autorisé par le seul volume du fichier.

---

## TD-ARCH-002 — `UGridPartyInventoryComponent` très volumineux

**Priorité : P2 — maintenabilité structurelle**

Le composant reste l'unique autorité d'état du groupe/inventaire. Les extractions doivent rester stateless ou transactionnelles et ne jamais fragmenter l'autorité.

Sous-jalons réalisés :

```text
TD-ARCH-002.1 — Equipment World Transfer    RÉALISÉ
TD-ARCH-002.2 — Inventory Diagnostics       RÉALISÉ
```

Fichiers dédiés :

```text
GridPartyInventoryComponentWorldTransfer.cpp
GridPartyInventoryComponentVisuals.cpp
```

Les diagnostics ont également été isolés dans une unité dédiée.

**État : actif, à traiter uniquement lorsqu'un coût concret réapparaît.**

---

## TD-ARCH-003 — `AGrimrockPartyPawn` historiquement trop chargé

**Priorité : P2 — dette surveillée / stop condition atteinte**

La campagne TD02 a réduit la concentration sans créer de nouvelle autorité.

Réalisé :

```text
TD02.4 — Input Buffer
TD02.5 — Held Item Presentation
TD02.6 — Save / Load Façade
TD02.7 — Item Transfer Façade
TD02.8 — UI Façade
TD02.9 — Movement & Rotation Façade
```

Unités dédiées actuelles :

```text
GrimrockPartyPawnInputBuffer.cpp
GrimrockPartyPawnHeldItem.cpp
GrimrockPartyPawnSave.cpp
GrimrockPartyPawnItemTransfer.cpp
GrimrockPartyPawnUI.cpp
GrimrockPartyPawnMovement.cpp
```

### TD02.8 — PartyPawn UI Façade — RÉALISÉ / VALIDÉ UE5.5.4

Caractérisation :

```text
Grimrock.TechnicalDebt.TD02_8.PartyUIFacade.ModalBlockingContract  Success
```

Commits :

```text
a1c25a031cb288565205b424cd2dbb05be198cf3  Characterize TD02.8 party UI facade
b9d3c189731f935cc185c0d83f9e2f32f04a4b39  Extract TD02.8 party UI facade
```

La création initiale de personnage a également été repassée après extraction : création valide et rejets atomiques restent verts.

### TD02.9 — Party Movement & Rotation — RÉALISÉ / VALIDÉ UE5.5.4

Caractérisation et régression :

```text
Grimrock.TechnicalDebt.TD02_9.PartyMovementFacade.GridStartContract  Success
Grimrock.Monsters.MON12.PartyMobility.Lifecycle                       Success
```

Commits :

```text
d3e7400f494713555ef2748912cce0b5422bed89  Characterize TD02.9 party movement facade
af3cc82f4ac3156a820d21911be565c04a3bce14  Extract TD02.9 party movement facade
3c91656b5996aad6d40719cc510aaf0126c01071  Document TD02.9 closure and PartyPawn stop condition
```

Décision autoritaire : **arrêter ici la décomposition de `AGrimrockPartyPawn`.** Les responsabilités restantes appartiennent naturellement au Pawn ou sont de petites façades. Une nouvelle extraction devra être motivée par une douleur concrète, pas par le nombre de lignes.

**État : actif comme dette surveillée, sans tranche immédiate.**

---

## TD-ARCH-004 — `AGrimrockPlayerController` volumineux

**Priorité : P2 — dette surveillée**

L'audit de frontière effectué à la clôture TD02.9 confirme actuellement une séparation cohérente :

```text
Pawn       -> état logique groupe, mouvement, façades transactions, UI propre au Pawn
Controller -> pointeur/curseur, intention clic, hover, ciblage souris, délégation
```

Aucune duplication d'autorité justifiant un refactor immédiat n'a été identifiée.

Décision : **ne pas découper le PlayerController isolément**. Un split d'unités de traduction serait aujourd'hui principalement cosmétique.

---

## TD-ARCH-005 — `UGridActivationComponent` concentré

**Priorité : P2 — maintenabilité / Event -> Command**

Le bus Event -> Command unique reste la bonne architecture. La dette porte sur l'organisation interne, pas sur le concept.

TD-EVENT-001 est résolu : la sémantique des cibles Gameplay / StateOnly / Unsupported est maintenant explicite et protégée par tests.

Décision : extractions internes seulement si une douleur concrète le justifie, sans créer de second bus ni de second état.

---

## TD-EDITOR-001 — Complexité Slate / Grid Editor concentrée

**Priorité : P2 — maintenabilité éditeur**

Les gros panneaux Slate restent des zones de concentration. En revanche, la tranche de duplication réelle entre Grid Editor Mode canonique et anciens boutons `CallInEditor` des Details a été traitée par TD03.

### TD03.1 — Canonical Actions / legacy Details — RÉALISÉ / VALIDÉ UE5.5.4

Contrat :

```text
Grimrock.TechnicalDebt.TD03_1.EditorDetailsRedundancy.CanonicalActionsContract  Success
```

Nettoyage : `SetStartFromSelection()` et `ValidateCurrentLevel()` restent `BlueprintCallable`, mais ne sont plus exposés comme actions `CallInEditor` redondantes.

Commits principaux :

```text
40b9a7927c3a5d4e86f91e419ca255115ba2dfed  Characterize TD03.1 editor Details redundancy
4fb419b4d43ca53fb5e61be08f08747c1aada87b  Remove TD03.1 redundant Details actions
760f11e64588c5c30f74c27473d8067858693c57  Fix TD03.1 CallInEditor metadata assertion
```

### TD03.2 — Object Inspector Move To Current Cell — RÉALISÉ / VALIDÉ UE5.5.4

Contrat :

```text
Grimrock.TechnicalDebt.TD03_2.ObjectInspectorDetails.MoveToCurrentCellContract  Success
```

Le bouton Slate `Move To Current Cell` reste canonique ; `MoveSelectedObjectToCurrentSelection()` conserve son API mais perd uniquement son exposition `CallInEditor` redondante.

Commits :

```text
fed38eda9deda62afcb66a6220f5340e1ef97d94  Characterize TD03.2 object inspector Details redundancy
2895309fa44dd20a7ab065e7a23549ea4238e722  Clean TD03.2 object inspector Details action
```

### TD03.3 — Definition Sync Details Actions — RÉALISÉ / VALIDÉ UE5.5.4

Contrat :

```text
Grimrock.TechnicalDebt.TD03_3.ObjectInspectorDetails.DefinitionSyncContract  Success
```

Les synchronisations Item/Monster restent `BlueprintCallable` et fonctionnelles via le workflow canonique, sans boutons Details historiques redondants.

Commits :

```text
101763657f5897447da29308b4add8a5f89a4781  Characterize TD03.3 definition sync Details redundancy
15e2f0e6b93d328d34e28d889f634674a51a8599  Clean TD03.3 definition sync Details actions
```

### TD03.4 — Dungeon Level Apply Details Action — RÉALISÉ / VALIDÉ UE5.5.4

Contrat :

```text
Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract  Success
```

`ApplyCurrentDungeonLevel()` reste réfléchi/BlueprintCallable et conserve son comportement de sélection niveau par défaut/explicite, mais l'action Details historique redondante a été nettoyée.

Commits :

```text
bf3093d1a5741bfff50ffae87c21ac4803f985c5  Characterize TD03.4 dungeon level Details redundancy
fbab179a7366cce9322b39fb4f70eabb5d618dc8  Clean TD03.4 dungeon level Details action
```

### Stop condition TD03

La duplication `CallInEditor` **prouvée** par un chemin Slate canonique a été traitée. Les actions avancées/debug restantes ne doivent pas être supprimées sans preuve d'équivalence.

Décision : **pas de TD03.5 cosmétique.**

`TD-EDITOR-001` reste actif à cause de la concentration Slate, mais une nouvelle extraction ne sera engagée que si une douleur réelle de maintenance apparaît.

---

## TD-LOG-001 — Taxonomie de logs encore partiellement basée sur `LogTemp`

**Priorité : P2 — diagnostic / exploitation**

TD01.4 a stabilisé plusieurs domaines : Door, Thrown Item, PIE playtest, Startup Mode et Game Instance. La dette reste volontairement active et doit être réduite opportunistement par domaine touché.

Décision : aucun remplacement global de `LogTemp`.

---

## TD-TOOL-001 — Validation CI / Shipping non autoritaire

**Priorité : P2 — tooling/process**

### TD04.1 — CI / Shipping Validation Contract Audit — RÉALISÉ

Audit de la baseline `fbab179a...` :

```text
.github/                        absent
.github/workflows/              absent
GitHub commit statuses HEAD     aucun
Scripts/CheckCppFormat.ps1      présent
Scripts/FormatCpp.ps1           présent
harness UBT/Automation/RunUAT   absent
```

Conclusion autoritaire :

- le dépôt automatise le formatage C++ ;
- il ne possède actuellement aucune CI GitHub UE5.5.4 ;
- un push GitHub ne prouve ni compilation ni Automation ni Shipping ;
- l'autorité actuelle reste la validation réellement exécutée sous UE5.5.4 : build, Automation et PIE ciblé.

Référence :

```text
docs/Design/TD04_1_CI_SHIPPING_VALIDATION_CONTRACT_AUDIT.md
```

**État : actif. Prochaine tranche = TD04.2 Local UE Validation Harness.**

---

## TD-UI-001 — Nommage historique `Inventory` pour le menu global

**Priorité : P3 — nettoyage opportuniste**

Exemples : `EInventoryTopTab`, `ToggleInventoryWidget()`.

Le contrat fonctionne et peut être sensible aux Blueprint/valeurs sérialisées. Aucun renommage transversal dédié.

---

## TD-RPG-001 — Lancer manuel non encore relié aux Skills

**Priorité : P3 — intégration fonctionnelle**

Le TODO de scaling du lancer existe encore dans la façade Item Transfer. Le socle Skills existe, mais le contrat de design reliant skill, vitesse, précision et dégâts n'est pas encore défini.

Traiter comme intégration gameplay/balance, pas comme correction structurelle urgente.

---

# 4. Dettes résolues

## TD-PERSIST-001 — Permission de retrait des réceptacles — RÉSOLU

- SaveGame v8 -> v9 ;
- `bCanRemoveItem` capturé/restauré ;
- migration v1-v8 explicite ;
- Automation round-trip/migration et MON20.9 validées.

Commits principaux :

```text
63fd803d1411bb87487b54c00f3c12f44cb1bfb2
897481d5cb6f1dd1d9eae321dfc770f6454ad0a9
18fa0da79ec052a5af54214b3bd7590cf21da0e5
```

## TD-PARTY-001 — Selection / Held Visual notification — RÉSOLU

- `UGridPartyInventoryComponent` reste autorité du personnage sélectionné ;
- notification via `OnPartyInventoryChanged(INDEX_NONE)` ;
- Pawn synchronise le held visual ;
- tests `SelectionChange` et `SelectedCharacterFilter` validés.

## TD-EVENT-001 — Event -> Command contract — RÉSOLU

- distinction Gameplay / StateOnly / Unsupported ;
- validation éditeur et runtime cohérentes ;
- faux succès state-only supprimés ;
- aucune seconde autorité Event -> Command.

## TD-STYLE-001 — Formatting baseline — RÉSOLU

- `.clang-format`, `.editorconfig`, `.gitattributes` ;
- `Scripts/FormatCpp.ps1` ;
- `Scripts/CheckCppFormat.ps1` ;
- baseline clang-format 19.1.5 ;
- mécanique de formatage isolée de la logique.

---

# 5. Ordre réel de la campagne

```text
TD01.1 — Receptacle persistence                         RÉSOLU
TD01.2 — Party selection / held visual                 RÉSOLU
TD01.3 — Event -> Command contract                     RÉSOLU
TD01.4 — Logging stabilization slice                   RÉALISÉ

TD02.1 — RuntimeActor World Items                      RÉALISÉ
TD02.2 — Inventory Equipment World Transfer            RÉALISÉ
TD02.3 — Inventory Diagnostics                         RÉALISÉ
TD02.4 — PartyPawn Input Buffer                        RÉALISÉ
TD02.5 — PartyPawn Held Item Presentation              RÉALISÉ
TD02.6 — PartyPawn Save / Load                         RÉALISÉ
TD02.7 — PartyPawn Item Transfer                       RÉALISÉ
TD02.8 — PartyPawn UI                                  RÉALISÉ
TD02.9 — PartyPawn Movement + stop condition           RÉALISÉ

TD03.1 — canonical editor actions Details cleanup      RÉALISÉ
TD03.2 — object move Details cleanup                   RÉALISÉ
TD03.3 — definition sync Details cleanup               RÉALISÉ
TD03.4 — dungeon level Details cleanup                 RÉALISÉ
TD03   — stop condition                                ATTEINTE

TD04.1 — CI / Shipping validation contract audit       RÉALISÉ
TD04.2 — Local UE Validation Harness                   PROCHAIN
TD04.3 — Cook / Package validation                     APRÈS TD04.2
TD04.4 — CI UE réelle                                  CONDITIONNEL
```

---

# 6. Règles autoritaires de réduction de dette

1. Pas de refactor massif.
2. Caractériser le comportement avant extraction/changement.
3. Une seule autorité d'état.
4. Préserver les API Blueprint sauf migration explicitement auditée.
5. Toute évolution SaveGame inclut compatibilité/migration et tests.
6. Event -> Command reste un bus unique.
7. Un sous-jalon = un commit logique autant que possible.
8. **Après caractérisation séparée, le changement de production et l'adaptation/renforcement du test correspondant appartiennent au même commit logique.**
9. Validation UE réelle dès qu'un asset, binding, présentation ou contrat moteur est impliqué.
10. Une extraction doit réduire un risque ou clarifier une frontière, pas seulement déplacer des lignes.
11. Helpers de namespace anonyme Unity-safe par nommage préfixé fichier/domaine.
12. Stop condition : si le changement ajoute de la complexité sans supprimer un risque observé, arrêter.

---

# 7. Contrat de validation actuel

### Persistance

```text
Automation capture/restore
migration legacy si version impactée
PIE Save -> Continue si assets réels impliqués
```

### Extraction structurelle

```text
mêmes tests avant/après
aucun changement sérialisé implicite
API publique conservée
Unity Build vérifié lorsque nouveaux .cpp/helpers locaux
```

### Editor

```text
Automation EditorContext
ouverture et workflow Grid Editor Mode si présentation/authoring touché
```

### UI / assets

```text
Automation read-model/transaction si disponible
PIE des bindings UMG/assets réellement touchés
```

### CI / Shipping

```text
Format script != build UE
Build Editor != Automation
Automation != PIE
Build Editor != Shipping
Shipping = cook/package réellement exécuté et validé
```

Voir `docs/Design/TD04_1_CI_SHIPPING_VALIDATION_CONTRACT_AUDIT.md`.

---

# 8. Documents de référence courants

```text
docs/Architecture/TECHNICAL_DEBT_REGISTER.md
docs/Architecture/ARCHITECTURE_CONSISTENCY_AUDIT.md
docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md
docs/Architecture/PROJECT_SYNTHESIS.md
docs/Design/UI_ARCHITECTURE_CURRENT.md
docs/Design/UI_GRIMROCK_MENU_CURRENT.md
docs/Design/GRID_EDITOR_ACTOR_UI_AUDIT.md
docs/Design/TD02_9_PARTY_MOVEMENT_AND_STOP_CONDITION.md
docs/Design/TD04_1_CI_SHIPPING_VALIDATION_CONTRACT_AUDIT.md
```

Les documents historiques MON/TD antérieurs ne sont pas réécrits pour simuler l'état courant ; ils restent des archives de jalon.

---

# 9. Prochain travail recommandé

```text
TD04.2 — Local UE Validation Harness
```

Objectif : versionner un script portable qui reproduit d'abord la validation locale réellement utile :

```text
- résolution paramétrable de UE5.5.4 ;
- build GrimrockPrototypeEditor Win64 Development ;
- lancement d'un filtre Automation explicite ;
- codes de sortie fiables ;
- logs/commandes visibles ;
- aucun chemin machine projet ou moteur codé en dur.
```

Ne pas créer de workflow GitHub Actions UE avant que ce harness local soit fiable et qu'un runner disposant réellement d'UE5.5.4 soit défini.

MON21.2 reste suspendu pendant TD04.
