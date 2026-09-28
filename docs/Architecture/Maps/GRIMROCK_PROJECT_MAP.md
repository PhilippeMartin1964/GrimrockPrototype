# GrimrockPrototype — Carte détaillée du projet

> Carte d’architecture textuelle, diffable et autoritaire du projet.
>
> **État courant : 28 septembre 2026 — MON21.6 repris ; MON21.6.1 Map Architecture Contract validé.**
>
> HEAD audité pour MON21.6.1 : `7750ede9220eb94341ff02c618d9e7fa4c8050be`.
>
> Dernière validation globale fournie par l’utilisateur après `CPP-CLEAN06.1` : **964 tests réussis / 0 warning / 0 échec / 0 non exécuté**. Le HEAD courant ajoute ensuite uniquement deux modifications d’assets `.uasset` (Door/Main Menu), sans modification C++.

## Légende

- ✅ **implémenté et validé** dans le périmètre indiqué ;
- 🟢 **architecture stable / autorité canonique** ;
- 🟡 **implémenté mais encore incomplet côté contenu, UI ou validation manuelle** ;
- 🟠 **shell / fondation présente, fonctionnalité métier future** ;
- ⬜ **futur / non commencé** ;
- ⚠️ **dette surveillée, limitation connue ou document historique à ne pas prendre comme état courant** ;
- ⏸️ **volontairement suspendu** ;
- 🎯 **prochaine action pertinente lorsqu’un chantier est repris**.

---

# 00 — Snapshot quantitatif et documentaire

## 00.1 — Corpus audité

- ✅ **511 documents Markdown** sous `docs/`.
  - 419 sous `docs/Design/` ;
  - 62 sous `docs/Architecture/` ;
  - 17 directement sous `docs/` ;
  - 5 ArtBook ;
  - 3 Rules ;
  - 3 Tests ;
  - 1 Images ;
  - 1 UI.
- ✅ **719 fichiers Source** dans les trois modules.
  - `GrimrockLua` ;
  - `GrimrockPrototype` ;
  - `GrimrockPrototypeEditor`.
- ✅ Tests très présents :
  - 234 fichiers sous `GrimrockPrototype/Private/Tests` ;
  - 48 fichiers sous `GrimrockPrototypeEditor/Private/Tests` ;
  - 4 fichiers de tests Lua.

## 00.2 — Hiérarchie documentaire à utiliser

1. **Code courant + tests validés + derniers commits**.
2. Documents de clôture / validation les plus récents.
3. `docs/Design/99_DECISIONS_LOG.md`.
4. `docs/Architecture/PROJECT_SYNTHESIS.md`.
5. `docs/Design/PROJECT_COMPLETION_ROADMAP.md`.
6. `docs/Architecture/ARCHITECTURE_INDEX.md`.
7. Fondations et tickets historiques.
8. Anciennes roadmaps / checklists uniquement comme historique.

## 00.3 — Dérives documentaires identifiées lors de cette mise à jour

- ⚠️ `docs/Design/00_PROJECT_OVERVIEW.md` reste daté du 23 août et s’arrête à MON20.3.
- ⚠️ `docs/Design/README.md` présente encore MON20.4 comme prochain jalon.
- ⚠️ l’en-tête de `UI_ARCHITECTURE_CURRENT.md` est plus ancien que plusieurs sections ajoutées ensuite.
- ⚠️ `TECHNICAL_DEBT_REGISTER.md` mentionne encore une dette `TD-LOG-001` alors que CPP-CLEAN05 a supprimé les **502 appels réels à `LogTemp`**.
- ⚠️ les anciennes cartes sous `docs/Architecture/Maps/` étaient des snapshots d’août 2026 ; elles sont remplacées par la présente génération.
- ✅ Les documents historiques ne sont pas réécrits rétroactivement : Git reste l’historique, les cartes courantes explicitent ce qui est superseded.

---

# 01 — Vision produit

## 01.1 — Jeu cible

- ✅ Dungeon crawler en vue subjective inspiré de Legend of Grimrock 2.
- ✅ Déplacement **case par case** et rotations cardinales.
- ✅ Grille logique **32 × 32** par niveau.
- ✅ Dimensions de référence :
  - cellule : **200 × 200 × 300 cm** ;
  - murs : **10 cm** ;
  - piliers : **20 × 20 cm**.
- ✅ Repère cardinal :
  - North = Y+ ;
  - East = X+ ;
  - South = Y− ;
  - West/Ouest = X−.
- ✅ Exploration, portes, secrets, mécanismes, puzzles, objets, combats, monstres, progression RPG et magie.
- ✅ Donjons multi-niveaux.
- 🎯 Long terme : outil de création de niveaux utilisable par les joueurs, sans dupliquer le modèle de données du jeu.

## 01.2 — Principes de conception

- 🟢 **Data-driven** : définitions réutilisables + instances placées.
- 🟢 **Une seule autorité par donnée**.
- 🟢 **Définition ≠ instance**.
- 🟢 **Authoring ≠ runtime**.
- 🟢 La grille et les identités stables priment sur les transforms monde et les pointeurs transient.
- 🟢 Le C++ porte logique, calculs, invariants et read models.
- 🟢 Blueprint/UMG porte composition, configuration et présentation.
- 🟢 Lua est réservé aux logiques spécifiques de puzzles/scénarios, pas aux primitives génériques.
- 🟢 Pas de couche architecturale supplémentaire sans bénéfice démontré.
- 🟢 Pas de compatibilité arrière Save/DataAsset pendant le prototype ; Git est l’historique.

---

# 02 — Architecture des modules

## 02.1 — Modules C++

```text
GrimrockLua
    ↓
GrimrockPrototype
    ↓
GrimrockPrototypeEditor
```

- ✅ `GrimrockLua`
  - VM Lua 5.4 embarquée ;
  - sandbox ;
  - exécution de callbacks ;
  - tests du langage/runtime.
- ✅ `GrimrockPrototype`
  - Core data model ;
  - runtime ;
  - RPG ;
  - magie ;
  - quêtes ;
  - Save ;
  - UI runtime.
- ✅ `GrimrockPrototypeEditor`
  - EdMode ;
  - toolkit ;
  - Slate ;
  - inspecteurs ;
  - palette ;
  - validation ;
  - preview ;
  - authoring Lua ;
  - tests Editor.
- 🟢 Le Runtime ne dépend jamais du module Editor.
- 🟢 L’Editor consomme le modèle runtime/core au lieu de créer un second modèle.

## 02.2 — Façades principales

- `AGridLevelRuntimeActor`
  - autorité du niveau vivant ;
  - construction/restauration du donjon ;
  - interactions avec l’état runtime.
- `AGrimrockPartyPawn`
  - groupe joueur ;
  - mouvement ;
  - interaction ;
  - orchestration UI ;
  - transitions.
- `UGridPartyInventoryComponent`
  - groupe ;
  - inventaire ;
  - équipement ;
  - sélection ;
  - hotbar ;
  - ownership.
- `UGridTurnManagerComponent`
  - combat ;
  - initiative ;
  - tours ;
  - actions ;
  - PA/PAM.
- `UGridActivationComponent`
  - Event → Command ;
  - Logic ;
  - Lua ;
  - commandes Quest.
- `UGridQuestSubsystem`
  - autorité runtime unique des quêtes.
- `AGridLevelEditorActor`
  - façade d’authoring dans le module Editor.

---

# 03 — Modèle de données du donjon

## 03.1 — Racine multi-niveaux

- ✅ `UGridDungeonAsset`
  - `DungeonName` ;
  - auteur/version ;
  - niveau par défaut ;
  - `TArray<FGridDungeonLevelEntry> Levels`.
- ✅ Chaque entrée référence un `UGridLevelAsset`.
- ✅ Résolution :
  - `GetLevelAssetById()` ;
  - `GetDefaultLevelAsset()` ;
  - `FindLevelBelow()`.
- 🟢 Le donjon orchestre plusieurs LevelAssets ; chaque niveau garde son asset de niveau unique.

## 03.2 — UGridLevelAsset

Contient les autorités d’authoring d’un niveau :

- ✅ grille/cellules : `Cells` ;
- ✅ point de départ du groupe ;
- ✅ `WorldObjectInstances` ;
- ✅ `LooseItemInstances` ;
- ✅ `MonsterSpawns` ;
- ✅ `ItemSpawns` ;
- ✅ `LogicObjects` ;
- ✅ `Links` ;
- ✅ `QuestDefinitions` ;
- ✅ `LevelVariables` ;
- ✅ `LuaScripts`.

## 03.3 — Buckets de placement typés

- ✅ World Object :
  - `FGridWorldObjectInstance`.
- ✅ Loose Item :
  - placement d’un item existant dans le monde.
- ✅ Monster Spawn :
  - définition monstre ;
  - position ;
  - facing ;
  - état initial ;
  - patrouille ;
  - encounter.
- ✅ Item Spawn :
  - générateur/placement d’item.
- ✅ Logic Object :
  - identité logique ;
  - placement ;
  - configuration Logic/Story.
- 🟢 WORLDOBJ-MIG09/MIG10 ont supprimé le modèle générique legacy comme autorité parallèle.

## 03.4 — Identités stables

- `ObjectId` / GUID : identité persistante d’une instance placée.
- `LogicId` : alias authoring lisible, unique dans son périmètre.
- `DefinitionId` : identité d’une World Object Definition.
- `ItemDefinitionId` : identité item.
- `MonsterDefinitionId` : identité monstre.
- `CharacterId` : personnage.
- `RuntimeObjectId` : instance d’item.
- `QuestId` / `ObjectiveId` : quêtes.
- 🟢 Un ID doit résoudre zéro ou une définition, jamais « la première parmi plusieurs ».
- ✅ CPP-CLEAN02 durcit l’enregistrement des `ItemDefinitionId` dupliqués.

---

# 04 — Grille, espace et placement

## 04.1 — Grille autoritaire

- ✅ Position logique par cellule `X/Y`.
- ✅ Facing cardinal.
- ✅ Les déplacements joueur et monstre utilisent la topologie, pas un déplacement libre monde.
- ✅ Occupation, collisions logiques, LOS, acoustique et ciblage se ramènent au modèle de grille.

## 04.2 — Placement local

WORLDOBJ-MIG a convergé vers une représentation par surface :

- Floor ;
- Wall ;
- Ceiling.

Position locale :

- `U` : tangent horizontal ;
- `V` : second axe tangent ;
- `N` : normale à la surface.

- ✅ Un objet centré au sol est un placement Floor avec offsets nuls.
- ✅ Les boundaries sont séparées du PlacementKind.
- ✅ Les transforms monde UE sont dérivés depuis cellule/surface/local transform.

## 04.3 — Frontières de cellule

Une boundary normalisée sert aux systèmes qui occupent ou bloquent une séparation :

- portes ;
- portes secrètes ;
- murs ;
- collision ;
- passage ;
- acoustique ;
- projectiles ;
- pathfinding monstre.

---

# 05 — World Object Definition / Instance

## 05.1 — Définition réutilisable

Autorité finale : `UGridWorldObjectDefinitionAsset`.

Contient notamment :

- `DefinitionId` ;
- `DisplayName` ;
- `SupportedType` / Gameplay Type ;
- defaults ;
- règles de placement ;
- comportement spatial ;
- interaction ;
- audio events ;
- lumière ;
- Static Part ;
- `MovingParts[]` ;
- aliases de matériaux runtime ;
- RuntimeActorClass ;
- validation de définition.

## 05.2 — Classification simplifiée

Après WORLDOBJ-CLASS01 :

- 🟢 `SupportedType` est la classification gameplay principale.
- 🟢 `FGridObjectPaletteEntry.PaletteCategory` reste l’autorité de groupement dans la palette Editor.
- ✅ `Display Name Override` relève de la présentation de palette, pas de l’identité gameplay.
- ✅ L’ancien Teleporter est remplacé conceptuellement par **Relocation** pour les escaliers, portails et passages automatiques.

## 05.3 — Instance placée

`FGridWorldObjectInstance` stocke :

- ObjectId ;
- WorldObjectDefinitionId ;
- LogicId ;
- cellule/surface/facing ;
- transform local optionnel ;
- données authoring ;
- `FGridWorldObjectInstanceConfig`.

Config d’instance :

- état initial Door ;
- état Relocation ;
- configuration Pit ;
- contenu initial Receptacle ;
- interaction overrides ;
- `MovingPartOverrides[]` ;
- chaîne Door ;
- Lock ;
- autres overrides sparse.

## 05.4 — Moving Parts

- ✅ WORLDOBJ-MOVINGPARTS01 : tableau **0..N** de moving parts.
- ✅ Door peut animer toutes ses parties.
- ✅ Button / Lever / PressurePlate utilisent l’index pertinent.
- ✅ Pit possède ses règles spécialisées.
- 🟢 Pas de structure Part0/Part1 parallèle.

---

# 06 — Grid Editor

## 06.1 — Shell Editor

- `FGridLevelEdMode`.
- `FGridLevelEdModeToolkit`.
- `AGridLevelEditorActor`.
- Services/panneaux Slate spécialisés.
- `GridLevelEditorActorParts/*.inl` pour fractionner l’implémentation sans multiplier les propriétaires.

## 06.2 — Workspace GEUI

Fenêtres persistantes :

- Dungeon Levels ;
- Palette ;
- Selected Object ;
- Connectors / Links ;
- Validation ;
- Playtest ;
- Overview Map.

Outils :

- Select ;
- Paint Cell ;
- Paint Wall ;
- Paint Object ;
- Erase ;
- Link.

## 06.3 — Authoring

- cellules et murs ;
- point de départ/facing ;
- placements World Object ;
- items ;
- MonsterSpawn ;
- Logic Objects ;
- liens Event → Command ;
- conditions ;
- variables ;
- Lua ;
- patrouilles ;
- instance overrides ;
- relocations ;
- pits.

## 06.4 — Selected Object

Inspecteurs spécialisés :

- identité ;
- placement ;
- Door motion overrides ;
- Pressure Plate ;
- Receptacle ;
- Lock ;
- Relocation ;
- Pit ;
- Monster Spawn ;
- Logic/Lua ;
- autres familles selon SupportedType.

## 06.5 — Undo / Erase / sécurité UI

- ✅ Undo/Redo transactionnel natif.
- ✅ Groupement des gestes.
- ✅ Erase ciblé.
- ✅ ERASE02 : gomme one-shot safe, évite d’effacer derrière les panneaux UI.

## 06.6 — Validation

Vérifications couvrant notamment :

- IDs ;
- références de définitions ;
- placements ;
- links ;
- MonsterSpawn ;
- LogicId ;
- scripts Lua ;
- cohérence des instances ;
- données manquantes.

## 06.7 — Preview / Playtest

- preview du niveau ;
- preview des objets ;
- sélection/stencil ;
- overview map ;
- lancement PIE depuis l’Editor ;
- diagnostics.
- 🟡 Slate/validation restent volumineux mais isolés dans le module Editor.
- 🟢 Aucun refactor de masse recommandé sans douleur concrète.

---

# 07 — Runtime de niveau et exploration

## 07.1 — AGridLevelRuntimeActor

Responsabilités :

- construire le niveau depuis `UGridLevelAsset` ;
- appliquer l’état runtime vivant ;
- gérer cellules, murs et objets ;
- provisionner actors runtime ;
- gérer items monde ;
- gérer monstres ;
- persistance ;
- feedback/diagnostics.

Implémentation déjà répartie dans plusieurs `.cpp` :

- principal ;
- diagnostics ;
- feedback UI ;
- persistence ;
- world items ;
- monster/runtime helpers.

🟢 TD05.9 : **stop condition atteinte**.  
⚠️ Classe centrale mais volontairement conservée comme autorité unique du niveau.

## 07.2 — AGrimrockPartyPawn

Exploration :

- Forward / Backward ;
- Strafe ;
- rotation 90° ;
- interpolation ;
- buffering de commande ;
- collision logique ;
- feedback mur ;
- head bob ;
- free look limité ;
- interaction ;
- chute Pit ;
- transitions de niveaux ;
- UI orchestration.

## 07.3 — Contrôle joueur

`AGrimrockPlayerController` :

- interaction souris ;
- line traces ;
- cursor state ;
- interaction monde ;
- ciblage throw ;
- ciblage combat ;
- routage input/debug.

🟢 Le hover ne fait pas de scan global du monde.

## 07.4 — Audio exploration

- pas ;
- rotations ;
- choc mur ;
- chute/cri/landing ;
- pitch/variantes selon contrat ;
- feedback piloté par gameplay, pas par Blueprint parallèle.

---

# 08 — Mécanismes et interactions

## 08.1 — Portes

- Door standard ;
- Secret Door ;
- Open / Close / Toggle ;
- blocage jusqu’à état réellement ouvert ;
- audio open/close ;
- reprise de fermeture ;
- tail audio ;
- moving parts génériques ;
- chain visuals configurées par assets depuis CPP-CLEAN04.

## 08.2 — Mechanisms

- Button ;
- Secret Button ;
- Lever ;
- Pressure Plate ;
- Trigger Enter/Exit ;
- événements d’activation ;
- états runtime.

## 08.3 — Receptacles

- règles d’acceptation ;
- contenu initial ;
- insertion/retrait ;
- événements ;
- identité d’item ;
- feedback de rejet ;
- transfert via service transactionnel.
- ✅ `ExplicitlyRejected` legacy supprimé par CPP-CLEAN03.

## 08.4 — Locks

- WallLock ;
- accepted key definitions / IDs ;
- consommation/interaction selon contrat ;
- système data-driven.

## 08.5 — Readables

- inscriptions ;
- notes ;
- contenus lisibles ;
- `UGridReadableContentAsset` ;
- feedback/read panel.

## 08.6 — Relocation

- Stairs Up ;
- Stairs Down ;
- anciens téléporteurs normalisés comme Relocation ;
- destination niveau/cellule/facing ;
- activation initiale.

## 08.7 — Pits

- PIT01 : chute inter-level du groupe ;
- PIT02 : World Items à travers les fosses ;
- PIT03/PIT03.2 : trapdoor contrôlée à deux volets ;
- ouverture gameplay immédiate ;
- fermeture stabilisée à l’endpoint ;
- landing fallback déterministe ;
- audio + impact caméra à l’atterrissage.

---

# 09 — Event → Command, Logic et Lua

## 09.1 — Dispatcher

`UGridActivationComponent` reste l’autorité :

```text
Source Object
  + Event
      -> FGridObjectLink
          -> Target
          + Command
          + condition optionnelle
```

## 09.2 — Trois chemins

```text
Event -> Command

Event -> Logic -> Event -> Command

Event -> Lua Callback
      -> grid.command(...)
      -> Command
```

## 09.3 — Variables

- Bool ;
- Int32 ;
- definitions dans `UGridLevelAsset` ;
- valeurs vivantes dans l’état runtime ;
- persistence ;
- conditions de links.

## 09.4 — Logic Nodes

Primitives génériques :

- Relay ;
- Set Bool ;
- Toggle Bool ;
- Set Int ;
- Add/Subtract ;
- Reset ;
- Compare ;
- Latch ;
- autres primitives documentées.

## 09.5 — Lua

- Lua 5.4 embarqué ;
- VM sandboxée ;
- callbacks ;
- `persistent` ;
- `grid.vars.*` ;
- `grid.command(...)` ;
- compilation authoring ;
- syntax highlighting ;
- validation Editor.
- 🟢 Décision actuelle : conserver Lua pour la logique de puzzle spécifique.

## 09.6 — Sécurité d’exécution

- budget d’actions runtime ;
- protection contre cycles ;
- source scope ;
- retours vers le bus Command canonique.

---

# 10 — Items, inventaire, équipement et transfert

## 10.1 — UGridItemDefinitionAsset

Décrit :

- ItemDefinitionId ;
- display/description ;
- type ;
- poids ;
- stack ;
- icon ;
- mesh monde ;
- compatibilité équipement ;
- stats/bonus/résistances ;
- combat actions ;
- light emitter ;
- sparkle ;
- présentation.

## 10.2 — UGridPartyInventoryComponent

Autorité :

`FGridPartyInventoryState`

- ActiveCharacters ;
- ActiveEquipment ;
- CharacterPool ;
- sélection ;
- cursor item ;
- hotbar ;
- états durables par personnage.

Implémentation répartie :

- principal ;
- CursorTransfer ;
- Equipment ;
- WorldTransfer ;
- Diagnostics ;
- Hotbar/services associés.

🟢 TD06.9 : stop condition atteinte.

## 10.3 — Capacité du sac

- capacité globale commune aux personnages ;
- nombre de colonnes configurable ;
- capacité indépendante du poids ;
- capacité visuelle fixe ;
- filtres/tri = projection, pas mutation physique.

## 10.4 — UGridItemTransferService

Routage entre :

- inventaire ;
- équipement ;
- curseur ;
- monde ;
- réceptacles ;
- portraits/personnages.

Principes :

- transaction atomique ;
- ownership exclusif ;
- pas de duplicate ;
- validation source/cible.

## 10.5 — Context actions

- Examine ;
- Equip ;
- AddToHotbar ;
- Drop ;
- Split pour stacks ;
- actions spécifiques selon item.
- 🟢 La scission est contextuelle, pas un mode caché du drag.

## 10.6 — World Items

- pickup ;
- drop ;
- throw ;
- projectile récupérable ;
- physics settling ;
- sparkle optionnel ;
- item light ;
- restore depuis Save.

---

# 11 — Groupe, personnages, recrutement et progression

## 11.1 — Groupe

- jusqu’à six personnages actifs ;
- CharacterPool pour réserve ;
- SelectedCharacterIndex unique ;
- équipement aligné sur l’index actif ;
- identités CharacterId stables.

## 11.2 — Création de personnage

Wizard :

- identité ;
- portrait ;
- race ;
- classe ;
- attributs ;
- résumé ;
- finalisation.
- réutilisé également pour certains parcours de recrutement custom.

## 11.3 — Recrutement

MON20 clos :

- Active Party Recruitment ;
- Story Companion data-driven ;
- Custom Recruit ;
- réserve/CharacterPool ;
- transactions atomiques ;
- validation identité/capacité ;
- normalisation inventory/equipment/hotbar.

## 11.4 — Progression

- XP ;
- Level dérivé de l’Experience ;
- progression de classe ;
- choix ;
- Level Up ;
- requirements ;
- notifications ;
- persistence current-schema.

## 11.5 — Skills / Talents

- Skills fonctionnels via MON20 ;
- SkillRanks durable par personnage ;
- skill checks déterministes ;
- page UI Skills/Talents fonctionnelle ;
- Talents réutilisent les ProgressionChoices/Requirements au lieu d’un second moteur.

---

# 12 — Combat

## 12.1 — UGridTurnManagerComponent

Autorité :

- combat active/inactive ;
- phases ;
- rounds ;
- initiative ;
- combattant actif ;
- PA individuels ;
- PAM ;
- actions ;
- cooldowns ;
- résultats ;
- combat log.

## 12.2 — Initiative

- ordre global Party + Monsters ;
- état Waiting / Active / Completed / Defeated ;
- preview ;
- modifiers dynamiques ;
- active combatant jamais réordonné rétroactivement.

## 12.3 — Actions joueur

Sources :

- attaque principale ;
- MainHand / OffHand ;
- unarmed fallback ;
- équipement ;
- quick items ;
- classe ;
- spells ;
- hotbar.

Transaction :

1. valider turn/attacker ;
2. résoudre action/source ;
3. valider cible ;
4. calculer coûts ;
5. payer atomiquement ;
6. appliquer effet ;
7. démarrer cooldown si applicable ;
8. notifier UI/presentation.

## 12.4 — Ressources

- PA ;
- PAM ;
- Mana ;
- items/charges ;
- cooldown.
- 🟢 Un rejet ne consomme pas les ressources.

## 12.5 — Ciblage

- monstre en face ;
- portée ;
- walls/doors ;
- cell ;
- area ;
- preview ;
- throw/ranged ;
- targeting UI.

## 12.6 — Présentation

Séparée du calcul :

- animation ;
- VFX ;
- audio ;
- projectiles ;
- HUD ;
- combat log ;
- feedback.

---

# 13 — Monstres et IA

## 13.1 — Définition / Actor

- `UGridMonsterDefinitionAsset` = données ;
- `AGridMonsterActor` = instance runtime.

## 13.2 — Composants

- Movement ;
- Behavior ;
- Combat ;
- Death ;
- Audio ;
- VFX ;
- Idle Variation ;
- occupancy/pathfinding via subsystems/services.

## 13.3 — Occupation

- `UGridMonsterOccupancySubsystem` ;
- cellule occupée ;
- réservations ;
- mouvement déterministe ;
- cohérence restore/death.

## 13.4 — Perception

- ligne de vue directionnelle ;
- hearing à travers la grille ;
- `CanHearThroughGrid()` unique depuis CPP-CLEAN03 ;
- dernière cellule connue du groupe ;
- awareness normalisée.

## 13.5 — Behavior

- Idle ;
- Alert ;
- Pursuing ;
- Attacking ;
- Hurt ;
- Dead ;
- patrouille ;
- investigation ;
- alarmes ;
- blocked hearing wait ;
- réconciliation après combat/restore.

## 13.6 — Encounters

- MonsterSpawn ;
- EncounterGroupId ;
- StartEncounter ;
- vagues ;
- activation ;
- désactivation ;
- despawn ;
- engagement différé sûr ;
- persistence encounter.

## 13.7 — Familles de combat

- baseline mêlée / WereRat-Rat historique ;
- ranged / Goblin-like family ;
- planners spécialisés ;
- projectiles ;
- loot ;
- XP ;
- dissolution death.
- 🟡 Bestiaire de production encore à densifier.

---

# 14 — Magic et Status Effects

## 14.1 — Spellbook

- `UGridPartySpellbookComponent` façade ;
- `KnownSpellIds` durable dans le personnage ;
- definitions data-driven ;
- ciblage ;
- transaction ;
- hotbar ;
- UI Spellbook.

## 14.2 — Sorts de référence

Le projet possède une bibliothèque de production/tests couvrant notamment :

- projectile offensif ;
- heal ;
- haste ;
- cure/cleanse selon définitions présentes.

## 14.3 — Status Effects

- definitions ;
- application ;
- stacks ;
- durée ;
- periodic damage ;
- control ;
- initiative modifiers ;
- presentation ;
- save/restore ;
- player + monster.

## 14.4 — Autorité current-schema

- `Character.StatusEffects` durable ;
- asset pointer transient et rehydraté par ID ;
- pas de snapshot parallèle legacy.

---

# 15 — Save / Restore / Persistence

## 15.1 — Contrat actuel

`UGrimrockPartySaveGame::CurrentSaveVersion = 22`.

- ✅ exact-match ;
- ✅ ancienne version = rejet ;
- ✅ aucune chaîne de migration arrière pendant le prototype ;
- ✅ données reconstructibles non persistées.

## 15.2 — Sauvegarde groupe

Contient les données durables nécessaires à :

- groupe ;
- personnages ;
- inventaire ;
- équipement ;
- hotbar ;
- CharacterPool ;
- progression ;
- skills ;
- spells ;
- status ;
- sélection ;
- position/facing/niveau.

## 15.3 — FGridDungeonRuntimeState

Par niveau :

- Doors ;
- InteractiveObjects ;
- ObjectPresence ;
- ObjectVisuals ;
- Items ;
- Pits ;
- PendingInboundItems ;
- Receptacles ;
- Monsters ;
- MonsterPlacements ;
- MonsterEncounters ;
- BoolVariables ;
- IntVariables.

## 15.4 — Politique combat

- sauvegarde régulière refusée pendant combat actif ;
- checkpoint pré-combat utilisé par certains flux ;
- load doit restaurer les identités et données current-schema.

## 15.5 — Gaps

- ⏸️ Quest CampaignState non persisté.
- 🟠 Autosave/checkpoint policy produit à finaliser.
- ⚠️ ordre de validation SaveVersion/rehydration identifié comme petit hardening P3, non urgent.

---

# 16 — Quêtes, Journal, Map, Codex

## 16.1 — Quêtes runtime existantes

MON21.2–21.3 :

- `UGridQuestDefinitionAsset` ;
- QuestId ;
- ObjectiveId ;
- objectifs ordonnés ;
- `UGridQuestSubsystem : UGameInstanceSubsystem` ;
- `FGridCampaignQuestRuntimeState` ;
- Start ;
- CompleteObjective ;
- Complete ;
- Fail ;
- `OnQuestStateChanged`.
- intégration Event → Command :
  - QuestStart ;
  - QuestCompleteObjective ;
  - QuestComplete ;
  - QuestFail.

## 16.2 — État actuel

- ✅ fondation métier runtime présente ;
- ⏸️ **Quest Persistence volontairement reportée** à une prochaine fonctionnalité ;
- ⏸️ aucune reprise MON21.4 dans le présent chantier.

## 16.3 — Journal

- 🟠 page/shell déjà présente dans `WBP_GrimrockMenu` ;
- ⬜ futur read model ;
- ⬜ affichage quêtes/notes/informations ;
- 🟢 ne doit jamais devenir une seconde autorité Quest.

## 16.4 — Map

- 🟠 `WBP_GridMap` / shell présents ;
- ✅ **MON21.6.1 Map Architecture Contract validé** ;
- 🟢 `UGridLevelAsset` reste l’autorité de géométrie d’une dalle 32×32 ;
- 🟢 un étage = toutes les entrées activées partageant `LogicalPosition.Z` ;
- 🟢 `LogicalPosition.X/Y` compose les dalles adjacentes sans couture visible ;
- 🟢 coordonnées carte : `MapX = TileX * 32 + LocalX`, `MapY = TileY * 32 + LocalY` ;
- 🟢 fog-of-war : connaissance durable par cellule, rayon de design 1.25, topologie bloquée par murs/portes fermées ;
- 🟢 feather visuel ~0.25 cellule sans révélation de géométrie supplémentaire ;
- 🟢 secret caché = mur normal ; découverte durable indépendante de l’état ouvert/fermé ;
- 🟢 le read model Map ne transmet jamais de géométrie inconnue au WBP ;
- 🟢 aucun `MapActor`, aucun Tick permanent, aucune seconde grille/autorité ;
- ✅ MON21.6.2 Exploration State validé : 4/4, 0 warning, 0 échec (`TD04-20260928-082142`) ;
- 🟢 `FGridMapExplorationState` porte Unknown/Explored sur 32×32, avec allocation paresseuse ;
- 🟢 `FGridLevelRuntimeState::MapExploration` isole l’exploration par `LevelId` et la conserve pendant la session ;
- 🟢 SaveGame volontairement inchangé en v22 : persistance disque réservée à MON21.6.5 ;
- 🟡 MON21.6.3 Topology-Aware Reveal implémenté, validation locale à fournir ;
- 🟢 rayon 1.25 : cellule courante + cardinales, diagonales exclues ;
- 🟢 murs vérifiés sur les deux côtés de la frontière ;
- 🟢 portes bloquantes arrêtent le reveal, portes ouvertes laissent voir la cellule voisine ;
- 🟢 `bBlocksOccupancy` n’est pas une occlusion Map ;
- 🟢 reveal déclenché au démarrage et après déplacement via `HandlePartyCellChanged()` ;
- ⬜ MON21.6.4+ : secrets, persistence, read model, projection multi-dalles, UI/navigation et rendu parchemin.

## 16.5 — Codex

- 🟠 page/shell présente ;
- ✅ contenu source déjà riche : items, monstres, sorts, skills, readables ;
- ⬜ discovery state ;
- ⬜ read model ;
- ⬜ catégories/filtrage.

## 16.6 — Craft / Recipes

- 🟠 shell de page présent ;
- ⬜ système métier futur ;
- ⬜ recettes, ingrédients, validation, transaction et UI à définir dans un chantier distinct.

---

# 17 — UI Runtime — architecture courante

## 17.1 — Principe

La refonte a remplacé le vieux menu monolithique par des surfaces séparées.

```text
Viewport
├── WBP_CharacterSheet      gauche
├── vue 3D                 centre interactive
├── WBP_InventoryBag        droite
├── WBP_GridCombatHud       combat seulement
└── WBP_GridPersistentHud   navigation + barre d’actions persistante
```

## 17.2 — Fondation / Split

- ✅ UI-FOUNDATION01 réalisé puis superseded visuellement.
- ✅ UI-SPLIT01 : CharacterSheet + InventoryBag indépendants.
- ✅ UI-SPLIT02 : centre 3D reste interactif.
- ✅ UI-SPLIT03 : paper doll uniquement dans CharacterSheet.
- ✅ UI-CLEAN01 : ancien Page_Inventory monolithique supprimé.
- ✅ UI-CLEAN02 : paper doll généré runtime supprimé.
- ✅ UI-CLEAN03 : alias armor legacy supprimé.
- ✅ UI-CLEAN04 : helpers texte primitifs supprimés.

## 17.3 — Navigation

- ✅ UI-NAV01 ;
- ✅ raccourcis `I/K/G/M/J/H/ESC` ;
- ✅ boutons X des grandes fenêtres ;
- ✅ navigation persistante ;
- ✅ mêmes commandes C++ pour clavier et boutons.

## 17.4 — Persistent HUD

`UGridPersistentHudWidget` :

- navigation globale ;
- barre générale d’actions ;
- slots dynamiques selon largeur ;
- minimum clavier prévu pour la disposition de projet ;
- séparation stricte du Combat HUD.

UI-GLOBALHUD01 :

- largeur dynamique ;
- séparation verticale des contrôles combat ;
- suppression du chrome legacy du Combat HUD.

## 17.5 — Character Sheet

- ✅ UI-CHAR01 : SelectedCharacter unique.
- ✅ UI-CHAR02 : feuille complète.
- portrait ;
- classe/niveau ;
- attributs ;
- health/mana ;
- armor ;
- initiative ;
- accuracy/evasion ;
- résistances ;
- paper doll/equipment.
- 🟢 poids retiré de la feuille : il appartient au sac/feedback surcharge.

## 17.6 — Inventory Bag

- ✅ UI-INV01 : un seul sac pour SelectedCharacter.
- ✅ UI-INV02 : drag/drop complet.
- ✅ capacité fixe.
- ✅ colonnes configurables.
- ✅ poids courant/max.
- ✅ header « Sac de … ».
- ✅ context menu.
- ✅ tri.
- ✅ filtres.
- ✅ état vide.
- ✅ projection in-place : changement filtre/tri ne recrée pas la grille si topologie inchangée.

## 17.7 — Tooltip

- ✅ UI-ITEM01 ;
- `FGridItemTooltipView` ;
- identité/description ;
- poids ;
- stats ;
- capacités ;
- résistances ;
- comparaison avec équipement ;
- comparaison possible à main vide ;
- icônes carrées 48×48.
- 🟢 examen détaillé futur séparé du tooltip.

## 17.8 — Weight

- ✅ UI-WEIGHT01 clos ;
- texte poids dans InventoryBag ;
- pas de progress bar ;
- surcharge portrait ;
- `IsAnyActiveCharacterOverloaded()` ;
- surcharge bloque les translations mais pas la rotation.

## 17.9 — Filter / Sort

UI-FILTER01 :

- Tous ;
- Équipement ;
- Consommables ;
- Magie ;
- Ingrédients ;
- Livres et clés ;
- Divers.

UI-INVENTORY02 :

- nom asc/desc ;
- type asc/desc ;
- poids asc/desc ;
- projection visuelle uniquement ;
- slots physiques inchangés.

## 17.10 — Hotbar / action bar

- ✅ état hotbar canonique dans PartyInventory ;
- ✅ quick slots liés aux objets/actions ;
- ✅ consommables décrémentés ;
- ✅ MainHand/OffHand/Unarmed supportés ;
- ✅ Spellbook peut binder des spells ;
- ✅ Persistent HUD présente la barre générale ;
- ✅ combat réutilise les mêmes autorités, sans deuxième hotbar.

## 17.11 — UI Feedback

- ✅ UI-FEEDBACK01.1 surcharge portrait validée.
- 🟡 UI-FEEDBACK01.2 effets de statut :
  - C++ présent ;
  - projection/icons/+N ;
  - UMG/validation finale encore à acter dans la documentation courante.
- 🟡 hover/selected/drop feedback existe par surfaces, mais la passe artistique finale reste future.
- 🟡 sons UI : polish global futur.

## 17.12 — Skills / Spellbook

- ✅ Skills/Talents fonctionnels via MON20.
- ✅ Spellbook fonctionnel via MON18.
- 🟢 `WBP_GrimrockMenu` sert encore de shell aux pages non split :
  - Skills ;
  - Spellbook ;
  - Journal ;
  - Map ;
  - Recipes ;
  - Codex.

## 17.13 — État de la roadmap UI originale

| Ticket | État courant |
|---|---|
| UI-FOUNDATION01 | ✅ réalisé, présentation superseded par UI-SPLIT |
| UI-NAV01 | ✅ validé |
| UI-CHAR01 | ✅ validé |
| UI-CHAR02 | ✅ validé |
| UI-INV01 | ✅ validé |
| UI-INV02 | ✅ validé |
| UI-ITEM01 | ✅ validé |
| UI-WEIGHT01 | ✅ clos |
| UI-FILTER01 | ✅ fonctionnel/validé |
| UI-HOTBAR01 | ✅ réalisé, absorbé par Persistent HUD |
| UI-FEEDBACK01 | 🟡 01.1 clos ; 01.2 à finaliser/acter |
| UI-SKILLS01 | ✅ dépassé : page fonctionnelle via MON20 |
| UI-CRAFT01 | 🟠 shell |
| UI-MAP01 | 🟠 shell |
| UI-JOURNAL01 | 🟠 shell |
| UI-CODEX01 | 🟠 shell |
| UI-POLISH01 | 🟡 partiellement réalisé, passe finale future |
| UI-QA01 | ⬜ jalon final dédié non réalisé |

---

# 18 — Main Menu et Startup Flow

## 18.1 — Frontend

- `L_MainMenu` = frontend.
- New Game ;
- Continue ;
- Load ;
- Options MVP ;
- Credits ;
- Quit.

## 18.2 — New Game

STARTUP-FLOW01 :

- Character Creation se déroule dans le frontend ;
- le donjon n’est chargé qu’après completion.

STARTUP-FLOW02 :

- barre de progression de construction/chargement restaurée ;
- transition vers `L_Dungeon`.

## 18.3 — Runtime map

- `L_Dungeon` = map runtime canonique.
- `Old_Tunnels` = niveau/map utilisé pour certains tests/pits selon contenu courant.

---

# 19 — Présentation : audio, VFX, lumière, matériaux

## 19.1 — Audio

- object audio events data-driven ;
- door audio ;
- generic mechanism activation ;
- monster audio component ;
- footsteps/blocked/pit ;
- combat audio.
- ✅ CPP-CLEAN01 a supprimé l’ancienne migration/fallback audio Door.

## 19.2 — VFX

- Monster VFX ;
- Death/dissolve ;
- attack presentation ;
- spells ;
- item sparkle ;
- téléportation/relocation presentation selon assets.

## 19.3 — Lumière

- item light data-driven ;
- party illumination dérivée de l’équipement ;
- world-object light ;
- point light authority nettoyée.

## 19.4 — Matériaux

- Static Mesh material slots autoritaires après MATERIAL-OWNERSHIP01 ;
- overrides runtime uniquement lorsqu’ils ont une sémantique explicite ;
- master materials documentés dans le pipeline art.

---

# 20 — Art pipeline / Content

## 20.1 — Static Mesh

Pipeline Blender → UE5 documenté :

- Apply Transform ;
- UV ;
- normals/smoothing ;
- FBX ;
- import UE5 ;
- collision ;
- material slots.

## 20.2 — Textures

Pipeline :

- Base Color ;
- Normal ;
- ORM ;
- compression ;
- settings UE5 ;
- material instances.

## 20.3 — UI assets

- organisation UI nettoyée ;
- icônes dédiées ;
- suppression d’assets UI obsolètes ;
- police UI revue fin septembre ;
- 🟡 harmonisation finale dans UI-POLISH01.

---

# 21 — Tests, build et packaging

## 21.1 — Harness local

`Scripts/ValidateUE.ps1`

- build Development Editor ;
- AutomationFilter ;
- rapports sous `Saved/Automation/TD04`.

## 21.2 — Packaging

`Scripts/ValidatePackage.ps1`

- Win64 Shipping ;
- build ;
- cook ;
- stage ;
- package ;
- pak ;
- archive.

## 21.3 — Baseline actuelle connue

Après CPP-CLEAN06.1 :

```text
Grimrock
Succeeded               : 964
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code        : 0
```

- ✅ `rg 'UE_LOG(LogTemp' Source` ne retourne aucun appel réel.
- ⚠️ Le HEAD `85951ce0ffaa3ad79c4e767db729b92922e2d501` ajoute ensuite uniquement deux assets binaires ; aucune nouvelle validation globale n’est documentée pour ces deux changements.

## 21.4 — Politique projet

- un test n’est déclaré validé qu’avec sortie UE fournie ;
- tests ciblés puis global si changement significatif ;
- pas de modification aveugle des `.uasset`.

---

# 22 — Audits et dette technique

## 22.1 — TD05 / RuntimeActor

- diagnostics extraits ;
- feedback extrait ;
- monsters séparés ;
- stop condition atteinte.
- 🟢 Pas de découpage supplémentaire sans signal concret.

## 22.2 — TD06 / PartyInventory

- Hotbar ;
- CursorTransfer ;
- Equipment ;
- Registry/Rehydration ;
- diagnostics ;
- stop condition atteinte.
- 🟢 Autorité PartyInventory conservée.

## 22.3 — TD07 / Current Schema

- build reproducibility ;
- compat UE ;
- data model reset ;
- save exact-match ;
- character authority ;
- derived stats/resources ;
- weight ;
- XP/Level ;
- skills ;
- spellbook ;
- status effects ;
- notifications ;
- authoring identity ;
- combat schema ;
- legacy purge ;
- asset repair ;
- strict schema ;
- stop condition atteinte.

## 22.4 — CPP-AUDIT01 / CPP-CLEAN01…06

✅ Audit architectural général clos.

- CPP-CLEAN01 : legacy Door Audio migration supprimée.
- CPP-CLEAN02 : Item Definition authority durcie.
- CPP-CLEAN03 : `ExplicitlyRejected` et `CanHear()` legacy supprimés.
- CPP-CLEAN04 : hard-coded asset fallbacks supprimés.
- CPP-CLEAN05 : **502 LogTemp réels → 0** ; catégories par domaine.
- CPP-CLEAN06 : deux stale test friends supprimés.
  - 49 déclarations friend de tests restantes ;
  - toutes réellement utilisées ;
  - dette acceptée/opportuniste.

## 22.5 — Ce qu’il ne faut pas refactorer maintenant

- ne pas éclater `AGridLevelRuntimeActor` uniquement pour la taille ;
- ne pas éclater `UGridPartyInventoryComponent` uniquement pour la taille ;
- ne pas rendre publics des internals uniquement pour supprimer des friends de tests ;
- ne pas introduire Asset Manager/preload complexe sans hitch mesuré ;
- ne pas encapsuler brutalement tout `PartyInventoryState` ;
- ne pas créer une deuxième autorité UI/Quest/Map/Codex.

---

# 23 — Performance et chargements

## 23.1 — LoadSynchronous

Des chemins de présentation utilisent encore `LoadSynchronous()` :

- inventory icons ;
- world item mesh/presentation ;
- spells ;
- monster audio/VFX/death/idle ;
- player attack presentation ;
- editor preview.

## 23.2 — Décision

- 🟡 risque potentiel de hitch au premier usage ;
- ⏸️ `CPP-PERF01 — Presentation Asset Preload` n’est pas lancé ;
- 🎯 profiler d’abord ;
- seulement si hitch mesuré : preload/cache ciblé.

## 23.3 — Tick / world scans

- pas de problème global identifié ;
- mechanisms activent généralement Tick seulement pendant animation/action ;
- PartyPawn tick naturel ;
- pas de `GetAllActorsOfClass` production massif documenté ;
- éviter nouveaux scans monde dans Tick.

---

# 24 — Roadmap actuelle

## 24.1 — Clos / stable

- ✅ WORLDOBJ-MIG00 → MIG10.
- ✅ MON13 → MON20.
- ✅ MON21.1.
- ✅ MON21.2.
- ✅ MON21.3.
- ✅ TD05/TD06 stop conditions.
- ✅ TD07 campagne current-schema.
- ✅ TEST-AUDIT01/02.
- ✅ CPP-AUDIT01 + CLEAN01…06.
- ✅ grande majorité de la refonte Inventory/Character/Persistent HUD.

## 24.2 — Volontairement reporté

- ⏸️ MON21.4 Quest Persistence.
- ⏸️ Journal métier.
- ⏸️ Map métier.
- ⏸️ Codex métier.
- ⏸️ Craft/Recipes métier.

## 24.3 — UI à terminer avant clôture de la refonte

Ordre recommandé :

1. 🎯 UI-FEEDBACK01.2 — finaliser/acter les Status Effects portraits.
2. 🎯 UI-POLISH01 — passe artistique cohérente.
3. 🎯 UI-QA01 — DPI / 16:9 / ultrawide / résolutions / interactions.
4. ✅ clôture globale de la refonte UI.

## 24.4 — Ensuite

- contenu : bestiaire, équipements, sorts, environnements ;
- fonctionnalités différées : Quest/Journal/Map/Codex/Craft ;
- MON22 : vertical slice 45–90 minutes ;
- performance/packaging final ;
- éditeur standalone / publication de niveaux joueurs.

---

# 25 — Invariants à protéger

1. **Un LevelAsset reste l’autorité d’authoring d’un niveau.**
2. **DungeonAsset orchestre plusieurs niveaux sans dupliquer leur contenu.**
3. **WorldObjectDefinition = définition réutilisable ; WorldObjectInstance = placement/config locale.**
4. **Typed placement arrays du LevelAsset sont autoritaires.**
5. **Event → Command reste le bus central.**
6. **Logic et Lua reviennent vers Command.**
7. **PartyInventoryState reste l’autorité du groupe/inventaire.**
8. **TurnManager reste l’autorité combat.**
9. **QuestSubsystem reste l’autorité Quest.**
10. **UI ne calcule pas une seconde vérité gameplay.**
11. **Save current-schema exact-match uniquement.**
12. **Les données dérivées sont reconstruites plutôt que persistées.**
13. **Aucun fallback asset hard-codé ne doit redevenir une seconde configuration.**
14. **Pas de LogTemp générique : catégories de domaine.**
15. **Refactor uniquement sur preuve, avec caractérisation + stop condition.**

---

# 26 — Références documentaires canoniques

## Architecture

- `docs/Architecture/PROJECT_SYNTHESIS.md`
- `docs/Architecture/ARCHITECTURE_INDEX.md`
- `docs/Architecture/TECHNICAL_DEBT_REGISTER.md`
- `docs/Architecture/CPP_GENERAL_AUDIT_2026_09_27.md`
- `docs/Architecture/WORLDOBJ_MIG10_FINAL.md`
- `docs/Architecture/WORLD_OBJECT_DEFINITIONS_AND_PLACED_OBJECTS.md`
- `docs/Architecture/SAVE_PERSISTENCE_FOUNDATION.md`

## Design / état fonctionnel

- `docs/Design/PROJECT_COMPLETION_ROADMAP.md`
- `docs/Design/99_DECISIONS_LOG.md`
- `docs/Design/UI_ARCHITECTURE_CURRENT.md`
- `docs/Design/COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md`
- `docs/Design/MONSTERS_AI_ANIMATIONS_TURN_BASED_COMBAT_RAT_GIANT.md`
- `docs/Design/MON21_1_QUESTS_JOURNAL_MAP_CODEX_ARCHITECTURE_AUDIT.md`

## Validation

- `docs/SCRIPTS_REFERENCE.md`
- `docs/Design/TD04_2_LOCAL_UE_VALIDATION_HARNESS.md`
- `docs/Design/TD04_3_COOK_PACKAGE_VALIDATION.md`
- `docs/Design/TEST_AUDIT01_FULL_AUTOMATION_REGRESSION_CLEANUP.md`
- `docs/Design/TEST_AUDIT02_AUTOMATION_WARNING_LEGACY_RELEVANCE_AUDIT.md`

## Cartographie

- **présent fichier** : carte textuelle détaillée et courante ;
- `Grimrock_MindMap_Architecture_Cible_v2_XMind.md` : arbre profond importable/lisible comme mindmap ;
- `GRIMROCK_PROJECT_MAP_MERMAID.md` : vues visuelles par domaine.
