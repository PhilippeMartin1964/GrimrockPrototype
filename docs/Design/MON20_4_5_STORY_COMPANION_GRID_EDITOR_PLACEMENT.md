# MON20.4.5 — Story Companion Grid Editor Placement

## Objectif

Fermer le dernier écart d'authoring entre le bridge MON20.4.4 et l'outil de niveau : un compagnon narratif doit pouvoir être placé depuis la palette du Grid Editor sans éditer manuellement `UGridLevelAsset::Objects`.

MON20.4.5 ne change pas la transaction de recrutement. Il prépare uniquement les données nécessaires au target `StoryCompanion` puis laisse `OfferRecruitment` ouvrir le modal déjà validé.

## Source de vérité d'authoring

La définition du compagnon est portée par l'entrée de palette :

```text
FGridObjectPaletteEntry
    DefaultArchetype
    DefaultStoryCompanionDefinition
```

Cette décision évite deux sources de vérité concurrentes entre :

- l'entrée de palette utilisée pour créer l'objet ;
- l'inspecteur d'une instance déjà placée.

Pour plusieurs compagnons, le projet peut réutiliser **un même archetype** `StoryCompanion` et créer plusieurs entrées de palette, chacune avec un `DefaultStoryCompanionDefinition` différent.

Exemple :

```text
Archetype: StoryCompanion_Recruit

Palette Entry: StoryCompanion_Scout
    DefaultArchetype = StoryCompanion_Recruit
    DefaultStoryCompanionDefinition = DA_Companion_Scout

Palette Entry: StoryCompanion_Mage
    DefaultArchetype = StoryCompanion_Recruit
    DefaultStoryCompanionDefinition = DA_Companion_Mage
```

## Placement

`AGridLevelEditorActor::PlaceSelectedObject()` copie désormais automatiquement :

```text
PaletteEntry.DefaultStoryCompanionDefinition
        ->
FGridLevelObjectData.StoryCompanionDefinition
```

lorsque le type placé est :

```text
EGridLevelObjectType::StoryCompanion
```

L'objet placé conserve également son `PaletteEntryId` et son `ArchetypeId` comme les autres objets du Grid Editor.

Aucun Actor NPC n'est créé par cette opération. `StoryCompanion` reste un target logique/data-only pour `OfferRecruitment`.

## Validation de palette

`UGridObjectPaletteAsset::ValidatePalette()` impose désormais pour une entrée dont l'archetype utilise `StoryCompanion` :

- `DefaultStoryCompanionDefinition` non nul ;
- `DefaultStoryCompanionDefinition->IsValidDefinition()` vrai.

Une entrée de palette d'un autre type ne reçoit pas cette contrainte.

Cette validation empêche de créer volontairement un tile de compagnon inutilisable, tout en conservant le rejet runtime MON20.4.4 comme deuxième ligne de défense si les données deviennent invalides ultérieurement.

## Archetype recommandé

Pour le premier test de production :

```text
ArchetypeId       = StoryCompanion_Recruit
DisplayName       = Story Companion
Gameplay Type     = StoryCompanion
Placement Kind    = Center
Palette Category  = Story
Functional Category = Decoration
Default Initially Enabled = true
Default Initially Active  = false
Runtime Actor Class = None
Runtime Interactable = false
Runtime Readable = false
```

`Functional Category = Decoration` est conservé pour cette tranche afin de ne pas ajouter une nouvelle taxonomie fonctionnelle uniquement pour le recrutement.

Un `PreviewMesh` peut être renseigné si l'on souhaite rendre le target visible dans la prévisualisation de l'éditeur. Il n'est pas requis par le contrat de recrutement.

## Liens

Le target placé reçoit les liens déjà définis en MON20.4.4 :

```text
Trigger.Activated
    -> StoryCompanion.OfferRecruitment
```

ou, par exemple :

```text
Button.Activated
    -> StoryCompanion.OfferRecruitment
```

ou via MON19 :

```lua
grid.command("CompanionTarget", "OfferRecruitment")
```

Le `StoryCompanion` n'émet aucun événement propre dans MON20.4.

## Tests MON20.4.5

Le filtre reste :

```text
Grimrock.MON20.4.RecruitmentUI
```

Trois tests supplémentaires sont ajoutés :

```text
PaletteContract
PaletteMissingDefinition
PalettePlacement
```

### PaletteContract

Vérifie :

- la présence réfléchie de `DefaultStoryCompanionDefinition` ;
- qu'une entrée non-StoryCompanion n'est pas forcée à fournir cette définition.

### PaletteMissingDefinition

Vérifie qu'une entrée `StoryCompanion` sans définition par défaut est refusée par `ValidatePalette()` avec un diagnostic explicite.

### PalettePlacement

Utilise un vrai `AGridLevelEditorActor` dans un monde de test et vérifie :

- sélection de l'entrée de palette ;
- placement d'un objet `StoryCompanion` ;
- conservation de `PaletteEntryId` ;
- conservation de `ArchetypeId` ;
- copie exacte de `DefaultStoryCompanionDefinition` vers `FGridLevelObjectData::StoryCompanionDefinition`.

## Validation UE5.5.4 obtenue

Le 24 août 2026, le filtre complet :

```text
Grimrock.MON20.4.RecruitmentUI
```

a été exécuté sous UE5.5.4 avec le résultat :

```text
16 tests / 16 Success
```

Les trois tests MON20.4.5 `PaletteContract`, `PaletteMissingDefinition` et `PalettePlacement` sont donc validés, en plus des treize tests MON20.4.2 à MON20.4.4 déjà verts.

## Validation PIE réelle

Un vrai `DA_StoryCompanion_Scout`, un archetype `StoryCompanion_Recruit`, une entrée de palette et le lien suivant ont été utilisés dans le niveau :

```text
Trigger.Activated
    -> StoryCompanion.OfferRecruitment
```

### Refuser — validé

Le passage sur le Trigger :

- exécute `OfferRecruitment` avec `Success=true` ;
- ouvre `WBP_RPGStoryCompanionRecruitment` ;
- applique le modal guard et met le jeu en pause ;
- le clic `Refuser` produit `Declined` ;
- restaure l'état d'input et ferme le modal ;
- ne recrute aucun personnage.

### Recruter — validé

Le même Trigger :

- ouvre le WBP de production ;
- exécute le recrutement du `Companion_Scout` ;
- conserve le `CharacterId` stable du Data Asset ;
- fait passer `ActiveCharacters` de `1` à `2` ;
- restaure ensuite le modal guard et l'input.

Le pipeline PIE complet est donc validé :

```text
Trigger
    -> Activated
    -> OfferRecruitment
    -> ShowStoryCompanionRecruitmentWidget
    -> WBP_RPGStoryCompanionRecruitment
    -> EnsureCandidateRegistered
    -> TryRecruitFromPool
```

## Correctif de focus modal après PIE

Les deux scénarios PIE ont aussi révélé le diagnostic UE5 :

```text
InputMode:UIOnly - Attempting to focus Non-Focusable widget SObjectWidget
```

La cause était l'appel explicite :

```cpp
InputMode.SetWidgetToFocus (TakeWidget ());
```

sur le `UUserWidget` racine, qui n'est pas focusable par défaut dans le WBP de production.

Le correctif supprime ce focus forcé et conserve uniquement `FInputModeUIOnly`, conformément au comportement déjà utilisé par `URPGLevelUpWidget`. Le modal reste pilotable à la souris et n'impose plus un focus invalide au widget racine.

Ce correctif doit être revalidé par un PIE rapide après récupération du commit de clôture afin de confirmer la disparition de cette ligne `Error`.

## Hors périmètre

MON20.4.5 n'ajoute pas :

- Actor NPC compagnon ;
- dialogue général ;
- déplacement ou IA de compagnon dans le donjon ;
- override per-instance de la définition dans le panneau contextualisé ;
- nouvelle catégorie fonctionnelle d'archetype ;
- persistance du refus ;
- équipement matérialisé ;
- logique Blueprint de recrutement.
