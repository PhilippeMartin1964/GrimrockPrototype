# MON18.6 — Spell Presentation

Statut : **IMPLÉMENTÉ — VALIDATION UE5.5.4 EN ATTENTE**  
Date : **21 août 2026**

## Objectif

Ajouter une couche de présentation pour les sorts sans déplacer l'autorité gameplay hors des systèmes MON18.3–MON18.5.

## Réutilisation des systèmes existants

MON18.6 réutilise directement :

- `FGridPlayerAttackAudioDefinition` et `FGridPlayerAttackVFXDefinition` du pipeline de présentation MON11 ;
- `AGridCombatProjectileActor` validé par MON17.3.2 pour la trajectoire visuelle déterministe ;
- les identités `SpellId` et `FGridSpellResolvedTarget` déjà autoritaires côté magie.

Aucune seconde physique de projectile et aucun second vocabulaire audio/VFX ne sont créés.

## Profil data-driven

`FGridSpellPresentationProfile` contient :

```text
CastAudio
CastVFX
ImpactAudio
ImpactVFX
Projectile
FeedbackDurationSeconds
```

`UGridSpellDefinitionAsset` expose désormais un champ `Presentation` séparé du contrat gameplay `Definition`.

La présentation est optionnelle : l'absence de mesh, son ou Niagara n'invalide jamais un sort gameplay.

## Projectile

`FGridSpellProjectilePresentationProfile` contient :

```text
bEnabled
VisualMesh
TravelDurationSeconds
VisualScale
RotationOffset
```

`Spell_ArcaneBolt` utilise un projectile de présentation de `0.20 s`. Les autres premiers sorts (`LesserHeal`, `Haste`, `CurePoison`) sont instantanés.

Le mesh reste volontairement non assigné dans la bibliothèque C++ : aucun chemin d'asset n'est hardcodé. Un DataAsset de production pourra fournir le mesh/VFX/audio sans changement de code.

## Plan déterministe

`FGridSpellPresentationService` produit :

```text
Arcane Bolt : CastStarted -> ProjectileLaunched -> Impact -> Completed
Instantané  : CastStarted -> Impact -> Completed
```

Les calculs de délai et de trajectoire délèguent explicitement à `AGridCombatProjectileActor`.

## Composant runtime

`UGridSpellPresentationComponent` :

- reçoit uniquement un plan déjà accepté/résolu et un profil de présentation ;
- joue éventuellement audio/Niagara ;
- crée éventuellement `AGridCombatProjectileActor` ;
- émet les événements de présentation ;
- ne possède aucune API de mutation de PV, mana, PA, inventaire ou Status Effects.

Un échec ou une absence de présentation visuelle n'annule donc jamais le gameplay déjà résolu.

## Tests Automation

```text
Grimrock.Magic.MON18.6.ProductionProfiles
Grimrock.Magic.MON18.6.ProjectilePlanSequence
Grimrock.Magic.MON18.6.InstantPlanSequence
Grimrock.Magic.MON18.6.ProjectileTrajectoryReuse
Grimrock.Magic.MON18.6.ProjectileTimingReuse
Grimrock.Magic.MON18.6.VisualOptionality
Grimrock.Magic.MON18.6.PresentationPurity
```

Attendu : **7/7 Success**.

## Hors périmètre

- assignation des assets visuels/audio finaux : production de contenu ;
- Spellbook/hotbar UI et déclenchement utilisateur : MON18.7 ;
- persistance du Spellbook : MON18.8.

Aucun `.uasset` n'est modifié par MON18.6.
