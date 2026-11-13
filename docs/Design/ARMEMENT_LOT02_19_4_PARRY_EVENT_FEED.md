# ARMEMENT-LOT02.19.4 — Parade ARM013 : restitution dans le flux d'événements

> **ARM-CLEAN06 — ARCHITECTURE CIBLE APPROUVÉE (13.11.2026).** [Contrat](../Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), [migration](../Architecture/ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques/tests historiques demeurent ; les anciens schémas DataAsset sont remplacés. **Non implémentée : zéro legacy dans le runtime final.**


**Date : 11 novembre 2026. Statut : correctif C++ et tests versionnés ; compilation UE5.5.4 et recette PIE locales à confirmer.**

## Constat et décision

ARMEMENT-LOT02.19.3 est validé en PIE pour la **mécanique** : la Main-Gauche ARM013 pare réellement les coups admissibles ; l'Output Log confirme les réactions et le bouclier conserve sa consommation de protection existante. Toutefois le flux d'événements affichait `ÉCHEC` sur une parade, car `bHit=false` y était interprété comme un raté ordinaire ; le tooltip structuré recalculait aussi `Touché` à partir du jet initial, sans tenir compte de la parade après jet.

Le résultat autoritaire `FGridAttackResult::bEquipmentParried` prévaut sur le seul booléen `bHit` **pour la présentation**. Aucune règle d'attaque, protection, esquive, probabilité, dégât, statut, initiative ou coût d'action n'est modifiée.

## Projection autoritaire

- `FGridCombatEventFeedFormatter::BuildEntry` reconnaît la parade comme `Combat.Attack.Parried`, catégorie Combat, sémantique Information. L'en-tête affiche `PARADE` et non `ÉCHEC`.
- Avec une trace valide, le texte compact **garde le jet d'attaque et la défense** puis annonce que la cible pare avec sa Main-Gauche et ne subit aucun dégât. Il ne crée aucune ligne de dégâts fictifs.
- Les détails génériques contiennent le résultat de parade. Le tooltip structuré `WBP_GridCombatTooltip` reçoit `bEquipmentParried` depuis le code d'événement, affiche une issue de parade au lieu de recalculer `Touché`, conserve le jet historique et utilise le champ `Text_Status` déjà présent pour expliciter l'absence de dégâts.
- Le journal `GridCombatLog.cpp` reste inchangé : il connaissait déjà la parade. `AppendCombatLogEntry` conserve **une seule entrée combat et un seul événement feed** par attaque ; aucun doublon réactif n'est créé.
- Les nouveaux textes dynamiques sont des `LOCTEXT`/`NSLOCTEXT` C++ : aucun nouveau `ST_UI`, `UI.csv` ou `WBP` n'est requis. Prévoir **gather et traduction EN du target Game** avant validation anglaise.

## Tests et recette

Couverture ajoutée dans `Grimrock.UI.EventHUD01.CombatRendering.MainGaucheParryProjection` : parade d'un coup qui avait touché, conservation du d20, dégâts nuls, code et en-tête dédiés, message compact, détails génériques, infobulle structurée cohérente, absence de protection fictive. `CentralPublication` vérifie qu'un coup ordinaire suivi d'une parade produit exactement **deux entrées** (une par attaque).

Exécuter localement :

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EventHUD01.CombatRendering"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT.LOT02.19.3"
```

Recette PIE : Ombre équipé ARM013 OffHand, `MainGaucheParryChancePercent=100` provisoirement, monstre physique de mêlée non critique. Vérifier **un** événement `PARADE` et `Main-Gauche` par parade, le jet détaillé sans faux `ÉCHEC`/`TOUCHÉ`, zéro perte de protection et PV, et aucune modification du blocage bouclier. Remettre la valeur à **15 %**.
