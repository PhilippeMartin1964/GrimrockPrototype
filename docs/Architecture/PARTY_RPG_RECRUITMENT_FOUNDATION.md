# Groupe, RPG et recrutement — Fondation d'architecture

> **Contrat courant — DOC-ARCH01, 9 octobre 2026.**
>
> Ce document remplace l'ancien état MON20 intermédiaire. Recrutement, Skills,
> Talents, Skill Points, Attribute Points et Level-Up non modal sont maintenant
> implémentés selon les autorités ci-dessous.

## 1. Autorité du groupe

\`UGridPartyInventoryComponent::PartyInventoryState\` reste l'unique autorité
du groupe :

\`\`\`text
FGridPartyInventoryState
    SelectedCharacterIndex
    MaxActiveCharacters = 6
    bInitialCharacterCreationCompleted
    ActiveCharacters[]
    ActiveEquipment[]
    CharacterPool[]
    CursorItem
\`\`\`

Il n'existe pas de second registre des membres actifs ou de la réserve.

## 2. État personnage

\`FGridCharacterInventoryState\` sépare désormais clairement durable et dérivé.

\`\`\`text
Durable
    CharacterId / identity
    ClassId
    RaceId
    PortraitGender
    PortraitVariantId
    Experience
    SelectedClassProgressionChoiceIds
    Attributes
    Resources
    SkillRanks
    KnownSpellIds
    StatusEffects
    InventorySlots
    CombatHotbarSlots

Transient / reconstruit
    ClassDefinition
    ClassDisplayName
    RaceDisplayName
    Level
    DerivedStats
    Portrait
    ClassIcon
\`\`\`

\`Level\` dérive de \`Experience\`. Les statistiques dérivées ne sont pas une
autorité Save.

## 3. Création et recrutement

Le wizard de création reste le chemin canonique de création d'un personnage.
Le recrutement réutilise le même modèle de personnage.

Services :

- \`FRPGPartyRecruitmentService\` : transfert pool → groupe ;
- \`FRPGStoryCompanionService\` : compagnons authored ;
- \`FRPGCustomRecruitService\` : recrutement personnalisé ;
- \`URPGStoryCompanionAsset\` : définition d'un compagnon.

Les transactions doivent préserver CharacterId, inventaire, équipement,
ownership et capacité du groupe.

## 4. XP et niveau

\`\`\`text
Experience durable
    -> FRPGLevelUpService
    -> Level transient
    -> derived stats / progression / UI refresh
\`\`\`

Le Level-Up ne crée plus de popup modale persistante.

\`URPGLevelUpNotificationSubsystem\` possède uniquement une file transitoire de
toasts. \`LastAcknowledgedLevel\` et \`URPGLevelUpWidget\` ont été supprimés.

## 5. Talents

Autorité :

\`\`\`text
URPGClassAsset::ProgressionChoices
    -> FRPGClassProgressionService
    -> FRPGClassProgressionTransactionService
    -> FGridSkillsPageService
    -> FGridTalentTreeView
\`\`\`

Production courante :

- 6 classes ;
- 18 branches ;
- 90 Talents conceptuels ;
- 86 nœuds simples ;
- 4 familles à variantes exclusives.

Les Talent Points sont dérivés des grants de classe et des choix acquis. Aucun
compteur parallèle n'est persisté.

Les anciennes façades \`FRPGTalentRuntimeService\` et la projection plate
\`FGridTalentEntryView\` ont été supprimées.

## 6. Skills

\`FGridCharacterInventoryState::SkillRanks\` est l'autorité durable sparse.

\`\`\`text
SkillRanks
    -> FRPGSkillService
    -> FRPGSkillPointService
    -> FGridSkillsPageService
\`\`\`

\`FRPGSkillService\` fournit les primitives métier de rang.
\`FRPGSkillPointService\` est l'économie joueur : achat, balance, caps et Safe
Undo de session.

Règles actuelles :

- niveau 1 : 4 points ;
- chaque niveau supplémentaire : +1 ;
- aucun compteur Skill Point persisté ;
- le budget est reconstruit depuis Level + SkillRanks ;
- Safe Undo limité aux achats de la session UI courante.

## 7. Attributes

\`Character.Attributes\` reste l'autorité durable.

\`FRPGAttributePointService\` dérive :

\`\`\`text
Granted   = floor(Level / 4)
Starting  = Class.BaseAttributes + Race.AttributeBonuses
Spent     = Character.Attributes - Starting
Remaining = Granted - Spent
\`\`\`

Aucun compteur Attribute Point n'est persisté. Le Safe Undo est limité à la
session courante du Character Sheet.

## 8. Requirements

\`FRPGClassProgressionService::CollectSatisfiedRequirements(...)\` reconstruit
le set générique depuis :

- ClassId ;
- grants automatiques de niveau ;
- \`SelectedClassProgressionChoiceIds\`.

Le helper historique \`CollectAutomaticSatisfiedRequirements()\` n'existe plus.

## 9. UI

\`UGridPartyInventoryComponent::SelectedCharacterIndex\` reste l'autorité unique
de sélection pour Character Sheet, Inventory, Skills/Talents et Spellbook.

\`WBP_GridSkills\` est une surface autonome Skills + Talents. UMG ne recalcule
ni coût, ni prérequis, ni balance de points.

## 10. Save

Save courant : **v24 exact-match**.

Les données RPG sont stockées directement dans l'état personnage. Les anciens
snapshots parallèles Progression/Skills/Spellbook/Status ont été supprimés.

## 11. Invariants

1. Une identité CharacterId stable par personnage.
2. Un seul SelectedCharacterIndex.
3. Pas de monnaie RPG persistée lorsqu'elle est reconstructible.
4. Transactions C++ autoritaires pour recrutement et acquisition.
5. UMG = présentation/commande, jamais seconde logique.
6. Aucun retour aux façades runtime supprimées de Skills/Talents.
