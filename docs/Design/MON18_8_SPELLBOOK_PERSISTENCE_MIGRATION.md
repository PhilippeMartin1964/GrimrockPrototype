# MON18.8 — Spellbook Persistence / Migration

## Statut

**VALIDÉ ET CLOS sous UE5.5.4 — 22 août 2026.**

Base d'implémentation :

```text
090c1c082e1bf6844ded4dbdb084da4b236e38c8
Close UI01.4.3e documentation and roadmap

cafdb3f8640497680fd75e507ef0dad618d7bc59
Implement MON18.8 spellbook persistence and migration

d25cf26e0d7c052fe30ba772e50e02b7debaa918
Fix MON16.5 status identity regression
```

MON18.8 rend persistante la connaissance des sorts sans sérialiser le runtime de magie, les définitions de sorts, la présentation ou les widgets.

## 1. Autorité et identités persistantes

Le runtime reste porté par :

```text
UGridPartySpellbookComponent
    -> FGridPartySpellbookState
        -> FGridCharacterSpellbookState
            - CharacterId
            - KnownSpellIds[]
```

Le composant reste runtime/transient. Le SaveGame persiste uniquement :

```text
CharacterId
SpellId
```

Ne sont jamais sérialisés :

- pointeurs d'acteurs ;
- pointeurs ou copies de `UGridSpellDefinitionAsset` ;
- widgets ;
- coûts, targeting ou effets déjà définis par les données de sort ;
- audio, Niagara ou présentation MON18.6.

## 2. Snapshot SaveGame

MON18.8 ajoute :

```text
FGridCharacterSpellbookSaveState
- CharacterId
- KnownSpellIds[]

UGrimrockPartySaveGame::CharacterSpellbookStates[]
```

Le snapshot est **sparse** : un personnage dont le Spellbook est vide ne produit pas d'entrée persistante.

La restauration recrée néanmoins un conteneur runtime vide pour chaque personnage actif et chaque personnage du `CharacterPool`.

## 3. Service de persistance

Nouveau service :

```text
FGridSpellbookPersistence
```

Responsabilités :

- capture runtime -> snapshot ;
- validation structurelle du snapshot ;
- restauration snapshot -> runtime ;
- réconciliation par `CharacterId` ;
- ordre déterministe ;
- restauration atomique.

### Capture

La capture est refusée si :

- un CharacterId du groupe est invalide ou ambigu ;
- le runtime Spellbook est structurellement invalide ;
- un Spellbook runtime référence un CharacterId absent du groupe ;
- un SpellId est `NAME_None` ;
- un SpellId est dupliqué dans le même Spellbook.

Les `KnownSpellIds` et les snapshots de personnages sont triés avant sauvegarde.

### Restore

La restauration construit un `FGridPartySpellbookState` candidat complet.

Elle est refusée si :

- un CharacterId sauvegardé est invalide ;
- deux snapshots ciblent le même CharacterId ;
- un snapshot cible un personnage absent/ambigu ;
- un SpellId est vide ou dupliqué.

Le runtime existant n'est remplacé qu'après construction complète du candidat.

## 4. SpellId dont la définition a disparu

La persistance ne résout volontairement **aucune** définition de sort.

Un identifiant tel que :

```text
Spell_RemovedContent
```

reste une connaissance persistante valide tant que le `FName` est non vide et non dupliqué.

Cela prolonge le contrat MON18.7 : l'UI peut afficher une connaissance dont la définition n'est momentanément plus disponible et la rendre non assignable/non exécutable sans perdre l'identité sauvegardée.

## 5. SaveGame version 6

MON18.8 fait évoluer :

```text
CurrentSaveVersion : 5 -> 6
MinimumCompatibleSaveVersion : 1 (inchangé)
```

### Migration v5 -> v6

La version 5 est déjà autoritaire pour :

- progression MON15 ;
- inventaire / équipement / hotbar ;
- status effects MON16 ;
- donjon / monde / monstres.

Elle ne doit jamais être envoyée dans le chemin de reconstruction v1-v3.

La migration v5 -> v6 :

- valide la progression v5 existante ;
- conserve `PartyInventoryState` ;
- conserve les hotbars ;
- conserve `ClassProgressionStates` ;
- conserve `PendingLevelUpNotifications` ;
- conserve `CharacterStatusEffectStates` ;
- conserve `DungeonRuntimeState` ;
- initialise `CharacterSpellbookStates` à vide ;
- n'invente aucun sort ;
- effectue zéro réconciliation de niveau.

Les versions v1-v4 continuent leurs migrations historiques puis obtiennent également un Spellbook persistant vide.

## 6. Hotbar Spell : référence, jamais autorité

Le binding MON12/MON18 reste :

```text
ActionId           = SpellId
SourcePolicy       = Spell
SourceDefinitionId = SpellId
```

La connaissance du sort est autoritaire dans le Spellbook.

Cas legacy accepté :

```text
Hotbar : Spell_ArcaneBolt
Spellbook : ArcaneBolt inconnu
```

Politique MON18.8 :

- conserver le binding ;
- ne pas enseigner automatiquement le sort ;
- ne pas effacer automatiquement le slot ;
- le catalogue combat ne contribue pas l'action tant que le sort n'est pas connu ;
- si le personnage apprend ultérieurement le même SpellId, le binding peut redevenir utilisable.

## 7. Protection SAVEFIX.1

`SourceDefinitionId` d'un binding `Spell` reste un **SpellId**.

`RehydrateOwnedItemDefinitions()` continue à résoudre uniquement les bindings :

```text
Equipment
QuickItem
```

Un binding Spell ne doit jamais produire une recherche de `ItemDefinition`.

Le test MON18.8 `SpellBindingIsNotItemDefinition` couvre explicitement cette régression.

## 8. Composant natif du Pawn

MON18.8 crée désormais `UGridPartySpellbookComponent` comme default subobject natif de `AGrimrockPartyPawn`.

La présence du runtime Spellbook ne dépend donc plus de l'ouverture préalable de l'onglet **Sorts**.

`UGridSpellbookWidget` continue à rechercher le composant existant et reste compatible avec ce contrat.

## 9. Save / Load du Pawn

### Save

```text
Validate inventory ownership
-> capture Spellbook snapshot
-> capture level runtime
-> create UGrimrockPartySaveGame v6
-> copy PartyInventoryState
-> copy CharacterSpellbookStates
-> copy DungeonRuntimeState / position
-> SaveGameToSlot
```

Une sauvegarde runtime normale est rejetée si le composant Spellbook attendu est indisponible ou invalide.

### Load

```text
LoadGameFromSlot
-> SaveGame migration / validation
-> build Spellbook candidate from saved PartyInventoryState
-> save previous runtime party/dungeon state
-> restore PartyInventoryState
-> restore dungeon state
-> rehydrate ItemDefinitions
-> apply dungeon runtime
-> commit Spellbook candidate
-> broadcast OnSpellbookChanged
```

Le Spellbook n'est donc pas muté avant la réussite des opérations de restauration susceptibles d'échouer.

## 10. New Game

Tous les chemins qui réinitialisent réellement le groupe en nouvelle partie réinitialisent également le Spellbook runtime.

Après création initiale d'un personnage, un conteneur Spellbook vide est assuré pour son `CharacterId` avant la première sauvegarde.

Aucun sort de production n'est accordé automatiquement par MON18.8.

## 11. Protection SAVEFIX.2

Le comportement d'un échec de **Continue** reste inchangé :

- aucun `ResetPartyForNewGame()` ;
- aucune ouverture de Character Creation ;
- aucune suppression/réécriture du fichier de sauvegarde ;
- autosave `EndPlay` désarmé pour le pawn en échec ;
- retour au menu principal.

Le Spellbook candidat étant construit avant son commit runtime, un échec de chargement ne remplace pas non plus le Spellbook courant.

## 12. Automation MON18.8 — VALIDÉ

Namespace :

```text
Grimrock.Magic.MON18.8
```

Résultat UE5.5.4 fourni le 22 août 2026 : **12/12 Success**.

Tests validés :

```text
AtomicRestoreFailure
DiskRoundTrip
DuplicateSpellRejected
HotbarSpellRoundTrip
HotbarWithoutKnowledgePreserved
InvalidSpellIdRejected
MultipleCharactersRoundTrip
OrphanCharacterRejected
SingleCharacterRoundTrip
SpellBindingIsNotItemDefinition
UnknownDefinitionPreserved
V5MigrationCreatesEmptySpellbook
```

Régression découverte pendant la campagne globale :

```text
Grimrock.RPG.MON16.5.NoParallelSystem
```

Elle provenait d'une comparaison directe de `EffectId` dans le chemin d'exécution des sorts de statut. Le correctif `d25cf26e` remplace cette comparaison par l'identité primaire canonique du Status Effect.

Validation après correctif :

```text
Grimrock.RPG.MON16.5.NoParallelSystem    Success
Grimrock.RPG.MON16.5                     9/9 Success
Grimrock.UI.UI01.4.3e.2                  6/6 Success
```

Le filtre UI correct est :

```text
Automation RunTests Grimrock.UI.UI01.4.3e.2
```

Référence détaillée : `docs/Design/MON18_8_VALIDATION.md`.

## 13. Validation PIE finale — VALIDÉE

Scénario effectué sous UE5.5.4 :

```text
New Game / Mage
-> Grimrock.Spellbook.SeedProduction
-> Added=4 AlreadyKnown=0
-> Save v6 avec Spellbooks=1
-> arrêt PIE
-> nouvelle session PIE
-> Continue
-> snapshot v6 accepté avec SpellbookCharacters=1
-> PartySave Continued
-> Grimrock.Spellbook.SeedProduction
-> Added=0 AlreadyKnown=4
```

La ligne finale `Added=0 AlreadyKnown=4` prouve que les quatre `KnownSpellIds` ont été restaurés depuis le SaveGame et ne proviennent pas d'un nouveau seed runtime.

La persistance des bindings hotbar Spell est en outre couverte par :

```text
HotbarSpellRoundTrip
HotbarWithoutKnowledgePreserved
SpellBindingIsNotItemDefinition
```

### Note sur les diagnostics de slots auxiliaires

Le menu principal peut également sonder les slots configurés `GrimrockParty_2` et `GrimrockParty_3`. Des slots auxiliaires anciens/incompatibles peuvent produire un `LoadValidation ... Result=Rejected` pendant l'énumération sans invalider le slot principal `GrimrockParty`, lequel a été chargé avec succès dans le scénario PIE ci-dessus.

## 14. Hors périmètre

MON18.8 n'ajoute :

- aucun nouveau sort ;
- aucune règle de combat ;
- aucune modification WBP ;
- aucun `.uasset` / `.umap` ;
- aucun apprentissage automatique par classe ;
- aucune réparation destructive de hotbar legacy.

## Conclusion

**MON18.8 — Spellbook Persistence / Migration est VALIDÉ ET CLOS sous UE5.5.4.**

Prochaine étape : `MON18.9 — Balance / Regression / Closure`.
