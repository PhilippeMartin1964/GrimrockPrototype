# RPG03.9.2 — Authoring Voleur

Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

Matérialiser les 15 talents du Voleur définis par `docs/Rules/RPG_Talents_Mechanics_v0_1.md` sans introduire de branche runtime sur un `TalentId`.

Le ticket conserve les autorités existantes :

- `URPGClassAsset` : choix de progression et actions actives ;
- C1 : applications de statuts ;
- C2 : modificateurs data-driven ;
- C4 : réactions ;
- C6 : surfaces persistantes ;
- C8 : filtres de cible et ciblabilité ;
- MON20 : Skill Checks et RequirementGrants ;
- `AGridLevelRuntimeActor` : état persistant du niveau.

## Authoring

Le Voleur produit :

- 11 actions actives ;
- 15 choix de progression ;
- 9 statuts :
  `Status_Bleeding`,
  `Status_ExposedPhysical`,
  `Status_Evasive`,
  `Status_Hidden`,
  `Status_ShadowReach`,
  `Status_Elusive`,
  `Status_HiddenPerfect`,
  `Status_Immobilized`,
  `Status_Sabotaged`.

Le commandlet Editor `RPGRogueAuthoring` modifie `DA_Class_Rogue`, crée/met à jour ces statuts via Unreal, valide les définitions puis sauvegarde les packages. Aucun `.uasset` n'est fabriqué par Git.

## Primitives génériques ajoutées

### Conditions de cible C2

`FGridCombatModifierProfile` peut filtrer sur :

- arc arrière ;
- cible ayant / n'ayant pas encore agi ;
- contrôle physique ;
- exclusion des actions Area.

Le contexte est construit à partir de la géométrie, de l'initiative et des tags de statuts, jamais d'un TalentId.

`WeaponDamagePercentModifier` modifie le coefficient WD entier avant critique, ce qui permet Attaque sournoise 125 -> 175 % WD sans détour par le multiplicateur flottant final.

### Source d'arme

`FGridCombatWeaponAttackProfile` peut imposer des subtypes physiques. Hémorragie accepte Slashing/Piercing et refuse Bludgeoning.

Les `ItemTags` de l'équipement sont projetés dans les `SourceTags` du catalogue ; Pas de l'ombre peut donc ajouter sa portée avant la recherche de cible.

### Skill progression

`FRPGSkillProgressionModifier` expose :

- bonus de jet ;
- bonus de rang effectif pour les RequirementGrants ;
- marge d'échec sûr.

Le résultat de Skill Check expose le bonus, la marge d'échec et `bSafeFailure`. Le consommateur métier décide ensuite quoi faire d'un échec sûr ; le moteur ne connaît ni serrure ni piège particulier.

### Furtivité et réactions

Un statut peut expirer à la prochaine activation de son propriétaire.

Les événements C4 transportent les tags de source et la sémantique `bOffensiveAction`. Une réaction peut :

- exiger une action offensive ;
- exiger des tags de source ;
- appliquer un statut à son propriétaire.

Cela couvre la rupture de Caché/Ombre parfaite, la consommation de Pas de l'ombre par une attaque légère et l'application d'Insaisissable.

### Pièges de cellule

`FGridCombatTrapEffectProfile` et `FGridCombatTrapState` ajoutent un piège temporaire sauvegardable au runtime de niveau.

Piège rapide :

- Cell R1 ;
- 3 rounds ;
- 6 + mod DEX dégâts Physical/Piercing ;
- premier hostile entrant ;
- Immobilisé 1 round si PhysicalArmor=0 après dégâts ;
- consommé après déclenchement.

### Fumée

Le type C6 `Smoke` existant reçoit ses règles runtime génériques :

- une cellule de fumée strictement entre tireur et cible bloque une LOS ciblée à distance ;
- un occupant de fumée reçoit +2 Evasion contre une attaque Ranged.

Les AoE, DoT et surfaces ne sont pas rendus non ciblables.

### Sabotage

Une action peut porter un `FGridCombatSkillCheckProfile`. Sabotage utilise `Skill_Mechanics` et le `SkillCheckDifficulty` générique de la définition du monstre. Un échec paie l'action mais n'applique pas le payload.

Les objets de monde disposent de `bCanBeSabotaged` et `SabotageDifficulty`. Après réussite d'un Skill Check résolu par le caller, `ExecuteRuntimeObjectSabotage` émet l'événement générique `Sabotaged`. Les conséquences restent dans les links/Lua.

## Dépendance Skills de production

Le dépôt ne contient pas encore de `URPGSkillAsset` de production pour `Skill_Traps`, `Skill_Lockpicking` ou `Skill_Mechanics`.

RPG03.9.2 référence leurs IDs canoniques et teste les contrats avec des définitions transitoires. Il ne crée pas un catalogue parallèle ni les 25 Skills dans ce ticket. Le Sabotage de combat nécessitera donc la matérialisation ultérieure du catalogue de Skills pour résoudre `Skill_Mechanics` en jeu.

## Authoring local

Depuis un working tree propre sur `master` :

```powershell
.\Scripts\AuthorRPGRogue.ps1 -EngineRoot D:\UE_5.5
```

Le script :

1. vérifie `master` et le working tree ;
2. compile l'Editor ;
3. exécute `RPGRogueAuthoring` ;
4. lance `Grimrock.RPG.RPG03.9.2` sauf `-SkipAutomation`;
5. affiche les changements binaires attendus.

## Validation

La validation n'est acquise que sur sortie TD04.2 fournie par l'utilisateur. Aucune réussite UE n'est présumée par ce document.
