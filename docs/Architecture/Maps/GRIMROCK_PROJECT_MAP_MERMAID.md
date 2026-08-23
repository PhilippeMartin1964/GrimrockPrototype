# GrimrockPrototype — Cartes d’architecture Mermaid

> Vue visuelle de synthèse. La source détaillée et autoritaire reste
> [`GRIMROCK_PROJECT_MAP.md`](GRIMROCK_PROJECT_MAP.md).
> État : 23 août 2026, après validation de MON20.3 et avant MON20.4.

## Légende

- ✅ implémenté / validé dans le périmètre indiqué
- 🟡 partiel ou à généraliser
- ⬜ à faire
- ⚠️ dette / risque
- 🎯 priorité immédiate

## 1. Carte globale

```mermaid
mindmap
  root((GrimrockPrototype))
    Core & Data
      ✅ DungeonAsset / LevelAsset
      ✅ grille 32x32
      ✅ identités stables
      ✅ architecture data-driven
    Grid Editor
      ✅ cellules / murs / objets
      ✅ palette / inspecteur
      ✅ liens / validation / preview
      ✅ patrol routes / Lua authoring
    Runtime Dungeon
      ✅ exploration case par case
      ✅ interactions / mécanismes
      ✅ multi-level / transitions
      ⚠️ GridLevelRuntimeActor central
    Logic & Scripting
      ✅ Event -> Command
      ✅ variables Bool / Int32
      ✅ Logic nodes
      ✅ Lua sandboxé / persistent / LogicId
    Party & RPG
      ✅ création de personnage
      ✅ inventaire / équipement
      ✅ XP / niveaux / progression
      ✅ CharacterPool / recrutement fondation
      🎯 MON20.4 Recruitment UI
    Combat
      ✅ initiative / rounds
      ✅ PA / PAM
      ✅ catalogue d’actions / hotbar
      ✅ ciblage / coûts / cooldowns
    Monsters & AI
      ✅ Rat géant
      ✅ Gobelin lanceur
      ✅ perception / dormance
      ✅ patrouille / investigation / alarmes
      🟡 bestiaire à densifier
    Magic & Status
      ✅ Status Effects MON16
      ✅ Spellbook MON18
      ✅ 4 sorts de production
      ✅ persistance
    Save & UI
      ✅ SaveGame v7
      ✅ menus / inventaire / combat
      ✅ Level Up / Spellbook
      ⬜ Quests / Journal / Map / Codex métier
    Production
      ⬜ Skills / Talents complets
      ⬜ Vertical Slice 45-90 min
      ⬜ CI / Shipping
      ⬜ éditeur joueur / partage
```

## 2. Flux de données et runtime

```mermaid
flowchart LR
    DA[DataAssets\nDungeon / Level / RPG / Monsters / Spells] --> ED[Grid Editor]
    ED --> LA[UGridLevelAsset]
    LA --> RT[AGridLevelRuntimeActor]
    RT --> ACT[Actors runtime]
    RT --> GRID[Grille autoritaire]
    ACT --> EVT[Events]
    EVT --> CMD[Event -> Command]
    EVT --> LOGIC[Logic nodes]
    EVT --> LUA[Lua sandboxé]
    LOGIC --> CMD
    LUA --> CMD
    CMD --> ACT
    RT --> STATE[FGridDungeonRuntimeState]
    PARTY[FGridPartyInventoryState] --> SAVE[UGrimrockPartySaveGame v7]
    STATE --> SAVE
    SAVE --> RT
    SAVE --> PARTY
```

## 3. Combat, monstres et RPG

```mermaid
flowchart TB
    PARTY[Party / Character states] --> INIT[Initiative globale]
    MON[Monster states / AI] --> INIT
    INIT --> TURN[Tour actif]
    TURN --> CAT[Catalogue d’actions]
    CAT --> TARGET[Ciblage grille]
    TARGET --> TX[Transaction PA / mana / items / cooldown]
    TX --> RES[Résolution déterministe]
    RES --> STATUS[Status Effects]
    RES --> HP[PV / mort / victoire]
    RES --> PRESENT[HUD / Anim / Audio / VFX]
    HP --> XP[XP / Level Progression]
    XP --> CHOICE[ProgressionChoices / prérequis]
    SPELL[Spellbook] --> CAT
    EQUIP[Équipement] --> CAT
```

## 4. Recrutement MON20

```mermaid
flowchart LR
    ASSET[URPGStoryCompanionAsset] --> REG[StoryCompanionService]
    REG --> POOL[CharacterPool]
    POOL --> RECRUIT[RecruitmentService]
    RECRUIT --> ACTIVE[ActiveCharacters max 6]
    ACTIVE --> INV[Inventory ownership / equipment / hotbar]
    ACTIVE --> SAVE[SaveGame v7]
    POOL --> SAVE
    UI[🎯 MON20.4 Recruitment UI] --> REG
    UI --> RECRUIT
```

## 5. Roadmap immédiate

```mermaid
flowchart LR
    A[✅ MON20.1 Audit] --> B[✅ MON20.2 Recruitment Foundation 6/6]
    B --> C[✅ MON20.3 Story Companion 6/6]
    C --> D[🎯 MON20.4 Recruitment UI]
    D --> E[⬜ Skills / Talents / Reserve / Regression]
    E --> F[⬜ MON21 Quests / Journal / Map / Codex]
    F --> G[⬜ MON22 Vertical Slice 45-90 min]
```
