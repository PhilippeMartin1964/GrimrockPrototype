# CPP-AUDIT01 — Audit général du code C++

Date : **27 septembre 2026**  
Moteur : **Unreal Engine 5.5.4**  
Branche : **master**  
Baseline C++ : la suite `Grimrock` a été validée localement à **962/962**, **0 warning**, **0 fail**, **0 not run** avant cet audit.  
HEAD au moment de la rédaction : `ee7cbe03fd2799965f962d2efdf03e344aa43b22`.

Les deux commits intervenus après TEST-AUDIT02 ne modifient pas `Source/` : ils concernent les assets/UI et `.gitignore`. Le périmètre C++ audité est donc identique à celui de la baseline automatisée validée.

---

## 1. Objet de l'audit

Cet audit cherche en priorité :

- les doubles autorités ;
- les fallbacks de compatibilité encore actifs ;
- le code mort ou devenu sans appelant ;
- les contrats trop permissifs ;
- les dépendances codées en dur ;
- les frontières runtime/editor/UI devenues confuses ;
- les risques de maintenance liés à la taille des classes ;
- les chargements synchrones et autres risques de hitch ;
- l'encapsulation de l'état persistant ;
- la qualité des frontières de test ;
- les fonctionnalités connues mais encore incomplètes.

L'objectif n'est **pas** de découper les gros fichiers par principe.

La règle d'architecture reste :

> un seul propriétaire d'état, des fichiers d'implémentation séparés quand cela aide, et aucune nouvelle couche si elle ne supprime ni duplication, ni couplage, ni risque.

---

## 2. Vue quantitative

Le dépôt contient environ :

```text
Source C++/headers/inl/cs                 : 717 fichiers
Production hors /Tests/                   : 433 fichiers
Tests                                     : 284 fichiers

Taille production                         : ~3.51 MB
Taille tests                              : ~2.82 MB
Ratio tests / production                  : ~80 %
```

Répartition production principale :

```text
GrimrockPrototype/Runtime                 : 177 fichiers / ~1.74 MB
GrimrockPrototypeEditor/EditorTools       :  67 fichiers / ~647 KB
GrimrockPrototype/UI                      :  58 fichiers / ~532 KB
GrimrockPrototype/RPG                     :  64 fichiers / ~262 KB
GrimrockPrototype/Core                    :  20 fichiers / ~138 KB
GrimrockPrototype/Magic                   :  23 fichiers / ~85 KB
GrimrockLua                               :   6 fichiers / ~41 KB
GrimrockPrototype/Save                    :   6 fichiers / ~24 KB
GrimrockPrototype/Quests                  :   5 fichiers / ~22 KB
```

Fichiers de production les plus volumineux observés :

```text
GridInventoryWidget.cpp                   ~2447 lignes
GridLevelRuntimeActor.cpp                 ~2035 lignes
SGridEditorObjectInspectorPanel.cpp       ~1603 lignes
GrimrockPlayerController.cpp              ~1627 lignes
GridCombatHudWidget.cpp                   ~1512 lignes
GridTurnManagerPlayerActionCatalog.cpp    ~1324 lignes
GridReceptacleActor.cpp                   ~1308 lignes
RPGCharacterCreationWizardWidget.cpp       ~929 lignes
RPGCharacterCreationWidget.cpp             ~923 lignes
```

La taille seule n'est pas retenue comme anomalie.

---

## 3. Évaluation globale

### Conclusion

L'architecture générale est **saine pour un prototype de cette taille**.

Les grands propriétaires sont maintenant correctement identifiés :

- `AGridLevelRuntimeActor` : runtime de niveau ;
- `UGridPartyInventoryComponent` : état de groupe/inventaire ;
- `AGrimrockPartyPawn` : groupe contrôlé et orchestration joueur ;
- `UGridTurnManagerComponent` : autorité combat ;
- `UGridActivationComponent` : Event -> Command / Logic / Lua ;
- `UGridQuestSubsystem` : état des quêtes ;
- `GrimrockPrototypeEditor` : édition et outils ;
- `GrimrockLua` : VM Lua.

Les gros propriétaires ont déjà été divisés en unités `.cpp` spécialisées sans créer plusieurs propriétaires concurrents. C'est préférable à une fragmentation en nombreux services artificiels.

### Points positifs confirmés

- séparation en trois modules : runtime, editor, Lua ;
- pas de dépendance du runtime vers le module Editor ;
- `bUseUnity = false`, utile pour détecter les dépendances d'include réelles ;
- pas de `TODO`, `FIXME`, `HACK` ou `WORKAROUND` résiduel détecté dans `Source/` ;
- mécanismes animés : Tick généralement désactivé au repos ;
- aucun usage de `GetAllActorsOfClass` dans la production ;
- forte couverture automatisée ;
- sauvegarde prototype en schéma strict exact-match ;
- nombreuses données de gameplay déjà portées par des DataAssets et identités stables ;
- UI combat récemment nettoyée : projection parent unique vers panneaux de personnages passifs ;
- Lua reste volontairement supporté et ne constitue pas une dette à traiter maintenant.

---

# 4. Constats prioritaires

## P1 — CPP-LEGACY-AUDIO : migration audio Door encore active

### Constat

`UGridWorldObjectDefinitionAsset` contient toujours cinq propriétés réellement dépréciées :

```text
DoorOpenSounds
DoorCloseSounds
DoorAudioVolume
DoorAudioPitchVariation
DoorAudioAttenuation
```

Elles ne sont pas seulement conservées pour réflexion :

- `PostLoad()` migre encore leurs valeurs vers `AudioEvents` ;
- `ResolveAudioEvent()` retombe encore dessus ;
- `AGridRuntimeObjectActor::ConfigureObjectAudio()` possède encore un fallback d'atténuation et reconstruit Open/Close depuis ces données ;
- `GridObjectGenericAudioTests.cpp` protège explicitement cette backward compatibility.

### Diagnostic

C'est le principal morceau de **compatibilité historique réellement vivant** encore trouvé dans le runtime.

Il est en contradiction avec la règle actuelle du prototype :

> pas de migration/backward compatibility inutile ; les assets doivent être mis au schéma courant.

### Action recommandée

Ticket dédié, avec audit d'assets avant suppression :

```text
CPP-CLEAN01 — Remove Legacy Door Audio Migration
```

Ordre :

1. auditer les `UGridWorldObjectDefinitionAsset` réels ;
2. confirmer que les portes utilisent `AudioEvents` + `DefaultAudioAttenuation` ;
3. corriger manuellement les éventuels assets restants ;
4. retirer les cinq propriétés dépréciées ;
5. retirer `PostLoad` de migration et les fallbacks runtime ;
6. remplacer le test de compatibilité par un test d'absence de legacy.

**Priorité : élevée.**

---

## P1 — CPP-ITEM-AUTHORITY : équipement permissif lorsque la définition manque

### Constat

`UGridPartyInventoryComponent::CanEquipItemToSlot()` fait :

```text
si ItemDefinition trouvée :
    Definition->CanEquipToSlot()

sinon :
    si slot supporté -> true
```

Le log annonce lui-même :

```text
GridInventory Equip Compatibility Fallback
```

Le test TD06.6 caractérise explicitement ce comportement comme :

```text
"The historical no-definition fallback accepts a valid item..."
```

### Risque

Un item dont le `ItemDefinitionId` ne peut plus être résolu peut être équipé dans n'importe quel slot supporté.

Cela contourne l'autorité data-driven de `UGridItemDefinitionAsset`.

Le système de rehydrate actuel sait déjà refuser une ItemDefinition possédée introuvable. Le fallback d'équipement est donc désormais plus permissif que le reste du pipeline.

### Action recommandée

```text
CPP-CLEAN02 — Enforce Item Definition Authority
```

Changer le contrat pour :

```text
definition introuvable -> équipement refusé
```

et supprimer le test qui protège le fallback historique.

**Priorité : élevée.**

---

## P1/P2 — CPP-ITEM-ID : doublons ItemDefinitionId silencieusement acceptés

### Constat

`RegisterItemDefinition()` :

- refuse null / `NAME_None` ;
- si l'ID existe déjà, retourne `true` ;
- conserve silencieusement le premier asset.

TD06.8 protège explicitement ce first-wins.

### Risque

Deux DataAssets distincts portant le même `ItemDefinitionId` deviennent dépendants de l'ordre de chargement/enregistrement.

Pour une architecture orientée données avec identités stables, un conflit d'identité devrait être une erreur, pas une résolution silencieuse.

### Contrat recommandé

Autoriser :

```text
même ID + même asset -> idempotent / true
```

Refuser :

```text
même ID + autre asset -> false + diagnostic explicite
```

Éventuellement ajouter un audit AssetRegistry côté Editor.

**Priorité : élevée à moyenne.**

---

# 5. Code mort / compatibilité résiduelle

## P2 — EGridReceptacleRejectReason::ExplicitlyRejected

La valeur :

```cpp
ExplicitlyRejected = 3 UMETA(Hidden)
```

est commentée :

> Legacy compatibility value. No runtime acceptance path produces it anymore.

Aucun producteur runtime n'a été trouvé. Il subsiste seulement dans des switches de présentation/log.

### Recommandation

Auditer les références Blueprint/binaires puis supprimer cette valeur et ses cases de switch.

---

## P2 — FGridMonsterPerception::CanHear()

`CanHear()` est documenté comme :

> Legacy geometric helper

Le runtime utilise `CanHearThroughGrid()`.

`CanHear()` n'est plus référencé par le runtime ; il subsiste dans le test MON4 et la documentation.

### Recommandation

Supprimer le helper et adapter le test afin qu'il protège uniquement le contrat actuel d'audition à travers la grille.

---

## P2/P3 — APIs sans appelant C++ confirmé

Les fonctions suivantes n'ont pas d'appelant C++ trouvé hors de leur propre définition :

```text
AGrimrockPartyPawn::StartNewGame()
AGrimrockPartyPawn::ShowInventoryWidget()
```

Elles sont toutefois `BlueprintCallable`.

Elles ne doivent donc **pas** être supprimées sur la seule base d'une recherche C++.

### Recommandation

Créer un petit audit de références d'assets/Blueprints. Si aucun Blueprint ne les appelle, les supprimer.

`StartNewGame()` est particulièrement suspect depuis STARTUP-FLOW01, car la nouvelle partie est maintenant orchestrée par le frontend.

---

# 6. Dépendances data-driven à durcir

## P2 — meshes de chaîne de porte codés en dur

`AGridDoorActor` charge dans son constructeur :

```text
/Game/GrimrockPrototype/Meshes/Door/SM_Door_Chain_Support_01
/Game/GrimrockPrototype/Meshes/Door/SM_Door_Chain_Moving_01
```

via `ConstructorHelpers::FObjectFinder`.

C'est le seul usage de `ConstructorHelpers::FObjectFinder` trouvé en production.

### Diagnostic

Le runtime de porte possède une dépendance directe à deux assets du projet.

Cela contourne partiellement l'architecture `UGridWorldObjectDefinitionAsset`.

### Recommandation

Déplacer ces références dans la définition de porte/chaîne ou dans une configuration explicitement data-driven.

---

## P2 — fallback WBP de création de personnage codé en dur

`UGrimrockMainMenuWidget::ResolveCharacterCreationWidgetClass()` utilise d'abord la classe configurée, puis :

```text
/Game/GrimrockPrototype/Blueprints/UI/RPG/WBP_CharacterCreationWizard
```

avec `LoadClass`.

### Diagnostic

Deux autorités de configuration :

1. `CharacterCreationWidgetClass` ;
2. chemin C++ de secours.

### Recommandation

Une fois le WBP correctement configuré dans le frontend, supprimer le chemin codé en dur et considérer l'absence de classe comme une erreur de configuration explicite.

---

# 7. Chargements synchrones

## P2 performance — 15 fichiers de production utilisent LoadSynchronous()

Les principaux chemins concernés :

- inventaire : icônes de slots ;
- items au sol : mesh et sparkle material ;
- présentation des sorts ;
- audio/VFX monstres ;
- hurt/death/idle presentation ;
- attaques joueur ;
- preview editor.

Exemples de chemins runtime sensibles :

```text
UGridInventorySlotWidget::SetItem()
UGridMonsterAudioComponent::Play...
UGridSpellPresentationComponent::PresentSpell()
UGridPlayerAttackPresentationComponent
```

### Évaluation

Ce n'est pas un bug fonctionnel et les assets, une fois chargés, resteront souvent en mémoire.

Mais le premier affichage/usage d'un item, sort ou effet peut déclencher une charge synchrone.

### Recommandation

Ne pas créer immédiatement un Asset Manager complexe.

D'abord profiler les hitches réels. Si nécessaire :

```text
CPP-PERF01 — Preload/Cached Presentation Assets
```

avec résolution au chargement du niveau / initialisation des définitions, pas au moment exact de l'attaque ou de l'ouverture d'UI.

---

# 8. Autorité d'état / encapsulation

## UGridPartyInventoryComponent

### Bon point

Il existe une seule structure de stockage :

```cpp
FGridPartyInventoryState PartyInventoryState;
```

Il n'y a pas de second inventaire concurrent.

### Risque

Cette structure reste publique en C++ et est lue/modifiée par de nombreux services :

- LevelUp ;
- progression ;
- skills ;
- spellbook ;
- status effects ;
- recruitment ;
- UI ;
- save ;
- combat.

Cela signifie que l'autorité de **stockage** est unique, mais l'autorité de **mutation** est distribuée.

### Exemple acceptable

Le Spellbook modifie `KnownSpellIds` directement, mais possède son propre `OnSpellbookChanged`.

### Risque à long terme

Une nouvelle mutation directe peut oublier :

- notification UI ;
- recompute ;
- validation d'ownership ;
- hotbar sanitization ;
- transaction atomique.

### Recommandation

Ne pas encapsuler brutalement toute la structure.

Pour les nouvelles fonctionnalités, imposer une règle :

> les mutations complexes passent par le service transactionnel du domaine ; les widgets restent lecteurs/orchestrateurs.

Le modèle actuel est suffisamment testé pour ne pas justifier une réécriture.

---

# 9. Combat

## UGridTurnManagerComponent

Le header reste important (~35 KB) et le système de combat est réparti sur plusieurs `.cpp`.

### Conclusion

**Ne pas découper le propriétaire.**

Les extractions actuelles :

```text
Actions
PlayerActions
PlayerActionCatalog
Phases
Initiative
Mobility
Targeting
...
```

réduisent déjà la taille des unités de compilation tout en gardant une autorité de combat.

### Dette réelle : couplage des tests

Le header de `UGridTurnManagerComponent` déclare environ **37 classes de test friend**.

D'autres headers de production ont aussi des friends de tests :

```text
GridActivationComponent
GrimrockPlayerController
GrimrockPartyPawn
GridMonsterIdleVariationComponent
```

### Risque

Les tests sont très complets, mais connaissent beaucoup de détails internes.

Un refactor interne exige donc souvent une modification simultanée de l'interface privée et de nombreux tests.

### Recommandation

À partir des prochains tickets combat :

- favoriser les événements et snapshots publics déjà disponibles ;
- ajouter, seulement si nécessaire, des hooks diagnostics sous `WITH_DEV_AUTOMATION_TESTS` ;
- réduire progressivement les `friend class FGrid...`.

Pas de chantier global de suppression des friends.

---

# 10. UI

## GridInventoryWidget.cpp

C'est le plus gros fichier de production (~2447 lignes).

Il gère notamment :

- sélection de personnage ;
- projection inventaire ;
- paper doll ;
- filtre/tri ;
- drag & drop ;
- actions contextuelles ;
- tooltip ;
- lecture d'objet ;
- génération et refresh de slots.

### Conclusion

Le fichier est gros, mais l'audit récent a déjà supprimé plusieurs doubles autorités.

Aucune nouvelle classe ne doit être créée uniquement pour faire baisser le nombre de lignes.

### Candidat d'extraction futur

Si ce fichier recommence à être difficile à modifier, la meilleure frontière est la **projection pure** filtre/tri/tooltip, pas un nouveau propriétaire d'inventaire.

---

## GridCombatHudWidget.cpp

Après UI-COMBAT-CLEAN01 :

- le HUD lit les autorités runtime ;
- `FGridCombatHudPartyMemberView` est construit une seule fois ;
- `UGridCombatActionPanelWidget` est passif.

Cette architecture est correcte.

### Petit couplage d'include

`GridCombatActionPanelWidget.h` inclut tout `GridCombatHudWidget.h` uniquement pour le type `FGridCombatHudPartyMemberView`.

Ce n'est pas urgent. Si le coût de compilation devient gênant, les structs de présentation combat pourraient être déplacés dans un petit header de types communs.

Ne pas créer ce fichier tant que le bénéfice n'est pas mesurable.

---

## Nommage UI restant

`EInventoryTopTab` contient maintenant :

```text
Inventory
Skills
Journal
Map
Recipes
Codex
Spellbook
```

alors que `Inventory` est un workspace séparé et que `UGrimrockMenuWidget` est le shell des autres pages.

Le nom est historique mais le fonctionnement est cohérent.

**Priorité faible**, car un rename reflété peut toucher les Blueprints.

---

# 11. Logs

## P2 — LogTemp reste très présent

La recherche globale trouve encore `LogTemp` dans environ **51 fichiers Source**.

Hotspots observés :

```text
GridInventoryWidget.cpp       ~98 appels
GridLevelRuntimeActor.cpp     ~53 appels
GrimrockPlayerController.cpp  ~13 appels
CharacterCreationWizard       plusieurs appels
PartyInventory                plusieurs appels
```

À l'inverse, certaines zones récentes utilisent déjà de vraies catégories :

```text
LogGridMonsterDeath
LogGridMonsterCombat
LogGridRecruitmentRuntime
LogGridRecruitmentWidget
LogGridTurnManagerInput
...
```

### Recommandation

```text
CPP-CLEAN04 — Logging Category Cleanup
```

mais **par domaine**, pas une substitution globale aveugle.

Priorité :

1. Inventory/UI ;
2. RuntimeActor ;
3. PlayerController ;
4. Save/Item transfer.

---

# 12. Editor

## SGridEditorObjectInspectorPanel.cpp

~1600 lignes, nombreuses sections par type d'objet.

## GridLevelEditorActor_Validation.inl

~40 KB.

### Conclusion

La taille est importante mais le code Editor est déjà isolé dans `GrimrockPrototypeEditor`.

Ne pas casser ces classes uniquement pour la taille.

### Déclencheur de refactor

Découper seulement si :

- conflits de modifications fréquents ;
- rebuild Editor trop coûteux ;
- ajout répété de nouveaux archetypes nécessite de toucher trop de branches ;
- logique de panneau devient réutilisable.

Le découpage naturel serait alors par familles d'inspecteurs, pas par nombre arbitraire de lignes.

---

# 13. Runtime / Editor preview

`AGridLevelRuntimeActor` possède un `UGridEditorPreviewComponent` dans le module runtime.

Le preview est protégé par les types de monde et les acteurs de preview sont marqués editor-only lorsque possible.

### Évaluation

La frontière n'est pas parfaitement pure, mais ce n'est pas une dette prioritaire :

- le module runtime ne dépend pas du module Editor ;
- l'éditeur travaille directement avec la représentation runtime ;
- le projet vise à terme la création de niveaux par les joueurs.

Conserver ce choix tant qu'il ne crée pas de coût Shipping ou de dépendance Editor effective.

---

# 14. Includes / temps de compilation

Avec `bUseUnity = false`, les headers publics coûteux ont un effet réel.

Exemples :

- `GridLevelRuntimeActor.h` inclut `GrimrockPartyPawn.h` alors que son interface semble n'utiliser que des pointeurs `AGrimrockPartyPawn*` ;
- `GridCombatActionPanelWidget.h` inclut tout le HUD pour un struct de view.

### Recommandation

Faire un petit ticket d'include hygiene seulement si les temps de build redeviennent pénalisants.

Priorité faible par rapport aux fallbacks fonctionnels.

---

# 15. Recherche d'acteurs runtime

`TActorIterator` apparaît dans environ 28 fichiers de production.

La majorité des usages inspectés sont :

- résolution de fallback ;
- initialisation ;
- mort de monstre ;
- persistence ;
- diagnostics ;
- subsystems.

Aucun `GetAllActorsOfClass` n'est présent en production.

Le hover souris n'effectue pas de scan global : il fait un line trace.

### Conclusion

Pas de problème de performance systémique identifié ici.

Éviter néanmoins d'ajouter de nouveaux scans monde dans des Tick.

---

# 16. Tick

Les mécanismes animés (`Door`, `Lever`, `Button`, `PressurePlate`, `PitTrapdoor`, projectiles) activent généralement leur Tick uniquement pendant l'animation/action.

Le `PartyPawn` reste naturellement tické pour mouvement/caméra/présentation.

### Conclusion

Pas de chantier Tick global recommandé.

---

# 17. Save / persistence

## Bon point

Le prototype applique le contrat :

```text
SaveVersion == CurrentSaveVersion
```

et ne maintient pas une chaîne de migrations de sauvegardes anciennes.

Version actuelle :

```text
22
```

## P3 — ordre de validation au chargement

Après `Super::Serialize(Ar)`, le code tente actuellement la rehydratation d'identité avant que `ValidateCurrentState()` ne rejette une `SaveVersion` différente.

Une vieille sauvegarde est bien rejetée, mais elle peut produire d'abord une erreur d'identité plus spécifique que l'erreur de version attendue.

### Recommandation

Sur load :

```text
1. vérifier immédiatement SaveVersion
2. seulement ensuite rehydrate les caches/transients
3. valider l'état courant
```

Petit durcissement, pas urgence.

---

# 18. Quêtes

Les tests MON21.4 documentent encore explicitement :

```text
SaveEnvelopeGap
SaveLoadPipelineGap
```

`UGridQuestSubsystem::CampaignState` est transient et n'est pas capturé dans `UGrimrockPartySaveGame`.

### Conclusion

Ce n'est pas du code mort : c'est une fonctionnalité encore incomplète connue.

Si les quêtes doivent survivre à Save/Load, le prochain jalon naturel reste la persistence de `FGridCampaignQuestRuntimeState`.

Ne pas mélanger ce travail avec un ticket de nettoyage C++.

---

# 19. Lua

Décision utilisateur actuelle : **laisser Lua tel quel**.

Audit :

- module séparé ;
- VM encapsulée ;
- `grid.vars.*` encore officiellement supporté ;
- compiler/highlighter/tests cohérents ;
- aucun besoin de suppression immédiate.

Aucune action recommandée dans CPP-AUDIT01.

---

# 20. Hardening des assets / IDs

Le projet dépend de plus en plus d'identités `FName` stables :

- ItemDefinitionId ;
- ClassId ;
- RaceId ;
- QuestId ;
- SpellId ;
- LogicId ;
- DefinitionId ;
- MonsterId.

C'est une bonne architecture pour des niveaux data-driven.

### Recommandation générale

Pour chaque registre runtime :

> une identité stable doit résoudre zéro ou une définition, jamais « la première parmi plusieurs ».

Le premier candidat concret est `RegisterItemDefinition()`.

---

# 21. Duplication mineure

Quelques helpers de résolution d'identité d'item sont recopiés dans :

- `GridReceptacleActor.cpp` ;
- `GridLevelRuntimeActorWorldItems.cpp` ;
- `GridLevelRuntimeActorPersistence.cpp`.

Or `AGridItemActor::GetItemDefinitionId()` contient déjà le comportement canonical :

```text
DefinitionAsset->ItemDefinitionId si disponible,
sinon ItemDefinitionId stocké
```

Ce n'est pas assez grave pour créer un nouveau utility global.

Lors d'une prochaine modification de ces fichiers, préférer réutiliser la méthode existante plutôt qu'ajouter de nouveaux helpers parallèles.

---

# 22. Debug code

`AGrimrockPlayerController` conserve les handlers :

```text
HandleMON5...
ResolveMON5TurnManager
LogMON5CommandResult
HandleMON11RequestSelectedCharacterAttack
```

Ils sont sous :

```cpp
#if !UE_BUILD_SHIPPING
```

et servent aux NumPad 1..7.

### Conclusion

Pas de coût Shipping significatif.

Les noms de milestone sont historiques, mais le code reste utile au développement.

Priorité très faible.

---

# 23. Ce qu'il ne faut PAS refactorer maintenant

L'audit déconseille explicitement :

### Ne pas éclater AGridLevelRuntimeActor

Il est gros, mais ses implémentations sont déjà divisées :

```text
main
diagnostics
feedback
monsters
typed monsters
persistence
world items
```

L'autorité unique de niveau est plus importante qu'une petite classe.

### Ne pas éclater UGridPartyInventoryComponent en plusieurs autorités

Les extractions Hotbar/Cursor/Equipment sont déjà des unités d'implémentation.

Le state container unique est un avantage.

### Ne pas éclater UGridTurnManagerComponent par principe

Le combat a besoin d'une autorité transactionnelle unique.

### Ne pas fusionner les sous-widgets UI répétés

Les panneaux de personnages/initiative/hotbar doivent rester des composants de présentation répétés.

### Ne pas retirer Lua

Décision explicite : pas gênant actuellement.

---

# 24. Roadmap de nettoyage recommandée

Ordre recommandé, avec **un ticket = un commit**.

## Phase A — supprimer les vraies compatibilités

```text
CPP-CLEAN01 — Remove Legacy Door Audio Migration
CPP-CLEAN02 — Enforce Item Definition Authority
CPP-CLEAN03 — Remove Proven Dead Compatibility Surface
```

CPP-CLEAN03 inclurait après audit Blueprint/assets :

- `EGridReceptacleRejectReason::ExplicitlyRejected` ;
- `FGridMonsterPerception::CanHear` ;
- APIs C++ sans appelant confirmées si non référencées en Blueprint.

## Phase B — supprimer les doubles configurations

```text
CPP-CLEAN04 — Remove Hard-Coded Asset Fallbacks
```

- meshes Door Chain ;
- WBP CharacterCreation fallback.

## Phase C — maintenance

```text
CPP-CLEAN05 — Logging Category Cleanup
CPP-CLEAN06 — Public Header / Test Friend Coupling Cleanup
```

À réaliser progressivement.

## Phase D — performance seulement sur preuve

```text
CPP-PERF01 — Presentation Asset Preload
```

uniquement si profiling/hitch observé.

## Fonctionnel séparé

```text
QUEST-SAVE01 — Persist Campaign Quest Runtime State
```

quand le jalon quêtes/persistence devient prioritaire.

---

# 25. Priorités synthétiques

| Priorité | Sujet | Nature |
|---|---|---|
| P1 | Legacy Door Audio migration/fallback | double schéma / compatibilité active |
| P1 | Equip fallback sans ItemDefinition | contrat trop permissif |
| P1/P2 | Duplicate ItemDefinitionId first-wins | identité data-driven ambiguë |
| P2 | ExplicitlyRejected + CanHear legacy | code mort/legacy |
| P2 | Hard-coded Door meshes / WBP fallback | double configuration |
| P2 | LogTemp massif | maintenance/diagnostic |
| P2 | 37 friends de tests sur TurnManager | couplage test/interne |
| P2 | LoadSynchronous sur présentation | risque de hitch |
| P2/P3 | mutation directe PartyInventoryState | encapsulation |
| P3 | ordre SaveVersion / rehydrate | robustesse/diagnostic |
| P3 | noms historiques UI/MON5 | lisibilité |
| Roadmap | Quest persistence | fonctionnalité incomplète |

---

# 26. Verdict

Le code n'appelle **pas** un grand refactor.

Le prototype a aujourd'hui une base saine et fortement testée. La meilleure stratégie est de continuer exactement comme les derniers audits :

1. caractériser un résidu précis ;
2. supprimer une compatibilité ou une autorité précise ;
3. conserver l'autorité principale ;
4. valider le filtre ciblé ;
5. valider ensuite le run global.

Les gains les plus nets ne viendront pas d'un découpage supplémentaire de classes, mais de la suppression de **quelques exceptions historiques qui contredisent maintenant l'architecture courante**.

La première intervention recommandée après cet audit est :

```text
CPP-CLEAN01 — Remove Legacy Door Audio Migration
```

puis :

```text
CPP-CLEAN02 — Enforce Item Definition Authority
```
