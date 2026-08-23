# MON20.4.4 — Event -> Command -> Story Companion Recruitment

## Statut

Bridge C++ data-driven entre le système de niveau `Event -> Command` et le modal de recrutement MON20.4.2 / MON20.4.3.

Cette étape ne crée pas de système de dialogue parallèle et ne modifie pas directement l'état du groupe.

**Validation UE5.5.4 : 23 août 2026 — filtre `Grimrock.MON20.4.RecruitmentUI` : 13/13 Success.**

## Contrat de données

`EGridLevelObjectType` reçoit un nouveau type ajouté en fin d'enum afin de préserver les valeurs sérialisées existantes :

```text
StoryCompanion
```

`EGridObjectCommand` reçoit explicitement :

```text
OfferRecruitment = 23
```

La valeur suit `LuaCallback = 22` et ne renumérote aucune commande existante.

`FGridLevelObjectData` expose :

```text
StoryCompanionDefinition : URPGStoryCompanionAsset*
```

La propriété n'est éditable/visible que lorsque :

```text
Type == StoryCompanion
```

Un `StoryCompanion` est un target data-only : aucun Actor runtime spécifique n'est requis pour exécuter `OfferRecruitment`.

## Flux runtime autoritaire

Le chemin de recrutement est désormais :

```text
Source Object Event
    -> FGridObjectLink
    -> Command = OfferRecruitment
    -> Target Type = StoryCompanion
    -> UGridActivationComponent::ApplyLinkCommand()
    -> AGrimrockPartyPawn::ShowStoryCompanionRecruitmentWidget()
    -> URPGStoryCompanionRecruitmentWidget
    -> FRPGStoryCompanionService::EnsureCandidateRegistered()
    -> FRPGPartyRecruitmentService::TryRecruitFromPool()
```

`ApplyLinkCommand()` ne touche jamais directement à `CharacterPool` ou `ActiveCharacters`.

La transaction de recrutement reste donc possédée par les services MON20.3 et MON20.2 via le widget MON20.4.2.

## Validation runtime de la commande

`OfferRecruitment` est accepté uniquement si :

- le target est de type `StoryCompanion` ;
- la commande est exactement `OfferRecruitment` ;
- `StoryCompanionDefinition` existe ;
- la définition passe `URPGStoryCompanionAsset::IsValidDefinition()` ;
- le Pawn du joueur peut être résolu ;
- `AGrimrockPartyPawn::ShowStoryCompanionRecruitmentWidget()` accepte l'ouverture.

Le Pawn conserve les protections MON20.4.3 :

- aucun empilement de deux modals de recrutement ;
- aucun recrutement pendant la création initiale du personnage ;
- buffer de commande vidé avant ouverture ;
- classe WBP configurée ou fallback C++ natif.

Un refus d'ouverture est un échec propre du link command ; il ne modifie pas le groupe.

## Lua

Aucune fonction Lua spéciale n'est ajoutée.

MON19.4 résout déjà les noms de commandes par `StaticEnum<EGridObjectCommand>()`. `OfferRecruitment` devient donc automatiquement disponible dans le chemin générique :

```lua
grid.command("CompanionTarget", "OfferRecruitment")
```

Le target peut continuer à être référencé par son `ObjectId` ou son `LogicId` selon le contrat MON19.4 existant.

Lua et les liens du niveau convergent ainsi vers le même `ApplyLinkCommand()` et le même comportement autoritaire.

## Grid Editor — politique des liens

`GridEditorLinkPolicy` expose pour un target `StoryCompanion` exactement :

```text
OfferRecruitment
```

Cette commande est classée :

```text
Gameplay
```

Les commandes génériques comme `Toggle` sont rejetées pour ce target.

Le compagnon est, dans ce périmètre, un target de commande uniquement ; aucun événement source propre n'est ajouté.

La création par palette et la copie automatique de la définition sont traitées en MON20.4.5.

## WBP de production

Le WBP a été créé et configuré manuellement après validation MON20.4.3 :

```text
WBP_RPGStoryCompanionRecruitment
```

Parent :

```text
URPGStoryCompanionRecruitmentWidget
```

Il est assigné à :

```text
BP_GrimrockPartyPawn
  -> StoryCompanionRecruitmentWidgetClass
```

La logique des boutons reste en C++.

## Tests MON20.4.4

Le filtre principal reste :

```text
Grimrock.MON20.4.RecruitmentUI
```

Trois tests ont été ajoutés :

```text
EventCommandContract
EventCommandMissingDefinition
EditorLinkPolicy
```

### EventCommandContract

Vérifie :

- `OfferRecruitment == 23` ;
- la résolution par nom d'enum utilisée par Lua ;
- l'existence du type `StoryCompanion` ;
- la présence réfléchie de `StoryCompanionDefinition` dans `FGridLevelObjectData`.

### EventCommandMissingDefinition

Construit un niveau runtime minimal :

```text
Trigger.Activated
    -> StoryCompanion.OfferRecruitment
```

avec définition absente et vérifie :

- rejet propre du link command ;
- aucune activation artificielle du target data-only.

Le warning `missing or invalid story companion definition` émis par ce scénario est intentionnel et fait partie du contrat testé.

### EditorLinkPolicy

Vérifie :

- une seule commande disponible ;
- `OfferRecruitment` présente ;
- support `Gameplay` ;
- `Toggle` refusé ;
- target recevable par l'éditeur ;
- aucun événement source implicite.

## Validation obtenue

Le 23 août 2026, UE5.5.4 a exécuté le filtre complet :

```text
Grimrock.MON20.4.RecruitmentUI
```

Résultat : **13 tests / 13 Success**.

Les tests MON20.4.2, MON20.4.3 et MON20.4.4 sont donc tous verts avant la tranche de placement Grid Editor MON20.4.5.

Le PIE de clôture reste à effectuer sur un vrai niveau :

```text
Trigger -> OfferRecruitment -> WBP
```

puis les cas :

- Recruter ;
- Refuser ;
- groupe complet ;
- compagnon déjà actif ;
- tentative d'ouverture d'un second modal.

## Hors périmètre

MON20.4.4 n'ajoute pas :

- système de dialogue général ;
- Actor NPC compagnon dédié ;
- persistance du refus ;
- mutation directe du groupe par Event/Command ;
- API Lua parallèle ;
- équipement matérialisé du compagnon ;
- logique Blueprint de recrutement.
