# UI-CLEAN01 — Remove monolithic inventory legacy

Date : **20 septembre 2026**  
Statut : **IMPLEMENTATION C++ ; validation locale UE5.5.4 à fournir**

## Objectif

UI-CLEAN01 rend l'architecture split exclusive. Il n'existe plus de fallback C++ vers l'ancien inventaire monolithique.

Architecture canonique :

~~~text
I
-> AGrimrockPartyPawn::ToggleInventoryWidget()
-> WBP_CharacterSheet / UGridCharacterSheetWidget
-> WBP_InventoryBag   / UGridInventoryBagWidget
~~~

L'ancien chemin est supprimé :

~~~text
WBP_GrimrockMenu
-> Page_Inventory
-> WBP_GridInventory
~~~

## Suppressions C++

UGrimrockMenuWidget ne connaît plus :

~~~text
Page_Inventory
Button_TabInventory
OpenInventoryWorkspace()
RefreshInventory()
GetInventoryWidget()
HandleInventoryTopTabClicked()
~~~

Les anciens boutons supérieurs du shell ne sont plus pilotés par le C++ : la navigation visible est la barre basse UI-NAV01.

UGridInventoryWidget ne connaît plus le faux workspace monolithique :

~~~text
Panel_CharacterSheet
Panel_InventoryBag
Button_CloseCharacterSheet
Button_CloseInventoryBag
bCharacterSheetPanelVisible
bInventoryBagPanelVisible
ResetInventoryWorkspace()
SetCharacterSheetPanelVisible()
SetInventoryBagPanelVisible()
IsCharacterSheetPanelVisible()
IsInventoryBagPanelVisible()
BindWorkspaceButtons()
ApplyWorkspacePanelVisibility()
HandleCloseCharacterSheetClicked()
HandleCloseInventoryBagClicked()
~~~

Le fichier de production devenu mort est supprimé :

~~~text
Private/UI/GridInventoryWidgetWorkspace.cpp
~~~

Le test historique associé est supprimé :

~~~text
Private/Tests/GridUIFoundation01WorkspaceTests.cpp
~~~

## Propriété des croix

Les boutons de fermeture appartiennent désormais aux widgets qui les affichent réellement :

~~~text
UGridCharacterSheetWidget
└── Button_CloseCharacterSheet

UGridInventoryBagWidget
└── Button_CloseInventoryBag
~~~

AGrimrockPartyPawn conserve le comportement validé :

~~~text
X gauche -> ferme seulement CharacterSheet
X droite -> ferme seulement InventoryBag
I avec une fenêtre fermée -> restaure les deux
I avec les deux ouvertes -> ferme les deux
ESC -> ferme le workspace
~~~

## Shell restant

WBP_GrimrockMenu / UGrimrockMenuWidget n'est pas supprimé dans UI-CLEAN01 car il reste réellement utilisé pour Skills, Spellbook, Journal, Map, Recipes et Codex.

Il ne contient plus aucune responsabilité Inventory côté C++.

Il sera supprimé lorsque ces six pages auront elles aussi quitté le shell.

## Nettoyage UMG manuel après validation C++

Une fois l'Automation validée :

1. dans WBP_GrimrockMenu, supprimer définitivement Page_Inventory ;
2. supprimer définitivement les anciens boutons supérieurs s'ils ne sont plus utilisés visuellement ;
3. Compile + Save ;
4. ouvrir Reference Viewer sur WBP_GridInventory ;
5. confirmer qu'aucun asset runtime ne le référence encore ;
6. supprimer WBP_GridInventory ;
7. Fix Up Redirectors sur le dossier UI ;
8. relancer le filtre UI-CLEAN01 puis un PIE inventaire.

Ne pas supprimer WBP_GridInventory avant la validation C++ : un Blueprint sérialisé peut encore contenir une référence tant que WBP_GrimrockMenu n'a pas été sauvegardé après nettoyage.

## Automation

Filtre :

~~~text
Grimrock.UI.Clean01
~~~

Test :

~~~text
Grimrock.UI.Clean01.NoInventoryShellFallback
~~~

Le test vérifie par réflexion que les propriétés/fonctions legacy ne sont plus exposées et que les deux croix appartiennent aux classes split.
