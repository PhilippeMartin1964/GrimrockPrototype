# MON18.5 — First Production Spells

Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
Date : **21 août 2026**

## Objectif

Passer du contrat générique MON18.1–MON18.4 à de premiers sorts réellement résolus côté gameplay, sans introduire de présentation VFX/audio (MON18.6) ni de `.uasset` obligatoire.

## Premiers sorts canoniques

`FGridProductionSpellLibrary` fournit quatre définitions de référence :

```text
Spell_ArcaneBolt   Arcane  — Damage 4 — Mana 3 — PA 2 — portée 1..5 — FirstAxialTarget
Spell_LesserHeal   Life    — Heal 5   — Mana 4 — PA 2 — portée 0..3 — Ally
Spell_Haste        Air     — Apply Status_Haste  — Mana 5 — PA 2 — portée 0..3 — Ally
Spell_CurePoison   Life    — Remove Status_Poison — Mana 4 — PA 2 — portée 0..3 — Ally
```

Les quatre définitions utilisent le contrat `FGridSpellDefinition`; elles ne créent aucune seconde hiérarchie de sorts.

## Resolver d'effets

`FGridSpellEffectResolver` exécute les quatre effets déclarés par MON18.1 :

- `Damage` : réduit la santé sans descendre sous zéro ;
- `Heal` : restaure la santé sans dépasser le maximum ;
- `ApplyStatusEffect` : résout `StatusEffectId` puis appelle directement `FGridStatusEffectCollection::TryApply` de MON16 ;
- `RemoveStatusEffect` : retire l'effet MON16 portant l'identité stable demandée.

Le resolver propose également `ResolveCharacterEffects` pour agir directement sur `FRPGDerivedStats::CurrentHealth/MaxHealth` et la collection de statuts du personnage.

## Atomicité

L'ensemble des effets d'un même sort est résolu sur des copies de travail. Si une définition de Status Effect est absente/invalide ou si l'application MON16 échoue, **aucun effet partiel n'est commité**.

Cette propriété complète l'atomicité PA/mana de MON18.3 :

```text
MON18.4 : cible valide
MON18.3 : paiement atomique
MON18.5 : batch d'effets atomique
```

L'orchestration complète paiement + effets sera raccordée au runtime joueur à mesure que MON18.6/18.7 ajoutent présentation et commandes UI.

## Réutilisation MON16

MON18.5 ne duplique ni durée, ni stacking, ni puissance, ni lifecycle de statut. `Haste` passe par la définition `Status_Haste` existante/résolue par le runtime. `Cure Poison` retire `Status_Poison` par identité stable.

## Tests Automation

```text
Grimrock.Magic.MON18.5.ProductionDefinitions
Grimrock.Magic.MON18.5.DamageResolution
Grimrock.Magic.MON18.5.HealingClamp
Grimrock.Magic.MON18.5.ApplyStatusBridge
Grimrock.Magic.MON18.5.RemoveStatus
Grimrock.Magic.MON18.5.AtomicFailureNoMutation
```

Attendu : **6/6 Success**.

## Hors périmètre

- résistances/damage scaling avancés : équilibrage et extension ultérieure ;
- VFX, audio, projectile visuel : MON18.6 ;
- Spellbook/hotbar UI : MON18.7 ;
- persistance Spellbook : MON18.8.

Aucun `.uasset` n'est requis pour cette étape.
