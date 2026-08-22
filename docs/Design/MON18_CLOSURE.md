# MON18 — Magic & Spellbook — Closure

Date : **22 août 2026**  
Statut : **VALIDÉ ET CLOS sous Unreal Engine 5.5.4**

## 1. Objectif du jalon

MON18 transforme l'infrastructure de sorts déjà présente dans le combat MON12 en un système de magie RPG complet, sans créer de second pipeline parallèle.

Architecture finale :

```text
Spell Definition / SpellId
        ↓
Spellbook par personnage
        ↓
WBP_GridSpellbook
        ↓ drag/drop
Hotbar MON12 — 10 slots persistants
        ↓
Combat Action Catalog
        ↓
Targeting MON18.4
        ↓
Transaction PA / mana MON18.3
        ↓
Effects MON18.5 / Status Effects MON16
        ↓
Commit runtime autoritaire
        ↓
Presentation MON18.6
```

La persistance repose sur `UGrimrockPartySaveGame` version 6 et conserve uniquement des identités stables pour le Spellbook.

## 2. Sous-jalons clos

```text
MON18.1 — Spell Data Model & Cast Contract      CLOS
MON18.2 — Spell Knowledge / Spellbook           CLOS
MON18.3 — Runtime Casting / Cost Transaction    CLOS
MON18.4 — Targeting Integration                 CLOS
MON18.5 — First Production Spells               CLOS
MON18.6 — Spell Presentation                    CLOS
MON18.7 — Spellbook / Hotbar UI                 CLOS
MON18.8 — Persistence / Migration               CLOS
MON18.9 — Balance / Regression / Closure        CLOS
  MON18.9.1 — Combat Save Policy / Checkpoint   CLOS
  MON18.9.2 — Spell Balance / Cross-System      CLOS
  MON18.9.3 — Final Diagnostics / Global Tests  CLOS
```

## 3. Modèle de données et connaissance des sorts

Le Spellbook est une autorité distincte de la hotbar.

Principes :

- un sort possède une identité stable `SpellId` ;
- la connaissance appartient au personnage ;
- la hotbar ne peut jamais enseigner un sort ;
- la hotbar conserve uniquement une référence vers un sort connu ;
- une définition absente ne détruit pas automatiquement l'identité persistée ;
- aucune duplication d'un système parallèle de status/capability n'est autorisée.

Le runtime natif est porté par `UGridPartySpellbookComponent`.

## 4. Exécution transactionnelle

Un cast suit un modèle atomique :

```text
Validate
  ↓
Resolve target
  ↓
Compute costs on copies
  ↓
Resolve effects on copies
  ↓
Useful mutation ?
  ├─ no  -> reject / no PA / no mana
  └─ yes -> commit authoritative state
```

Les erreurs de définition, cible, mana, status manquant ou effet inutile n'entraînent pas de coût partiel.

MON18.9.2 ajoute explicitement le rejet :

```text
NoEffectWouldApply
```

pour les cas tels que :

- `Lesser Heal` sur une cible déjà à PV maximum ;
- `Cure Poison` sur une cible sans `Status_Poison`.

## 5. Quatre sorts de production

Baseline initiale gelée :

```text
Spell_ArcaneBolt
  Damage      4
  Mana        3
  PA          2
  Range       1..5
  Cooldown    0

Spell_LesserHeal
  Heal        5
  Mana        4
  PA          2
  Range       0..3
  Cooldown    0

Spell_Haste
  Apply       Status_Haste
  Mana        5
  PA          2
  Range       0..3
  Cooldown    0

Spell_CurePoison
  Remove      Status_Poison
  Mana        4
  PA          2
  Range       0..3
  Cooldown    0
```

Ces valeurs constituent une baseline testée, pas une promesse d'équilibrage définitif du jeu final.

## 6. Réutilisation de MON16

`Haste` et `Cure Poison` réutilisent le système générique de Status Effects MON16.

Aucun branchement spécial par nom de sort ou `EffectId` n'est créé dans le combat. L'identité canonique d'un Status Effect reste celle de son Primary Asset.

La régression détectée pendant MON18.8 dans `MON16.5.NoParallelSystem` a été corrigée par le commit :

```text
d25cf26e0d7c052fe30ba772e50e02b7debaa918
Fix MON16.5 status identity regression
```

## 7. UI Spellbook / Hotbar

L'onglet Sorts du menu joueur est intégré au contrat C++ de `GrimrockMenuWidget`.

Les sorts connus sont affichés dans `WBP_GridSpellbook`, puis peuvent être :

- glissés vers l'un des dix slots MON12 ;
- déplacés ou échangés ;
- désassignés ;
- exécutés au clic ou au raccourci clavier selon le contrat hotbar existant.

L'UI ne devient jamais autorité de gameplay.

## 8. Présentation

La présentation des sorts est séparée du resolver de gameplay :

- audio et VFX data-driven ;
- projectile visuel réutilisant l'infrastructure existante ;
- dégâts/soins/status déterminés avant la présentation ;
- aucune collision projectile ne devient source autoritaire de dégâts.

## 9. Persistance — SaveGame version 6

MON18.8 introduit :

```text
FGridCharacterSpellbookSaveState
  CharacterId
  KnownSpellIds[]
```

Le snapshot est sparse et déterministe.

Le SaveGame ne persiste pas :

- widgets ;
- acteurs runtime ;
- pointeurs de définitions ;
- audio/VFX ;
- état de présentation.

Migration v5 -> v6 :

- crée un Spellbook vide ;
- n'invente aucun sort ;
- conserve les anciens contrats de progression ;
- restaure atomiquement par `CharacterId`.

PIE validé :

```text
Seed initial                Added=4 AlreadyKnown=0 TotalProduction=4
Save                        Version=6 Spellbooks=1
Stop PIE -> Continue        SpellbookCharacters=1
Seed après Continue         Added=0 AlreadyKnown=4 TotalProduction=4
```

## 10. Politique de sauvegarde en combat

MON18.9.1 fixe le contrat suivant :

```text
Exploration / Victory      -> sauvegarde autorisée
Combat / Defeat            -> sauvegarde régulière interdite
Engagement automatique     -> checkpoint pré-combat obligatoire
Checkpoint                 -> <slot courant>_AutoCombat
```

Le SaveGame ne persiste donc pas l'état transitoire du scheduler de combat :

- initiative courante ;
- round actif ;
- combattant actif ;
- PA du tour ;
- action/ciblage en cours ;
- projectile/animation transitoire.

Un `EndPlay` pendant le combat ne peut pas écraser la sauvegarde principale.

Le PIE final confirme :

```text
PartySave Saved Slot=GrimrockParty_AutoCombat ...
PreCombatCheckpoint Saved ...
Automatic combat started ... Checkpoint=Saved
PartySave SaveRejected Slot=GrimrockParty Reason=CombatStateNotSaveable
```

## 11. Diagnostics SaveGame résiduels

MON18.9.3 identifie désormais le slot exact lorsqu'un ancien SaveGame est incompatible.

Le PIE final a isolé :

```text
Slot=GrimrockParty_2
Reason=IncompatibleSave
Detail=Le snapshot contient 0 états de progression pour 1 personnages actifs.
```

`GrimrockParty` reste chargeable et `Continue` fonctionne normalement.

Aucune suppression automatique des anciens slots n'est effectuée.

## 12. Validation finale

### Automation MON18.9.3

```text
CheckpointIsolation   Success
SaveSlotDiagnostics   Success
```

**2/2 Success**.

### Campagne globale projet

```text
Automation RunTests Grimrock
221 tests terminés
221 Success
0 Fail
```

Aucun `Ensure condition failed`, `Assertion failed` ou fatal error n'a été relevé.

### Régressions majeures déjà validées pendant MON18

```text
MON18.1                         4/4 Success
MON18.2                         5/5 Success
MON18.3                         6/6 Success
MON18.4                         8/8 Success
MON18.5                         6/6 Success
MON18.6                         7/7 Success
MON18.8                        12/12 Success
UI01.4.3e.2                     6/6 Success
MON18.9.1                       6/6 Success
MON18.9.2                       5/5 Success
MON18.9.2 cross-system         51/51 Success
MON18.9.3                       2/2 Success
Global Grimrock              221/221 Success
```

Ces nombres correspondent à des campagnes partiellement recouvrantes ; ils ne doivent pas être additionnés comme un nombre de tests uniques du projet.

## 13. État architectural à la clôture

MON18 apporte un système complet et réutilisable :

```text
Data-driven Spell Definition
+ Spell Knowledge per Character
+ C++ Spellbook runtime
+ Spellbook UI
+ MON12 Hotbar integration
+ MON18 Targeting
+ atomic PA/Mana transaction
+ Damage / Heal / Status effects
+ presentation layer
+ SaveGame v6 migration and restore
+ combat-safe save policy
+ pre-combat checkpoint
+ final diagnostics and global regression coverage
```

Aucun second système de combat, status effects, inventaire ou hotbar n'a été créé.

## 14. Documents de référence

```text
MON18_1_SPELL_DATA_MODEL_CAST_CONTRACT.md
MON18_2_SPELL_KNOWLEDGE_SPELLBOOK.md
MON18_3_RUNTIME_CASTING_COST_TRANSACTION.md
MON18_4_TARGETING_INTEGRATION.md
MON18_5_FIRST_PRODUCTION_SPELLS.md
MON18_6_SPELL_PRESENTATION.md
MON18_7_SPELLBOOK_HOTBAR_UI.md
MON18_8_SPELLBOOK_PERSISTENCE_MIGRATION.md
MON18_8_VALIDATION.md
MON18_9_1_COMBAT_SAVE_POLICY.md
MON18_9_2_SPELL_BALANCE_CROSS_SYSTEM_REGRESSION.md
MON18_9_3_FINAL_DIAGNOSTICS_GLOBAL_REGRESSION.md
UI_SPELLBOOK_HOTBAR_EXECUTION.md
UI_GRIMROCK_MENU_CURRENT.md
```

## 15. Suite

Le prochain jalon autoritaire devient :

```text
MON19 — Advanced Dungeon Logic / Scripting
```

Objectif : permettre au level designer de construire des mécanismes et énigmes plus riches en s'appuyant sur l'architecture `Event -> Command` et l'asset de niveau, sans multiplier le C++ spécifique par puzzle.

---

**MON18 — Magic & Spellbook est VALIDÉ ET CLOS sous UE5.5.4.**
