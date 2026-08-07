# MON12.1 — Premier panneau d’actions de combat

> **Statut historique.** MON12.7 conserve ce Widget Blueprint comme panneau de
> statut d'un personnage, mais ses vues et widgets `MainHand / OffHand` ont été
> supprimés. Les actions actuelles proviennent exclusivement du catalogue
> générique. Voir `MON12_7_ACTION_ORIENTED_COMBAT_HUD.md`.

## Résultat

MON12.1 ajoute un panneau UMG réutilisable pour un membre du groupe. Il lit
directement :

- le portrait, le nom, les points de vie et la mana depuis
  `UGridPartyInventoryComponent` ;
- `MainHand` et `OffHand` depuis l’équipement réel du personnage ;
- les icônes depuis les `UGridItemDefinitionAsset` enregistrés ;
- les quantités depuis les véritables `FGridItemInstance` ;
- l'état de tour, les PA et l’autorisation d’agir depuis
  `UGridTurnManagerComponent`.

Le panneau ne possède aucun inventaire parallèle. Sa structure `View` est
uniquement un instantané de rendu reconstruit depuis les sources réelles.

MON12.1 ne contient aucun clic d’attaque, aucune formule de dégâts et aucune
mutation directe d’objet.

## Architecture

### `UGridCombatActionPanelWidget`

`UGridCombatActionPanelWidget` représente exactement un personnage.

- `InitializeCombatActionPanel(PartyPawn, CharacterIndex, TurnManager)` associe
  les sources ;
- un `CharacterIndex` explicite fixe le panneau à un membre ;
- `INDEX_NONE` (`-1` dans les détails Unreal) suit le personnage sélectionné ;
- les widgets UMG sont tous `BindWidgetOptional` afin que le style reste dans
  le Widget Blueprint ;
- aucun `Tick` n’est activé.

La même classe pourra donc être instanciée quatre fois dans MON12.7 avec les
indices `0`, `1`, `2` et `3`.

### Autorités

| Donnée | Autorité |
| --- | --- |
| personnage, portrait, PV, mana | `UGridPartyInventoryComponent` |
| `MainHand`, `OffHand`, quantité | `UGridPartyInventoryComponent` |
| icône réelle | `UGridItemDefinitionAsset::Icon` |
| état de tour et PA | `UGridTurnManagerComponent::GetPlayerCharacterTurnState()` |
| autorisation actuelle | `UGridTurnManagerComponent::CanCharacterAct()` |
| rendu et couleur | `UGridCombatActionPanelWidget` / Widget Blueprint |

`CanCharacterAct()` centralise les conditions de phase joueur, combat actif,
groupe au repos, personnage vivant, état `Active` et PA restants.
`CanCharacterSpendActionPoints()` vérifie en plus le coût précis. Le widget ne
réimplémente aucune de ces règles.

### Actualisation sans `Tick`

Le panneau écoute :

- `OnPartyInventoryChanged` ;
- `OnPhaseChanged` ;
- `OnRoundStarted` ;
- `OnPlayerAttackRequested` ;
- `OnPlayerAttackResolved` ;
- `OnAttackResolved` ;
- `OnCombatEnded`.

`OnPartyInventoryChanged` ne transporte qu’un index. Le destinataire relit
toujours l’état courant dans le composant d’inventaire.

Le recalcul de poids déjà exécuté après les transferts d’inventaire et
d’équipement déclenche maintenant cette notification. Cela couvre notamment
le décrément d’une pile de shurikens équipée sans introduire de polling.

## Comportement visuel natif

- le panneau est masqué hors combat par défaut ;
- `Active` correspond à un personnage capable de dépenser ses PA ;
- `Completed` correspond à un personnage sans PA ou dont la phase est finie ;
- `Waiting`, `Incapacitated` et `Defeated` complètent l'état de tour ;
- le panneau est désactivé par `SetIsEnabled(false)` et reçoit une opacité de
  `0.45` quand `CanCharacterAct()` vaut `false` ;
- la quantité est affichée pour un objet empilable ;
- un objet non empilable conserve son icône mais masque le texte de quantité ;
- une main vide masque son icône et sa quantité.

La torche équipée demeure une source lumineuse. MON12.1 n’appelle aucune
fonction de synchronisation du visuel tenu et ne réintroduit donc pas les armes
non lumineuses devant la caméra.

## Construction manuelle précise du Widget Blueprint

Les assets Unreal binaires n’étant pas modifiés automatiquement, effectuer une
fois les opérations suivantes après compilation C++.

### 1. Créer le Widget Blueprint

1. Ouvrir le Content Browser.
2. Créer le dossier
   `/Game/GrimrockPrototype/Blueprints/UI/Combat` s’il n’existe pas.
3. Cliquer avec le bouton droit dans ce dossier.
4. Choisir **User Interface > Widget Blueprint**.
5. Dans **Pick Parent Class**, rechercher
   `GridCombatActionPanelWidget`.
6. Sélectionner cette classe native.
7. Nommer l’asset `WBP_GridCombatActionPanel`.
8. Ouvrir l’asset et vérifier dans **Class Settings** que **Parent Class**
   vaut `GridCombatActionPanelWidget`.

### 2. Construire la hiérarchie

Créer cette hiérarchie. Respecter exactement les noms en gras et cocher
**Is Variable** pour les onze widgets nommés :

```text
Canvas_Root
└── SizeBox_ActionPanel
    └── Border_Background
        └── Overlay_Content
            ├── HorizontalBox_Content
            │   ├── SizeBox_Portrait
            │   │   └── Image_Portrait
            │   └── VerticalBox_Details
            │       ├── Text_Name
            │       ├── HorizontalBox_Vitals
            │       │   ├── Text_PV_Label
            │       │   ├── Text_Health
            │       │   ├── Text_Mana_Label
            │       │   └── Text_Mana
            │       ├── HorizontalBox_Hands
            │       │   ├── SizeBox_MainHand
            │       │   │   └── Overlay_MainHand
            │       │   │       ├── Border_MainHand
            │       │   │       ├── Image_MainHandIcon
            │       │   │       └── Text_MainHandQuantity
            │       │   └── SizeBox_OffHand
            │       │       └── Overlay_OffHand
            │       │           ├── Border_OffHand
            │       │           ├── Image_OffHandIcon
            │       │           └── Text_OffHandQuantity
            │       └── Border_ActionState
            │           └── Text_ActionState
            └── Panel_DisabledOverlay (Border)
```

Les noms liés au C++ sont exactement :

1. `Image_Portrait` ;
2. `Text_Name` ;
3. `Text_Health` ;
4. `Text_Mana` ;
5. `Image_MainHandIcon` ;
6. `Text_MainHandQuantity` ;
7. `Image_OffHandIcon` ;
8. `Text_OffHandQuantity` ;
9. `Border_ActionState` ;
10. `Text_ActionState` ;
11. `Panel_DisabledOverlay`.

### 3. Régler une présentation fonctionnelle

1. Sélectionner `Canvas_Root` et vérifier qu’il remplit l’écran.
2. Sélectionner `SizeBox_ActionPanel`.
3. Régler **Width Override** à `420` et **Height Override** à `170`.
4. Dans son `Canvas Panel Slot`, choisir l’ancre **Bottom Left**.
5. Régler **Position X** à `24` et **Position Y** à `-194`.
6. Régler **Alignment X** à `0` et **Alignment Y** à `0`.
7. Donner à `Border_Background` une couleur sombre avec un alpha d’au moins
   `0.85`.
8. Régler `SizeBox_Portrait` à `112 × 144`.
9. Régler **Width Override** et **Height Override** de `SizeBox_MainHand` et
   `SizeBox_OffHand` à `64`.
10. Dans chaque `Overlay`, placer l’icône en remplissage et la quantité en bas à
   droite.
11. Saisir `PV` dans `Text_PV_Label` et `Mana` dans `Text_Mana_Label`.
12. Créer `Panel_DisabledOverlay` avec le type **Border**, le placer en dernier
   dans `Overlay_Content`, puis régler son slot horizontal et vertical sur
   **Fill**.
13. Donner à `Panel_DisabledOverlay` une couleur noire avec un alpha compris
   entre `0.25` et `0.40`, en remplissage complet.
14. Régler `Panel_DisabledOverlay` sur **Not Hit-Testable (Self & All
   Children)**.
15. Ne créer aucun `Button` et ne placer aucune logique dans l’Event Graph.
16. Compiler puis sauvegarder `WBP_GridCombatActionPanel`.

Le C++ écrit l'état de tour, les PA, les valeurs `actuel / max`,
les icônes et les quantités. Aucun binding Blueprint n’est nécessaire.

### 4. Associer le panneau au Pawn

1. Ouvrir
   `/Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockPartyPawn`.
2. Cliquer **Class Defaults**.
3. Ouvrir la catégorie **Combat > UI**.
4. Affecter `WBP_GridCombatActionPanel` à
   **Combat Action Panel Widget Class**.
5. Laisser **Combat Action Panel Character Index** à `-1` pour suivre le
   personnage sélectionné.
6. Pour tester une association fixe, saisir un index valide, par exemple `0`.
7. Laisser **Combat Action Panel ZOrder** à `50`.
8. Compiler puis sauvegarder le Blueprint.

Le Pawn crée une seule instance au démarrage. Le panneau reste masqué jusqu’au
début du combat.

## Tests automatisés ajoutés

Filtre :

```text
Grimrock.Monsters.MON12
```

Deux tests sont ajoutés :

| Test | Vérifications |
| --- | --- |
| `CombatActionPanel.LiveData` | nom, portrait, PV, mana, mains, icônes, quantité empilable, torche non empilable, `Active`, PA, autorisation et décrément notifié |
| `CombatActionPanel.TurnAuthority` | suivi du personnage sélectionné, changement d’index sans panneau spécialisé, désactivation hors phase joueur, `Completed` et actualisation des PV après attaque ennemie |

Les tests n’utilisent aucun asset `Content`.

## Procédure détaillée de validation PIE

### Préparation

1. Compiler le projet en **Development Editor / Win64**.
2. Effectuer la construction manuelle du Widget Blueprint ci-dessus.
3. Ouvrir le niveau de validation MON11 déjà utilisé.
4. Vérifier qu’un personnage finalisé est présent.
5. Équiper une pile d’au moins trois shurikens en `MainHand`.
6. Équiper la torche en `OffHand`.
7. Vérifier que `DA_Item_Shuriken` et la définition de torche possèdent leur
   véritable propriété `Icon`.
8. Conserver un Rat géant vivant dans la zone de perception.

### Affichage initial

1. Lancer PIE.
2. Avant le combat, vérifier que le panneau est masqué.
3. Approcher le Rat jusqu’au déclenchement du combat.
4. Attendre `PlayerPhase`.
5. Vérifier que le panneau apparaît en bas à gauche.
6. Vérifier le portrait et le nom du personnage.
7. Vérifier `PV actuel / PV max`.
8. Vérifier `Mana actuelle / Mana max`.
9. Vérifier l’icône réelle du shuriken dans `MainHand`.
10. Vérifier l’icône réelle de la torche dans `OffHand`.
11. Vérifier la quantité initiale de shurikens.
12. Vérifier que la quantité `1` de la torche n’est pas affichée.
13. Vérifier `Active`, `PA 4 / 4`, la couleur verte et l’opacité normale.

### Action déjà accomplie

1. Sans cliquer le panneau, utiliser la commande de test MON11 existante :
   appuyer une fois sur `NumPad 7`.
2. Vérifier immédiatement `PA 2 / 4` et le maintien de l'état `Active`.
3. Vérifier la couleur grise, l’overlay sombre et l’état désactivé.
4. Vérifier que la quantité de shurikens diminue exactement d’une unité.
5. Vérifier le shuriken visible pendant le vol puis récupérable au sol.
6. Appuyer une seconde fois sur `NumPad 7`.
7. Après une seconde attaque acceptée, vérifier que la troisième est refusée
   avec `InsufficientActionPoints`.
8. Vérifier qu’aucun second projectile et aucun second décrément ne se
   produisent.

### PV et nouvelle manche

1. Appuyer sur `NumPad 2` pour terminer la phase joueur.
2. Pendant `EnemyPhase`, vérifier que le panneau reste visible mais désactivé.
3. Si le Rat touche le personnage, vérifier l’actualisation immédiate des PV.
4. Attendre la manche suivante.
5. Vérifier le retour à `Active`, `PA 4 / 4` et la réactivation du panneau.

### Non-régressions MON11.4.2

1. Vérifier que la torche équipée continue d’éclairer.
2. Vérifier que le shuriken n’est pas affiché en permanence devant la caméra.
3. Vérifier qu’il reste visible pendant le vol.
4. Vérifier qu’il reste visible et récupérable au sol.
5. Vérifier qu’il ne revient pas comme un boomerang.
6. Vérifier qu’une collision ne produit aucun dégât supplémentaire.
7. Terminer le combat et vérifier que le panneau disparaît.

## Hors périmètre historique

- clic `MainHand` ou `OffHand` pour attaquer : MON12.2 ;
- quatre panneaux simultanés : désormais MON12.7 après la fondation V2 ;
- sorts : désormais MON12.8 via le catalogue d'actions commun ;
- calcul de dégâts dans l’interface ;
- modification d’inventaire depuis l’interface ;
- modification automatique d’un `.uasset`.
