# MON20.4.3 — Story Companion Recruitment Modal Runtime Integration

## Statut

Intégration C++ du modal de recrutement dans le runtime du groupe.

Validation UE5.5.4 effectuée le 23 août 2026 : **10/10 tests `Grimrock.MON20.4.RecruitmentUI` Success** après le correctif de build Unity `e1c06cce262cf0f00b8857af7d27d82b985eb819`.

Cette étape raccorde le noyau UI de MON20.4.2 à `AGrimrockPartyPawn`. Elle ne raccorde pas encore le système `Event -> Command` du niveau et ne nécessite aucun `.uasset` / `.umap`.

## Point d'intégration

`AGrimrockPartyPawn` reste le point de présentation gameplay existant. Aucun `ModalManager`, subsystem UI ou état parallèle n'est ajouté.

Le Pawn expose désormais :

```text
StoryCompanionRecruitmentWidgetClass
StoryCompanionRecruitmentWidgetInstance
StoryCompanionRecruitmentZOrder

ShowStoryCompanionRecruitmentWidget()
CloseStoryCompanionRecruitmentWidget()
IsStoryCompanionRecruitmentModalActive()
GetStoryCompanionRecruitmentWidget()
```

`StoryCompanionRecruitmentWidgetClass` est configurable pour le futur Widget Blueprint de production.

Si aucune classe n'est configurée, le runtime utilise automatiquement `URPGStoryCompanionRecruitmentWidget::StaticClass()`. Le fallback Slate natif de MON20.4.2 reste donc utilisable avant la création du WBP.

## Ouverture

Le flux runtime est :

```text
AGrimrockPartyPawn::ShowStoryCompanionRecruitmentWidget(CompanionDefinition)
  -> valide PartyInventoryComponent
  -> valide URPGStoryCompanionAsset
  -> refuse si Character Creation est active
  -> refuse si un recrutement est déjà affiché
  -> résout PlayerController
  -> choisit WBP configuré ou classe C++ native
  -> CreateWidget
  -> InitializeRecruitmentWidget(PartyInventory, CompanionDefinition)
  -> ClearBufferedCommand()
  -> bind OnClosed
  -> AddToViewport(ZOrder)
```

Le Pawn ne modifie jamais directement `ActiveCharacters` ou `CharacterPool`.

## Garde d'input

Le Pawn ne duplique pas la logique modale.

La garde d'input reste entièrement possédée par `URPGStoryCompanionRecruitmentWidget`, conformément à MON20.4.2 et au précédent `URPGLevelUpWidget` :

```text
SetInventoryUiOpen(true)
DisableInput()
pause si nécessaire
FInputModeUIOnly
curseur visible
```

À la fermeture, le widget restaure l'état précédent. Le Pawn se contente de libérer sa référence lorsque `OnClosed` est émis.

Cette séparation évite deux propriétaires concurrents du pause/input mode.

## Exclusivité modale

MON20.4.3 applique les règles minimales suivantes :

- aucun recrutement pendant la création initiale du personnage ;
- aucun second recrutement empilé sur un recrutement déjà visible ;
- le buffer de déplacement/rotation/utilisation est vidé juste avant l'ouverture ;
- le combat hotbar reste bloqué par le mécanisme existant `bInventoryUiOpen` utilisé par la garde modale.

Aucun framework modal générique n'est créé à ce stade.

## Fermeture

`CloseStoryCompanionRecruitmentWidget()` délègue à `CloseRecruitment()` du widget.

Le callback natif `OnClosed` nettoie `StoryCompanionRecruitmentWidgetInstance` uniquement si le widget fermé est bien l'instance courante.

Les chemins `Recruit`, `Decline` et fermeture explicite conservent donc un cycle de vie unique.

## Tests

Le filtre reste :

```text
Grimrock.MON20.4.RecruitmentUI
```

MON20.4.3 ajoute :

```text
RuntimeContract
RuntimeDefaultState
```

`RuntimeContract` vérifie que les propriétés et fonctions de présentation sont exposées sur `AGrimrockPartyPawn`.

`RuntimeDefaultState` vérifie :

- Z-order par défaut = 500 ;
- aucun WBP obligatoire par défaut ;
- aucune instance modale au démarrage ;
- état modal inactif.

Validation locale UE5.5.4 :

```text
AlreadyActiveNoDoubleRecruitment  Success
AlreadyInPool                     Success
DeclineNoMutation                 Success
IdentityCollision                 Success
InvalidDefinition                 Success
NominalRecruitment                Success
PartyFull                         Success
RuntimeContract                   Success
RuntimeDefaultState               Success
ViewProjection                    Success
```

Résultat final MON20.4.3 : **10/10 Success**.

## WBP futur

Le futur asset recommandé reste :

```text
WBP_RPGStoryCompanionRecruitment
  parent C++ : URPGStoryCompanionRecruitmentWidget
```

Il sera assigné à `StoryCompanionRecruitmentWidgetClass` sur `BP_GrimrockPartyPawn` une fois la chaîne C++ validée.

La logique des boutons reste en C++ ; le WBP ne doit contenir aucune mutation du groupe.

## Hors périmètre

MON20.4.3 n'ajoute pas :

- `EGridLevelObjectType::StoryCompanion` ;
- `EGridObjectCommand::OfferRecruitment` ;
- référence compagnon dans `FGridLevelObjectData` ;
- raccord `UGridActivationComponent -> AGrimrockPartyPawn` ;
- API Lua spéciale ;
- placement Grid Editor ;
- persistence du refus ;
- système de dialogue général.

Le raccord data-driven `Event -> Command -> modal` appartient à MON20.4.4.
