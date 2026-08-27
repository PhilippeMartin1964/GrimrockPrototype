# TD07.3.3.1 — Character State Authority Audit

Date : **27 août 2026**  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Parent : **TD07.3.3 — Character State Normalization**  
Baseline GitHub : `a82340df79d69c33d4ccc2e77617b984c9f8eb79`  
Statut : **VALIDÉ — AUDIT STATIQUE / AUCUN CHANGEMENT GAMEPLAY**

## 1. Objet

TD07.3.3.1 caractérise l'état courant du personnage avant toute suppression de champ ou changement de schéma.

Cette tranche :

- ne modifie aucun comportement gameplay ;
- ne modifie aucun DataAsset, Blueprint ou map ;
- ne modifie pas le schéma SaveGame ;
- n'applique aucune migration ;
- ne corrige aucun des 41 findings DataAsset TD07.3.1 ;
- établit la matrice d'autorité qui pilote TD07.3.3.2 et les sous-tranches suivantes.

La règle autoritaire reste :

```text
une donnée gameplay durable = une seule autorité
donnée dérivable            = projection / calcul
cache runtime               = reconstruisible
présentation UI             = jamais autorité
compatibilité historique    = aucune pendant le prototype
```

## 2. Agrégat courant

Le personnage durable est aujourd'hui principalement porté par :

```text
FGridPartyInventoryState
├── ActiveCharacters : TArray<FGridCharacterInventoryState>
├── ActiveEquipment
└── CharacterPool : TArray<FGridCharacterInventoryState>
```

`FGridCharacterInventoryState` contient actuellement :

```text
CharacterId
DisplayName

ClassId
ClassDisplayName
ClassDefinition

RaceId
RaceDisplayName

Level
Experience

Attributes
DerivedStats

SkillRanks              [Transient]
StatusEffects           [Transient]

PortraitGender
PortraitVariantId
Portrait
ClassIcon

bRPGAttributesInitialized
Strength                [DeprecatedProperty]

CurrentWeight
MaxCarryWeight

InventorySlots
CombatHotbarSlots
```

Le SaveGame ajoute cinq collections parallèles :

```text
ClassProgressionStates
PendingLevelUpNotifications
CharacterStatusEffectStates
CharacterSpellbookStates
CharacterSkillStates
```

## 3. Matrice d'autorité — état courant et cible

| Domaine | Représentation actuelle | Autorité actuelle | Problème | Cible TD07.3.3 |
|---|---|---|---|---|
| Identité personnage | `CharacterId` | personnage | aucune duplication métier | conserver |
| Nom | `DisplayName` | personnage | valeur joueur / contenu durable | conserver |
| Classe | `ClassId` + `ClassDefinition` + `ClassDisplayName` | multiple | identité + asset + présentation | différer TD07.3.4 |
| Race | `RaceId` + `RaceDisplayName` | multiple | identité + présentation | différer TD07.3.4 |
| Niveau / XP | `Level` + `Experience` | deux champs synchronisés | même progression représentée deux fois | `Experience` candidat autoritaire, `Level` dérivé |
| Attributs | `Attributes` | personnage | aucune raison de double représentation | conserver comme autorité |
| Force legacy | `Strength` | historique | doublon de `Attributes.Strength` | supprimer |
| Marqueur init RPG | `bRPGAttributesInitialized` | historique | sert au fallback Strength -> Attributes | supprimer |
| Stats calculées | partie de `DerivedStats` | personnage | valeurs recalculables persistées | calculer |
| Ressources courantes | partie de `DerivedStats` | personnage | mélangées avec les valeurs calculées | isoler comme état mutable |
| Poids courant | `CurrentWeight` | cache personnage | calculable depuis inventaire + équipement | calculer |
| Capacité de charge | `MaxCarryWeight` | cache personnage | calculable depuis Attributes, puis bonus équipement en projection | calculer |
| Skills | `Character.SkillRanks` + `CharacterSkillStates` | runtime = Character, save = snapshot | duplication complète runtime/save | rendre `SkillRanks` durable |
| Talents / choix | `RuntimeStates.SelectedChoiceIds` + `ClassProgressionStates` | runtime cache + SaveGame | donnée joueur hors personnage | rattacher au personnage |
| Spellbook | `UGridPartySpellbookComponent::SpellbookState` + `CharacterSpellbookStates` | composant runtime + SaveGame | connaissance de sorts séparée du personnage | rattacher `KnownSpellIds` au personnage |
| Status Effects | `Character.StatusEffects` + `CharacterStatusEffectStates` | runtime = Character, save = snapshot | structures quasi identiques | état unique durable + ref asset transient |
| Notification Level Up | queue subsystem + miroir statique + SaveGame | plusieurs | persistance d'un workflow UI | réduire à un état métier minimal ou transient |
| Inventaire | `InventorySlots` | personnage | autorité unique | conserver |
| Hotbar | `CombatHotbarSlots` | personnage | autorité unique | conserver |
| Portrait / icône | ids + soft refs | multiple | identité / présentation | différer TD07.3.4 |

## 4. Finding A — pont legacy Attributes

Le code contient encore :

```cpp
bool bRPGAttributesInitialized = false;

UPROPERTY(... DeprecatedProperty ...)
float Strength = 10.0f;
```

et le fallback :

```cpp
if (!CharacterState.bRPGAttributesInitialized)
{
    CharacterState.Attributes.Strength =
        FMath::RoundToInt(FMath::Max(0.0f, CharacterState.Strength));
    CharacterState.bRPGAttributesInitialized = true;
}
```

Les chemins de création courants écrivent encore simultanément :

```text
Attributes
bRPGAttributesInitialized = true
Strength = Attributes.Strength
```

Cela est observé dans :

```text
GridPartyInventoryComponent.cpp
RPGCustomRecruitService.cpp
RPGStoryCompanionService.cpp
```

et `RPGPartyRecruitmentService.cpp` exige encore `bRPGAttributesInitialized`.

Conclusion :

```text
Attributes = autorité courante légitime
Strength = champ legacy pur
bRPGAttributesInitialized = shim legacy pur
```

**TD07.3.3.2 doit supprimer les deux champs et tout fallback associé, sans shim.**

## 5. Finding B — DerivedStats est un conteneur mixte

`FRPGDerivedStats` contient :

```text
MaxHealth
CurrentHealth
MaxMana
CurrentMana
PhysicalArmor
MagicalArmor
Initiative
Accuracy
Evasion
```

### 5.1 Valeurs recalculables

`URPGCharacterRulesLibrary::CalculateDerivedStats()` reconstruit :

```text
MaxHealth
MaxMana
PhysicalArmor initial
MagicalArmor initial
Initiative
Accuracy
Evasion
```

à partir de :

```text
Attributes
ClassDefinition
Level
```

### 5.2 Valeurs réellement mutables

Le runtime mutile directement :

```text
CurrentHealth
CurrentMana
PhysicalArmor
MagicalArmor
```

Exemples :

- `GridMonsterCombatComponent` retire les dégâts des pools d'armure puis de CurrentHealth ;
- `GridStatusEffectLifecycleSubsystem` fait de même pour les dégâts périodiques ;
- `GridSpellCastTransaction` dépense CurrentMana ;
- `GridSpellEffectResolver` peut modifier CurrentHealth.

Par conséquent, supprimer simplement `DerivedStats` et tout recalculer à la volée serait incorrect.

### 5.3 Cible

TD07.3.3.3 doit séparer :

```text
Calculable / projection
    MaxHealth
    MaxMana
    Initiative
    Accuracy
    Evasion
    valeurs initiales / maximales d'armure selon contrat gameplay

Mutable / durable
    CurrentHealth
    CurrentMana
    CurrentPhysicalArmor
    CurrentMagicalArmor
```

Le nom exact de la future structure reste à choisir pendant l'implémentation, mais la frontière d'autorité est désormais fixée.

## 6. Finding C — comportement équipement à préserver explicitement

`GetCharacterSummary()` produit un read model :

```text
BaseAttributes = Character.Attributes
Attributes = BaseAttributes + EquipmentStatBonus

BaseDerivedStats = Character.DerivedStats
DerivedStats = BaseDerivedStats
DerivedStats.MaxHealth += MaxHealthBonus
DerivedStats.MaxMana += MaxManaBonus
DerivedStats.PhysicalArmor += ArmorBonus

MaxWeight = BaseMaxWeight + CarryWeightBonus
```

Mais plusieurs systèmes de combat lisent directement `Character.DerivedStats`.

Exemple confirmé :

```text
GridMonsterCombatComponent
    Evasion         = Character.DerivedStats.Evasion
    PhysicalArmor   = Character.DerivedStats.PhysicalArmor
    MagicalArmor    = Character.DerivedStats.MagicalArmor
    CurrentHealth   = Character.DerivedStats.CurrentHealth
```

Ainsi, les bonus d'équipement affichés/projetés ne traversent pas nécessairement le même chemin que les stats consommées par tous les systèmes de combat.

**TD07.3.3 ne doit pas modifier ce comportement implicitement.**

Avant TD07.3.3.3, les tests doivent caractériser au minimum :

- bonus de DEX et initiative/accuracy/evasion ;
- ArmorBonus et absorption réelle ;
- MaxHealthBonus / MaxManaBonus et limites de ressources ;
- retrait d'un équipement lorsque CurrentHealth/CurrentMana dépassent une nouvelle limite.

Une normalisation d'autorité n'est pas une autorisation de rebalance gameplay.

## 7. Finding D — CurrentWeight / MaxCarryWeight sont des caches

Le runtime recalcule explicitement :

```cpp
CharacterState.MaxCarryWeight =
    URPGCharacterRulesLibrary::CalculateMaxCarryWeight(CharacterState.Attributes);

CharacterState.CurrentWeight = InventoryWeight + EquipmentWeight;
```

Après restore :

```text
PartyInventoryState = restored snapshot
-> RecalculateAllWeights()
```

Donc la valeur chargée n'est déjà pas l'autorité réelle.

Le read model ajoute ensuite :

```text
CarryWeightBonus
```

au maximum affiché.

Conclusion :

```text
InventorySlots + ActiveEquipment = autorité du poids porté
Attributes + equipment bonuses   = autorité de la capacité
CurrentWeight / MaxCarryWeight   = caches supprimables
```

**TD07.3.3.4 supprimera ces caches persistants.**

## 8. Finding E — Level / Experience représentent la même progression

Les chemins courants montrent :

```text
ExperienceRewardService
    -> modifie Experience

LevelUpService
    -> TargetLevel = GetLevelForExperience(Experience)
    -> modifie Level
```

Le Save v10 rejette déjà un couple incohérent.

Les créations garantissent également un couple cohérent :

```text
custom/new character : Level=1, Experience=0
story companion      : Experience = floor du Level authoré
```

Conclusion d'architecture :

```text
Experience est suffisant pour déterminer Level
Level est actuellement un cache durable synchronisé
```

Cible recommandée TD07.3.3.5 :

```text
Experience = autorité
Level      = projection GetLevelForExperience(Experience)
```

La suppression physique de `Level` ne doit intervenir qu'après caractérisation de tous ses consommateurs.

## 9. Finding F — progression de classe / Talents

`FRPGClassProgressionTransactionService` possède un cache global :

```text
TMap<FGuid, FRuntimeClassProgressionState> RuntimeStates
    -> SelectedChoiceIds
```

Le commentaire courant indique explicitement que ce cache est l'autorité pendant le play, puis :

```text
CapturePersistentState()
    -> ClassProgressionStates

RestorePersistentState()
    -> RuntimeStates
```

Les Talents lisent ce service au lieu du personnage.

Il s'agit d'une donnée choisie par le joueur, donc durable par nature.

Cible :

```text
FGridCharacterInventoryState
    -> SelectedProgressionChoiceIds

FRPGClassProgressionTransactionService
    -> validation / transaction / projections reconstruites
    -> aucune seconde autorité
```

Le cache de RequirementIds reste acceptable uniquement s'il est intégralement reconstructible.

## 10. Finding G — Skills

Le runtime Skill utilise directement :

```text
FGridCharacterInventoryState::SkillRanks
```

`FRPGSkillService` lit et modifie ce tableau.  
`FRPGSkillRequirementProjectionService` et `GridSkillsPageService` le lisent directement.

Pourtant il est marqué :

```text
Transient
```

et le SaveGame porte :

```text
CharacterSkillStates
    -> CharacterId
    -> SkillRanks
```

Le pipeline est donc :

```text
Character.SkillRanks
    -> CapturePartySkills()
    -> CharacterSkillStates
    -> RestorePartySkills()
    -> Character.SkillRanks
```

Conclusion :

```text
Character.SkillRanks = autorité runtime déjà établie
CharacterSkillStates = duplication de persistance
```

Cible TD07.3.3.6 : rendre `SkillRanks` durable directement et supprimer le snapshot parallèle.

## 11. Finding H — Spellbook

Le Spellbook est le seul de ces domaines qui n'est même pas porté par le personnage.

Autorité runtime courante :

```text
UGridPartySpellbookComponent
    -> FGridPartySpellbookState
        -> CharacterSpellbooks[]
            -> CharacterId
            -> KnownSpellIds[]
```

Persistance :

```text
CharacterSpellbookStates[]
    -> CharacterId
    -> KnownSpellIds[]
```

Le combat confirme que le composant Spellbook est aujourd'hui l'autorité de connaissance des sorts.

Cette donnée est cependant intrinsèquement propre à un personnage et doit suivre naturellement :

```text
ActiveCharacters <-> CharacterPool
```

Cible TD07.3.3.7 :

```text
FGridCharacterInventoryState
    -> KnownSpellIds

UGridPartySpellbookComponent
    -> façade / notification / opérations
    -> aucune seconde collection propriétaire
```

Le snapshot `CharacterSpellbookStates` devient alors inutile.

## 12. Finding I — Status Effects

Le personnage possède :

```text
FGridStatusEffectCollection StatusEffects [Transient]
```

Le runtime effect contient :

```text
EffectId
SourceId
StackCount
DurationUnit
RemainingDuration
Potency
DefinitionAsset [Transient]
```

Le SaveGame duplique exactement les six valeurs stables dans :

```text
FGridStatusEffectSaveState
FGridCharacterStatusEffectSaveState
CharacterStatusEffectStates
```

La seule donnée réellement runtime-only est :

```text
DefinitionAsset
```

qui est résoluble depuis `EffectId`.

Cible TD07.3.3.8 :

```text
une structure d'état stable unique
DefinitionAsset = cache transient rehydraté
Character.StatusEffects = état durable
```

La capture/restore peut être remplacée par validation + rehydration sans copie miroir.

## 13. Finding J — Pending Level Up est un workflow UI persisté

Le système possède simultanément :

```text
PendingNotifications
ActiveNotification
PersistentNotificationMirror [statique]
PendingPersistentRestoreStates
PendingLevelUpNotifications [SaveGame]
```

Cela persiste une queue de présentation plutôt qu'un état métier minimal.

Deux cibles sont recevables :

### Option A — état minimal durable

```text
LastAcknowledgedLevel
```

ou équivalent, puis reconstruction de la notification depuis le niveau courant.

### Option B — notification totalement transient

Aucune persistance si le design accepte qu'un Continue ne représente pas une notification non acquittée.

TD07.3.3.9 devra choisir explicitement.  
**La solution actuelle à quatre représentations ne doit pas être conservée.**

## 14. Read models et caches acceptables

Toutes les duplications ne sont pas mauvaises.

`FGridInventoryCharacterSummary` est explicitement un read model temporaire contenant :

```text
BaseAttributes
final Attributes
BaseDerivedStats
final DerivedStats
EquipmentStatBonus
resistances
weights
presentation
```

Il est reconstruit à la demande et n'est pas une autorité durable.

Même principe pour :

- contextes de combat ;
- views Skills / Talents / UI ;
- Initiative entries ;
- RequirementId projections ;
- soft pointers rehydratés depuis une identité stable.

La cible TD07.3.3 est donc **une seule autorité durable**, pas « une seule structure temporaire dans tout le code ».

## 15. Éléments explicitement hors périmètre TD07.3.3.1

### Authoring identity — TD07.3.4

Les couples suivants sont observés mais ne sont pas nettoyés ici :

```text
ClassId + ClassDefinition + ClassDisplayName
RaceId + RaceDisplayName
PortraitVariantId + Portrait
ClassId + ClassIcon
```

Ils relèvent principalement de :

```text
TD07.3.4 — Authoring Identity Normalization
```

### DataAssets

Les 41 findings TD07.3.1, dont les deux conflits Shuriken/Stone, restent intacts.

## 16. Ordre d'implémentation confirmé

L'audit valide l'ordre suivant :

```text
TD07.3.3.2 — Remove Legacy Attribute Bridge
    Strength
    bRPGAttributesInitialized
    fallback Strength -> Attributes

TD07.3.3.3 — Normalize Derived Stats / Mutable Resources
    séparer calculable et mutable
    caractériser les bonus équipement avant modification

TD07.3.3.4 — Normalize Weight State
    supprimer CurrentWeight / MaxCarryWeight comme autorités

TD07.3.3.5 — Normalize XP / Level / Class Progression
    Experience autoritaire
    Level dérivé
    choix de progression dans le personnage

TD07.3.3.6 — Normalize Skills
    SkillRanks durable directement

TD07.3.3.7 — Normalize Spellbook
    KnownSpellIds dans le personnage

TD07.3.3.8 — Normalize Status Effects
    état unique + DefinitionAsset transient

TD07.3.3.9 — Normalize Level-Up Notification State
    état métier minimal ou transient

TD07.3.3.10 — Current Save Schema / Regressions / Closure
```

## 17. Risques à protéger

### R1 — Ressources de combat

Ne pas perdre :

```text
CurrentHealth
CurrentMana
PhysicalArmor courant
MagicalArmor courant
```

### R2 — équipement

Ne pas changer implicitement la sémantique des :

```text
attribute bonuses
MaxHealthBonus
MaxManaBonus
CarryWeightBonus
ArmorBonus
```

### R3 — progression

Préserver :

```text
XP award
Level Up
déficit HP/Mana lors d'un Level Up
talents / prérequis
pending Level Up UX
```

### R4 — ActiveCharacters / CharacterPool

Les données propres au personnage doivent suivre le `CharacterId` et non dépendre de sa présence temporaire dans le groupe actif.

C'est particulièrement important pour :

```text
Skills
Spellbook
Status Effects
Progression choices
```

## 18. Validation de TD07.3.3.1

Cette tranche est documentaire/statique.

Vérifié sur `master` à la baseline :

```text
a82340df79d69c33d4ccc2e77617b984c9f8eb79
```

Sources principales relues :

```text
Runtime/GridInventoryTypes.h
Runtime/GridPartyInventoryComponent.cpp
RPG/RPGCharacterTypes.h
RPG/RPGCharacterRulesLibrary.cpp
RPG/RPGLevelUpService.cpp
RPG/RPGExperienceRewardService.cpp
RPG/RPGClassProgressionTransactionService.cpp
RPG/RPGSkillService.cpp
RPG/RPGSkillPersistence.cpp
Magic/GridPartySpellbookComponent.*
Magic/GridSpellbookPersistence.*
RPG/StatusEffects/GridStatusEffectTypes.h
RPG/StatusEffects/GridStatusEffectPersistence.cpp
RPG/StatusEffects/GridStatusEffectLifecycleSubsystem.cpp
RPG/RPGLevelUpNotificationSubsystem.cpp
Save/GrimrockPartySaveGame.*
Runtime/GrimrockPartyPawnSave.cpp
Runtime/Combat/*
```

Aucune compilation UE n'est requise car aucun C++, asset ou Blueprint n'est modifié.

## 19. Stop condition — ATTEINTE

TD07.3.3.1 est clos lorsque :

- [x] tous les champs durables du personnage sont classés ;
- [x] les caches et projections sont distingués des autorités ;
- [x] les cinq snapshots annexes Save sont caractérisés ;
- [x] les champs legacy Attributes sont identifiés ;
- [x] la nature mixte de `DerivedStats` est démontrée ;
- [x] les poids dérivés sont identifiés ;
- [x] les risques équipement/combat sont consignés ;
- [x] la frontière TD07.3.4 / DataAssets reste intacte ;
- [x] l'ordre des sous-tranches suivantes est fixé.

Prochaine tranche :

```text
TD07.3.3.2 — Remove Legacy Attribute Bridge
```
