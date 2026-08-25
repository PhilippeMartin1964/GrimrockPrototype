# GrimrockPrototype — Registre autoritaire de dette technique

Date de référence : **25 août 2026**  
Baseline fonctionnelle validée : `d3bc5f0e8a8c1b3526506cace457982c94ae3a07` — post-TD02.1 World Items extraction
Statut : **ACTIF — PHASE EXPLOITATION / STABILISATION**

Ce document est la source autoritaire pour la dette technique du projet. Les rubriques `Dette`, `Dette technique`, `Risques` ou `Points futurs` présentes dans d'autres documents restent utiles comme contexte historique ou local, mais doivent être interprétées à travers le présent registre.

MON21.2 reste suspendu. La réduction de dette est compatible avec la phase actuelle uniquement lorsqu'elle corrige un risque concret, clarifie un contrat existant ou réduit une concentration devenue coûteuse à maintenir.

---

## 1. Ce que nous appelons dette technique

Une entrée est une dette technique lorsqu'un choix historique, un contrat incomplet ou une concentration de responsabilités augmente aujourd'hui au moins un des risques suivants :

- régression fonctionnelle ;
- incohérence Save / Continue ;
- comportement implicite ou non testable ;
- difficulté à modifier une zone sans effet de bord ;
- duplication d'autorité ;
- diagnostic ou maintenance inutilement difficiles ;
- dépendance manuelle évitable dans le workflow de validation.

Ne sont **pas** considérés comme dette technique :

- petit bestiaire ou catalogue de sorts encore limité ;
- icônes, sons, VFX ou contenu de production manquants ;
- Quests / Journal / Map / Codex non encore implémentés ;
- Recipes et autres fonctionnalités de roadmap ;
- futur éditeur joueur / publication ;
- nécessité de valider les `.uasset/.umap` dans Unreal : c'est une contrainte normale du moteur, à couvrir par le processus et les tests.

---

## 2. Niveaux de priorité

```text
P0 — bloque le projet / corruption / perte de données connue
P1 — risque fonctionnel, persistance ou contrat incohérent à traiter tôt
P2 — maintenabilité / architecture / tooling à réduire de manière ciblée
P3 — nettoyage opportuniste, nommage ou intégration non bloquante
```

État au 25 août 2026 :

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

Indicateur de départ :

```text
GridLevelRuntimeActor.cpp ≈ 139 KB post-STYLE01 (142 626 octets)
```

Le header expose encore de nombreux domaines : géométrie, reconstruction, portes, items, lancers, interaction, monstres, persistance, transitions, feedback UI, preview éditeur, diagnostics, etc.

Des extractions existent déjà (`UGridDoorSystemComponent`, `UGridActivationComponent`, `UGridMonsterEncounterComponent`, `UGridEditorPreviewComponent`, présentation d'attaque), donc la bonne stratégie est de **poursuivre les frontières déjà prouvées**, pas de réécrire l'acteur.

### TD-ARCH-001.1 — extraction de la persistance — RÉALISÉ

Le 25 août 2026, la douleur concrète rencontrée pendant TD01.1 a justifié une première extraction ciblée :

- `CaptureCurrentLevelRuntimeState()` et `ApplyCurrentLevelRuntimeState()` ont été déplacées dans `GridLevelRuntimeActorPersistence.cpp` ;
- 551 lignes ont été retirées de `GridLevelRuntimeActor.cpp` ;
- aucune nouvelle classe, aucun `.inl` et aucune modification d'API publique ;
- `AGridLevelRuntimeActor` reste la façade/orchestrateur ;
- le hotfix `18fa0da7...` a rendu les helpers locaux compatibles avec les Unity Builds Unreal ;
- build UE5.5.4 et Automation TD01.1 / MON20.9 validés après extraction.

### TD-ARCH-001.2 — extraction des World Items — RÉALISÉ

Le 25 août 2026, TD02.1 a isolé une seconde frontière déjà cohérente et testable : les opérations de pickup, drop, lancer, résolution impact -> cellule et poids des objets monde.

Le contrat a d'abord été caractérisé **avant extraction** par :

```text
Grimrock.TechnicalDebt.TD02_1.WorldItemsContract  Success
```

Puis :

- les méthodes `CanPartyPickupItemEntry()`, `CanPartyPickupItemActor()`, `TryPickupItemAtCell()`, `TryPickupItemActor()`, `TryDropItemInstanceAtCell()`, `TrySpawnThrownItemProjectile()`, `SpawnThrownItemProjectile()`, `TryResolveWorldCellFromImpactPoint()` et `GetWorldItemWeightAtCell()` ont été déplacées vers `GridLevelRuntimeActorWorldItems.cpp` ;
- 460 lignes ont été retirées de `GridLevelRuntimeActor.cpp` ;
- aucune nouvelle classe propriétaire, aucun nouvel état, aucun changement de header public et aucun changement SaveGame ;
- `AGridLevelRuntimeActor` reste l'unique façade et autorité ;
- les helpers locaux du nouveau fichier ont été rendus compatibles avec les Unity Builds Unreal ;
- aucun `.uasset/.umap` n'a été modifié.

Validation UE5.5.4 après extraction :

```text
Grimrock.TechnicalDebt.TD02_1.WorldItemsContract                  Success
Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle       Success
Grimrock.Monsters.MON11.Presentation.PlacedItemRebuildUniqueness Success
```

Les tests couvrent notamment le RuntimeObjectId, le poids de stack, la distinction centre/bord pour les plaques de pression, les règles de pickup selon cellule/facing, le transfert vers l'inventaire, la disparition du poids monde après pickup, le lancer -> impact -> world drop et l'unicité lors d'un rebuild.

Commits associés :

```text
c08f85723e96c67d73a8dad7e6478a7a303d72f7  Characterize TD02.1 world item contract
ad8b1e30ee19141bd576d5f118a7ad722344176b  Extract TD02.1 world item implementation
246bae8d29e034e88aeaafdbe657bf6ce12feecf  Remove moved TD02.1 world item implementation
d3bc5f0e8a8c1b3526506cace457982c94ae3a07  Make TD02.1 world item helpers Unity-safe
```

Ces deux réalisations réduisent une concentration réelle, mais **ne clôturent pas TD-ARCH-001** : d'autres responsabilités restent regroupées dans l'acteur. Les extractions futures ne seront engagées que lorsqu'une douleur concrète les justifiera.

Traitement recommandé :

- caractériser une responsabilité avant extraction ;
- préserver l'API publique quand elle est utilisée par Blueprint/tests ;
- déplacer une responsabilité par sous-jalon ;
- garder `AGridLevelRuntimeActor` comme façade/orchestrateur de niveau ;
- choisir les extractions à partir des douleurs rencontrées en playtest, pas à partir du seul nombre de lignes.

Le volume est un signal de risque, pas une justification suffisante pour un refactor massif.

---

## TD-ARCH-002 — `UGridPartyInventoryComponent` très volumineux

**Priorité : P2 — maintenabilité structurelle**

Indicateur actuel :

```text
GridPartyInventoryComponent.cpp ≈ 93 KB post-STYLE01 (95 417 octets)
```

Le composant est à juste titre l'autorité du groupe/inventaire, mais concentre :

- création/restauration du groupe ;
- inventaires et stacks ;
- équipement ;
- curseur ;
- hotbar ;
- définitions d'items ;
- diagnostics ;
- calculs de bonus/résistances ;
- sélection du personnage.

Une petite extraction `GridPartyInventoryComponentVisuals.cpp` existe déjà.

Traitement recommandé : conserver **une seule autorité d'état** et extraire seulement des services stateless ou helpers de transaction/diagnostic. Ne pas découper l'état en plusieurs composants propriétaires.

TD-PARTY-001 est désormais résolu ; la frontière de notification sélection / présentation est stabilisée avant toute décomposition plus large.

---

## TD-ARCH-003 — `AGrimrockPartyPawn` trop chargé

**Priorité : P2 — maintenabilité structurelle**

Indicateur actuel :

```text
GrimrockPartyPawn.cpp ≈ 76 KB post-STYLE01 (77 540 octets)
+ fichiers dédiés Recruitment / CustomRecruit déjà extraits
```

Responsabilités encore mélangées :

- mouvement case par case et rotation ;
- input et buffer ;
- caméra/head bob/free look ;
- interaction monde ;
- held item / lancer ;
- UI principale ;
- Save / Continue ;
- orchestration combat ;
- création de personnage.

La présence des fichiers `GrimrockPartyPawnRecruitment.cpp` et `GrimrockPartyPawnCustomRecruit.cpp` montre que l'extraction incrémentale est déjà viable.

Traitement recommandé : extraire uniquement une responsabilité stable lorsqu'elle doit être modifiée. Les candidats naturels sont présentation held item, save/load façade, ou interaction/input, mais l'ordre doit être dicté par les problèmes réels rencontrés.

---

## TD-ARCH-004 — `AGrimrockPlayerController` reste volumineux

**Priorité : P2 — maintenabilité structurelle**

Indicateur actuel :

```text
GrimrockPlayerController.cpp ≈ 49 KB post-STYLE01 (50 463 octets)
```

Le contrôleur porte encore une part importante de souris, interaction, curseur et coordination UI/runtime.

Traitement recommandé : ne pas le découper isolément. Auditer d'abord la frontière Pawn / Controller / Inventory / Interaction afin d'éviter de déplacer la complexité sans réduire les dépendances.

---

## TD-ARCH-005 — `UGridActivationComponent` devient un second point de concentration

**Priorité : P2 — maintenabilité / Event -> Command**

Indicateur de référence :

```text
GridActivationComponent.cpp ≈ 48 KB post-STYLE01 (49 646 octets)
```

Il regroupe aujourd'hui interaction, triggers, pressure plates, link dispatch, conditions, Logic, Lua, recrutement et command routing.

MON19 a volontairement conservé un bus unique, ce qui est correct. La dette porte donc sur l'organisation interne, pas sur le concept Event -> Command.

TD-EVENT-001 est désormais fermé : la sémantique des commandes supportées n'est plus un préalable bloquant à une future décomposition interne.

Traitement recommandé : seulement lorsqu'une douleur concrète le justifie, envisager des services internes stateless pour évaluation de conditions, dispatch ou résolution de commandes. Ne jamais créer un second bus.

---

## TD-EDITOR-001 — Complexité Slate / Grid Editor concentrée

**Priorité : P2 — maintenabilité éditeur**

Indicateurs actuels :

```text
SGridEditorObjectInspectorPanel.cpp ≈ 88 KB post-STYLE01 (89 765 octets)
SGridEditorLinksPanel.cpp           ≈ 54 KB post-STYLE01 (55 200 octets)
SGridEditorLuaScriptsPanel.cpp      ≈ 41 KB post-STYLE01 (41 770 octets)
GridLevelEdModeToolkit.cpp          ≈ 39 KB post-STYLE01 (40 003 octets)
GridEditorLuaService.cpp            ≈ 29 KB post-STYLE01 (29 473 octets)
GridLevelEdMode.cpp                 ≈ 27 KB post-STYLE01 (27 380 octets)
```

`GRID_EDITOR_ACTOR_UI_AUDIT.md` identifie en parallèle de nombreux contrôles `CallInEditor` historiques qui doublonnent désormais le workflow du Grid Editor Mode.

Traitement recommandé :

- finir de rendre le Grid Editor Mode canonique pour l'usage quotidien ;
- cacher/supprimer des Details uniquement après couverture équivalente ;
- extraire view-models/helpers Slate lorsque plusieurs panneaux répètent les mêmes règles ;
- conserver les diagnostics de récupération nécessaires ;
- ne pas refondre l'éditeur pendant une correction gameplay sans rapport.

---

## TD-LOG-001 — Taxonomie de logs encore largement basée sur `LogTemp`

**Priorité : P2 — diagnostic / exploitation**

De nombreux fichiers runtime/UI/editor utilisent encore `UE_LOG(LogTemp, ...)`, alors que plusieurs domaines récents ont déjà leurs catégories (`LogGridReceptacle`, recrutement, monstres, etc.).

Risque : filtrage difficile pendant les longues sessions PIE et distinction moins nette entre diagnostic attendu, warning de contrat et erreur réelle.

Traitement recommandé : migration incrémentale par domaine touché, avec catégories stables ; ne pas faire un commit géant de remplacement global.

### TD01.4 — tranche de stabilisation réalisée

Le 25 août 2026, TD01.4 a établi la taxonomie incrémentale sur cinq domaines runtime réellement exercés, sans remplacement transversal :

- `UGridDoorSystemComponent` -> `LogGridDoorSystem` ;
- `AGridThrownItemActor` -> `LogGridThrownItem` ;
- `GridPIEPlaytestRequest` -> `LogGridPIEPlaytest` ;
- `UGrimrockStartupModeComponent` -> `LogGrimrockStartupMode` ;
- `UGrimrockGameInstance` -> `LogGrimrockGameInstance`.

La passe a aussi corrigé une ambiguïté de sévérité : les rejets de `SlotProbe` utilisés pour sonder les sauvegardes du menu sont désormais `Verbose`, tandis qu'une vraie requête utilisateur `LoadSlot Request Failed` reste `Warning`.

Validation UE5.5.4 / PIE :

```text
Grimrock.MON19.8.ProductionPuzzles                        4/4 Success
Grimrock.Monsters.MON11.Presentation.ThrownWeaponLifecycle Success
PIE Grid Editor Playtest                                  Begin/Clear sous LogGridPIEPlaytest
New Game                                                  démarrage complet sous LogGrimrockStartupMode
Load / Continue                                           chargement complet sous LogGrimrockStartupMode / LogGrimrockGameInstance
```

Commits associés :

```text
ad0d05b87388a7b957b676cea0dda38a40b9fff1  Declare door runtime log category
0906f4993a13b0865bf0b86d89d6e38da0c778b3  Use door runtime log category
0c7c859c31da0c97633b17b9c3a14fdd5b27b2a5  Declare thrown item runtime log category
80856449fe0e2e07a9efbd0fd5bfe7c999c6c5a9  Use thrown item runtime log category
71621c2ece1d6aa1c6932c59a347ea826e401a5e  Use PIE playtest runtime log category
5bb73c63d8e8286e114cf6a12e7770ea9adf6fd0  Use startup mode runtime log category
a154839e6118be4a8dd472f0cd1223ddf519389f  Use game instance runtime log category
d308df500907950288d800d61c23db25b3d35672  Demote save slot probe diagnostics
```

TD01.4 est **terminé comme tranche de stabilisation**. TD-LOG-001 reste volontairement actif en P2 : les futurs `LogTemp` seront migrés opportunistement lorsqu'un domaine est touché ou qu'un diagnostic ambigu produit un coût réel. Ce statut évite de transformer l'hygiène de logs en refactor cosmétique sans fin.

---

## TD-TOOL-001 — Validation CI/Shipping non autoritaire

**Priorité : P2 — tooling/process**

Le dépôt courant ne présente pas de `.github/workflows` exploitable sur `master`. La compilation UE5.5.4 et les Automation/PIE restent donc principalement validées sur la machine de développement.

Ce n'est pas une faiblesse de l'architecture gameplay, mais cela augmente le coût de détection des régressions et la dépendance à une validation manuelle.

Traitement recommandé, lorsque l'environnement de build UE est disponible :

- build Development Editor Win64 ;
- exécution d'un socle Automation stable ;
- éventuellement packaging/commandlet validation ;
- garder PIE manuel pour les assets/présentations qui l'exigent réellement.

---

## TD-UI-001 — Nommage historique `Inventory` pour le menu global

**Priorité : P3 — nettoyage opportuniste**

Exemples :

```text
EInventoryTopTab
ToggleInventoryWidget()
```

Le menu est désormais un shell RPG multipage. Le nom est donc historiquement trompeur, mais le contrat fonctionne et les valeurs sérialisées/API Blueprint peuvent être sensibles.

Décision : **ne pas lancer de renommage transversal dédié**. Traiter uniquement si une modification future de cette API justifie une migration contrôlée.

---

## TD-RPG-001 — Lancer manuel non encore relié aux Skills

**Priorité : P3 — intégration fonctionnelle**

`AGrimrockPartyPawn` contient encore :

```text
TODO: Scale throw speed, accuracy and damage with the selected character's ranged/throwing skill.
```

MON20 fournit maintenant le socle Skills, mais aucun contrat de design n'impose encore quelle compétence gouverne quelle partie du lancer.

Ce point doit être traité comme une intégration gameplay/balance à définir, pas comme une correction structurelle urgente.

---

# 4. Éléments reclassés — ne plus les compter comme dette technique

## Contenu de production

```text
bestiaire limité
catalogue de sorts / statuts à densifier
icônes finales
sons / VFX / animations supplémentaires
salles et puzzles de production
```

Ce sont des besoins de contenu. Le document `COMBAT_MONSTER_AI_FOUNDATION.md` a raison sur le fond : il faut davantage exploiter l'architecture, pas construire un nouveau framework d'IA.

## Fonctionnalités de roadmap

```text
Quests / Journal / Map / Codex
Recipes
éditeur joueur
publication / partage
campagne complète
```

Ce sont des fonctionnalités futures, actuellement suspendues ou planifiées. Leur absence n'est pas une dette technique.

## Validation des assets Unreal

Les fichiers `.uasset/.umap` ne sont pas pleinement auditables comme du texte hors Unreal. Cela impose une stratégie Automation + PIE, mais ce n'est pas une dette du code métier.

---

# 5. Dettes documentaires identifiées pendant l'audit

Plusieurs documents « courants » avaient dérivé par rapport au code :

- SaveGame encore indiqué v7 alors que MON20.9 a introduit v8 ;
- persistance Spellbook encore indiquée « à faire MON18.8 » alors qu'elle est livrée ;
- Skills encore présentés comme futurs dans certains résumés UI ;
- MON20.4 encore annoncé comme prochain dans des fondations datant du 23 août ;
- contenu futur mélangé à la dette technique.

Les documents historiques de jalon ne doivent pas être réécrits : ils décrivent correctement leur époque. En revanche, les documents portant explicitement `CURRENT`, `FOUNDATION`, `INDEX`, `PROJECT_SYNTHESIS` ou carte autoritaire doivent pointer vers l'état courant ou vers ce registre.

La présente passe corrige les fondations les plus directement concernées et fait de ce registre la référence unique pour la dette.

---

# 6. Ordre recommandé pour attaquer la dette

La réduction de dette ne doit pas commencer par les plus gros fichiers. Elle doit commencer par les contrats qui peuvent produire une incohérence de jeu.

```text
TD01.1 — Receptacle Removal Permission Persistence      [RÉSOLU]
         -> TD-PERSIST-001

TD01.2 — Party Selection / Held Visual Notification Contract [RÉSOLU]
         -> TD-PARTY-001

TD01.3 — Event -> Command Unsupported/Fallback Contract [RÉSOLU]
         -> TD-EVENT-001

TD01.4 — Runtime Logging Categories / Diagnostic Hygiene [RÉALISÉ]
         -> TD-LOG-001 reste actif / opportuniste

TD02.1 — GridLevelRuntimeActor targeted extraction       [RÉALISÉ]
         -> TD-ARCH-001 reste actif
         -> TD-ARCH-001.1 Persistence extraction [RÉALISÉ]
         -> TD-ARCH-001.2 World Items extraction  [RÉALISÉ]

TD02.2 — PartyInventory targeted service extraction
         -> TD-ARCH-002

TD02.3 — PartyPawn / PlayerController responsibility audit + extraction
         -> TD-ARCH-003 / TD-ARCH-004

TD02.4 — ActivationComponent internal decomposition
         -> TD-ARCH-005

TD03 — Grid Editor Slate / legacy Details cleanup
       -> TD-EDITOR-001

TD04 — CI / Shipping validation
       -> TD-TOOL-001
```

`TD-UI-001` et `TD-RPG-001` restent opportunistes et ne doivent pas interrompre un chantier plus rentable.

---

# 7. Règles de réduction de dette

1. **Pas de refactor massif.** Une responsabilité stabilisée à la fois.
2. **Comportement avant structure.** Écrire/identifier les tests de caractérisation avant extraction.
3. **Une seule autorité.** Ne jamais résoudre une grosse classe en dupliquant son état dans plusieurs composants.
4. **API Blueprint prudente.** Ne pas renommer/supprimer une API exposée sans audit d'assets.
5. **SaveGame explicite.** Toute modification persistante inclut migration/compatibilité et tests Save -> Restore.
6. **Event -> Command reste unique.** Une décomposition interne ne crée pas un second bus.
7. **Un sous-jalon = un commit logique** autant que possible : code + tests + documentation.
8. **Validation UE réelle** dès qu'un asset, un binding Blueprint ou une présentation est impliqué.
9. **Mesurer le résultat.** Une extraction doit réduire un couplage ou clarifier une frontière, pas seulement déplacer des lignes.
10. **Stop condition.** Si le code devient plus complexe sans supprimer de risque observé, le refactor s'arrête.

---

# 8. Validation minimale par catégorie

### Persistance

```text
Automation capture/restore
migration/legacy si version impactée
PIE Save -> Continue si le domaine possède des assets réels
```

### Extraction structurelle

```text
mêmes tests avant/après
aucune modification de format sérialisé
aucun changement d'asset sans besoin
API publique conservée ou migration documentée
```

### Editor

```text
Automation editor-only
ouverture du Grid Editor Mode
édition / validation / PIE preparation réelles
```

### UI

```text
Automation read-model / transaction
PIE des bindings UMG touchés
```

---

# 9. Sources revues

La consolidation s'appuie notamment sur :

```text
docs/Architecture/ARCHITECTURE_CONSISTENCY_AUDIT.md
docs/Architecture/Maps/GRIMROCK_PROJECT_MAP.md
docs/Architecture/PROJECT_SYNTHESIS.md
docs/Architecture/UI_GAME_FLOW_FOUNDATION.md
docs/Architecture/COMBAT_MONSTER_AI_FOUNDATION.md
docs/Architecture/RECEPTACLE_SYSTEM_CLEANUP_NOTES.md
docs/Design/UI_ARCHITECTURE_CURRENT.md
docs/Design/UI_GRIMROCK_MENU_CURRENT.md
docs/Design/GRID_EDITOR_ACTOR_UI_AUDIT.md
docs/Design/MON21_1_QUESTS_JOURNAL_MAP_CODEX_ARCHITECTURE_AUDIT.md
```

et sur l'état courant des principaux fichiers runtime/editor.

---

# 10. Prochain travail recommandé

La prochaine tranche est :

```text
TD02.2 — PartyInventory targeted service extraction
```

Pourquoi :

- TD02.1 a prouvé que les extractions `.cpp` ciblées, précédées d'un test de caractérisation, réduisent la concentration sans créer une nouvelle autorité ;
- `UGridPartyInventoryComponent` reste le prochain point de concentration P2 dans l'ordre du registre ;
- le domaine `EquipmentWorldTransfer` est un candidat initial crédible parce que l'extraction d'une unité équipée, son rollback et la consommation associée sont déjà des transactions explicites ;
- `ThrownWeaponLifecycle` exerce déjà le chemin extraction -> rollback et extraction -> lancer ;
- la décision d'extraction reste conditionnée à un audit des helpers partagés : si la séparation nécessite de dupliquer l'autorité ou des helpers transversaux, TD02.2 devra choisir une autre frontière.

MON21.2 reste suspendu pendant cette campagne d'exploitation/stabilisation.

---

# 11. Dettes résolues

## TD-EVENT-001 — Branches Event -> Command incomplètes pour ItemSpawn / Teleporter — RÉSOLU

**Priorité historique : P1 — contrat d'authoring**  
**Résolu le : 25 août 2026**

TD01.3 a fermé le contrat `Event -> Command` sans ajouter de fonctionnalité gameplay artificielle :

- `GridEditorLinkPolicy` est désormais l'autorité utilisée par la validation éditeur pour distinguer `Gameplay`, `StateOnly` et `Unsupported` ;
- seules les commandes classées `Gameplay` sont authorables et considérées valides ;
- `Teleporter`, `Light` et `ItemSpawn` ne sont plus proposés comme cibles de commandes génériques tant qu'aucun comportement gameplay spécialisé n'existe ;
- leur classification `StateOnly` reste explicite pour diagnostiquer les données legacy ;
- `Logic`, `StoryCompanion` et `CustomRecruiter` sont correctement reconnus comme cibles gameplay par la validation ;
- `UGridActivationComponent` refuse désormais au runtime les anciennes commandes génériques `StateOnly` au lieu de modifier `ActiveObjectIds` puis de retourner un faux succès ;
- les fallbacks `state stored, spawn behavior TODO`, `state stored, teleport behavior TODO` et le succès générique sans handler gameplay ont été supprimés ;
- les commandes gameplay spécialisées Door, Receptacle, MonsterSpawn, Logic, StoryCompanion et CustomRecruiter restent routées vers leurs autorités existantes ;
- le chemin générique d'état reste réservé à Lever / PressurePlate ;
- aucun second bus Event -> Command, aucun changement SaveGame et aucun `.uasset/.umap`.

Validation UE5.5.4 du 25 août 2026 :

```text
Grimrock.TechnicalDebt.TD01_3.EventCommandContract
  Policy            Success
  Validation        Success
  RuntimeHardening  Success

Grimrock.MON19.4.LuaBridge
  5/5 Success

Grimrock.MON19.8.ProductionPuzzles
  4/4 Success

Grimrock.MON19.2.Editor.LinkPolicyMatrix
  1/1 Success
```

Le test `RuntimeHardening` vérifie aussi l'absence de mutation silencieuse : une commande rejetée ne peut ni activer une cible `StateOnly` inactive, ni désactiver un état legacy préexistant.

Commits associés :

```text
57927020ece7f8a0c2f865debdca988c1a38d70b  Enforce TD01.3 event command contract
51cd7da4e2af4052bfb38e0a9ea90382fff999d3  Add TD01.3 runtime hardening coverage
9786eaf5a66fc8d325e5b95ee26c73e2fb9adac0  Reject state-only event commands at runtime
```

---

## TD-PARTY-001 — Synchronisation sélection / held visual dépendante des appelants — RÉSOLU

**Priorité historique : P1 — cohérence runtime/UI**  
**Résolu le : 25 août 2026**

TD01.2 a fermé le contrat de notification sélection / présentation :

- `UGridPartyInventoryComponent` reste l'unique autorité de `SelectedCharacterIndex` ;
- `SetSelectedCharacterIndex()` continue d'émettre `OnPartyInventoryChanged(INDEX_NONE)` ;
- `AGrimrockPartyPawn` s'abonne au delegate autoritaire dans `PostInitializeComponents()` ;
- `HandlePartyInventoryChanged()` resynchronise le held visual pour `INDEX_NONE` ou pour le personnage actuellement sélectionné ;
- les notifications relatives à un autre personnage sont ignorées ;
- aucun second état de sélection, aucun second bus et aucun changement `.uasset/.umap` ;
- Automation `SelectionChange` et `SelectedCharacterFilter` validées sous UE5.5.4.

Le fixture Automation initialise son monde avec `UWorld::InitializeActorsForPlay(FURL())` afin d'exercer le vrai cycle de delegate dynamique Unreal.

Commits associés :

```text
a640759a9449074fb865b187e66d2ecf057bf640
ed36777d5bb3fcb06aaf7d1eb39aea4f546b067a
fa91600a81334e53449fd71d20d6d48a401a922f
275965ca55099b6582a6dd3a0cd7150d21b43aab
6d8b36ec5cdd0f98872f7503a9f525a2eeaa8707
12a93241c79078c3f8438b5e98a6f8e4c9da537c
b04697a901a61764c369aea012f10c38735af0ae
```

---

## TD-PERSIST-001 — Permission de retrait des réceptacles non persistée — RÉSOLU

**Priorité historique : P1 — correction fonctionnelle / SaveGame**  
**Résolu le : 25 août 2026**

TD01.1 a fermé le contrat de persistance de `bCanRemoveItem` :

- `FGridRuntimeReceptacleState` persiste désormais `bCanRemoveItem` ;
- `CaptureRuntimeReceptacleState()` capture la permission ;
- `ApplyCurrentLevelRuntimeState()` la restaure explicitement avant la reconstruction du contenu ;
- SaveGame passe de v8 à v9 ;
- les sauvegardes v1-v8 migrent avec la politique legacy explicite `bCanRemoveItem = true` ;
- aucun changement `.uasset/.umap` ;
- Automation `DisabledRoundTrip`, `EnabledRoundTrip` et `V8Migration` validées ;
- les 8 tests `Grimrock.MON20.9.SkillPersistence` restent verts.

Commits associés :

```text
63fd803d1411bb87487b54c00f3c12f44cb1bfb2  Extract GridLevelRuntimeActor persistence
897481d5cb6f1dd1d9eae321dfc770f6454ad0a9  Persist receptacle removal permission
18fa0da79ec052a5af54214b3bd7590cf21da0e5  Fix persistence helpers for Unity builds
```

La validation UE5.5.4 du 25 août 2026 confirme le nouveau test Save/Continue, la migration v8 -> v9 et l'absence de régression MON20.9.

---

## TD-STYLE-001 — Formatage C++ hétérogène / baseline non reproductible — RÉSOLU

**Priorité historique : P2 — maintenabilité / tooling**  
**Résolu le : 25 août 2026**

STYLE01 a fermé la dette de formatage transversal :

- contrat `.clang-format` versionné ;
- clang-format 19.1.5 figé ;
- `.editorconfig` versionné ;
- scripts `FormatCpp.ps1` / `CheckCppFormat.ps1` versionnés ;
- 486 fichiers C++ first-party validés UTF-8 ;
- 480 fichiers reformatés mécaniquement dans `3c4032be...` ;
- contrôle hors espaces identique au `HEAD` pré-formatage ;
- build UE5.5.4 validé ;
- Automation post-correction validée ;
- commit mécanique ajouté à `.git-blame-ignore-revs` ;
- `.h`, `.cpp` et `.inl` normalisés en LF par `.gitattributes`.

Les baisses de taille observées dans TD-ARCH-001..005 et TD-EDITOR-001 sont **principalement dues au compactage mécanique STYLE01**. Elles ne constituent pas une réduction de responsabilités, de couplage ou de complexité architecturale. Les priorités de ces dettes restent donc inchangées.

STYLE01 n'est plus compté comme dette active. Les compteurs P1/P2/P3 du registre actif restent inchangés parce que TD-STYLE-001 n'avait pas été ajouté comme entrée active lors de l'audit initial : il a été traité comme préalable immédiat à TD01.

La référence détaillée est :

```text
docs/Design/STYLE01_CPP_FORMATTING_BASELINE.md
```

Le prochain travail recommandé est désormais :

```text
TD02.2 — PartyInventory targeted service extraction
```
