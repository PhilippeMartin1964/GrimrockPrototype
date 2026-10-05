# RPG03.2 — Status Application + ArmorGate (C1)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : **RPG03.1 validé 7/7**  
Statut : **VALIDÉ — TD04.2 / Grimrock.RPG.RPG03.2 / 7 réussis / 0 warning / 0 échec / 5 octobre 2026**

## Objectif

RPG03.2 raccorde les actions/attaques au système MON16 existant sans créer de second moteur de statut.

Le flux autoritaire devient :

```text
CombatAction / MonsterAttack
        ↓
résolution dégâts
        ↓
état d'armure post-dégâts
        ↓
FGridCombatStatusApplicationResolver
        ↓
UGridStatusEffectLifecycleSubsystem
        ↓
FGridStatusEffectCollection::TryApply()
        ↓
stacking / durée / initiative / contrôle / feedback existants
```

Aucune logique de production ne compare un nom tel que `Status_Stunned`, `Status_Burning` ou `Status_Immobilized`.

## Profil C1

`FGridCombatStatusApplicationProfile` contient :

- `StatusEffectId` ;
- `Trigger` :
  - `AfterResolution`,
  - `AfterSuccessfulHit` ;
- `ArmorGate` :
  - `None`,
  - `PhysicalArmorDepleted`,
  - `MagicalArmorDepleted` ;
- `InitialStackCount` ;
- `DurationOverride` ;
- `PotencyOverride`.

Le profil est authorable sur :

- `FGridCombatActionDefinition::StatusApplications` ;
- `FGridMonsterAttackDefinition::StatusApplications`.

Les attaques de monstres utilisent volontairement `AfterSuccessfulHit` uniquement.

## ArmorGate post-dégâts

Pour une attaque :

```text
PostPhysicalArmor = max(0, PhysicalArmorBefore - PhysicalArmorDamage)
PostMagicalArmor  = max(0, MagicalArmorBefore  - MagicalArmorDamage)
```

Le gate est évalué **après les dégâts de la même attaque**.

Exemple :

```text
PhysicalArmorBefore = 5
PhysicalArmorDamage = 5
→ PostPhysicalArmor = 0
→ PhysicalArmorDepleted = vrai
→ Stun physique autorisé
```

Avec seulement 4 dégâts d'armure, le même statut est bloqué.

Une cible vaincue par l'attaque ne reçoit pas de nouveau statut.

## Status-only actions

Une action `ResolutionProfile=Effect`, `TargetingPolicy=Self` peut désormais être valide sans soin/mana si elle possède au moins une `StatusApplication`.

Le catalogue préflight le stacking sur une copie de la collection actuelle :

- effet absent / Refresh / stronger / stacks disponibles → action applicable ;
- `NoStack` déjà présent sans mutation possible → `NoApplicableEffect`.

Cela prépare directement des Talents tels que Posture défensive, Esquive ou Bénédiction.

Le ciblage Ally / groupe reste du ressort de C8.

## Pont StatusEffect → C2

`UGridStatusEffectDefinitionAsset` porte désormais :

```cpp
TArray<FGridCombatModifierProfile> CombatModifiers;
```

Les modificateurs actifs sont reconstruits depuis `FGridStatusEffectCollection`.

Un statut `AddStacks` contribue son profil une fois par stack. Les autres policies ont un seul état actif.

Les mêmes profils C2 sont utilisés pour :

- catalogue PA/mana/portée du personnage ;
- Accuracy / dégâts / critique sortants ;
- Evasion / dégâts entrants / résistances ;
- attaques joueur → monstre ;
- attaques monstre → personnage ;
- DoT sur personnage ;
- DoT sur monstre.

Aucun agrégat n'est sauvegardé.

## Résolution d'identité

`StatusEffectId` reste l'identité stable MON16.

`FGridCombatStatusApplicationResolver::ResolveDefinition()` réutilise d'abord le resolver de persistence MON16, puis accepte un objet déjà chargé de même `EffectId` pour les contextes Editor/Automation.

Le précédent resolver local du chemin Spellbook est remplacé par ce resolver générique.

## Limites volontaires

RPG03.2 ne traite pas :

- restauration/dégâts directs de pools d'armure : **C3 / RPG03.3** ;
- réactions, ripostes, on-kill : **C4** ;
- déplacement forcé : **C5** ;
- surfaces : **C6** ;
- recettes/QuickItem avancés : **C7** ;
- multi-cible Ally/Party, filtres de cible ou dispel : **C8**.

MON18 conserve son pipeline de sort existant. Son lookup de Status Effect réutilise désormais le resolver générique C1, mais RPG03.2 ne refond pas le moteur de Spell Effects.

## Automation ajoutée

Filtre :

```text
Grimrock.RPG.RPG03.2
```

Tests :

1. `ProfileValidation`
2. `PhysicalArmorGate`
3. `MagicalTriggerAndDeath`
4. `StatusModifierStackProjection`
5. `StatusOnlyCatalog`
6. `LifecycleBridge`
7. `MonsterAttackValidation`

Validation utilisateur reçue le 5 octobre 2026 : 7 tests réussis, 0 warning, 0 échec, 0 not run, process exit code 0.
