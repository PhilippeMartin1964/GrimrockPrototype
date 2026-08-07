# MON12.8.2 — Barre de combat fixe et glisser-déposer

## Résultat

Le HUD n'affiche plus automatiquement toutes les actions du catalogue. Il
construit une seule fois dix instances de `WBP_GridCombatHudAction`, dans
l'ordre clavier suivant :

```text
1 2 3 4 5 6 7 8 9 0
```

Les dix emplacements existent même lorsque la barre du personnage actif est
vide. Ils sont actualisés lors des notifications du TurnManager et de
`OnPartyInventoryChanged`, sans être détruits et recréés à chaque événement.

MON12.8.2 configure et organise les raccourcis. Le clic gauche et les touches
numériques n'exécutent encore aucune action ; cette responsabilité appartient
à MON12.8.3.

## Sources acceptées

Un dépôt depuis l'inventaire ne déplace, ne divise et ne consomme jamais
l'objet source.

- une potion ou un parchemin présent dans l'inventaire crée un binding
  `QuickItem`, identifié par son `ItemDefinitionId` ;
- une arme réellement équipée en `MainHand` ou `OffHand` crée un binding
  `Equipment`, identifié par son `RuntimeObjectId` ;
- une arme non équipée, un objet de curseur, une armure ou un objet sans action
  compatible est refusé sans modifier la barre ;
- si une définition d'équipement expose plusieurs `CombatActions`, le dépôt de
  l'objet choisit sa première action valide comme action principale. Une future
  liste d'actions permettra d'assigner explicitement les autres.

Les actions sans objet source — notamment le combat à mains nues, les sorts
appris et les capacités de classe — conservent déjà leur modèle de binding,
mais leur palette de glisser-déposer sera raccordée avec l'exécution en
MON12.8.3. MON12.8.2 ne crée aucun raccourci automatique pour elles.

Le binding d'un consommable utilise l'identité stable `Use_<ItemDefinitionId>`.
Il pourra donc rester configuré lorsque la quantité atteindra zéro. Le binding
d'une arme suit au contraire son instance précise et continue de se résoudre
si cette même arme change de main.

## Déplacement, échange et suppression

`UGridCombatHotbarDragDropOperation` transporte uniquement le personnage,
l'index source et le binding persistant.

- raccourci vers slot vide : déplacement ;
- raccourci vers slot occupé : échange atomique ;
- raccourci vers lui-même : aucune modification ;
- clic droit : suppression du binding ;
- dépôt refusé : aucun slot n'est modifié.

`MoveOrSwapCharacterCombatHotbarBinding()` effectue les deux écritures avant
une unique notification. Les `SlotIndex` sont normalisés après chaque échange.

## Résolution et présentation

`FGridCombatHudViewModelBuilder::BuildHotbarActions()` produit toujours dix
vues. Un binding est résolu contre le catalogue courant sans sérialiser ce
dernier :

- `Equipment` exige la même instance runtime, mais pas la même main ;
- `Universal`, `Ability`, `Spell` et `QuickItem` utilisent leur identité stable
  et leur définition source ;
- une source absente laisse le raccourci visible mais grisé.

Les potions et parchemins conservent déjà leur nom et leur icône depuis le
`GridItemDefinitionAsset`, mais restent grisés jusqu'à l'exécuteur MON12.8.4.

## Widget Blueprint

Aucune modification binaire `.uasset` n'est obligatoire pour compiler ce
jalon. Le C++ crée une `HorizontalBox_Hotbar_Runtime` lorsque le panneau du
Designer n'est pas déjà horizontal. Les dix raccourcis restent ainsi sur une
seule ligne et reçoivent chacun un dixième de la largeur disponible.

Dans `WBP_GridCombatHud`, la configuration canonique reste néanmoins :

- `Panel_Actions` doit être un `Horizontal Box` vide dans le Designer ;
- si l'ancien `Wrap Box` est conservé, le fallback C++ y insère une unique
  rangée horizontale et empêche tout retour à une grille multiligne ;
- `Action Widget Class` doit rester `WBP_GridCombatHudAction` ;
- le C++ injecte les dix enfants fixes.

Dans `WBP_GridCombatHudAction`, un `TextBlock` optionnel nommé exactement
`Text_ShortcutNumber` peut être ajouté pour placer le numéro dans un coin. S'il
n'existe pas, le C++ préfixe automatiquement `Text_ActionName` avec `[1]` à
`[0]`, de sorte que les dix numéros restent visibles sans changement du WBP.

Le `Button_Action` doit rester hit-testable afin de recevoir sur son widget
parent les opérations de glisser-déposer et le clic droit.

## Tests automatisés

Le filtre suivant couvre les nouvelles règles :

```text
Grimrock.Monsters.MON12.8.2
```

Il vérifie :

1. déplacement vers un slot vide et échange entre deux slots occupés ;
2. création d'un raccourci de potion sans déplacer la pile ;
3. identité stable du consommable sans identifiant runtime ;
4. résolution d'une arme par instance après changement de main ;
5. projection fixe de dix slots numérotés `1` à `0`.

Les tests MON12.7 sont adaptés : ils attendent désormais dix slots vides au
démarrage, configurent explicitement l'épée de test, puis vérifient encore le
routage autoritaire de la requête.

## Suite

MON12.8.3 reliera clic gauche et touches `1` à `0` aux seuls bindings résolus,
avec interception du clavier par les écrans modaux. MON12.8.4 ajoutera les
contributions et exécuteurs des potions et parchemins.
