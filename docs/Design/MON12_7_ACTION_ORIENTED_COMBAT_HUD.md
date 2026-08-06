# MON12.7 — HUD de combat orienté actions

## Résultat

MON12.7 ajoute `UGridCombatHudWidget`, un HUD racine événementiel qui affiche :

- quatre instances distinctes de `UGridCombatActionPanelWidget`, une par index
  de personnage de `0` à `3` ;
- les PA et l'état de tour de chaque membre présent ;
- les PAM communs lus depuis `FGridPartyMobilityState` ;
- les actions du personnage actif générées depuis le catalogue MON12.6 ;
- un bouton `Fin du tour` autorisé exclusivement par le TurnManager ;
- la chronologie d'initiative runtime, fixée à huit activations par défaut,
  avec le combattant actif agrandi et les frontières de rounds indiquées.

Le HUD ne recalcule ni l'initiative, ni les coûts, ni les disponibilités. Il
n'utilise pas `Tick`. Ses actualisations proviennent des événements du
TurnManager et de l'inventaire du groupe.

`AGrimrockPartyPawn::CombatHudWidgetClass` est optionnelle. Tant qu'elle n'est
pas configurée, `CombatActionPanelWidgetClass` et le panneau historique restent
le fallback affiché.

## Architecture C++

`FGridCombatHudViewModelBuilder` construit des projections testables pour les
quatre membres, les actions, les PAM et l'initiative. Il copie l'ordre fourni
par `GetInitiativePreview()` et applique seulement la capacité visuelle
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

## Ce qui doit être réalisé manuellement dans Unreal Editor

Le C++ ne crée aucun asset `.uasset`. Les quatre opérations suivantes sont
donc obligatoires dans Unreal Editor 5.5.4 :

1. vérifier le Widget Blueprint historique `WBP_GridCombatActionPanel` ;
2. créer `WBP_GridCombatHudAction` ;
3. créer `WBP_GridCombatHudInitiativeSlot` ;
4. créer `WBP_GridCombatHud` et l'affecter à `BP_GrimrockPartyPawn`.

> **Correction importante — référence C++ publiée `a0a86af`**
>
> `WBP_GridCombatHudAction` et `WBP_GridCombatHudInitiativeSlot` sont bien
> nécessaires. Le commit publié déclare réellement les classes parentes
> `UGridCombatHudActionWidget` et `UGridCombatHudInitiativeSlotWidget`, ainsi
> que les propriétés `ActionWidgetClass` et `InitiativeSlotWidgetClass` dans
> `UGridCombatHudWidget`. Il ne faut donc ni supprimer ces deux assets, ni les
> remplacer par des widgets construits directement dans le HUD racine.
>
> Les noms `Panel_CharacterPanels`, `Panel_ActionBar`,
> `Panel_InitiativeBar`, `Panel_ActionArea` et `Text_PartyMobility` appartiennent
> à une autre variante locale du code et **ne correspondent pas** au commit
> MON12.7 publié. Avec `a0a86af`, utiliser exclusivement les huit noms du
> tableau de l'étape UE5.4.8, notamment `Panel_PartyMembers`, `Panel_Actions`,
> `Panel_Initiative` et `Text_MobilityActionPoints`.
>
> En revanche, l'éditeur d'un **Widget Blueprint** UE 5.5.4 n'affiche pas le
> bouton `Class Defaults` montré dans l'éditeur d'un Blueprint d'Actor. La
> présente procédure utilise désormais la méthode adaptée : `Graph`, puis
> `My Blueprint`, `Show Inherited Variables` et enfin `Default Value` dans le
> panneau `Details`.

Les propriétés C++ utilisent `BindWidgetOptional`. Cela évite un crash si un
widget manque, mais ne signifie pas que les widgets décrits ci-dessous sont
facultatifs. Un nom absent ou un type incorrect produit une partie vide du HUD
sans nécessairement produire d'erreur de compilation Blueprint.

Les noms `BindWidget` doivent être recopiés exactement, avec les mêmes
majuscules, les mêmes caractères `_` et le type UMG indiqué.

## Étape UE5.0 — Compiler le C++ et préparer le dossier

1. Fermer complètement Unreal Editor.
2. Dans Visual Studio 2022, sélectionner :
   - configuration : `Development Editor` ;
   - plateforme : `Win64` ;
   - cible : `GrimrockPrototypeEditor`.
3. Exécuter `Build > Build Solution`.
4. Vérifier que la compilation se termine sans erreur C++, UHT ou édition de
   liens.
5. Rouvrir `GrimrockPrototype.uproject` dans Unreal Engine 5.5.4.
6. Dans le Content Browser, créer le dossier suivant s'il n'existe pas :

   ```text
   /Game/GrimrockPrototype/Blueprints/UI/Combat
   ```

7. Dans le Content Browser, activer si nécessaire :

   ```text
   Settings > Show C++ Classes
   ```

8. Vérifier que les trois classes parentes suivantes sont proposées lors de
   la création ou du reparentage d'un Widget Blueprint :
   - `GridCombatHudActionWidget` ;
   - `GridCombatHudInitiativeSlotWidget` ;
   - `GridCombatHudWidget`.

Dans l'éditeur, Unreal affiche généralement les noms sans le préfixe C++ `U`.
Si une classe n'apparaît pas, ne créer aucune logique de remplacement en
Blueprint : fermer l'éditeur, recompiler le C++ puis rouvrir le projet.

## Étape UE5.1 — Vérifier le panneau de personnage existant

MON12.7 ne demande pas de recréer le panneau de MON12.1. Il réutilise
`WBP_GridCombatActionPanel` quatre fois.

1. Ouvrir l'asset existant :

   ```text
   /Game/GrimrockPrototype/Blueprints/UI/Combat/WBP_GridCombatActionPanel
   ```

   Si l'asset se trouve actuellement dans un autre dossier, le conserver à cet
   endroit et sélectionner cet asset comme valeur de
   `PartyMemberPanelWidgetClass` à l'étape UE5.5.

2. Cliquer `Class Settings`.
3. Vérifier que `Parent Class` vaut `GridCombatActionPanelWidget`.
4. Dans le `Designer`, vérifier au minimum les widgets de statut utilisés par
   les quatre panneaux :

   | Nom exact | Type UMG | Information écrite par le C++ |
   | --- | --- | --- |
   | `Image_Portrait` | Image | portrait du membre |
   | `Text_Name` | TextBlock | nom du membre |
   | `Text_Health` | TextBlock | PV actuels et maximum |
   | `Text_Mana` | TextBlock | mana actuel et maximum |
   | `Text_ActionPoints` | TextBlock | PA actuels et maximum |
   | `Text_ActionState` | TextBlock | état du tour |
   | `Border_ActionState` | Border | couleur de l'état du tour |
   | `Panel_DisabledOverlay` | Widget, par exemple Border | voile d'indisponibilité |

5. Les widgets historiques suivants peuvent rester dans l'asset :

   | Nom exact | Type UMG |
   | --- | --- |
   | `Button_MainHand` | Button |
   | `Image_MainHandIcon` | Image |
   | `Text_MainHandQuantity` | TextBlock |
   | `Button_OffHand` | Button |
   | `Image_OffHandIcon` | Image |
   | `Text_OffHandQuantity` | TextBlock |

6. Ne pas supprimer et ne pas masquer manuellement les deux boutons de main.
   Lorsque le HUD MON12.7 crée ses quatre panneaux, le C++ applique
   `bShowHandActionButtons = false` et les masque automatiquement. Ils restent
   disponibles lorsque le panneau historique est utilisé seul en fallback.
7. Cocher `Is Variable` pour chaque widget portant un nom de la liste.
8. Cliquer `Compile` puis `Save`.

Le HUD fournit lui-même les indices `0`, `1`, `2` et `3`. Ne créer ni quatre
copies manuelles de `WBP_GridCombatActionPanel`, ni quatre variables d'index
dans le Blueprint.

## Étape UE5.2 — Créer le Widget Blueprint d'une action

### UE5.2.1 — Créer l'asset et définir sa classe parente

1. Ouvrir le dossier :

   ```text
   /Game/GrimrockPrototype/Blueprints/UI/Combat
   ```

2. Faire un clic droit dans une zone vide.
3. Choisir :

   ```text
   User Interface > Widget Blueprint
   ```

4. Dans `Pick Parent Class`, rechercher
   `GridCombatHudActionWidget` et sélectionner cette classe.
5. Nommer l'asset exactement :

   ```text
   WBP_GridCombatHudAction
   ```

6. Ouvrir le nouvel asset.
7. Cliquer `Class Settings` et confirmer :

   ```text
   Parent Class = GridCombatHudActionWidget
   ```

Si Unreal crée directement un `UserWidget` sans proposer de classe parente,
ouvrir `Class Settings`, modifier `Parent Class`, rechercher
`GridCombatHudActionWidget`, puis compiler.

### UE5.2.2 — Construire la hiérarchie du Designer

Créer la hiérarchie suivante. Les noms précédés de `BindWidget` sont
fonctionnels et obligatoires ; les autres noms sont seulement des conteneurs
de mise en page.

- `SizeBox_ActionRoot` — `Size Box`, widget racine ;
  - `Button_Action` — `Button`, **BindWidget** ;
    - `Overlay_ActionContent` — `Overlay` ;
      - `HorizontalBox_ActionContent` — `Horizontal Box` ;
        - `SizeBox_ActionIcon` — `Size Box` ;
          - `Image_ActionIcon` — `Image`, **BindWidget** ;
        - `VerticalBox_ActionTexts` — `Vertical Box` ;
          - `Text_ActionName` — `TextBlock`, **BindWidget** ;
          - `Text_ActionCost` — `TextBlock`, **BindWidget** ;
          - `Text_DisabledReason` — `TextBlock`, **BindWidget**.

Un `Button` n'accepte qu'un seul enfant direct. Il faut donc mettre
`Overlay_ActionContent` dans le bouton, puis placer l'image et les textes dans
les conteneurs internes.

### UE5.2.3 — Régler les propriétés utiles

1. Sélectionner `SizeBox_ActionRoot` :
   - `Width Override` : `180` ;
   - `Min Desired Height` : `72`.
2. Sélectionner `Button_Action` :
   - Horizontal Alignment : `Fill` ;
   - Vertical Alignment : `Fill` ;
   - `Is Focusable` : activé pour permettre la navigation clavier ;
   - ne créer aucun événement `OnClicked` dans le Graph.
3. Sélectionner `SizeBox_ActionIcon` :
   - `Width Override` : `48` ;
   - `Height Override` : `48` ;
   - marge recommandée : `6`.
4. Sélectionner `Image_ActionIcon` :
   - Horizontal Alignment : `Fill` ;
   - Vertical Alignment : `Fill` ;
   - `Visibility` initiale : `Hit Test Invisible`.
5. Sélectionner `Text_ActionName` :
   - justification : `Center` ou `Left` selon le style du HUD ;
   - police recommandée : semi-grasse ;
   - ne pas saisir un nom d'action définitif : le C++ le remplace.
6. Sélectionner `Text_ActionCost` :
   - texte de prévisualisation possible : `2 PA` ;
   - ne pas lier le texte dans le Graph.
7. Sélectionner `Text_DisabledReason` :
   - `Auto Wrap Text` : activé ;
   - couleur recommandée : rouge ou orange atténué ;
   - `Visibility` initiale : `Collapsed`.
8. Pour chacun des cinq widgets fonctionnels, cocher `Is Variable` et vérifier
   le nom exact dans le Widget Tree.

Le C++ effectue automatiquement les opérations suivantes :

- remplit `Image_ActionIcon` depuis l'icône de la définition d'action ;
- remplit `Text_ActionName` ;
- produit `Text_ActionCost` sous la forme `2 PA`, éventuellement complétée par
  `| N mana` et `| xN` ;
- désactive `Button_Action` si l'action est indisponible ;
- affiche `Text_DisabledReason` uniquement dans ce cas ;
- applique une opacité de `0.45` à l'ensemble du widget indisponible ;
- route le clic vers `RequestCharacterCombatAction()`.

Il ne faut donc créer dans le Graph :

- aucun événement `OnClicked` ;
- aucun appel au TurnManager ;
- aucun calcul de coût ;
- aucun `Tick` ;
- aucun binding UMG exécuté chaque frame.

Cliquer `Compile`, corriger toute erreur de type ou de nom, puis cliquer
`Save`.

## Étape UE5.3 — Créer le Widget Blueprint d'un slot d'initiative

### UE5.3.1 — Créer l'asset et définir sa classe parente

1. Dans le même dossier, créer un nouveau `Widget Blueprint`.
2. Choisir comme classe parente :

   ```text
   GridCombatHudInitiativeSlotWidget
   ```

3. Nommer l'asset exactement :

   ```text
   WBP_GridCombatHudInitiativeSlot
   ```

4. Ouvrir l'asset et vérifier dans `Class Settings` :

   ```text
   Parent Class = GridCombatHudInitiativeSlotWidget
   ```

### UE5.3.2 — Construire la hiérarchie du Designer

Créer la hiérarchie suivante :

- `SizeBox_InitiativeRoot` — `Size Box`, widget racine ;
  - `Overlay_Initiative` — `Overlay` ;
    - `Border_Background` — `Border`, décoration non liée ;
    - `Image_Portrait` — `Image`, **BindWidget** ;
    - `VerticalBox_InitiativeTexts` — `Vertical Box` ;
      - `Text_Name` — `TextBlock`, **BindWidget** ;
      - `Text_Health` — `TextBlock`, **BindWidget** ;
    - `HorizontalBox_InitiativeState` — `Horizontal Box` ;
      - `Text_Side` — `TextBlock`, **BindWidget** ;
      - `Text_State` — `TextBlock`, **BindWidget** ;
    - `Border_Active` — `Border`, **BindWidget**.

`Border_Active` doit être le dernier enfant de l'Overlay afin d'être dessiné
au-dessus du slot. Utiliser un brush de cadre avec un centre transparent : il
ne doit pas cacher le portrait ni les textes.

### UE5.3.3 — Régler les propriétés utiles

1. Sélectionner `SizeBox_InitiativeRoot` :
   - `Width Override` : `72` ;
   - `Height Override` : `72` ;
   - marge extérieure recommandée dans son futur slot : `4`.
2. Sélectionner `Image_Portrait` :
   - Horizontal Alignment : `Fill` ;
   - Vertical Alignment : `Fill` ;
   - `Visibility` : `Hit Test Invisible`.
3. Placer `Text_Name` et `Text_Health` en bas du portrait, sur un fond sombre
   semi-transparent si nécessaire pour conserver la lisibilité.
4. Placer `Text_Side` et `Text_State` en haut du portrait avec une petite
   police. Leur valeur est écrite par le C++.
5. Sélectionner `Border_Active` :
   - Horizontal Alignment : `Fill` ;
   - Vertical Alignment : `Fill` ;
   - `Visibility` initiale : `Collapsed` ;
   - couleur suggérée : or ou jaune clair ;
   - ne pas utiliser une couleur de remplissage opaque.
6. Cocher `Is Variable` pour :
   - `Image_Portrait` ;
   - `Text_Name` ;
   - `Text_Health` ;
   - `Text_State` ;
   - `Text_Side` ;
   - `Border_Active`.
7. Ne chercher aucun bouton `Class Defaults` dans cet éditeur. La valeur C++
   par défaut de `ActiveScale` est déjà `1.28`, ce qui est précisément la
   valeur requise par MON12.7. Il n'y a donc rien à renseigner manuellement.

   Pour modifier volontairement cette valeur plus tard :
   - cliquer `Graph` en haut à droite ;
   - afficher le panneau `My Blueprint` avec `Window > My Blueprint` s'il est
     masqué ;
   - ouvrir les options d'affichage de `My Blueprint` et activer
     `Show Inherited Variables` ;
   - rechercher et sélectionner `ActiveScale` ;
   - modifier `Default Value` dans le panneau `Details`.

   Cette modification est facultative et ne doit pas être effectuée pour la
   configuration standard MON12.7.

Le C++ :

- écrit le portrait, le nom, `PV actuel / maximum`, l'état et le camp ;
- affiche `Border_Active` uniquement pour le premier combattant actif ;
- applique une échelle de `1.28` au slot actif et `1.0` aux autres ;
- conserve l'ordre exact fourni par le TurnManager.

Ne créer aucun Graph, aucun tri, aucune animation d'initiative et aucun
`Tick` à ce stade. Cliquer `Compile`, puis `Save`.

## Étape UE5.4 — Créer le Widget Blueprint racine du HUD

### UE5.4.1 — Créer l'asset et définir sa classe parente

1. Dans le même dossier, créer un troisième `Widget Blueprint`.
2. Choisir comme classe parente :

   ```text
   GridCombatHudWidget
   ```

3. Nommer l'asset exactement :

   ```text
   WBP_GridCombatHud
   ```

4. Ouvrir l'asset et vérifier dans `Class Settings` :

   ```text
   Parent Class = GridCombatHudWidget
   ```

### UE5.4.2 — Créer la racine plein écran

1. Dans le Designer, supprimer l'éventuel widget racine créé par défaut s'il
   n'est pas un `Canvas Panel`.
2. Glisser un `Canvas Panel` comme racine.
3. Le renommer exactement :

   ```text
   Panel_CombatHud
   ```

4. Cocher `Is Variable`.
5. Dans les propriétés du Widget Blueprint, utiliser une taille de conception
   correspondant à la résolution de travail, par exemple `1920 x 1080`. Cette
   taille n'impose pas la résolution du jeu ; elle sert uniquement au Designer.

Le C++ applique `Collapsed` à `Panel_CombatHud` hors combat et
`Self Hit Test Invisible` pendant le combat. Si ce nom est absent, le HUD peut
rester visible ou vide à un moment incorrect.

### UE5.4.3 — Construire la barre d'initiative

1. Ajouter dans `Panel_CombatHud` un `Horizontal Box` non lié nommé
   `HorizontalBox_InitiativeArea`.
2. Dans son `Canvas Panel Slot`, régler :
   - Anchors : `Top Center` ;
   - Alignment : `0.5, 0.0` ;
   - Position X : `0` ;
   - Position Y : `24` ;
   - `Auto Size` : activé.
3. Ajouter comme premier enfant un `Horizontal Box` nommé exactement :

   ```text
   Panel_Initiative
   ```

4. Cocher `Is Variable` sur `Panel_Initiative`.
5. Laisser `Panel_Initiative` complètement vide dans le Designer.
6. Le `TextBlock` historique suivant peut rester comme second enfant de
   `HorizontalBox_InitiativeArea` :

   ```text
   Text_InitiativeOverflow
   ```

7. S'il existe, cocher `Is Variable` sur `Text_InitiativeOverflow` et conserver
   sa visibilité initiale sur `Collapsed`. MON12.7.1 le maintient masqué.

Important : `Text_InitiativeOverflow` doit être un frère de
`Panel_Initiative`, pas un enfant. Le C++ gère un pool de slots et de
séparateurs dans `Panel_Initiative` ; tout enfant statique placé dans ce panneau
serait retiré au premier rafraîchissement.

### UE5.4.4 — Construire la zone des quatre personnages

1. Ajouter directement dans `Panel_CombatHud` un `Horizontal Box` nommé :

   ```text
   Panel_PartyMembers
   ```

2. Cocher `Is Variable`.
3. Dans son `Canvas Panel Slot`, régler :
   - Anchors : `Bottom Left` ;
   - Alignment : `0.0, 1.0` ;
   - Position X : `24` ;
   - Position Y : `-24` ;
   - `Auto Size` : activé.
4. Laisser ce `Horizontal Box` complètement vide.

Ne déposer aucune instance de `WBP_GridCombatActionPanel` dans le Designer.
Le C++ crée exactement quatre instances, leur affecte les indices `0` à `3`
et masque automatiquement celles dont le membre n'est pas présent.

### UE5.4.5 — Construire la barre d'actions

1. Ajouter directement dans `Panel_CombatHud` un `Wrap Box` nommé :

   ```text
   Panel_Actions
   ```

   Un `Horizontal Box` est également compatible, mais le `Wrap Box` évite de
   sortir de l'écran lorsque le catalogue contiendra davantage d'actions.

2. Cocher `Is Variable`.
3. Dans son `Canvas Panel Slot`, régler :
   - Anchors : `Bottom Center` ;
   - Alignment : `0.5, 1.0` ;
   - Position X : `0` ;
   - Position Y : `-24` ;
   - taille recommandée : `700 x 180` ;
   - `Auto Size` : désactivé pour que le Wrap Box puisse revenir à la ligne.
4. Régler l'espacement interne du `Wrap Box` selon le style du HUD, par
   exemple `8` horizontalement et verticalement.
5. Laisser `Panel_Actions` complètement vide.

Le C++ détruit les anciens enfants et recrée un
`WBP_GridCombatHudAction` pour chaque action du catalogue du personnage actif.
Ne placer donc ni bouton fixe `MainHand`, ni bouton fixe `OffHand`, ni libellé
statique à l'intérieur de `Panel_Actions`.

### UE5.4.6 — Afficher les PAM communs

1. Ajouter directement dans `Panel_CombatHud` un `TextBlock` nommé :

   ```text
   Text_MobilityActionPoints
   ```

2. Cocher `Is Variable`.
3. Régler son texte de prévisualisation sur :

   ```text
   PAM 2 / 2
   ```

4. Le placer près des commandes de déplacement, par exemple :
   - Anchors : `Bottom Right` ;
   - Alignment : `1.0, 1.0` ;
   - Position X : `-240` ;
   - Position Y : `-72` ;
   - `Auto Size` : activé.

Le texte est remplacé par le C++ après chaque notification de mobilité. Ne
créer aucun binding Blueprint sur cette valeur.

### UE5.4.7 — Construire la commande Fin du tour

1. Ajouter dans `Panel_CombatHud` un `Vertical Box` non lié nommé
   `VerticalBox_EndTurnArea`.
2. Dans son `Canvas Panel Slot`, régler :
   - Anchors : `Bottom Right` ;
   - Alignment : `1.0, 1.0` ;
   - Position X : `-24` ;
   - Position Y : `-24` ;
   - `Auto Size` : activé.
3. Ajouter dans ce `Vertical Box` un `Button` nommé exactement :

   ```text
   Button_EndTurn
   ```

4. Cocher `Is Variable` sur `Button_EndTurn`.
5. Ajouter dans le bouton un `TextBlock` non variable contenant :

   ```text
   Fin du tour
   ```

6. Sous le bouton, ajouter un `TextBlock` nommé exactement :

   ```text
   Text_EndTurnDisabledReason
   ```

7. Cocher `Is Variable` sur `Text_EndTurnDisabledReason`.
8. Activer `Auto Wrap Text` et donner au texte une largeur raisonnable, par
   exemple `220` via un `Size Box` non lié si nécessaire.
9. Utiliser comme texte de prévisualisation `Déplacement en cours`, puis régler
   la visibilité initiale sur `Collapsed`.

Ne créer aucun événement `OnClicked`. Le C++ :

- branche le clic sur `EndActivePlayerTurn()` ;
- demande d'abord l'autorisation à `CanEndActivePlayerTurn()` ;
- désactive le bouton pendant le tour ennemi, le déplacement ou une résolution ;
- affiche automatiquement `Combat inactif`, `Tour ennemi`,
  `Déplacement en cours` ou `Résolution en cours`.

### UE5.4.8 — Vérifier les huit BindWidgets du HUD racine

Avant de continuer, contrôler cette liste dans le Widget Tree :

| Nom exact | Type compatible obligatoire | Doit être vide dans le Designer |
| --- | --- | --- |
| `Panel_CombatHud` | Canvas Panel ou autre Widget racine | non |
| `Panel_Initiative` | Horizontal Box ou autre PanelWidget | oui |
| `Text_InitiativeOverflow` | TextBlock | non applicable |
| `Panel_PartyMembers` | Horizontal Box ou autre PanelWidget | oui |
| `Panel_Actions` | Wrap Box, Horizontal Box ou autre PanelWidget | oui |
| `Text_MobilityActionPoints` | TextBlock | non applicable |
| `Button_EndTurn` | Button | non |
| `Text_EndTurnDisabledReason` | TextBlock | non applicable |

Les trois panneaux dynamiques marqués `oui` sont vidés avec
`ClearChildren()` par le C++. Les cadres, titres et décorations permanentes
doivent donc être placés autour d'eux ou dans leurs conteneurs parents, jamais
comme enfants.

## Étape UE5.5 — Renseigner les classes générées dans le HUD

Cette étape concerne `WBP_GridCombatHud`, qui est un **Widget Blueprint**.
Sur UE 5.5.4, il est normal que sa barre d'outils ne contienne pas le bouton
`Class Defaults`. Ne le cherchez ni dans `Designer`, ni dans `Palette`, ni
dans `Hierarchy`.

1. Ouvrir `WBP_GridCombatHud`.
2. Cliquer `Compile` une première fois. Cette compilation garantit que les
   propriétés héritées de la classe C++ parente sont chargées.
3. En haut à droite de l'éditeur, cliquer l'onglet :

   ```text
   Graph
   ```

   Il se trouve immédiatement à droite de `Designer`.

4. Repérer le panneau `My Blueprint`, normalement situé à gauche en mode
   `Graph`.

   S'il n'est pas visible, utiliser le menu supérieur :

   ```text
   Window > My Blueprint
   ```

5. Dans l'en-tête du panneau `My Blueprint`, ouvrir `View Options` — l'icône
   peut apparaître sous la forme d'un œil ou d'un petit engrenage selon la
   disposition de l'éditeur — puis cocher :

   ```text
   Show Inherited Variables
   ```

   Cette option est indispensable : les trois propriétés ont été déclarées
   dans la classe C++ parente et non dans le Widget Blueprint lui-même.

6. Dans le champ de recherche de `My Blueprint`, saisir successivement le nom
   de chacune des trois variables héritées :

   ```text
   PartyMemberPanelWidgetClass
   ActionWidgetClass
   InitiativeSlotWidgetClass
   ```

   Selon l'option d'affichage des noms conviviaux, Unreal peut les présenter
   avec des espaces :

   ```text
   Party Member Panel Widget Class
   Action Widget Class
   Initiative Slot Widget Class
   ```

7. Sélectionner `PartyMemberPanelWidgetClass` dans `My Blueprint`.
8. Dans le panneau `Details` situé à droite, descendre jusqu'à la section
   `Default Value`.
9. Ouvrir la liste de classes et choisir :

   ```text
   WBP_GridCombatActionPanel
   ```

10. Sélectionner ensuite `ActionWidgetClass`, puis régler son `Default Value`
    sur :

    ```text
    WBP_GridCombatHudAction
    ```

11. Sélectionner enfin `InitiativeSlotWidgetClass`, puis régler son
    `Default Value` sur :

    ```text
    WBP_GridCombatHudInitiativeSlot
    ```

12. Le résultat final doit être exactement :

   | Propriété | Valeur |
   | --- | --- |
   | `Party Member Panel Widget Class` | `WBP_GridCombatActionPanel` |
   | `Action Widget Class` | `WBP_GridCombatHudAction` |
   | `Initiative Slot Widget Class` | `WBP_GridCombatHudInitiativeSlot` |

13. Ne laisser aucune de ces trois propriétés à `None`.
14. Cliquer `Compile`.
15. Examiner le `Compiler Results` : aucune erreur ni alerte de type
   `BindWidget` ne doit rester.
16. Cliquer `Save`.

Si une variable n'apparaît pas :

1. revenir dans `Designer` ;
2. cliquer `Class Settings` dans la barre d'outils ;
3. vérifier que `Parent Class` vaut bien `GridCombatHudWidget` ;
4. cliquer `Compile` ;
5. revenir dans `Graph` ;
6. vérifier de nouveau que `Show Inherited Variables` est coché.

Il ne faut pas créer trois nouvelles variables Blueprint portant ces noms.
Il faut modifier les valeurs par défaut des trois variables **héritées** du
C++.

Symptômes d'une classe manquante :

- `Party Member Panel Widget Class = None` : aucun panneau de personnage ;
- `Action Widget Class = None` : aucune action, même si le catalogue est rempli ;
- `Initiative Slot Widget Class = None` : aucune entrée d'initiative.

## Étape UE5.6 — Affecter le HUD à BP_GrimrockPartyPawn

1. Ouvrir l'asset existant :

   ```text
   /Game/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockPartyPawn
   ```

2. Cet asset est un Blueprint d'Actor, pas un Widget Blueprint. Dans son
   éditeur complet, cliquer `Class Defaults` dans la barre d'outils
   supérieure, à proximité de `Class Settings`.

   Cette instruction ne concerne donc pas les fenêtres
   `WBP_GridCombatHudAction` et `WBP_GridCombatHudInitiativeSlot` montrées dans
   la capture précédente.

   Si `BP_GrimrockPartyPawn` s'ouvre dans l'éditeur simplifié `Data Only`,
   cliquer d'abord `Open Full Blueprint Editor`. Si les propriétés par défaut
   sont déjà affichées directement dans `Details`, il est également possible
   de les renseigner sans ouvrir l'éditeur complet.
3. Dans `Details`, rechercher `Combat Hud Widget Class` ou ouvrir :

   ```text
   Combat > UI
   ```

4. Renseigner :

   | Propriété | Valeur |
   | --- | --- |
   | `Combat Hud Widget Class` | `WBP_GridCombatHud` |
   | `Combat Action Panel Widget Class` | `WBP_GridCombatActionPanel` |
   | `Combat Action Panel ZOrder` | conserver la valeur existante, normalement `50` |

5. Ne pas mettre `Combat Action Panel Widget Class` à `None`. Cette propriété
   reste le fallback si le HUD MON12.7 est désactivé. La classe utilisée pour
   créer les quatre panneaux du nouveau HUD est, elle, la propriété distincte
   `Party Member Panel Widget Class` déjà renseignée dans
   `WBP_GridCombatHud` à l'étape UE5.5.
6. Laisser `Combat Action Panel Character Index` à sa valeur existante. Le
   nouveau HUD n'utilise pas cette valeur : il affecte lui-même `0`, `1`, `2`
   et `3` à ses quatre panneaux.
7. Ne rien ajouter dans l'Event Graph :
   - ne pas appeler `Create Widget` ;
   - ne pas appeler `Add to Viewport` ;
   - ne pas appeler `Initialize Combat Hud` ;
   - ne pas appeler `Refresh From Sources` sur `Tick`.
8. Cliquer `Compile`, puis `Save`.

`AGrimrockPartyPawn::BeginPlay()` appelle déjà
`ShowCombatActionPanelWidget()`. Si `Combat Hud Widget Class` est renseignée,
le Pawn crée `WBP_GridCombatHud`, l'initialise avec le Pawn et le TurnManager,
puis l'ajoute automatiquement au viewport avec le Z-order configuré.

## Étape UE5.7 — Sauvegarder et contrôler les assets

1. Dans le Content Browser, sélectionner les quatre assets :
   - `WBP_GridCombatActionPanel` ;
   - `WBP_GridCombatHudAction` ;
   - `WBP_GridCombatHudInitiativeSlot` ;
   - `WBP_GridCombatHud`.
2. Utiliser `File > Save All`.
3. Vérifier qu'aucun des quatre Widget Blueprints n'affiche une icône de
   compilation en erreur.
4. Ouvrir une dernière fois `WBP_GridCombatHud` et contrôler dans
   `Graph > My Blueprint`, avec `Show Inherited Variables` activé, que les
   trois `Default Value` ne sont pas revenues à `None`.

## Tests automatisés dans Unreal Editor

Les tests du jalon sont :

- `Grimrock.Monsters.MON12.CombatHUD.ViewModel` ;
- `Grimrock.Monsters.MON12.CombatHUD.Lifecycle`.

Pour les exécuter dans Unreal Editor :

1. Ouvrir :

   ```text
   Tools > Test Automation
   ```

   Si cette entrée n'est pas visible, ouvrir `Tools > Session Frontend`, puis
   l'onglet `Automation`.
2. Dans le champ de filtre, saisir :

   ```text
   Grimrock.Monsters.MON12.CombatHUD
   ```

3. Cocher les deux tests `ViewModel` et `Lifecycle`.
4. Cliquer `Start Tests`.
5. Attendre la fin des deux tests.
6. Vérifier que les deux lignes sont vertes et qu'aucune erreur n'apparaît
   dans `Automation Test Log`.

Ils couvrent les quatre panneaux, la projection du catalogue, le routage de
l'action générique, la mise à jour événementielle des PA, le changement de
combattant actif, la chronologie de huit activations, le refus de fin de tour en
mouvement et l'absence de dépense de PA après refus.

## Validation PIE détaillée

### PIE.1 — Apparition et disparition du HUD

1. Ouvrir la carte de test habituelle avec `BP_GrimrockPartyPawn` comme Pawn
   actif.
2. Lancer `Play In Editor`.
3. Avant le combat, vérifier que `Panel_CombatHud` est masqué.
4. Démarrer une rencontre.
5. Vérifier que le HUD complet apparaît sans action Blueprint manuelle.
6. Terminer le combat et vérifier que le HUD est de nouveau masqué.

### PIE.2 — Quatre panneaux de personnages

1. Utiliser un groupe de quatre membres.
2. Vérifier que quatre instances distinctes sont visibles.
3. Contrôler pour chaque membre : portrait, nom, PV, mana, PA et état.
4. Vérifier que les boutons `MainHand` et `OffHand` historiques ne sont pas
   visibles dans ces quatre panneaux.
5. Retirer temporairement un membre du groupe ou charger un groupe incomplet :
   le panneau correspondant doit être `Collapsed`, sans décaler les indices
   des membres restants.
6. Mettre un membre dans l'état `Defeated` : son panneau doit rester présent,
   être désactivé et utiliser l'apparence correspondante du panneau MON12.1.

### PIE.3 — Barre d'actions générique

1. Attendre le tour d'un personnage du groupe.
2. Sans arme offensive, vérifier la présence de l'action à mains nues fournie
   par le catalogue.
3. Équiper le shuriken en `MainHand`.
4. Vérifier que la barre est reconstruite et affiche l'action du shuriken avec :
   - son icône ;
   - son nom ;
   - `2 PA` ;
   - `x1` lorsque la quantité consommée vaut un.
5. Équiper une torche non offensive et vérifier qu'elle ne crée aucun bouton
   d'attaque.
6. Ne rechercher aucun bouton fixe `MainHand` ou `OffHand` dans la barre : la
   source d'équipement est portée par l'action générique.

### PIE.4 — Exécution et refus d'une action

1. Noter les PA du personnage actif, normalement `4 / 4`.
2. Cliquer une action coûtant `2 PA`.
3. Vérifier immédiatement :
   - l'exécution de l'attaque ;
   - le passage des PA de `4 / 4` à `2 / 4` ;
   - la mise à jour de la quantité du shuriken si l'action en consomme un ;
   - la reconstruction de la barre sans polling ni délai de `Tick`.
4. Créer ensuite une situation dans laquelle l'action est indisponible : PA
   insuffisants, cible invalide ou ressource manquante.
5. Vérifier :
   - bouton grisé ;
   - opacité réduite ;
   - texte `Text_DisabledReason` visible ;
   - infobulle contenant la même raison ;
   - clic impossible ;
   - aucun PA, mana ou item consommé.

### PIE.5 — PAM et fin du tour

1. Au début de la manche, vérifier :

   ```text
   PAM 2 / 2
   ```

2. Avancer d'une case.
3. Pendant l'interpolation, vérifier :
   - `Button_EndTurn` désactivé ;
   - raison `Déplacement en cours` visible.
4. À la fin du déplacement, vérifier la dépense existante de `1 PA + 1 PAM`
   et l'actualisation du texte PAM.
5. Tourner le groupe de 90 degrés et vérifier que la rotation reste gratuite.
6. Lorsque le groupe est immobile et que le tour du personnage est actif,
   cliquer `Fin du tour`.
7. Vérifier que le TurnManager active le combattant suivant et que les quatre
   panneaux, les actions et l'initiative sont actualisés ensemble.
8. Pendant un tour ennemi, vérifier que le bouton est désactivé et que la
   raison `Tour ennemi` est affichée.

### PIE.6 — Initiative glissante

1. Démarrer un combat avec plusieurs personnages et monstres.
2. Vérifier que le premier slot correspond au combattant actif.
3. Vérifier que `Border_Active` est visible uniquement sur ce slot.
4. Vérifier que ce slot est agrandi par l'échelle `1.28`.
5. Terminer le tour et contrôler que le nouvel actif passe en première
   position sans tri réalisé par l'interface.
6. Vérifier que huit activations restent visibles même avec moins de huit
   participants, grâce à la projection sur les rounds suivants.
7. Vérifier que `ROUND 2` apparaît entre les deux rounds sans remplacer un
   slot et que `Text_InitiativeOverflow` reste masqué.
8. Se reporter à
   `docs/Design/MON12_7_1_SLIDING_DYNAMIC_INITIATIVE.md` pour les tests de
   retrait d'un vaincu et de changement dynamique de l'ordre.

### PIE.7 — Vérifier le fallback historique

1. Arrêter PIE.
2. Ouvrir `BP_GrimrockPartyPawn`.
3. Dans le panneau `Details` affiché après avoir cliqué `Class Defaults`,
   rechercher `Combat Hud Widget Class` et mettre temporairement :

   ```text
   Combat Hud Widget Class = None
   ```

4. Ne pas modifier :

   ```text
   Combat Action Panel Widget Class = WBP_GridCombatActionPanel
   ```

5. Compiler et sauvegarder le Pawn.
6. Relancer PIE et vérifier que le panneau historique unique s'affiche.
7. Arrêter PIE.
8. Rétablir :

   ```text
   Combat Hud Widget Class = WBP_GridCombatHud
   ```

9. Compiler et sauvegarder de nouveau le Pawn.

## Diagnostic des problèmes courants

| Symptôme | Cause probable | Vérification / correction |
| --- | --- | --- |
| Les classes parentes MON12.7 n'apparaissent pas | module C++ non recompilé ou Editor non redémarré | fermer UE5, compiler `GrimrockPrototypeEditor Development Editor Win64`, rouvrir |
| Aucun HUD pendant le combat | `Combat Hud Widget Class` vaut `None` ou création du widget en échec | vérifier `BP_GrimrockPartyPawn > Combat > UI` et filtrer l'Output Log sur `GridCombatHud Show Failed` |
| Le HUD existe mais reste vide | classes générées du HUD laissées à `None` | ouvrir `WBP_GridCombatHud > Graph > My Blueprint`, activer `Show Inherited Variables` et vérifier les trois `Default Value` |
| Aucun panneau de personnage | nom/type de `Panel_PartyMembers` incorrect ou classe de panneau absente | utiliser un `PanelWidget` nommé exactement `Panel_PartyMembers` et renseigner `WBP_GridCombatActionPanel` |
| Aucun bouton d'action | `Panel_Actions` absent, `Action Widget Class` absent ou tour ennemi | vérifier le nom, la classe et attendre un tour du groupe |
| Une décoration de barre disparaît | décoration placée dans un panneau dynamique vidé par `ClearChildren()` | déplacer la décoration hors de `Panel_PartyMembers`, `Panel_Actions` ou `Panel_Initiative` |
| Un ancien `+ N` reste visible | `Text_InitiativeOverflow` forcé visible dans le Blueprint | le laisser lié ou le supprimer, avec visibilité initiale `Collapsed` |
| Aucun slot d'initiative | `Initiative Slot Widget Class` ou `Panel_Initiative` absent | renseigner la classe et vérifier le nom du panneau |
| Portrait d'action vide | l'action ne possède pas d'icône dans sa définition | vérifier l'`Icon` du Data Asset d'action ou d'équipement ; le HUD ne fabrique pas d'icône fallback |
| Portrait d'initiative vide | `FGridCombatantInitiativeEntry::Portrait` est vide | vérifier la donnée de portrait du personnage ou du monstre ; MON12.7 n'ajoute pas de portrait fallback |
| Bouton Fin du tour désactivé | comportement autoritaire normal | lire `Text_EndTurnDisabledReason` : combat inactif, tour ennemi, déplacement ou résolution |
| Deux HUD se superposent | une ancienne création manuelle existe dans un Level Blueprint, Pawn Blueprint ou autre widget | supprimer l'appel Blueprint manuel ; le Pawn C++ crée déjà le HUD |
| Les boutons MainHand/OffHand sont encore visibles dans les quatre panneaux | le panneau n'est pas créé par `UGridCombatHudWidget` ou utilise une mauvaise classe parente | vérifier `Party Member Panel Widget Class` et le parent `GridCombatActionPanelWidget` |

## Hors périmètre

- exécution des sorts, zones et coûts de mana transactionnels : MON12.8 ;
- défense, objets rapides et réactions : MON12.9 ;
- pagination d'un grand livre de sorts ;
- surcharge, immobilisation et bonus de mobilité ;
- sélection d'une cible par clic sur la barre d'initiative ;
- modification des coûts PA/PAM ;
- modification des rotations gratuites du groupe.
