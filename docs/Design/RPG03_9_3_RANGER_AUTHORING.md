# RPG03.9.3 — Authoring Rôdeur

Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

Matérialiser les 15 talents du Rôdeur décrits par `docs/Rules/RPG_Talents_Mechanics_v0_1.md` sans branche runtime sur un `TalentId`.

Le ticket réutilise les autorités C1–C8, RPG03.9.0 et RPG03.9.2. Les extensions ajoutées sont génériques et data-driven.

## Authoring

Le Rôdeur produit neuf actions actives :

- Tir précis ;
- Tir perforant ;
- Tir rapide ;
- Volée ;
- Marque de la proie ;
- Tir immobilisant ;
- Frappe du prédateur ;
- Piège de chasse ;
- Repli tactique.

Il crée un seul nouveau statut de production : `Status_MarkedByRanger`. `Status_Immobilized` est réutilisé depuis RPG03.9.2.

Le nombre de Choice records est `14 + nombre de CategoryId de bestiaire` car `Ennemi juré` est un talent logique à variantes exclusives.

## Ennemi juré

Aucune catégorie de bestiaire n'est codée en dur.

Le commandlet Editor interroge l'Asset Registry pour les `UGridMonsterDefinitionAsset` de `/Game/GrimrockPrototype/Monsters`, collecte leurs `CategoryId` et crée une variante :

`Talent_Ranger_Hunter_FavoredEnemy_<Category>`

pour chaque catégorie réelle.

Toutes les variantes :

- appartiennent à `TalentGroup_Ranger_Hunter_FavoredEnemy` ;
- accordent l'alias logique `Talent_Ranger_Hunter_FavoredEnemy` ;
- donnent +15 % dégâts contre leur catégorie ;
- donnent +2 aux checks contextuellement liés de Perception/Nature/Histoire/Religion contre cette catégorie.

## Portée d'arme et LOS

`FGridCombatWeaponAttackProfile` sait désormais :

- exiger une arme dont la portée est supérieure à 1 ;
- utiliser la portée de l'arme comme autorité ;
- ajouter un modificateur de portée avant clamp global 32.

Tir précis utilise ainsi `portée arme +2` au lieu d'une portée fixe.

Les actions Cell/Area peuvent exiger une LOS de grille via `bRequiresLineOfSight`. Volée l'utilise vers sa cellule cible ; obstacles et fumée bloquante sont contrôlés avant toute dépense.

## Marque personnelle

Les status effects peuvent opter pour une identité `(EffectId, SourceId)`.

`Status_MarkedByRanger` active :

- `bDistinctPerSource=true` : deux Rôdeurs peuvent marquer le même monstre ;
- `bUniquePerSourceAcrossMonsters=true` : un Rôdeur ne peut avoir qu'une cible marquée à la fois.

Les filtres C2/C8 savent exiger un statut provenant de l'auteur de l'action.

## Chasseur alpha

Le trigger C4 générique `OwnedStatusTargetDefeated` est distinct du `TargetDefeated` historique afin de ne pas modifier les réactions de kill existantes.

Lors de la mort centrale d'un monstre, chaque personnage reçoit l'identité des statuts qu'il avait lui-même posés sur cette cible. Une réaction peut transférer l'un de ces statuts vers l'hostile vivant le plus proche, avec rayon et durée authorés.

Chasseur alpha :

- une fois par round ;
- exige la propre `Status_MarkedByRanger` ;
- rayon 3 autour de la cible morte ;
- nouvelle durée 2 rounds ;
- aucun PA.

## Progression de groupe

`FRPGPartyProgressionModifier` projette des contributions de groupe depuis les choix des personnages vivants.

Un `StackingGroupId` empêche le cumul de contributions équivalentes entre plusieurs personnages tout en permettant à deux talents différents de se cumuler.

Vigilance :

- meilleur check de groupe `Skill_Perception` +2, non cumulable entre Vigilance ;
- propriétaire : Initiative +2 uniquement au premier round.

Guide du groupe :

- meilleurs checks de groupe `Skill_Perception` et `Skill_Survival` +2 ;
- MaximumMobilityActionPoints +1 ;
- contribution non cumulable entre plusieurs Guides.

MON20 fournit `TryResolveBestPartySkillCheck` : sélection du meilleur membre vivant éligible selon son bonus statique, un seul d20, puis application du bonus de groupe une fois.

## Maître du terrain

Le TurnManager garde un compteur transitoire de translations du groupe. À chaque activation d'un personnage, sa valeur est mémorisée.

Un modifier C2 peut exiger l'absence de translation depuis la précédente activation. La condition devient immédiatement fausse dès qu'une translation normale ou une translation issue d'une action est acceptée.

Aucun statut artificiel n'est créé.

## Dépendance Skills

Comme pour RPG03.9.2, le catalogue complet des `URPGSkillAsset` de production n'est pas encore matérialisé. Le ticket référence les SkillIds canoniques et couvre les contrats avec des définitions transitoires.

## Authoring local

Depuis un working tree propre sur `master` :

```powershell
.\Scripts\AuthorRPGRanger.ps1 -EngineRoot D:\UE_5.5
```

Le script :

1. vérifie `master` et le working tree ;
2. compile l'Editor ;
3. collecte les catégories réelles du bestiaire et exécute `RPGRangerAuthoring` ;
4. lance `Grimrock.RPG.RPG03.9.3` ;
5. affiche les changements binaires.

Changements binaires attendus :

- `DA_Class_Ranger.uasset` modifié ;
- `DA_Status_MarkedByRanger.uasset` créé.

## Validation

La validation n'est acquise que sur sortie TD04.2 fournie par l'utilisateur. Aucun succès UE n'est présumé.
