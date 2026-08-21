# GrimrockPrototype — Active Completion Roadmap

Statut : **MON18.4 implémenté — validation UE5.5.4 en attente**  
Date de référence : **21 août 2026**

Ce document est la feuille de route active et autoritaire du projet. `04_IMPLEMENTATION_ROADMAP.md` reste historique.

---

## 1. État de référence

Jalons majeurs clos :

```text
MON13 — Monster Spawn / Encounters / Persistence
MON14 — Automatic Engagement / Patrol / Investigation / Alarm
MON15 — XP & Level Progression
MON16 — Status Effects
MON17 — Second Monster Family / Gobelin lanceur
```

MON17.7 est **VALIDÉ ET CLOS sous UE5.5.4**.

MON18 est maintenant le jalon actif : **Magic & Spellbook**.

---

## 2. Ordre des grands jalons

```text
MON18 — Magic & Spellbook
        ↓
MON19 — Advanced Dungeon Logic / Scripting
        ↓
MON20 — Recruitment / Skills / Talents
        ↓
MON21 — Quests / Journal / Map / Codex
        ↓
MON22 — 45–90 Minute Vertical Slice
```

---

# MON18 — Magic & Spellbook — EN COURS

## Objectif

Construire un système RPG complet de magie en réutilisant les systèmes existants de combat, PA/mana, ciblage, hotbar, inventaire, effets de statut, projectiles, progression et sauvegarde.

Sous-jalons :

```text
MON18.1 — Spell Data Model & Cast Contract      CLOS
MON18.2 — Spell Knowledge / Spellbook           CLOS
MON18.3 — Runtime Casting / Cost Transaction    CLOS
MON18.4 — Targeting Integration                 EN VALIDATION
MON18.5 — First Production Spells
MON18.6 — Spell Presentation
MON18.7 — Spellbook / Hotbar UI
MON18.8 — Persistence / Migration
MON18.9 — Balance / Regression / Closure
```

### MON18.1 — CLOS

`MON18.1 — Spell Data Model & Cast Contract` est **VALIDÉ ET CLOS sous UE5.5.4**.

Contrats livrés :

- identité stable `SpellId` ;
- définition data-driven via `UGridSpellDefinitionAsset` ;
- coûts mana/PA ;
- portée/LOS ;
- réutilisation de `EGridCombatTargetingPolicy` ;
- effets déclaratifs `Damage`, `Heal`, `ApplyStatusEffect`, `RemoveStatusEffect` ;
- pont MON16 par `StatusEffectId` ;
- requête de cast sans pointeur d'acteur ;
- contrat pur sans consommation de ressource ni résolution runtime.

Validation automatisée :

```text
Grimrock.Magic.MON18.1.CastRequestValidation   Success
Grimrock.Magic.MON18.1.ContractIsPure          Success
Grimrock.Magic.MON18.1.DefinitionValidation    Success
Grimrock.Magic.MON18.1.StatusEffectBridge      Success
Total                                           4/4 Success
```

Référence :

```text
docs/Design/MON18_1_SPELL_DATA_MODEL_CAST_CONTRACT.md
```

### MON18.2 — CLOS

`MON18.2 — Spell Knowledge / Spellbook` est **VALIDÉ ET CLOS sous UE5.5.4**.

Contrats livrés :

- un Spellbook runtime distinct par `CharacterId` ;
- connaissance représentée uniquement par `SpellId` stable ;
- apprentissage / oubli explicites ;
- refus des doublons et des identités invalides ;
- isolation stricte entre personnages ;
- aucune consommation PA/mana ni exécution d'effet ;
- persistance volontairement différée à MON18.8.

Validation automatisée :

```text
Grimrock.Magic.MON18.2.CharacterIsolation     Success
Grimrock.Magic.MON18.2.CharacterRegistration  Success
Grimrock.Magic.MON18.2.LearnForget            Success
Grimrock.Magic.MON18.2.StableIdentity         Success
Grimrock.Magic.MON18.2.TransientContract      Success
Total                                          5/5 Success
```

Références :

```text
docs/Design/MON18_2_SPELL_KNOWLEDGE_SPELLBOOK.md
docs/Design/MON18_2_VALIDATION.md
```

### MON18.3 — CLOS

`MON18.3 — Runtime Casting / Cost Transaction` est **VALIDÉ ET CLOS sous UE5.5.4**.

Contrats livrés :

- réutilisation de `FRPGDerivedStats::CurrentMana` comme mana autoritaire ;
- réutilisation de `FGridPlayerCharacterTurnState::RemainingActionPoints` comme PA autoritaires ;
- validation du sort connu et des identités avant paiement ;
- transaction atomique PA + mana ;
- aucune mutation en cas d'échec ;
- reçu de coût uniquement après succès ;
- ciblage explicitement différé à MON18.4.

Validation automatisée :

```text
Grimrock.Magic.MON18.3.IdentityMismatchNoMutation          Success
Grimrock.Magic.MON18.3.InsufficientActionPointsNoMutation  Success
Grimrock.Magic.MON18.3.InsufficientManaNoMutation          Success
Grimrock.Magic.MON18.3.SuccessfulCommit                    Success
Grimrock.Magic.MON18.3.TargetingDeferred                   Success
Grimrock.Magic.MON18.3.UnknownSpellNoMutation              Success
Total                                                       6/6 Success
```

Références :

```text
docs/Design/MON18_3_RUNTIME_CASTING_COST_TRANSACTION.md
docs/Design/MON18_3_VALIDATION.md
```

### MON18.4 — EN VALIDATION

`FGridSpellTargetingService` réutilise `EGridCombatTargetingPolicy` et valide `Self`, `Ally`, `FirstAxialTarget`, `Cell` et `Area`.

Le contrat vérifie :

- identité de cible résolue ;
- relation alliée/hostile selon la politique ;
- portée `MinRangeCells..MaxRangeCells` ;
- alignement axial pour `FirstAxialTarget` ;
- résultat LOS fourni par la couche grille runtime autoritaire lorsque `bRequiresLineOfSight` est actif.

`FGridSpellCastPipelineService` impose l'ordre :

```text
Targeting validation / resolution
-> si échec : zéro mutation
-> MON18.3 cost transaction
-> si échec : zéro mutation
-> resolved target + cost receipt
```

Aucun effet de sort n'est encore appliqué ; cette responsabilité commence avec MON18.5.

Référence :

```text
docs/Design/MON18_4_TARGETING_INTEGRATION.md
```

---

# MON19 — Advanced Dungeon Logic / Scripting

Objectif : permettre au level designer de construire des énigmes riches sans ajouter du C++ spécifique.

---

# MON20 — Recruitment / Skills / Talents

Objectif : enrichir le groupe avec recrutement, compétences, talents et spécialisations.

---

# MON21 — Quests / Journal / Map / Codex

Objectif : structurer la campagne avec quêtes, journal, carte et codex.

---

# MON22 — 45–90 Minute Vertical Slice

Objectif : construire un jeu testable de bout en bout avant la phase de production étendue.

---

# Horizon MON23+

```text
MON23 — Containers / Lock Traps / Crafting
MON24 — Production Audio / VFX / Atmosphere
MON25 — Menus / Options / Accessibility
MON26 — Performance / Optimization
MON27 — Packaging / Shipping / Installer
MON28 — Standalone Player Level Editor
MON29 — Dungeon Publication / Validation / Sharing
MON30 — Full Campaign
```

---

## Règles de conduite

1. Un sous-jalon doit être petit, compilable et testable.
2. Travail sur `master`, sans branche de fonctionnalité.
3. Un commit logique par sous-jalon ou étape de clôture.
4. Pousser sur `origin/master` après chaque étape validée.
5. Aucun refactor massif préventif.
6. Réutiliser les systèmes existants avant d'ajouter une abstraction parallèle.
7. Les tests C++ valident la logique ; assets/WBP/maps exigent une validation UE/PIE lorsqu'ils sont impliqués.
8. À la clôture d'un jalon majeur, mettre à jour overview, roadmap et document de clôture.

---

## Prochain travail autoritaire

```text
Valider MON18.4 sous UE5.5.4, puis MON18.5 — First Production Spells
```
