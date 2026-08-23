# Groupe, RPG et recrutement — Fondation d’architecture

## Autorité du groupe

`UGridPartyInventoryComponent::PartyInventoryState` est l’unique autorité persistante :

```text
FGridPartyInventoryState
  ActiveCharacters
  ActiveEquipment
  CharacterPool
  SelectedCharacterIndex
  MaxActiveCharacters = 6
```

Aucun second registre de personnages actifs ou de réserve ne doit être créé.

## Personnage

`FGridCharacterInventoryState` porte `CharacterId`, identité, race/classe, niveau/XP, attributs, statistiques dérivées, inventaire, hotbar, portraits et Status Effects. `CharacterId` reste l’identité durable pour progression, spellbook, status et recrutement.

## Création

Le wizard existant valide Race → Class → Attributes → Identity → Summary. MON20 doit le réutiliser pour une future recrue personnalisée au lieu de créer un second processus de création.

## Progression MON15

XP, calcul de niveau, Level Up, progression de classe et notifications sont déjà en production. Les choix de progression possèdent niveau minimum, coût, prérequis et `GrantedRequirementIds`.

### Talents

La stratégie MON20 est de modéliser d’abord les talents de classe comme une extension/présentation des `ProgressionChoices`. Un système TalentPoints parallèle n’est justifié que si un besoin non exprimable apparaît.

### Skills

Un domaine Skill autonome n’existe pas encore. Il ne doit être ajouté que pour des rangs/progressions/tests hors combat réellement indépendants de la classe. Les `RequirementIds` sont déjà un point d’intégration transversal.

## Recrutement MON20.2

`FRPGPartyRecruitmentService::TryRecruitFromPool` transfère atomiquement un candidat de `CharacterPool` vers `ActiveCharacters`, aligne `ActiveEquipment`, normalise l’ownership et rollback si la validation finale échoue. Validation UE5.5.4 : 6/6.

## Compagnons MON20.3

`URPGStoryCompanionAsset` porte `CompanionId`, `CharacterId`, identité visuelle, race/classe, niveau et équipement déclaré. `FRPGStoryCompanionService::EnsureCandidateRegistered` est idempotent : absent → pool, déjà pool → aucun doublon, déjà actif → reconnu, collision GUID → rejet. Validation : 6/6.

Le SaveGame reste v7. `PartyMemberKind` est différé jusqu’à ce qu’une vraie règle de réserve/migration en ait besoin.

## Suite

MON20.4 doit ajouter le Recruitment UI, puis les tranches suivantes traiteront custom recruit, skills, talents, réserve et régression.
