# UI-SPLIT01 — Independent Character Sheet and Inventory Bag Windows

Date : **20 septembre 2026**  
Statut : **AUTOMATION VALIDÉE LOCALEMENT — 20 septembre 2026 ; migration UMG à réaliser**

## Décision

La page Inventaire n'est plus conçue comme un grand écran monolithique contenu dans WBP_GrimrockMenu.

La cible devient :

~~~text
Viewport
├── WBP_CharacterSheet
│   └── fenêtre gauche indépendante, pleine hauteur utile
├── vue 3D du donjon visible au centre
├── WBP_InventoryBag
│   └── fenêtre droite indépendante, pleine hauteur utile
└── WBP_GridCombatHud
    └── barre basse persistante UI-NAV01
~~~

Il s'agit de **deux UUserWidget réellement séparés et de deux instances viewport distinctes**.

## Autorité unique conservée

La séparation est purement visuelle.

~~~text
UGridPartyInventoryComponent
└── SelectedCharacterIndex
    ├── WBP_CharacterSheet
    └── WBP_InventoryBag
~~~

Aucun second inventaire, aucun second personnage sélectionné et aucun stockage d'équipement parallèle n'est introduit.

Les deux fenêtres réutilisent UGridInventoryWidget comme implémentation commune des projections et interactions déjà validées.

Deux classes sémantiques très fines sont ajoutées :

~~~text
UGridCharacterSheetWidget : UGridInventoryWidget
UGridInventoryBagWidget   : UGridInventoryWidget
~~~

Elles ne possèdent aucun état gameplay supplémentaire.

## Synchronisation

UGridInventoryWidget::InitializeInventoryWidget() s'abonne désormais à UGridPartyInventoryComponent::OnPartyInventoryChanged.

Chaque fenêtre se rafraîchit donc à partir de la même autorité.

~~~text
clic PartyMember_2 dans CharacterSheet
-> SetSelectedCharacterIndex(1)
-> OnPartyInventoryChanged
-> CharacterSheet RefreshInventory()
-> InventoryBag RefreshInventory()
~~~

Le sac suit immédiatement le personnage choisi dans la feuille.

## Routage runtime

Lorsque les deux classes split sont configurées sur AGrimrockPartyPawn :

~~~text
I
-> ToggleInventoryWidget()
-> ShowInventoryWorkspace()
-> WBP_CharacterSheet ajouté au viewport
-> WBP_InventoryBag ajouté au viewport
~~~

WBP_GrimrockMenu n'est plus créé pour l'Inventaire dans ce mode.

Il reste temporairement disponible comme shell legacy pour Skills, Spellbook, Journal, Map, Recipes et Codex jusqu'à leur migration éventuelle.

## Sémantique de I et des croix

~~~text
les deux fenêtres visibles + I
    -> ferme les deux

les deux fenêtres fermées + I
    -> ouvre les deux

une seule fenêtre encore visible + I
    -> restaure les deux
~~~

Les croix restent indépendantes :

~~~text
X CharacterSheet -> ferme seulement la fenêtre gauche
X InventoryBag   -> ferme seulement la fenêtre droite
~~~

Si la seconde fenêtre restante est elle aussi fermée, le runtime clôt le mode UI global et restaure l'état de fermeture normal.

## WBP à créer

### WBP_CharacterSheet

Parent : UGridCharacterSheetWidget.

Contenu à déplacer depuis l'ancien WBP_GridInventory :

~~~text
Panel_CharacterSheet
├── PartyMember_1..6
├── identité
├── attributs
├── stats dérivées
├── résistances
├── paper doll
├── 18 slots équipement
└── Button_CloseCharacterSheet
~~~

Le widget racine couvre le viewport pour permettre l'ancrage, mais reste Self Hit Test Invisible ; seule la fenêtre gauche capte la souris.

Ancrage de la fenêtre visuelle :

~~~text
Left / Top-Bottom
Top    = 0
Bottom = hauteur réservée à la barre HUD
Left   = 0
~~~

### WBP_InventoryBag

Parent : UGridInventoryBagWidget.

Contenu à déplacer depuis l'ancien WBP_GridInventory :

~~~text
Panel_InventoryBag
├── Text_InventoryBagTitle
├── InventorySlotsGridPanel
├── Text_InventoryBagSlotUsage
├── Text_InventoryBagWeight
├── ProgressBar_InventoryBagWeight
└── Button_CloseInventoryBag
~~~

Même règle : racine viewport Self Hit Test Invisible, panneau droit seul hit-testable.

Ancrage :

~~~text
Right / Top-Bottom
Top    = 0
Bottom = hauteur réservée à la barre HUD
Right  = 0
~~~

## WBP_GrimrockMenu

En mode split, il ne possède plus l'Inventaire.

UI-SPLIT01 rend facultatifs côté C++ :

~~~text
Button_TabInventory
Button_TabSkills
Button_TabJournal
Button_TabMap
Button_TabRecipes
Button_TabCodex
Button_TabSpellbook
Page_Inventory
~~~

Les boutons supérieurs peuvent donc être retirés ou collapsed pendant la migration sans casser les bindings C++.

Le shell reste uniquement un fallback pour les pages encore non migrées.

## Drag/drop inter-fenêtres

Le drag/drop UI-INV02 reste valide entre deux widgets différents parce que l'opération transporte SourceCharacterIndex, SourceSlotType, SourceSlotIndex, SourceItemDefinitionId et SourceRuntimeObjectId.

Le portrait cible délègue toujours vers le même UGridItemTransferService.

## Paper doll

Le sac indépendant ne contient volontairement aucun paper doll.

La validation native des 18 slots n'est exécutée que si un widget expose effectivement un conteneur ou au moins un slot paper doll. L'absence du paper doll dans WBP_InventoryBag n'est donc pas un warning.

## Migration UMG

Ne pas recréer les contenus à la main si un déplacement ou une copie depuis l'ancien asset est possible.

~~~text
1. créer WBP_CharacterSheet parent UGridCharacterSheetWidget
2. y déplacer/copier la branche CharacterSheet existante
3. créer WBP_InventoryBag parent UGridInventoryBagWidget
4. y déplacer/copier la branche InventoryBag existante
5. configurer les deux classes sur BP_GrimrockPartyPawn
6. tester I et les deux X
7. tester sélection personnage
8. tester drag/drop sur portrait
9. seulement ensuite décommissionner Page_Inventory de WBP_GrimrockMenu
~~~

Aucun asset binaire Unreal n'est modifié automatiquement par ce ticket.

## Automation

Filtre :

~~~text
Grimrock.UI.Split01
~~~

Tests :

~~~text
Grimrock.UI.Split01.SemanticWindows
Grimrock.UI.Split01.SharedAuthority
~~~

Ils vérifient les deux classes sémantiques, la configuration split, l'autorité partagée de UGridPartyInventoryComponent, la synchronisation de SelectedCharacterIndex et la priorité du sac split pour l'accesseur runtime.

## Validation locale demandée

~~~powershell
.\Scripts\ValidateUE.ps1 \
    -EngineRoot D:\UE_5.5 \
    -AutomationFilter "Grimrock.UI.Split01"
~~~

Ne pas déclarer UI-SPLIT01 validé avant la sortie locale de ce filtre.


## Validation locale

Validation locale confirmée par l'utilisateur le 20 septembre 2026 pour le filtre `Grimrock.UI.Split01`.

La partie C++ de UI-SPLIT01 est donc considérée comme validée. La suite porte exclusivement sur la création/migration UMG de `WBP_CharacterSheet` puis `WBP_InventoryBag`.


## Validation UMG / PIE partielle

Le 20 septembre 2026, la séparation physique en deux fenêtres a été validée manuellement :

~~~text
X gauche  -> ferme seulement CharacterSheet
I         -> restaure les deux
X droite  -> ferme seulement InventoryBag
I         -> restaure les deux
I avec les deux visibles -> ferme les deux
I à nouveau             -> rouvre les deux
ESC                      -> ferme les deux
~~~

Deux exigences complémentaires ont ensuite été identifiées :

- le workspace split doit rester non modal vis-à-vis du monde ;
- le presenter Blueprint du menu clic droit doit être migré de WBP_GridInventory vers WBP_InventoryBag.

La suite est décrite dans UI_SPLIT02_WORLD_INTERACTION_AND_CONTEXT_MENU.md.
