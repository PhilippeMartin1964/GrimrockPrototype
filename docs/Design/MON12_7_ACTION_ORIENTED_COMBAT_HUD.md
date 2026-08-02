# MON12.7 — HUD de combat orienté actions

## Résultat

MON12.7 ajoute `UGridCombatHudWidget`, un HUD racine événementiel qui affiche :

- quatre instances distinctes de `UGridCombatActionPanelWidget`, une par index
  de personnage de `0` à `3` ;
- les PA et l'état de tour de chaque membre présent ;
- les PAM communs lus depuis `FGridPartyMobilityState` ;
- les actions du personnage actif générées depuis le catalogue MON12.6 ;
- un bouton `Fin du tour` autorisé exclusivement par le TurnManager ;
- l'ordre d'initiative runtime, limité à huit entrées, avec le combattant actif
  agrandi et un indicateur `+ N` exact.

Le HUD ne recalcule ni l'initiative, ni les coûts, ni les disponibilités. Il
n'utilise pas `Tick`. Ses actualisations proviennent des événements du
TurnManager et de l'inventaire du groupe.

`AGrimrockPartyPawn::CombatHudWidgetClass` est optionnelle. Tant qu'elle n'est
pas configurée, `CombatActionPanelWidgetClass` et le panneau historique restent
le fallback affiché.

## Architecture C++

`FGridCombatHudViewModelBuilder` construit des projections testables pour les
quatre membres, les actions, les PAM et l'initiative. Il copie l'ordre fourni
par `GetUpcomingInitiativeOrder()` et applique seulement la capacité visuelle
de huit entrées ; il ne trie aucun combattant.

`UGridCombatHudWidget` :

- obtient les actions avec `GetAvailableCombatActions()` ;
- exécute chaque clic uniquement avec `RequestCharacterCombatAction()` ;
- obtient l'autorisation de fin de tour avec `CanEndActivePlayerTurn()` ;
- exécute la commande uniquement avec `EndActivePlayerTurn()` ;
- se rafraîchit sur les événements d'inventaire, PA, PAM, phase, ordre,
  combattant actif, état de combattant, résolution et fin de combat.

Les boutons `MainHand` et `OffHand` du panneau historique sont masqués lorsque
le panneau est intégré au nouveau HUD. Les sources d'équipement restent dans
les données de l'action générique, sans devenir des boutons codés en dur.

## Configuration exacte dans Unreal Editor 5.5.4

Fermer l'éditeur avant une compilation C++. Après compilation réussie, rouvrir
le projet et procéder dans cet ordre.

### 1. Widget d'une action

Créer dans `Content/GrimrockPrototype/Blueprints/UI/Combat` un Widget Blueprint
nommé `WBP_GridCombatHudAction`, avec comme classe parente
`UGridCombatHudActionWidget`.

Créer les widgets suivants et cocher `Is Variable` pour chacun. Les noms et les
types doivent être exacts :

| Nom BindWidget | Type UMG | Rôle |
| --- | --- | --- |
| `Button_Action` | Button | clic et état activé/désactivé |
| `Image_ActionIcon` | Image | icône du catalogue |
| `Text_ActionName` | TextBlock | nom de l'action |
| `Text_ActionCost` | TextBlock | coût PA et ressources |
| `Text_DisabledReason` | TextBlock | raison d'indisponibilité |

Placer l'image et les textes dans le bouton. Ne créer aucun événement de clic
Blueprint : la classe C++ route déjà le clic. Prévoir visuellement l'opacité
réduite et suffisamment de place sous le nom pour la raison de désactivation.

### 2. Slot d'initiative

Créer `WBP_GridCombatHudInitiativeSlot`, parent
`UGridCombatHudInitiativeSlotWidget`.

Créer les variables exactes :

| Nom BindWidget | Type UMG | Rôle |
| --- | --- | --- |
| `Image_Portrait` | Image | portrait personnage ou icône monstre |
| `Text_Name` | TextBlock | nom du combattant |
| `Text_Health` | TextBlock | PV courants / maximum |
| `Text_State` | TextBlock | état autoritaire |
| `Text_Side` | TextBlock | camp Party ou Monster |
| `Border_Active` | Border | cadre lumineux du combattant actif |

Donner au slot normal une taille proche de `56 x 56` et assez d'espace pour
les textes compacts. Le C++ applique une échelle `1.28` au premier slot actif ;
le cadre `Border_Active` est visible uniquement pour celui-ci.

### 3. HUD racine

Créer `WBP_GridCombatHud`, parent `UGridCombatHudWidget`.

Construire une racine plein écran, puis ajouter les variables exactes :

| Nom BindWidget | Type UMG conseillé | Position |
| --- | --- | --- |
| `Panel_CombatHud` | Canvas Panel ou Overlay | conteneur global |
| `Panel_Initiative` | Horizontal Box | haut-centre |
| `Text_InitiativeOverflow` | TextBlock | après la barre d'initiative |
| `Panel_PartyMembers` | Horizontal Box | bas-gauche |
| `Panel_Actions` | Wrap Box ou Horizontal Box | bas-centre |
| `Text_MobilityActionPoints` | TextBlock | près des contrôles de déplacement |
| `Button_EndTurn` | Button | bas-droite |
| `Text_EndTurnDisabledReason` | TextBlock | près du bouton Fin du tour |

Les trois panneaux dynamiques doivent être vides dans le Designer : le C++ y
crée les quatre panneaux de membres et les entrées correspondant aux
instantanés runtime.

Dans les Class Defaults de `WBP_GridCombatHud`, affecter :

- `Party Member Panel Widget Class` = le Widget Blueprint historique dérivé de
  `UGridCombatActionPanelWidget` (actuellement
  `WBP_GridCombatActionPanel`) ;
- `Action Widget Class` = `WBP_GridCombatHudAction` ;
- `Initiative Slot Widget Class` = `WBP_GridCombatHudInitiativeSlot`.

Le bouton `Button_EndTurn` peut contenir un TextBlock non variable avec le
libellé `Fin du tour`. Ne brancher aucune logique Blueprint sur ce bouton.

### 4. Association au Pawn

Ouvrir `BP_GrimrockPartyPawn`, puis dans `Combat | UI` :

1. affecter `Combat Hud Widget Class` = `WBP_GridCombatHud` ;
2. conserver `Combat Action Panel Widget Class` =
   `WBP_GridCombatActionPanel` pour le fallback ;
3. conserver le Z-order existant, sauf conflit visuel explicite ;
4. compiler et sauvegarder le Blueprint.

Pour revenir temporairement au panneau historique, mettre uniquement
`Combat Hud Widget Class` à `None`. Aucun autre changement n'est nécessaire.

## Tests automatisés

Les tests du jalon sont :

- `Grimrock.Monsters.MON12.CombatHUD.ViewModel` ;
- `Grimrock.Monsters.MON12.CombatHUD.Lifecycle`.

Ils couvrent les quatre panneaux, la projection du catalogue, le routage de
l'action générique, la mise à jour événementielle des PA, le changement de
combattant actif, la limite de huit entrées, `+ N`, le refus de fin de tour en
mouvement et l'absence de dépense de PA après refus.

## Vérifications PIE

1. Lancer une carte avec quatre personnages et une rencontre comprenant au
   moins un monstre.
2. Vérifier quatre panneaux distincts, avec portraits, noms, PV, mana, PA et
   états indépendants. Un membre absent doit être masqué et un vaincu désactivé.
3. Vérifier que seul le personnage actif est mis en évidence et que la barre
   centrale contient les actions réellement fournies par son catalogue.
4. Équiper une arme offensive puis une torche non offensive : l'arme doit
   contribuer son action, la torche ne doit pas créer un bouton d'attaque.
5. Utiliser une action à `2 PA` : le panneau doit passer immédiatement de
   `4 / 4` à `2 / 4` sans attente et sans polling.
6. Tenter une action devenue indisponible : le bouton doit être grisé, la
   raison visible, et aucun PA ni item ne doit être consommé.
7. Déplacer le groupe : vérifier `1 PA + 1 PAM`, le texte PAM mis à jour et le
   bouton `Fin du tour` désactivé pendant l'interpolation.
8. Tourner le groupe de 90 degrés : vérifier que la rotation reste gratuite.
9. Cliquer `Fin du tour` au repos : le TurnManager doit activer le combattant
   suivant et mettre à jour les quatre panneaux, les actions et l'initiative.
10. Provoquer un ordre de plus de huit combattants : vérifier huit slots au
    maximum et `+ N` avec la valeur exacte.
11. Vérifier que le premier slot correspond toujours à l'actif, est agrandi et
    que les suivants glissent sans changement de leur ordre runtime.
12. Mettre `Combat Hud Widget Class` à `None`, relancer PIE et confirmer que le
    panneau historique s'affiche encore.

MON12.8, les sorts, le mana transactionnel et les zones d'effet restent hors
périmètre de ce jalon.
