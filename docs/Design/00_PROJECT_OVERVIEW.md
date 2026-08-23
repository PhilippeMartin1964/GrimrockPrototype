# GrimrockPrototype — Vue d’ensemble du projet

## Objectif

GrimrockPrototype est un dungeon crawler Unreal Engine 5.5.4 en C++ inspiré de *Legend of Grimrock 2*.

Le projet vise :

- vue subjective ;
- déplacement case par case sur grille ;
- rotation à 90° ;
- édition directe des niveaux ;
- objets interactifs et connecteurs Event -> Command ;
- logique de donjon avancée data-driven avec variables, nœuds Logic et Lua sandboxé ;
- monstres data-driven ;
- combat tactique au tour par tour ;
- progression RPG persistante ;
- magie et Spellbook intégrés au combat ;
- à terme, création et partage de niveaux par les joueurs.

L'architecture reste simple, modulaire et orientée données. Les DataAssets et l'état de grille sont les autorités logiques ; Actors, animations, VFX et widgets sont des projections runtime ou de présentation.

---

## État actuel — 23 août 2026

Les fondations suivantes sont disponibles :

- donjons multi-niveaux et `UGridLevelAsset` ;
- Grid Editor intégré avec palette, inspecteur, connecteurs, validation et authoring Lua ;
- déplacement et interaction en vue subjective ;
- items, inventaire, équipement, réceptacles, portes, serrures et passages secrets ;
- création de personnage, races/classes et statistiques dérivées ;
- sauvegarde/Continue et migrations explicites ;
- combat MON1–MON12 à initiative globale, PA/PAM et hotbar 0–9 ;
- pipeline `MonsterSpawn` MON13 avec rencontres, vagues et persistance ;
- exploration IA MON14 avec vision directionnelle, dormance, patrouille, investigation et alarme locale ;
- progression XP/niveaux MON15 ;
- effets de statut MON16 communs groupe/monstres ;
- seconde famille de monstres MON17 avec Gobelin lanceur et profil tactique `RangedKeeper` ;
- magie MON18 avec Spellbook, ciblage, transactions PA/mana, quatre sorts de production, présentation, hotbar et checkpoint pré-combat ;
- logique avancée MON19 avec variables persistantes, conditions, primitives Logic, Lua sandboxé, `persistent`, `LogicId`, pont Event -> Lua -> Command et puzzles de production.

Les jalons majeurs suivants sont **validés et clos** :

```text
MON13 — Monster Spawn / Encounter / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
MON16 — Status Effects
MON17 — Second Monster Family
MON18 — Magic & Spellbook
MON19 — Advanced Dungeon Logic / Scripting
```

Le prochain jalon autoritaire est :

```text
MON20 — Recruitment / Skills / Talents
```

Le backlog actif autoritaire est :

```text
docs/Design/PROJECT_COMPLETION_ROADMAP.md
```

---

## Combat tactique — MON12

Le combat utilise initiative globale, tours individuels, PA personnels, PAM communs, catalogue générique d’actions, HUD orienté actions, dix raccourcis persistants, quick items, ciblage, transaction de ressources et cooldowns.

Référence : `COMBAT_SYSTEM_V2_ACTION_POINTS_INITIATIVE.md`.

---

## Monstres — MON13 / MON14 / MON17

Le pipeline de contenu reste :

```text
MonsterSpawn persistant
    -> MonsterDefinition
    -> Actor runtime
    -> état persistant par SpawnId
```

MON13 gère placement, lifecycle, encounters, vagues et persistance. MON14 apporte vision directionnelle, engagement automatique, dormance, patrouille, investigation et alarme. MON17 prouve la généricité avec `MON_GoblinThrower`, attaque projectile et profil `RangedKeeper`.

Références : `MON14_CLOSURE.md`, `MON17_CLOSURE.md`.

---

## Progression RPG — MON15 / MON16

MON15 fournit XP, niveaux, progression de classe et Level Up jusqu’au niveau 20. MON16 fournit le modèle générique de Status Effects avec durée, stacking, DoT, Haste/Slow, contrôles, HUD et Save/Restore.

Références : `MON15_CLOSURE.md`, `MON16_CLOSURE.md`.

---

## Magie & Spellbook — MON18

MON18 est validé et clos.

Pipeline :

```text
Spell Definition
    -> Spellbook par personnage
    -> WBP_GridSpellbook
    -> Hotbar MON12
    -> Targeting
    -> transaction PA/mana
    -> effets / Status Effects
    -> présentation
```

Sorts de production :

```text
Spell_ArcaneBolt
Spell_LesserHeal
Spell_Haste
Spell_CurePoison
```

La politique de sauvegarde refuse la sauvegarde régulière en combat et crée un checkpoint pré-combat avant engagement automatique.

Référence : `MON18_CLOSURE.md`.

---

## Advanced Dungeon Logic / Scripting — MON19

MON19 est **validé et clos**.

Hiérarchie finale :

```text
cas simple
    -> Event -> Command direct

état / compteur / comparaison
    -> variables persistantes + nœuds Logic

orchestration complexe
    -> Lua sandboxé
       -> grid.command(...)
       -> runtime existant
```

Capacités principales :

- conditions de liens typées ;
- variables persistantes `Bool` / `Int32` ;
- primitives Logic ;
- SaveGame / migration ;
- VM Lua embarquée, quotas mémoire/instructions et isolation ;
- pont Event -> Lua -> Command ;
- outils Lua dans le Grid Editor ;
- sandbox/packaging durcis ;
- déclaration `persistent = { ... }` ;
- `LogicId` lisible pour remplacer les GUID dans l’authoring Lua.

Validation finale :

```text
Compilation UE5.5.4          OK
Grimrock.MON19.8             4/4 Success
Grimrock.MON19              55/55 Success
PIE puzzle représentatif     VALIDÉ
```

Le puzzle PIE final confirme : premier clic `RuneCount=1` porte fermée ; second clic `RuneCount=2` puis `SecretDoor` ouverte.

Référence : `MON19_CLOSURE.md`.

---

## Persistance

Le SaveGame a évolué avec les jalons RPG et logique de donjon. La migration MON19 couvre les variables de niveau ; les tests de régression montrent un chargement au contrat courant de version 7.

La VM Lua elle-même n’est pas sérialisée : l’état durable reste dans les données/runtime persistants.

---

## Prochaine phase — MON20 à MON22

Ordre actif :

```text
MON20 — Recruitment / Skills / Talents
MON21 — Quests / Journal / Map / Codex
MON22 — 45–90 Minute Vertical Slice
```

MON20 commence par un audit de l’existant personnage/groupe/classes/statistiques/progression/actions/inventaire/UI/SaveGame avant définition de MON20.1.

Référence : `MON20_START.md`.

---

## Contraintes générales

- Unreal Engine : 5.5.4
- Langage : C++
- Dépôt : `GrimrockPrototype`
- Branche : `master`
- Architecture : data-first, classes C++ simples, comportement piloté par assets
- Grille cible : 32 × 32
- Cellule standard : 200 × 200 cm
- Hauteur de cellule : environ 300 cm

---

## Principes d'architecture

### Objets concrets, comportements factorisés

Des objets distincts dans la palette peuvent partager une même classe C++ si leur différence est portée par les données et la présentation.

### Event / Command / Link

Le modèle reste :

```text
SourceObjectId + SourceEvent -> TargetObjectId + TargetCommand
```

MON19 l’étend sans le remplacer : les nœuds Logic et Lua repassent toujours par le dispatcher et les commandes runtime normales.

### Mémoire stable

La connaissance durable du projet se trouve sous `docs/Design/`.

Documents autoritaires à maintenir :

```text
00_PROJECT_OVERVIEW.md
PROJECT_COMPLETION_ROADMAP.md
MON14_CLOSURE.md
MON15_CLOSURE.md
MON16_CLOSURE.md
MON17_CLOSURE.md
MON18_CLOSURE.md
MON19_CLOSURE.md
99_DECISIONS_LOG.md
```

---

## Règle de travail

Pour chaque sous-jalon :

1. auditer le code existant ;
2. documenter le contrat ;
3. modifier peu de fichiers ;
4. compiler sous UE5.5.4 ;
5. exécuter les Automation Tests dédiés et les régressions utiles ;
6. valider en PIE lorsque des assets/WBP sont impliqués ;
7. produire un commit Git clair ;
8. pousser sur `origin/master` ;
9. mettre à jour la documentation durable à la clôture.
