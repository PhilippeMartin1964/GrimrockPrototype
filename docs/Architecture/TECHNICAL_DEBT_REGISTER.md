# GrimrockPrototype — Registre autoritaire de dette technique

Date de référence : **26 août 2026**  
Baseline fonctionnelle validée : `4722e3d3d77d32a9722aa075dfce2f00823a8d35` — TD04.3 Shipping cook/package harness validé sous UE5.5.4  
Statut : **ACTIF — DETTES SURVEILLÉES / STOP CONDITION TD04 ATTEINTE**

Ce document est la source autoritaire pour la dette technique du projet. Les documents historiques de jalon restent valides pour leur époque, mais l'état courant, les priorités et le prochain travail sont définis ici.

La campagne locale TD04 a atteint sa stop condition. MON21.2 n'est plus bloqué par TD04 et peut reprendre lorsque la roadmap produit est relancée.

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
P2 : 8 dettes actives ou surveillées
P3 : 2 dettes actives
```

Aucune dette P2 restante ne justifie actuellement un refactor transversal immédiat. Elles doivent être traitées à nouveau lorsqu'une douleur concrète, une régression ou un coût de maintenance observable réapparaît.

---

# 3. Registre actif

## TD-ARCH-001 — `AGridLevelRuntimeActor` trop centralisé

**Priorité : P2 — maintenabilité structurelle**

L'acteur reste la façade/orchestrateur du niveau et expose plusieurs domaines. La stratégie reste ciblée : extraire uniquement une frontière déjà cohérente lorsqu'une douleur concrète le justifie.

Sous-jalons réalisés :

```text
TD-ARCH-001.1 — Persistence   RÉALISÉ
TD-ARCH-001.2 — World Items   RÉALISÉ
```

Fichiers dédiés :

```text
GridLevelRuntimeActorPersistence.cpp
GridLevelRuntimeActorWorldItems.cpp
```

API publique et autorité conservées ; aucune extraction supplémentaire n'est prévue par simple critère de taille.

**État : actif mais non prioritaire.**

---

## TD-ARCH-002 — `UGridPartyInventoryComponent` très volumineux

**Priorité : P2 — maintenabilité structurelle**

Le composant reste l'unique autorité d'état du groupe/inventaire.

Sous-jalons réalisés :

```text
TD-ARCH-002.1 — Equipment World Transfer   RÉALISÉ
TD-ARCH-002.2 — Inventory Diagnostics      RÉALISÉ
```

Les extractions futures doivent rester stateless ou transactionnelles et ne jamais fragmenter l'autorité.

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

Unités dédiées :

```text
GrimrockPartyPawnInputBuffer.cpp
GrimrockPartyPawnHeldItem.cpp
GrimrockPartyPawnSave.cpp
GrimrockPartyPawnItemTransfer.cpp
GrimrockPartyPawnUI.cpp
GrimrockPartyPawnMovement.cpp
```

### TD02.8 — PartyPawn UI Façade — RÉALISÉ / VALIDÉ UE5.5.4

```text
Grimrock.TechnicalDebt.TD02_8.PartyUIFacade.ModalBlockingContract  Success
```

Commits :

```text
a1c25a031cb288565205b424cd2dbb05be198cf3  Characterize TD02.8 party UI facade
b9d3c189731f935cc185c0d83f9e2f32f04a4b39  Extract TD02.8 party UI facade
```

### TD02.9 — Party Movement & Rotation — RÉALISÉ / VALIDÉ UE5.5.4

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

Décision autoritaire : **arrêter ici la décomposition de `AGrimrockPartyPawn`.**

**État : actif comme dette surveillée, sans tranche immédiate.**

---

## TD-ARCH-004 — `AGrimrockPlayerController` volumineux

**Priorité : P2 — dette surveillée**

L'audit de frontière TD02.9 confirme actuellement :

```text
Pawn       -> état logique groupe, mouvement, façades transactions, UI propre au Pawn
Controller -> pointeur/curseur, intention clic, hover, ciblage souris, délégation
```

Aucune duplication d'autorité justifiant un refactor immédiat n'a été identifiée.

Décision : **ne pas découper le PlayerController isolément**.

---

## TD-ARCH-005 — `UGridActivationComponent` concentré

**Priorité : P2 — maintenabilité / Event -> Command**

Le bus Event -> Command unique reste la bonne architecture. La dette porte sur l'organisation interne, pas sur le concept.

TD-EVENT-001 est résolu : Gameplay / StateOnly / Unsupported sont explicites et protégés par tests.

Décision : extractions internes seulement si une douleur concrète le justifie, sans créer de second bus ni de second état.

---

## TD-EDITOR-001 — Complexité Slate / Grid Editor concentrée

**Priorité : P2 — maintenabilité éditeur**

La concentration Slate reste surveillée. La duplication réelle entre Grid Editor Mode canonique et anciens boutons `CallInEditor` des Details a été traitée par TD03.

### TD03.1 — Canonical Actions / legacy Details — RÉALISÉ / VALIDÉ

```text
Grimrock.TechnicalDebt.TD03_1.EditorDetailsRedundancy.CanonicalActionsContract  Success
```

`SetStartFromSelection()` et `ValidateCurrentLevel()` restent `BlueprintCallable` mais ne sont plus exposés comme actions Details redondantes.

### TD03.2 — Move To Current Cell — RÉALISÉ / VALIDÉ

```text
Grimrock.TechnicalDebt.TD03_2.ObjectInspectorDetails.MoveToCurrentCellContract  Success
```

`MoveSelectedObjectToCurrentSelection()` conserve son API ; le bouton Slate reste canonique.

### TD03.3 — Definition Sync — RÉALISÉ / VALIDÉ

```text
Grimrock.TechnicalDebt.TD03_3.ObjectInspectorDetails.DefinitionSyncContract  Success
```

Les synchronisations Item/Monster restent `BlueprintCallable`, sans actions Details historiques redondantes.

### TD03.4 — Dungeon Level Apply — RÉALISÉ / VALIDÉ

```text
Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract  Success
```

`ApplyCurrentDungeonLevel()` reste réfléchi/BlueprintCallable et fonctionnel ; l'action Details redondante a été retirée.

### Stop condition TD03

La duplication `CallInEditor` **prouvée** par un chemin Slate canonique a été traitée. Les actions avancées/debug restantes n'ont pas d'équivalence suffisamment prouvée pour être supprimées.

Décision : **pas de TD03.5 cosmétique.**

**État : actif comme dette Slate surveillée, sans tranche immédiate.**

---

## TD-LOG-001 — Taxonomie de logs encore partiellement basée sur `LogTemp`

**Priorité : P2 — diagnostic / exploitation**

TD01.4 a stabilisé Door, Thrown Item, PIE playtest, Startup Mode et Game Instance.

Décision : migration opportuniste par domaine réellement touché ; aucun remplacement global de `LogTemp`.

---

## TD-TOOL-001 — Validation CI / Shipping non autoritaire

**Priorité : P2 — tooling/process / dette différée**

### TD04.1 — CI / Shipping Validation Contract Audit — RÉALISÉ

Baseline TD04.1 :

```text
.github/                        absent
.github/workflows/              absent
GitHub commit statuses          aucun check UE autoritaire
Scripts/CheckCppFormat.ps1      présent
Scripts/FormatCpp.ps1           présent
harness UBT/Automation/RunUAT   absent à la baseline
```

Conclusion : un push GitHub seul ne prouve ni compilation, ni Automation, ni Shipping.

Référence :

```text
docs/Design/TD04_1_CI_SHIPPING_VALIDATION_CONTRACT_AUDIT.md
```

### TD04.2 — Local UE Validation Harness — RÉALISÉ / VALIDÉ UE5.5.4

`Scripts/ValidateUE.ps1` automatise :

```text
GrimrockPrototypeEditor Win64 Development build
+ filtre Automation explicite via UnrealEditor-Cmd.exe
+ export/lecture index.json
+ échec si 0 test exécuté ou failed > 0
```

Validation réelle du 26 août 2026 :

```text
Filter                 : Grimrock.TechnicalDebt.TD03_4.DungeonLevelsDetails.ApplyCurrentLevelContract
Succeeded              : 1
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Report                 : Saved\Automation\TD04\TD04-20260826-133532
[OK] Automation filter validated.
TD04.2 validation completed successfully.
```

Commits :

```text
2245dc187d981be2187948911f6354efb0f1e80b  Add TD04.2 local UE validation harness
c196cafc1f4ef2018571f3f95e83f58898f914db  Validate TD04.2 local UE harness
```

Référence :

```text
docs/Design/TD04_2_LOCAL_UE_VALIDATION_HARNESS.md
```

### TD04.3 — Cook / Package Validation — RÉALISÉ / VALIDÉ UE5.5.4 SHIPPING

`Scripts/ValidatePackage.ps1` automatise :

```text
RunUAT BuildCookRun
Target        : GrimrockPrototype
Platform      : Win64
Configuration : Shipping
Build + Cook + Stage + Package + Pak + Archive
```

Validation réelle du 26 août 2026 :

```text
Executable    : Saved\Packaging\TD04\TD04-Shipping-20260826-141330\Windows\GrimrockPrototype.exe
Pak files     : 1
Archive files : 41
Archive bytes : 905582948
[OK] Cook / package validated.
TD04.3 validation completed successfully.
```

Commit d'implémentation :

```text
4722e3d3d77d32a9722aa075dfce2f00823a8d35  Add TD04.3 cook package validation harness
```

Référence :

```text
docs/Design/TD04_3_COOK_PACKAGE_VALIDATION.md
```

### TD04.4 — CI UE réelle — CONDITIONNEL / DIFFÉRÉ

Une vraie CI UE n'est autoritaire que si un runner provisionné dispose réellement de :

```text
Windows
UE5.5.4
prérequis Visual Studio/SDK correspondants
contenu du projet
disponibilité disque/temps suffisante
capacité à exécuter Scripts/ValidateUE.ps1
capacité à exécuter Scripts/ValidatePackage.ps1
```

Aucun runner UE5.5.4 provisionné et vérifiable n'est actuellement établi par le dépôt ou le workflow du projet. Créer un YAML sans runner réel produirait une pseudo-CI ou un workflow bloqué, pas une validation.

Décision : **ne pas créer TD04.4 tant que cette condition externe n'est pas satisfaite.** Lorsqu'un runner réel sera disponible, le workflow devra réutiliser les harness locaux plutôt que dupliquer les commandes UE.

### Stop condition TD04

Le projet possède maintenant deux contrats reproductibles et réellement validés :

```text
Editor build + Automation  -> Scripts/ValidateUE.ps1
Win64 Shipping package     -> Scripts/ValidatePackage.ps1
```

TD04.4 dépend d'une infrastructure externe non disponible dans l'état vérifiable actuel.

**Décision : stop condition TD04 atteinte. TD-TOOL-001 reste une dette différée uniquement pour la CI distante.**

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
- baseline clang-format 19.1.5.

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
TD04.2 — Local UE Validation Harness                   RÉALISÉ / VALIDÉ
TD04.3 — Cook / Package validation                     RÉALISÉ / VALIDÉ SHIPPING
TD04.4 — CI UE réelle                                  CONDITIONNEL / DIFFÉRÉ
TD04   — stop condition locale                         ATTEINTE
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
8. Après caractérisation séparée, le changement de production et l'adaptation du test correspondant appartiennent au même commit logique.
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
Scripts/ValidateUE.ps1
+ Automation EditorContext ciblée
+ ouverture/workflow Grid Editor Mode si présentation/authoring touché
```

### UI / assets

```text
Automation read-model/transaction si disponible
PIE des bindings UMG/assets réellement touchés
```

### Shipping

```text
Scripts/ValidatePackage.ps1
Win64 Shipping
Build + Cook + Stage + Package + Pak + Archive
exécutable + .pak + archive non vide obligatoires
```

### CI distante

```text
non autoritaire tant qu'aucun runner UE5.5.4 réel n'est provisionné
```

Voir :

```text
docs/Design/TD04_1_CI_SHIPPING_VALIDATION_CONTRACT_AUDIT.md
docs/Design/TD04_2_LOCAL_UE_VALIDATION_HARNESS.md
docs/Design/TD04_3_COOK_PACKAGE_VALIDATION.md
```

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
docs/Design/TD04_2_LOCAL_UE_VALIDATION_HARNESS.md
docs/Design/TD04_3_COOK_PACKAGE_VALIDATION.md
```

Les documents historiques MON/TD antérieurs ne sont pas réécrits pour simuler l'état courant ; ils restent des archives de jalon.

---

# 9. Prochain travail recommandé

La campagne immédiate de réduction de dette atteint sa stop condition locale après TD04.3.

```text
TD04.4 — différé jusqu'à disponibilité d'un vrai runner UE5.5.4
```

Il n'est pas rentable de poursuivre les dettes P2 surveillées par des refactors cosmétiques. La roadmap produit peut reprendre ; **MON21.2 peut être réactivé** lorsqu'on décide de poursuivre les fonctionnalités prévues.

Les harness TD04 doivent désormais être réutilisés pour les validations futures :

```text
Scripts/ValidateUE.ps1       -> Editor build + Automation
Scripts/ValidatePackage.ps1  -> Win64 Shipping cook/package
```
