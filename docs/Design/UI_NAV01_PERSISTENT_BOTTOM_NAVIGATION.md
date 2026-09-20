# UI-NAV01 — Persistent Bottom Navigation Bar

Date : **20 septembre 2026**  
Statut : **IMPLEMENTATION C++ + CONTRAT UMG ; validation locale UE5.5.4 à fournir**

## Décision

La barre de navigation est une surface **permanente du HUD runtime**. Elle n'appartient ni à `WBP_GrimrockMenu`, ni à `WBP_GridInventory`, et son affichage ne dépend jamais de l'ouverture de l'Inventaire.

Elle est collée au bord inférieur de l'écran.

```text
┌──────────────────────────────────────────────────────────────────────┐
│                         vue 3D / panneaux                            │
│                                                                      │
├──────────────────────────────────────────────────────────────────────┤
│ ESC | I | K | G | M | J | H || 1 | 2 | 3 | ... | 9 | 0            │
└──────────────────────────────────────────────────────────────────────┘
                                  ↑
                         bord inférieur écran
```

La partie gauche est la navigation globale. La partie droite reste la hotbar MON12 à dix slots. Visuellement, les deux peuvent former une seule bande continue ; fonctionnellement, elles gardent leurs responsabilités séparées.

## Réutilisation de l'existant

Aucun nouveau HUD global n'est créé.

`WBP_GridCombatHud / UGridCombatHudWidget` est déjà :

- créé par `AGrimrockPartyPawn::BeginPlay()` ;
- présent en exploration ;
- propriétaire de la hotbar à dix slots ;
- remonté au-dessus du workspace quand le menu est ouvert.

UI-NAV01 réutilise donc cette surface persistante et lui ajoute uniquement le chrome de navigation.

Les éléments combat-only restent conditionnels. La navigation, elle, ne l'est pas.

## Contrat UMG

Dans `WBP_GridCombatHud`, ajouter un panneau nommé exactement :

```text
Panel_GlobalNavigation
```

avec les boutons optionnels :

```text
Button_NavEscape
Button_NavInventory
Button_NavSkills
Button_NavCrafting
Button_NavMap
Button_NavJournal
Button_NavHelp
```

Structure recommandée :

```text
CanvasPanel_Root
├── éléments HUD combat existants
└── HorizontalBox_BottomBar          Anchors Bottom / Left-Right
    ├── Panel_GlobalNavigation
    │   ├── Button_NavEscape
    │   ├── Button_NavInventory
    │   ├── Button_NavSkills
    │   ├── Button_NavCrafting
    │   ├── Button_NavMap
    │   ├── Button_NavJournal
    │   └── Button_NavHelp
    └── Panel_Actions                hotbar MON12 existante
```

**Important :** `Panel_GlobalNavigation` ne doit pas être enfant de `Panel_CombatHud` si ce dernier est masqué hors combat. Il doit rester dans une branche toujours visible du Widget Tree.

Le conteneur inférieur doit être ancré au bas de la surface avec un offset inférieur nul. Les grands panneaux du menu doivent s'arrêter au-dessus de cette bande.

## Routage unique clavier / souris

Les boutons ne possèdent aucune logique Blueprint métier. Ils appellent les mêmes commandes C++ que le clavier.

| Commande | Touche | Destination actuelle |
|---|---|---|
| Retour | `ESC` | priorité ciblage/modal, puis ferme le menu, puis hook du futur menu principal en jeu |
| Inventaire | `I` | `Inventory` |
| Compétences | `K` | `Skills` |
| Artisanat | `G` | shell actuel `Recipes` |
| Carte | `M` | `Map` |
| Journal | `J` | `Journal` |
| Aide / Codex | `H` | `Codex` |

Le mapping `G -> Recipes` est provisoirement volontaire : la page existante est réutilisée comme shell d'artisanat jusqu'à la réalisation de UI-CRAFT01.

Le mapping `H -> Codex` fournit le point d'entrée actuel pour l'aide/codex. La structuration interne Aide + Codex sera traitée dans UI-CODEX01.

## Sémantique des raccourcis

Pour `I/K/G/M/J/H` :

```text
panneau fermé
    -> ouvre le menu directement sur la page demandée

autre page déjà ouverte
    -> bascule vers la page demandée

même page déjà ouverte
    -> ferme le menu
```

Ainsi, la barre permanente ne dépend pas du menu ; elle ne fait que lui envoyer une intention.

## ESC

`AGrimrockPlayerController::RequestGlobalEscape()` est l'unique route clavier de `ESC`.

Ordre actuel :

```text
1. lancer physique en visée ?
      -> annuler la visée

2. ciblage de combat actif ?
      -> annuler le ciblage

3. autre UI modale propriétaire de l'input ?
      -> ne pas traverser la modale

4. menu Grimrock ouvert ?
      -> fermer d'abord un ItemActionMenu s'il existe
      -> sinon fermer le menu

5. aucun panneau principal ouvert ?
      -> OnInGameMainMenuRequested()
```

`OnInGameMainMenuRequested()` est un hook de présentation pour le futur menu principal/pause **en jeu**. Il ne doit pas appeler directement `RequestReturnToMainMenu()`, qui renvoie au niveau de titre et représente une autre action.

## Invariants

1. La navigation reste visible hors combat.
2. Ouvrir/fermer `WBP_GrimrockMenu` ne masque jamais `Panel_GlobalNavigation`.
3. Aucun second stockage de hotbar n'est ajouté.
4. Aucun second `SelectedCharacterIndex` n'est ajouté.
5. Clavier et clic utilisent les mêmes fonctions C++.
6. La navigation n'exécute aucune logique gameplay.
7. `Panel_GlobalNavigation` est extérieur aux conteneurs combat-only.
8. La barre est collée au bord inférieur de l'écran.
9. Les panneaux Inventory/Skills/etc. réservent la hauteur de cette barre.
10. Les assets UMG ne sont pas modifiés à l'aveugle depuis le dépôt.

## Automation

Filtre :

```text
Grimrock.UI.Navigation01
```

Tests ajoutés :

```text
Grimrock.UI.Navigation01.PersistentBottomBar
Grimrock.UI.Navigation01.PageToggleContract
```

Ils caractérisent :

- visibilité forcée du panneau de navigation même sans combat ;
- comportement toggle de I/K/G/M/J/H sur leur page active ;
- priorité de fermeture du menu par ESC.

## Validation locale demandée

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Navigation01"
```

Après la passe UMG, PIE ciblé :

1. démarrer dans le donjon : la barre est visible sans ouvrir l'inventaire ;
2. `I` ouvre Inventory ; la barre reste visible ;
3. cliquer `K` : passage direct à Skills ;
4. `M`, `J`, `H`, `G` ouvrent leur page ;
5. appuyer ou cliquer la même commande une seconde fois : le menu se ferme ;
6. ESC ferme d'abord le menu ;
7. les slots `1..0` existants restent visibles dans la même bande ;
8. entrée en combat / sortie de combat : la navigation ne disparaît jamais ;
9. aucune régression drag/drop hotbar ;
10. aucun `BindWidget` critique.

Ne pas déclarer UI-NAV01 validé tant que l'Automation locale et le PIE n'ont pas été fournis.
