# MON21.1 — Quests / Journal / Map / Codex — Audit & Architecture Contract

Date : **25 août 2026**  
Statut : **AUDIT TERMINÉ — CONTRAT FIGÉ — AUCUNE IMPLÉMENTATION MON21 DÉMARRÉE**  
Baseline auditée : `f7a5c93750296e6828675aef22f848e05204be08`

---

## 1. Intention de MON21.1

MON21.1 est volontairement limité à un audit de l'existant et à la définition des frontières d'architecture.

Décision explicite : **ne pas enchaîner sur l'implémentation Quests / Journal / Map / Codex après cet audit.**

Le projet entre d'abord dans une phase d'exploitation, de playtest, de stabilisation et de production de contenu avec les systèmes déjà livrés jusqu'à MON20.

Aucun C++, `.uasset`, `.umap`, format SaveGame, commande runtime ou système métier n'est ajouté par MON21.1.

---

## 2. Note Git importante

Le repository contient un commit :

```text
05b1863a520b8312f1e53d9fffbcd44a98dbfa96
Close MON21.1
```

Ce commit ne constitue pas le présent audit : il ne modifie que le pointeur Git LFS de :

```text
Content/GrimrockPrototype/Blueprints/UI/WBP_GridSkills.uasset
```

Le contrat d'architecture MON21.1 est le présent document.

---

## 3. Résultat exécutif

MON21 ne part pas d'un terrain vide.

Le projet possède déjà :

- un menu multipage fonctionnel avec onglets Journal, Carte et Codex ;
- une grille autoritaire 32x32 et toute la géométrie statique du niveau dans `UGridLevelAsset` ;
- un `UGridDungeonAsset` contenant les niveaux, leurs noms et leurs positions logiques ;
- une persistance v8 du groupe, du donjon, des monstres, des objets, des variables de niveau et de la position du groupe ;
- un bus Event -> Command commun ;
- des événements gameplay déjà riches (`MonsterDied`, `EncounterCompleted`, `ItemRemoved`, `Entered`, etc.) ;
- Logic + variables persistantes Bool/Int32 ;
- Lua sandboxé capable d'orchestrer les commandes existantes ;
- des DataAssets riches pour monstres, objets, sorts, compétences, classes et contenus lisibles.

En revanche, le repository ne contient actuellement :

- aucun `QuestId`, `QuestState`, `ObjectiveState` ou service Quest métier ;
- aucun état Journal métier ;
- aucun état de découverte cellule par cellule pour une carte type fog-of-war ;
- aucun état de découverte/déverrouillage Codex ;
- aucune persistance top-level MON21 dans `UGrimrockPartySaveGame` ;
- aucune classe native dédiée `UGridJournalWidget`, `UGridMapWidget` ou `UGridCodexWidget`.

Conclusion : **les futures implémentations MON21 devront brancher une couche campagne sur les autorités existantes, pas reconstruire les systèmes déjà présents.**

---

# 4. Audit de l'UI existante

## 4.1 Shell déjà en place

`WBP_GrimrockMenu` / `UGrimrockMenuWidget` gère déjà sept onglets :

```text
Inventaire
Compétences
Sorts
Journal
Carte
Recettes
Codex
```

Les pages suivantes existent déjà comme assets UMG :

```text
WBP_GridJournal
WBP_GridMap
WBP_GridCodex
```

Elles sont intégrées dans :

```text
WidgetSwitcher_MainContent
```

Le shell C++ sait déjà activer :

```text
EInventoryTopTab::Journal
EInventoryTopTab::Map
EInventoryTopTab::Codex
```

## 4.2 Différence avec les pages fonctionnelles

Les pages fonctionnelles récentes disposent d'une couche native spécialisée :

```text
WBP_GridSkills     -> UGridSkillsWidget
WBP_GridSpellbook  -> UGridSpellbookWidget
```

Le shell :

- initialise ces pages avec le Pawn ;
- leur fournit une source de vérité runtime ;
- appelle `RefreshSkills()` / `RefreshSpellbook()` lors de l'activation.

Journal, Map et Codex sont encore déclarés comme :

```cpp
TObjectPtr<UWidget>
```

et ne possèdent ni `Initialize...Widget()` ni `Refresh...()` métier.

### Contrat futur

Lorsque MON21 sera repris, les pages existantes doivent être **reparentées/branchées**, pas remplacées par un second menu.

Le shell `UGrimrockMenuWidget` reste l'autorité de navigation supérieure.

---

# 5. Audit Quest

## 5.1 Aucun système Quest métier actuel

La recherche du repository ne révèle aucun modèle canonique du type :

```text
QuestId
QuestDefinition
QuestState
QuestObjective
QuestStage
QuestLog
```

Les variables de niveau MON19 ne doivent pas être transformées artificiellement en système de quêtes global.

## 5.2 Ce qui existe déjà et doit être réutilisé

Le bus Event -> Command fournit déjà des événements susceptibles de faire progresser une future quête :

```text
Activated
Deactivated
Used
Entered
Exited
ItemInserted
ItemRemoved
ItemChanged
MonsterDied
MonsterSpawned
MonsterDespawned
EncounterWaveStarted
EncounterCompleted
```

Les conditions de liens peuvent déjà consulter :

```text
LevelVariableBoolEquals
LevelVariableIntCompare
état de receptacle / objet / quantité / type / tag
```

MON19 fournit également :

```text
Event -> Command direct
Event -> Logic -> Command
Event -> Lua -> grid.command(...) -> Command
```

## 5.3 Frontière d'autorité future

Une quête est un état de **campagne/groupe**, pas un état local d'une seule grille.

Le futur état Quest ne doit donc pas être stocké comme autorité dans :

```text
FGridLevelRuntimeState::BoolVariables
FGridLevelRuntimeState::IntVariables
```

Ces variables restent excellentes pour :

- puzzles locaux ;
- préconditions ;
- compteurs de niveau ;
- orchestration Lua locale ;
- signaux entrant vers une future progression de quête.

Mais elles ne doivent pas devenir la base de données du journal de campagne.

## 5.4 Contrat futur sans implémentation

Quand MON21 sera repris, le modèle Quest devra respecter au minimum :

```text
QuestDefinition data-driven
    -> identité stable QuestId

Quest runtime state unique par QuestId
    -> campagne / groupe

Event -> Command / Logic / Lua
    -> demande de mutation Quest

Quest service autoritaire
    -> validation / transaction

Journal
    -> projection read-only

SaveGame
    -> snapshot stable par QuestId
```

Le nom exact des classes et structs est volontairement différé à MON21.2.

---

# 6. Audit Journal

## 6.1 Surface existante

`WBP_GridJournal` existe déjà dans le menu.

Aucune classe C++ Journal spécialisée n'existe actuellement.

## 6.2 Données déjà disponibles

Le projet dispose déjà de contenus narratifs réutilisables via :

```text
UGridReadableContentAsset
    ReadableContentId
    Title
    BodyText
    ShortDescription
    ContentType
```

Les objets/items peuvent également porter :

```text
ReadableContentAsset
ReadableContentId
ReadTitleOverride
ReadTextOverride
```

`UGridItemDefinitionAsset` possède en outre :

```text
DisplayName
Description
ReadText
ItemType
```

avec notamment les types :

```text
Book
Quest
Scroll
```

## 6.3 Contrat futur

Le Journal ne doit pas être une nouvelle source de vérité.

Il devra être une projection de données autoritaires, par exemple :

```text
Quest runtime state
    -> quêtes actives / terminées / échouées

Readable / lore discoveries
    -> entrées narratives éventuellement mémorisées

Journal view model
    -> présentation uniquement
```

L'UI ne doit pas faire progresser elle-même une quête.

---

# 7. Audit Map

## 7.1 Géométrie déjà disponible

`UGridLevelAsset` expose déjà :

```text
Width = 32
Height = 32
Cells[]
Objects[]
Links[]
```

Chaque cellule connaît notamment :

```text
CellType
North/East/South/West wall
bHasCeiling
bBlocksOccupancy
```

La carte statique ne devra donc jamais être reconstruite à partir des Actors 3D du monde.

## 7.2 Structure de donjon déjà disponible

`UGridDungeonAsset` expose :

```text
DungeonName
DefaultLevelId
Levels[]
```

Chaque niveau possède :

```text
LevelId
DisplayName
LevelAsset
LogicalPosition (X/Y/Z)
bEnabled
```

Cette structure est déjà adaptée à une future vue multi-étages / multi-niveaux.

## 7.3 Runtime déjà persistant

`FGridLevelRuntimeState` possède :

```text
LevelId
bHasBeenVisited
Doors
InteractiveObjects
Items
Monsters
MonsterPlacements
MonsterEncounters
LevelVariables
```

Le SaveGame possède également :

```text
CurrentDungeonLevelId
PartyCellX
PartyCellY
PartyFacing
```

## 7.4 Ce qui manque réellement

Il n'existe pas de collection persistante du type :

```text
VisitedCells
ExploredCells
DiscoveredEdges
MapMarkers
PlayerNotes
```

Donc :

- une carte complète statique serait techniquement dérivable dès maintenant ;
- une vraie carte de dungeon crawler avec brouillard de guerre **ne l'est pas encore** ;
- `bHasBeenVisited` ne fournit qu'une granularité niveau, pas cellule.

## 7.5 Contrat futur

La future Map doit combiner :

```text
UGridDungeonAsset / UGridLevelAsset
    -> géométrie statique

FGridDungeonRuntimeState
    -> état dynamique utile

Exploration state futur
    -> visibilité / fog-of-war / marqueurs

Party cell + facing
    -> curseur courant

WBP_GridMap
    -> projection UI
```

Aucun Actor `MapActor` ou duplication de grille n'est justifié.

---

# 8. Audit Codex

## 8.1 Les définitions de contenu sont déjà riches

Le Codex pourra réutiliser les assets de gameplay existants plutôt que copier leurs informations.

### Monstres

`UGridMonsterDefinitionAsset` fournit déjà :

```text
MonsterId
DisplayName
Description
CategoryId
DangerLevel
Icon
stats
attaques
résistances
loot
XP
AI/perception
```

### Objets

`UGridItemDefinitionAsset` fournit déjà :

```text
ItemDefinitionId
DisplayName
Description
ReadText
ItemType
Icon
Equipment / Combat / Tags
```

### Contenus lisibles

`UGridReadableContentAsset` fournit :

```text
ReadableContentId
Title
BodyText
ShortDescription
ContentType
```

### Sorts / Skills / classes

Les domaines MON18 et MON20 possèdent déjà leurs DataAssets et identités stables.

Ils sont de bons candidats à des sections de Codex si le design final le souhaite.

## 8.2 Ce qui manque

Aucun état persistant n'indique actuellement :

```text
Monster découvert
Item identifié
Livre lu
Lieu découvert
Lore débloqué
Entrée Codex visible
```

## 8.3 Contrat futur

Le Codex ne doit pas dupliquer les définitions gameplay.

Architecture cible :

```text
Existing definition assets
    -> contenu canonique

Codex discovery state futur
    -> identités débloquées seulement

Codex projection service
    -> construit les entrées visibles

WBP_GridCodex
    -> présentation
```

Un asset Codex dédié ne devra être créé que pour du contenu qui ne possède réellement aucune définition existante ou qui nécessite une couche de lore indépendante.

---

# 9. Audit SaveGame / persistance

`UGrimrockPartySaveGame` est actuellement en :

```text
CurrentSaveVersion = 8
```

Il persiste déjà :

```text
PartyInventoryState
ClassProgressionStates
PendingLevelUpNotifications
CharacterStatusEffectStates
CharacterSpellbookStates
CharacterSkillStates
DungeonRuntimeState
CurrentDungeonLevelId
PartyCellX/Y
PartyFacing
```

Aucun snapshot MON21 n'existe actuellement.

## Contrat

MON21.1 **ne modifie pas SaveVersion**.

Une future montée de version ne sera justifiée que lorsque MON21 introduira réellement une nouvelle donnée persistante autoritaire, par exemple :

```text
Quest states
Map exploration states
Codex discoveries
```

Les read models Journal/Map/Codex ne devront jamais être sauvegardés s'ils peuvent être reconstruits.

---

# 10. Contrat transversal MON21

Les règles suivantes sont désormais figées pour la reprise future de MON21.

1. `UGridLevelAsset` reste l'autorité de géométrie du niveau.
2. `UGridDungeonAsset` reste l'autorité de composition logique du donjon.
3. `FGridDungeonRuntimeState` reste l'autorité de l'état dynamique du donjon.
4. `UGrimrockPartySaveGame` reste la frontière persistante globale.
5. Event -> Command reste le bus gameplay principal.
6. Logic et Lua complètent Event -> Command ; ils ne créent pas un second système de quêtes.
7. Les variables de niveau restent locales au niveau et ne deviennent pas l'autorité Quest globale.
8. Les pages `WBP_GridJournal`, `WBP_GridMap`, `WBP_GridCodex` sont conservées.
9. `UGrimrockMenuWidget` reste le shell de navigation.
10. Journal, Map et Codex seront des projections UI ; ils ne possèdent pas l'état autoritaire.
11. Les définitions Monster/Item/Spell/Skill/etc. doivent être réutilisées par le Codex.
12. La Map est construite depuis les DataAssets, jamais depuis une duplication d'Actors monde.
13. Aucun nouveau Actor runtime permanent n'est justifié pour Quest/Journal/Map/Codex.
14. Aucun Tick permanent n'est justifié pour ces systèmes.
15. Aucun nouveau SaveVersion n'est créé pendant la phase d'exploitation.

---

# 11. Points volontairement non décidés

MON21.1 ne fige pas prématurément :

- le nom exact du futur `QuestAsset` ;
- le nombre exact de niveaux d'objectifs / étapes ;
- le format des conditions de quête ;
- la stratégie d'échec/branchement de quête ;
- la politique de notifications Quest ;
- le mode exact de fog-of-war ;
- la granularité des marqueurs de carte ;
- le mécanisme d'identification/découverte du Codex ;
- l'éventuel système de dialogue/narration conversationnelle ;
- les catégories finales du Journal et du Codex.

Ces décisions doivent être prises au moment où un besoin de gameplay concret les exige.

---

# 12. Phase active après MON21.1 — Exploitation de l'existant

Après cet audit, **MON21.2 n'est pas lancé automatiquement**.

La phase active devient :

```text
EXPLOITATION / PLAYTEST / STABILISATION
```

Objectif : utiliser réellement les systèmes déjà construits pour révéler leurs limites avant d'ajouter une nouvelle couche de campagne.

## 12.1 Domaines à exploiter

Priorité à des sessions de jeu et de création utilisant notamment :

```text
Grid Editor 32x32
multi-level dungeon
portes / portes secrètes
boutons / leviers / plaques
serrures / clés
receptacles / torches / alcoves
items / notes / livres / readable content
inventaire / équipement / hotbar
combat PA/PAM
Rat Giant
Goblin Thrower
perception / patrol / engagement automatique
loot / XP / level up
effets de statut
Spellbook / sorts
recrutement histoire
recrutement personnalisé
Skills / Talents
Event -> Command
Logic
variables persistantes
Lua
encounters / waves
save / Continue v8
transitions de donjon
```

## 12.2 Travaux autorisés pendant cette phase

Sont compatibles avec l'intention :

- correction de bugs réels découverts en jouant ;
- amélioration UX d'un système existant ;
- enrichissement des DataAssets ;
- ajout/ajustement d'animations, sons, VFX, icônes, meshes, matériaux ;
- construction de salles, puzzles et séquences de combat ;
- équilibrage ;
- diagnostics ;
- documentation de l'existant ;
- tests de persistance et transitions ;
- nettoyage ciblé uniquement lorsqu'il élimine une dette concrète observée.

## 12.3 Travaux explicitement suspendus

Jusqu'à décision explicite de reprise MON21 :

```text
pas de Quest runtime
pas de Quest DataAsset
pas de Journal métier
pas de fog-of-war Map
pas de Map discovery state
pas de Codex discovery state
pas de nouvelle migration SaveGame MON21
pas de nouveau bus d'événements
pas de refactor massif préparatoire
```

---

# 13. Ordre futur proposé — INACTIF

Quand la phase d'exploitation sera jugée suffisante, la reprise de MON21 pourra suivre cet ordre indicatif :

```text
MON21.1 — Audit & Architecture Contract                 CLOS

--- PHASE D'EXPLOITATION / STABILISATION ---

MON21.2 — Quest Definition + Campaign Runtime State
MON21.3 — Quest Event/Command Integration
MON21.4 — Quest Persistence / Migration
MON21.5 — Journal Read Model + Existing WBP Integration
MON21.6 — Map Geometry + Exploration State + Existing WBP
MON21.7 — Codex Discovery + Existing Definition Projection
MON21.8 — Cross-System Regression / PIE / Closure
```

Ce découpage n'est pas une autorisation d'implémenter MON21.2.

La reprise se fera uniquement sur instruction explicite.

---

# 14. Conclusion

MON21.1 confirme que le projet a déjà la majorité des infrastructures nécessaires à la future couche campagne :

```text
menu
DataAssets
identités stables
Event -> Command
Logic
Lua
SaveGame v8
dungeon state
level geometry
content definitions
```

Les manques MON21 sont ciblés et identifiables :

```text
Quest authority
campaign Quest persistence
cell exploration state
Codex discovery state
native projections Journal / Map / Codex
```

Décision finale :

```text
MON21.1 — AUDIT & ARCHITECTURE CONTRACT — TERMINÉ
MON21.2 — NON DÉMARRÉ
PHASE ACTIVE — EXPLOITATION / PLAYTEST / STABILISATION DE L'EXISTANT
```
