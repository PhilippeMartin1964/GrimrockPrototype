# Project Context - GrimrockPrototype

## 1. Vision
Développer un dungeon crawler inspiré de Legend of Grimrock 2 sous Unreal Engine 5.5.4 et Visual Studio 2022.

Développer un jeu de type dungeon crawler en vue subjective, à déplacement case par case, inspiré de Legend of Grimrock 2, avec une architecture reposant sur un asset de niveau unique.

Edition de niveaux, ajout de mécanismes pour la résolution de puzzles, inventaires des fonctionnalités de Legend of Grimrock 2 comprenant toutes améliorations possibles.

Source GitHub : https://github.com/PhilippeMartin1964/GrimrockPrototype

## 2. Current Goal
Obtenir une exploration case par case façon dungeon crawler avec déplacement fluide, collisions, interactions simples et affichage 3D minimal.

Le projet doit permettre :
- l’édition directe de niveaux (Nord-Sud, Est-Ouest et Hauteur-profondeur), chacun ayant une grille 32x32 ;
- la création de géométrie jouable : cellules, murs, portes, plafonds, passages secrets ;
- le placement d’objets interactifs : boutons, leviers, plaques de pression, téléporteurs, triggers, spawns ;
- la définition de liens logiques entre objets ;
- l’ajout de mécanismes programmables via un système d’événements et un langage léger à définir ;
- l’exécution du niveau dans un runtime jouable avec déplacement, rotation, interaction et résolution d’énigmes ;
- à terme, l’ouverture à la création de niveaux par les joueurs.

## 3. Gameplay Pillars
Les principes qui doivent guider les choix.
- Exploration en grille
- Ambiance donjon
- Progression par énigmes
- Combat temps réel ou tour par tour (à privilégier)
- Gestion de groupe, inventaire, sorts, etc.

## 4. Technical Stack
Le développement se fait en C++ sous Unreal Engine 5.5.4, avec Visual Studio 2022, en privilégiant une architecture simple, modulaire et orientée données.
- Unreal Engine 5.5.4
- C++ (Visual Studio 2022)
- clang-format 19.1.5 fourni par Visual Studio 2022 pour la baseline STYLE01
- FAB UE5
- Le jeu devra à terme pouvoir être lancé en mode standalone. Question ouverte : Comment rendre l'éditeur de niveau standalone ?

## 5. Repository Map
Vue rapide des dossiers/fichiers importants. Structure de projet (sujet à évolution et amélioration) :
C++ :
Source/
└── GrimrockPrototype/
    ├── GrimrockPrototype.Build.cs
    ├── GrimrockPrototype.cpp
    ├── Public/
    │   ├── Core/
    │   │   ├── GridDirectionUtils.h
    │   │   ├── GridLevelAsset.h
    │   │   ├── GridObjectArchetypeAsset.h
    │   │   ├── GridObjectBehavior.h
    │   │   ├── GridObjectPaletteAsset.h
    │   │   └── GridTypes.h	
    │   ├── Runtime/
    │   │   ├── GridActivationComponent.h
    │   │   ├── GridButtonActor.h
    │   │   ├── GridDoorActor.h
    │   │   ├── GridDoorSystemComponent.h
    │   │   ├── GridEditorPreviewComponent.h
    │   │   ├── GridEditorPreviewObjectActor.h
    │   │   ├── GridLevelRuntimeActor.h
    │   │   ├── GridLeverActor.h
    │   │   ├── GridMechanismActor.h
    │   │   ├── GridPressurePlateActor.h
    │   │   ├── GridReceptacleActor.h
    │   │   ├── GridRuntimeObjectActor.h
    │   │   ├── GridSecretDoorActor.h
    │   │   ├── GridTriggerActor.h
    │   │   └── GrimrockPartyPawn.h	
    │   └── EditorTools/
    │       ├── GridLevelEditorActor.h
	│		├── GridLevelEdMode.h
	│		└── GridLevelEdModeToolkit.h	
    └── Private/
        ├── Core/
        │   └── GridLevelAsset.cpp
        ├── Runtime/
		│   ├── GridActivationComponent.cpp
		│   ├── GridButtonActor.cpp
		│   ├── GridDoorActor.cpp
		│   ├── GridDoorSystemComponent.cpp
		│   ├── GridEditorPreviewComponent.cpp
		│   ├── GridEditorPreviewObjectActor.cpp
		│   ├── GridLevelRuntimeActor.cpp
		│   ├── GridLeverActor.cpp
		│   ├── GridMechanismActor.cpp
		│   ├── GridPressurePlateActor.cpp
		│   ├── GridReceptacleActor.cpp
		│   ├── GridRuntimeObjectActor.cpp
		│   ├── GridSecretDoorActor.cpp
		│   ├── GridTriggerActor.cpp
		│   └── GrimrockPartyPawn.cpp		
        └── EditorTools/
            ├── GridLevelEditorActor.cpp
			├── GridLevelEdMode.cpp
			└── GridLevelEdModeToolkit.cpp

UE5 :
Content/GrimrockPrototype/
├───Blueprints
│   ├───Editor
│   │       BP_GridEditorPreviewObjectActor.uasset
│   │       BP_GridLevelEditorActor.uasset
│   │
│   └───Runtime
│           BP_GridButtonActor.uasset
│           BP_GridDoorActor.uasset
│           BP_GridLevelRuntimeActor.uasset
│           BP_GridLeverActor.uasset
│           BP_GridPressurePlateActor.uasset
│           BP_GridSecretDoor.uasset
│           BP_GridTriggerActor.uasset
│           BP_GrimrockPartyPawn.uasset
│           BP_TorchHolder.uasset
│
├───Core
│   ├───DataAssets
│   │       DA_Arch_Button_ToggleDoor.uasset
│   │       DA_Arch_Lever_OpenSecret.uasset
│   │       DA_Arch_Plate_HoldDoor.uasset
│   │       DA_Button_Secret_Stone.uasset
│   │       DA_Door_Stone.uasset
│   │       DA_GridLevelAsset.uasset
│   │       DA_ObjectPalette_Default.uasset
│   │       DA_SecretDoor_Stone1.uasset
│   │       DA_TorchHolder.uasset
│   │       DA_Trigger_Cell.uasset
│   │
│   └───Input
│           IA_MoveBackward.uasset
│           IA_MoveForward.uasset
│           IA_StrafeLeft.uasset
│           IA_StrafeRight.uasset
│           IA_TurnLeft.uasset
│           IA_TurnRight.uasset
│           IA_Use.uasset
│           IMC_Grimrock.uasset
│
├───Icons
│       T_Tool_Erase.uasset
│       T_Tool_Link.uasset
│       T_Tool_PaintCell.uasset
│       T_Tool_PaintObject.uasset
│       T_Tool_PaintWall.uasset
│       T_Tool_Select.uasset
│
├───Maps
│       L_GrimrockEditor.umap
│       L_GrimrockRuntime.umap
│
├───Materials
│   │   M_Ceiling_Editor.uasset
│   │   M_EditorGrid.uasset
│   │   M_GridInteractable_Master.uasset
│   │   M_Master_Floor.uasset
│   │   M_Master_Metalic.uasset
│   │   M_Master_Wood.uasset
│   │   M_Object_Hover.uasset
│   │   M_PP_EditorOutline.uasset
│   │
│   └───Textures
│       ├───Button
│       │   │   MI_Button_01.uasset
│       │   │   MI_Button_02.uasset
│       │   │   MI_Button_Static_01.uasset
│       │   │
│       │   ├───Generated_01
│       │   │       T_ButtonSigil_BaseColor.uasset
│       │   │       T_ButtonSigil_Height.uasset
│       │   │       T_ButtonSigil_Normal_DirectX_UE.uasset
│       │   │       T_ButtonSigil_Opacity.uasset
│       │   │       T_ButtonSigil_ORM_UE_R_AO_G_Roughness_B_Metallic.uasset
│       │   │       T_Button_BaseColor.uasset
│       │   │       T_Button_Height.uasset
│       │   │       T_Button_Normal_DirectX_UE.uasset
│       │   │       T_Button_Opacity.uasset
│       │   │       T_Button_ORM_UE_R_AO_G_Roughness_B_Metallic.uasset
│       │   │
│       │   ├───Metallic_01
│       │   │       Metal053B_4K-PNG_Color.uasset
│       │   │       Metal053B_4K-PNG_Metalness.uasset
│       │   │       Metal053B_4K-PNG_NormalDX.uasset
│       │   │       Metal053B_4K-PNG_Roughness.uasset
│       │   │
│       │   └───Mettalic_02
│       │           Metal047B_4K-PNG_Color.uasset
│       │           Metal047B_4K-PNG_Metalness.uasset
│       │           Metal047B_4K-PNG_NormalDX.uasset
│       │           Metal047B_4K-PNG_Roughness.uasset
│       │
│       ├───Ceil
│       │   │   MI_CeilEditing.uasset
│       │   │   MI_CeilVault_Stone_01.uasset
│       │   │   MI_CeilVault_Stone_02.uasset
│       │   │   MI_CeilVault_Wood_01.uasset
│       │   │
│       │   ├───StoneCeil-01
│       │   │       Tiles089_4K-PNG_AmbientOcclusion.uasset
│       │   │       Tiles089_4K-PNG_Color.uasset
│       │   │       Tiles089_4K-PNG_NormalDX.uasset
│       │   │       Tiles089_4K-PNG_Roughness.uasset
│       │   │
│       │   └───WoodBeamCeil_01
│       │           wooden_rough_planks_ao_4k.uasset
│       │           wooden_rough_planks_diff_4k.uasset
│       │           wooden_rough_planks_nor_dx_4k.uasset
│       │           wooden_rough_planks_rough_4k.uasset
│       │
│       ├───Door
│       │   │   MI_WoodenDoor_01.uasset
│       │   │
│       │   └───Wood
│       │           wooden_garage_door_ao_4k.uasset
│       │           wooden_garage_door_arm_4k.uasset
│       │           wooden_garage_door_diff_4k.uasset
│       │           wooden_garage_door_disp_4k.uasset
│       │           wooden_garage_door_nor_dx_4k.uasset
│       │           wooden_garage_door_rough_4k.uasset
│       │
│       ├───Floor
│       │   │   MI_Floor_Stone_01.uasset
│       │   │
│       │   └───Stone_01
│       │           Tiles090_4K-PNG_AmbientOcclusion.uasset
│       │           Tiles090_4K-PNG_Color.uasset
│       │           Tiles090_4K-PNG_NormalDX.uasset
│       │           Tiles090_4K-PNG_Roughness.uasset
│       │           T_Floor_Stone_01_AO.uasset
│       │           T_Floor_Stone_01_BaseColor.uasset
│       │           T_Floor_Stone_01_Normal.uasset
│       │           T_Floor_Stone_01_Roughness.uasset
│       │
│       └───Wall
│           │   MI_Wall_Stone_01.uasset
│           │   MI_Wall_Stone_02.uasset
│           │   MI_Wall_Stone_03.uasset
│           │   MI_Wall_Stone_04.uasset
│           │
│           ├───Stone_01
│           │       T_Wall_Stone_01_AO.uasset
│           │       T_Wall_Stone_01_BaseColor.uasset
│           │       T_Wall_Stone_01_Normal.uasset
│           │       T_Wall_Stone_01_Roughness.uasset
│           │
│           ├───Stone_02
│           │       T_Wall_Stone_02_AO.uasset
│           │       T_Wall_Stone_02_BaseColor.uasset
│           │       T_Wall_Stone_02_Normal.uasset
│           │       T_Wall_Stone_02_Roughness.uasset
│           │
│           └───Stone_03
│                   T_Wall_Stone_03_AO.uasset
│                   T_Wall_Stone_03_BaseColor.uasset
│                   T_Wall_Stone_03_Normal.uasset
│                   T_Wall_Stone_03_Roughness.uasset
│
└───Meshes
    │   SM_Button.uasset
    │   SM_EditorGridPlane.uasset
    │   SM_Grid_Lever.uasset
    │   SM_Grid_PressurePlate.uasset
    │   SM_Grid_SecretWall.uasset
    │   SM_SecretButton_03.uasset
    │   SM_TorchHolder.uasset
    │
    ├───Button
    │       SM_Button_Mettalic_Mobile.uasset
    │       SM_Button_Mettalic_Static.uasset
    │
    ├───Ceil
    │       SM_CeilVault_01.uasset
    │
    ├───Door
    │       SM_Door_Stone_static_01.uasset
    │       SM_Door_Wood_Mobile_01.uasset
    │
    ├───Floor
    │       SM_Floor_Stone_01.uasset
    │
    ├───Object
    │       SM_Torch.uasset
    │       SM_Torch_Support.uasset
    │
    └───Wall
            SM_Wall_Stone_03.uasset
            SM_Wall_Stone_SecretDoor-01.uasset
            SM_Wall_Stone_SecretDoorStatic-01.uasset

## 6. Current State
Ce qui fonctionne déjà.
- Création d'un premier niveau
	- Ajout de cellule avec murs et plafonds
	- Ajout d'objets; porte, pressure plate, levier, bouton, porte secrète, ...
	- Link entre les objets pour les activer
- Déplacements, touches ADSW et QE, hochement de tête RBM
- Caméra, Head bob, camera sway
- Grimrock Grid Editor, interface à améliorer
- Ouverture/fermeture de portes animées.

## 7. Known Issues
Bugs ou limites connues.
- ...
- ...

## 8. Next Tasks
Liste courte et priorisée.
- [ ] Validation de l'architecture
- [ ] Implémenter toutes les mécaniques fondamentales de Legend of Grimrock 2
- [ ] Inventaire de tous les objets possibles et leur utilisation ou non utlisation (éléments de décor ou d'ambiance)
- [ ] Système d'inventaire de groupe ou de personnage séparé de GrimrockPartyPawn.
- [ ] Elaboration de mécaniques JdR (Race, Classe, ...)
- [ ] Tester ...

## 9. Design Decisions
Décisions déjà prises, pour éviter de les rediscuter.
- Le déplacement est basé sur une grille sur chaque niveau 32 x 32
- Les niveaux sont décrits en cellule 32 x 32  avec des murs avec pilier
- Le joueur contrôle la classe GrimrockPartyPawn
- La caméra, Head bob, camera sway, avance avortée sur case non accessible.

## 10. Open Questions
Questions encore non tranchées.
- Combat temps réel ou tour par tour ? 
- Format des niveaux ? 
- Style graphique ? 
- Plateforme cible ? 

## 11. ChatGPT Notes
Résumé des idées importantes produites dans le projet ChatGPT.

Trop pour résumé ici... on verra à mesure.

## 12. Codex Working Notes
À utiliser par Codex pour noter les changements importants, conventions découvertes, ou prochaines pistes.
