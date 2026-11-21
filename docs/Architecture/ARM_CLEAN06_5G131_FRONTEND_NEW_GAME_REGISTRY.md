# ARM-CLEAN06.5G.131 — Réparer l'ItemSystem du menu « Nouvelle partie »

**Parent :** `48bda88d02ef9cce4af3a9bc2415bf922e13b736`, deux statuts Windows SUCCESS. Reproduction PIE réelle : les parties existantes fonctionnent, mais le bouton « Créer le personnage » affiche une erreur et les logs indiquent `StartingEquipment requires configured ItemSystem`, puis `CharacterCreationWizard InitialCreate rejected`.

## Cause et correction

`UGrimrockMainMenuWidget::OpenNewGameCharacterCreation` créait `UGridPartyInventoryComponent` avec `this` (un `UUserWidget`) comme Outer. Or `UGridPartyInventoryComponent` dérive de `UActorComponent` : hors d'un acteur du monde, `GetWorld()` n'identifie pas le monde du menu. `CreateInitialCharacter` ne retrouve donc pas le `UGridItemSubsystem` déjà configuré dans le `GameInstance`.

Le composant provisoire a maintenant pour Outer le `APlayerController` du menu, est ajouté comme instance component, puis enregistré dans son monde. Un contrôle explicite vérifie le même monde et le registre canonique du `GameInstance` **avant** d'ouvrir le wizard. En cas de fermeture, annulation, validation ou échec de création du widget, le composant est détruit sans laisser d'objet provisoire sur le contrôleur.

Les deux `FInputModeUIOnly` du menu ne tentent plus de forcer le focus Slate sur une racine `UUserWidget` non focusable. Le mode UI-only et le curseur de souris restent inchangés.

## Régression et validation

Un nouveau test Automation `Grimrock.StartupFlow.STARTUPFLOW01.FrontendControllerCanonicalInventory` reproduit l'inventaire temporaire sous `PlayerController`, contrôle son `World`, l'accès à `DA_ItemSystem`, puis la création réelle du héros Guerrier avec son kit canonique. Le test détruit ensuite le composant. Les 18 kits déjà testés par `STARTUPFLOW01.NewGameStarterVisualsHandoff18` restent intacts.

**Contrats inchangés :** `DA_ItemSystem` reste l'unique autorité ; pas de fallback legacy ni de modification de `.uasset`, WBP, `.umap`, SaveGame, localisation, nombre de slots ou chargement du donjon. Gates : compilation UE5.5.4, Automation `Grimrock`, deux statuts Windows SUCCESS pour le nouveau SHA ; puis recette PIE manuelle du bouton « Créer le personnage ». Les tests NullRHI ne remplacent pas cette validation visuelle.
