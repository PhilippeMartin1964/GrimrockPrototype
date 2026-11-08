# ARMEMENT-LOT02.18 — Correction des icônes absentes après New Game

## Incident PIE (8 novembre 2026)

**Observation utilisateur :** après L_MainMenu → création du personnage → L_Dungeon, les objets N1 apparaissent dans l'inventaire et sur le paperdoll par leur nom et quantité mais leurs images manquent. Après quitter PIE puis revenir par Continue, les mêmes équipements ont leurs visuels normaux.

### Cause démontrée dans le C++

Le frontend crée les 18 kits par UGridPartyInventoryComponent::CreateInitialCharacter. Cette opération construit les instances et enregistre les UGridItemDefinitionAsset dans RuntimeItemDefinitionsById (registre runtime non sérialisé).

UGrimrockMainMenuWidget::HandleInitialCharacterCreationCommitted transmet **seulement** FGridPartyInventoryState à UGrimrockGameInstance::SetPendingNewPartyState, puis OpenLevel ouvre L_Dungeon. Dans UGrimrockStartupModeComponent::BeginPlay, RestorePartyInventoryState était exécuté mais **RehydrateOwnedItemDefinitions était omis** : le nouveau UGridPartyInventoryComponent possède donc les ItemDefinitionId, niveaux, quantités et GUID mais son RuntimeItemDefinitionsById reste vide.

UGridInventorySlotWidget::SetItem appelle GetItemDefinition → FindItemDefinition avant d'appeler FGridWeaponVisualUiResolver et de charger l'icône. Définition manquante = absence d'icône. Les emplacements restent renseignés car les instances sont correctes. Plusieurs fonctions de combat ou d'équipement s'appuient aussi sur les mêmes définitions.

Le chemin Continue appelle déjà AGrimrockPartyPawn::RehydrateLoadedItemDefinitions après RestorePartyInventoryState ; cela reconstitue les définitions, d'où le changement visible après rechargement. Il ne s'agit ni d'une corruption des 18 recettes, ni d'une image d'arme manquante, ni d'un souci de l'UMG Designer.

### Correction C++ minimale

UGrimrockStartupModeComponent::RestorePendingNewGameParty matérialise désormais les **deux moitiés** du transfert avant l'entrée dans le donjon :
1. RestorePartyInventoryState (état sérialisable),
2. RehydrateOwnedItemDefinitions via AGridLevelRuntimeActor::ResolveRuntimeItemDefinition, ou FRPGStartingEquipmentPresetCatalog::ResolveKnownStartingItemDefinition avant disponibilité du runtime,
3. notification finale OnPartyInventoryChanged pour resynchroniser tout slot, HUD et affichage d'objets maintenus par les widgets.

Un objet inconnu interrompt le transfert et annule le personnage finalisé dans le composant de destination au lieu de poursuivre avec une interface et des équipements partiellement inexploitables. Aucun DataAsset, WBP, carte, recette, image ni objet sauvegardé n'est remplacé.

### Tests de non-régression

Grimrock.StartupFlow.STARTUPFLOW01.NewGameStarterVisualsHandoff18 effectue les **18 parcours** (6 classes × 3 kits) avec des classes et items de production :

- CreateInitialCharacter dans un inventaire frontend ; passage par SetPendingNewPartyState / ConsumePendingNewPartyState ;
- destination neuve sans registre d'objets, application de la même routine que UGrimrockStartupModeComponent::BeginPlay ;
- contrôle de tous les objets des kits : résolution des définitions depuis le premier New Game, MainHand/OffHand/sac, niveau N1, quantité, GUID du héros, icônes N1 accessibles au UI VisualResolver, invariants d'ownership ;
- rejet d'un objet inconnu, sans héros finalisé incomplet.

Les tests antérieurs ARMEMENT-LOT02.17 continuent de vérifier les 18 sauvegardes sur disque et leur rechargement. La validation GitHub UE5.5.4/LocalAuto (build + suite Grimrock) porte sur le **SHA final**.

### Recette PIE à confirmer sur écran

1. Dans L_MainMenu, New Game, créer un Ranger / Scout (arc ARM048 en MainHand, dague ARM001 et torche au sac, flèches ×12). Entrer dans L_Dungeon, ouvrir Personnage et Inventaire **sans sauvegarder/continuer** : les icônes des quatre entrées doivent apparaître immédiatement, en particulier MainHand.
2. Refaire avec Warrior / Guardian (ARM014 + ARM075 OffHand) et Mage / Arcanist (ARM061 + ARM070 OffHand). Confirmer l'absence d'emplacements nommés mais visuellement vides.
3. Déplacer un objet entre sac et main, vérifier l'icône, la prévisualisation/infobulle, les contraintes d'équipement. Vérifier que le chargement de l'icône ne crée aucun doublon.
4. Save → quitter le PIE → revenir par Continue : les mêmes objets et icônes doivent subsister, sans différence avec l'état après New Game.

**Statut de clôture :** le test automatisé NullRHI confirme la résolution des définitions et ressources iconographiques mais ne prouve pas le rendu réel du Blueprint. Si UE5.5.4/SilentTask et LocalAuto sont success sur le SHA final : code et automatisation validés ; confirmation graphique PIE à observer.
