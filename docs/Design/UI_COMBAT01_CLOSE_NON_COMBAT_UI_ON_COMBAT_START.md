# UI-COMBAT01 — Close Non-Combat UI On Combat Start

## Statut

Implémentation C++ prête pour validation UE5.5.4.

## Objectif

Quand le combat devient actif, les surfaces gameplay non-combat doivent libérer immédiatement l'écran pour le HUD de combat.

Surfaces concernées :

- fenêtre Character Sheet ;
- fenêtre Inventory Bag ;
- shell Skills / Recipes / Map / Journal / Codex ;
- menus contextuels d'items ;
- message lisible runtime éventuellement affiché.

La barre inférieure persistante portée par `WBP_GridCombatHud` n'est pas supprimée.

## Autorité

`UGridTurnManagerComponent` reste l'unique autorité du combat.

`UGridTurnManagerComponent::StartCombatInternal()` est l'unique point où le combat devient autoritaire. Immédiatement après `bCombatActive = true`, il ordonne au PartyPawn de fermer les surfaces non-combat :

```text
TurnManager.StartCombatInternal
    -> bCombatActive = true
    -> PartyPawn.CloseNonCombatUiForCombat()
```

Aucun abonnement UI, état parallèle ou dépendance au cycle de création du HUD n'est nécessaire.

Aucun Monster, système de perception ou appel `StartCombat...` ne connaît les widgets.

## Fermeture sans autosave

`HideInventoryWidget()` conserve son contrat utilisateur historique : une fermeture manuelle peut déclencher l'autosave configuré.

La fermeture imposée par le combat utilise une route distincte :

```cpp
CloseNonCombatUiForCombat()
```

Elle replie les widgets et rend l'input au HUD sans appeler `SaveCurrentGame()`.

Cela évite de créer une seconde sauvegarde au moment où MON18.9.1 possède déjà l'autorité du checkpoint pré-combat.

## Réouverture pendant le combat

Tant que `TurnManager->bCombatActive` est vrai :

- `ToggleInventoryWidget()` ne rouvre pas l'inventaire ;
- `ToggleMenuPage()` ne rouvre aucune page ;
- `ShowInventoryWorkspace()` refuse une ouverture directe ;
- `ShowMenuPage()` refuse une ouverture directe.

Les touches/boutons `I/K/G/M/J/H` restent donc visibles via UI-NAV01 mais sont sans effet sur les grands panneaux pendant le combat.

À la fin du combat, `bCombatActive=false` lève automatiquement ce verrou ; aucun état UI combat séparé n'est persisté.

## Automation

Filtre :

```text
Grimrock.UI.Combat01
```

Tests :

```text
Grimrock.UI.Combat01.CloseNonCombatUi
Grimrock.UI.Combat01.CombatLifecycle
```

Validation PIE attendue :

1. ouvrir l'inventaire ou une page majeure ;
2. déclencher un combat ;
3. vérifier que le panneau disparaît immédiatement ;
4. vérifier que le HUD combat reste visible ;
5. essayer `I/K/G/M/J/H` pendant le combat : aucun grand panneau ne se rouvre ;
6. terminer le combat ;
7. vérifier que la navigation peut de nouveau ouvrir les panneaux.
