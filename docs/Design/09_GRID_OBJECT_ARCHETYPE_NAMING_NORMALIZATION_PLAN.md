# Plan de normalisation des noms d'archétypes GridObject

Plan Phase 4D pour normaliser les noms de DataAssets `UGridObjectArchetypeAsset` et leurs `ArchetypeId`.

Ce document est un plan de migration uniquement. Il ne modifie pas le code C++, les `.uasset`, les Blueprints, les enums, les liens, les niveaux ou la sérialisation.

## 1. Objectif

Le projet contient actuellement un mélange de :

- archétypes génériques utilisables comme blocs de base (`Door_Stone`, `Item_Torch`) ;
- variantes visuelles concrètes (`Button_Secret_Stone`, décorations de sol) ;
- archétypes orientés puzzle/test (`Button_ToggleDoor`, `Lever_OpenSecret`, `DA_Arch_Plate_HoldDoor`) ;
- objets historiques à préserver (`WallInscription`).

L'objectif de cette phase est de définir un schéma canonique avant toute modification dans Unreal. Le renommage des assets et des `ArchetypeId` doit être fait ensuite dans des lots contrôlés, avec migration explicite des références.

## 2. Règles de nommage

- Le fichier DataAsset doit suivre la forme `DA_<ArchetypeId>`.
- `ArchetypeId` doit être stable, explicite et orienté gameplay.
- `DisplayName` doit être lisible par un humain et utilisable dans l'éditeur.
- Les archétypes génériques ne doivent pas décrire une énigme précise.
- Les archétypes puzzle/test conservés doivent être clairement préfixés avec `Test_` ou `Example_`.
- Ne jamais renommer un `ArchetypeId` sans migrer toutes ses références.
- Les variantes visuelles peuvent être explicites quand c'est utile : `Button_Secret_Stone`, `Door_Secret_Stone`, etc.
- Un `ArchetypeId` doit rester un identifiant de contenu, pas une description de lien logique. Exemple : préférer `Lever_Standard` à `Lever_OpenSecret` pour un levier générique.
- `DisplayName` peut être plus court que `ArchetypeId`. Exemple : `Receptacle_TorchHolder` -> `Torch Holder`.

## 3. Table canonique des archétypes

| Canonical ArchetypeId | DataAsset Name | DisplayName | SupportedType | ObjectCategory | PlacementKind | Runtime Class | Status |
|---|---|---|---|---|---|---|---|
| `Button_Normal` | `DA_Button_Normal` | `Button` | Button | Mechanism | Wall | `AGridButtonActor` / BP dérivé | À créer |
| `Button_Secret` | `DA_Button_Secret` | `Secret Button` | Button | Mechanism | Wall | `AGridButtonActor` / BP dérivé | À créer ou migrer depuis `Button_Secret_Stone` |
| `Button_Wall` | `DA_Button_Wall` | `Wall Button` | Button | Mechanism | Wall | `AGridButtonActor` / BP dérivé | À créer |
| `Lever_Standard` | `DA_Lever_Standard` | `Lever` | Lever | Mechanism | Wall | `AGridLeverActor` / `BP_GridLeverActor_C` | À créer ou migrer depuis `Lever_OpenSecret` |
| `PressurePlate_Stone` | `DA_PressurePlate_Stone` | `Stone Pressure Plate` | PressurePlate | Mechanism | Floor ou Center | `AGridPressurePlateActor` / `BP_GridPressurePlateActor_C` | À créer ou migrer depuis `DA_Arch_Plate_HoldDoor` |
| `Door_Stone` | `DA_Door_Stone` | `Stone Door` | Door | Mechanism | Edge | `AGridDoorActor` / BP dérivé | Déjà présent |
| `Door_Secret` | `DA_Door_Secret` | `Secret Door` | Door | Mechanism | Edge ou Wall | `AGridSecretDoorActor` / BP dérivé de `AGridDoorActor` | À créer ou migrer depuis `Secret_Door_Stone` |
| `Receptacle_TorchHolder` | `DA_Receptacle_TorchHolder` | `Torch Holder` | Receptacle | Receptacle | Wall | `AGridReceptacleActor` / `BP_Receptacle_WallTorchHolder_C` | À migrer depuis `Receptacle_WallTorchHolder` |
| `Receptacle_Alcove` | `DA_Receptacle_Alcove` | `Alcove` | Receptacle | Receptacle | Wall | `AGridReceptacleActor` / BP dérivé | À créer |
| `Receptacle_Altar` | `DA_Receptacle_Altar` | `Altar` | Receptacle | Receptacle | Floor ou Center | `AGridReceptacleActor` / BP dérivé | À créer |
| `Receptacle_OfferingBowl` | `DA_Receptacle_OfferingBowl` | `Offering Bowl` | Receptacle | Receptacle | Floor ou Center | `AGridReceptacleActor` / BP dérivé | À créer |
| `Lock_Keyhole` | `DA_Lock_Keyhole` | `Keyhole` | Receptacle | Receptacle | Wall | `AGridReceptacleActor` ou futur `AGridLockActor` | À créer |
| `Teleporter_Rune` | `DA_Teleporter_Rune` | `Rune Teleporter` | Teleporter | Teleporter | Floor ou Center | `AGridTeleporterActor` / BP dérivé | À créer |
| `Item_Torch` | `DA_Item_Torch` | `Torch` | Item | Item | Floor | `AGridItemActor` / `BP_Item_Torch_C` via `ItemActorClass` | Déjà présent |
| `Item_Key` | `DA_Item_Key` | `Key` | Item | Item | Floor ou Center | `AGridItemActor` / BP dérivé | À créer |
| `Item_Coin` | `DA_Item_Coin` | `Coin` | Item | Item | Floor ou Center | `AGridItemActor` / BP dérivé | À créer |
| `ItemSpawn_Torch` | `DA_ItemSpawn_Torch` | `Torch Spawn` | ItemSpawn | Spawn | Floor ou Center | Optionnel ; spawn via behavior | Déjà présent |
| `WallInscription` | `DA_WallInscription` | `Wall Inscription` | Decoration | Readable | Wall | Système readable existant | À conserver pour l'instant |
| `Readable_WallInscription` | `DA_Readable_WallInscription` | `Wall Inscription` | Decoration | Readable | Wall | Système readable existant | Alias canonique possible plus tard, migration non prioritaire |
| `Spawn_Player` | `DA_Spawn_Player` | `Player Spawn` | MonsterSpawn ou futur type dédié à confirmer | Spawn | Floor ou Center | Marker/donnée, pas forcément actor runtime | À créer |

Notes :

- `ObjectCategory=Mechanism` pour les portes reste cohérent avec l'enum actuelle, même si la documentation de design parle de `Passage`.
- `FloorRuneCircle` reste une décoration jusqu'à la création explicite de `Teleporter_Rune`.
- Les décorations de sol existantes peuvent conserver leurs IDs actuels.

## 4. Mapping des assets existants

| Current Asset | Current ArchetypeId | Proposed Asset | Proposed ArchetypeId | Action | Migration Risk | Notes |
|---|---|---|---|---|---|---|
| `DA_Arch_Button_ToggleDoor` | `Button_ToggleDoor` | `DA_Button_Normal` ou `DA_Button_Wall` pour le générique ; `DA_Example_Button_ToggleDoor` si conservé | `Button_Normal` / `Button_Wall` / `Example_Button_ToggleDoor` | Créer un bouton générique, puis migrer les placements test si approprié. Conserver l'asset puzzle seulement s'il sert d'exemple. | Élevé | ID utilisé dans niveaux/palette. Ne pas renommer directement sans migration. |
| `DA_Button_Secret_Stone` | `Button_Secret_Stone` | `DA_Button_Secret` ou `DA_Button_Secret_Stone` | `Button_Secret` ou `Button_Secret_Stone` | Choisir si la variante pierre doit rester explicite. Migrer les références si l'ID change. | Moyen | La variante existe et semble cohérente ; le sujet principal est le naming canonique. |
| `DA_Arch_Lever_OpenSecret` | `Lever_OpenSecret` | `DA_Lever_Standard` pour le générique ; `DA_Example_Lever_OpenSecret` si conservé | `Lever_Standard` / `Example_Lever_OpenSecret` | Créer un levier générique. Migrer les objets non spécifiques. | Élevé | `Lever_OpenSecret` décrit une logique d'énigme, pas un composant générique. |
| `DA_Arch_Plate_HoldDoor` | `PressurePlate` ou à vérifier | `DA_PressurePlate_Stone` pour le générique ; `DA_Example_PressurePlate_HoldDoor` si conservé | `PressurePlate_Stone` / `Example_PressurePlate_HoldDoor` | Vérifier l'ID réel dans Unreal, puis créer/migrer la plaque générique. | Élevé | Asset orienté puzzle avec `TriggerMode=Hold`. |
| `DA_Door_Stone` | `Door_Stone` | `DA_Door_Stone` | `Door_Stone` | Conserver. Vérifier seulement `DisplayName`, runtime class, meshes et catégorie palette. | Faible | Déjà canonique. |
| `DA_SecretDoor_Stone1` | `Secret_Door_Stone` | `DA_Door_Secret` ou `DA_Door_Secret_Stone` | `Door_Secret` ou `Door_Secret_Stone` | Migrer vers un ID commençant par `Door_`. Ne pas faire de rename aveugle. | Élevé | ID inversé ; peut déjà être référencé par les niveaux. |
| `DA_Receptacle_WallTorchHolder` | `Receptacle_WallTorchHolder` | `DA_Receptacle_TorchHolder` | `Receptacle_TorchHolder` | Migrer après vérification des références et des règles d'acceptation `Item_Torch`. | Élevé | Le comportement est bon, le nom doit devenir canonique. |
| `DA_Item_Torch` | `Item_Torch` | `DA_Item_Torch` | `Item_Torch` | Conserver. Vérifier `ItemActorClass` et behavior inutile éventuel. | Faible | Déjà canonique. |
| `DA_ItemSpawn_Torch` | `ItemSpawn_Torch` | `DA_ItemSpawn_Torch` | `ItemSpawn_Torch` | Conserver. Vérifier `SpawnedItemArchetypeId=Item_Torch`. | Faible | Déjà conforme au schéma `DA_<ArchetypeId>`. |
| `DA_WallInscription` | `WallInscription` | `DA_WallInscription` maintenant ; `DA_Readable_WallInscription` plus tard si migration décidée | `WallInscription` maintenant ; `Readable_WallInscription` plus tard | Ne pas renommer immédiatement. Améliorer seulement DisplayName/classification si nécessaire. | Élevé | Les documents disent explicitement de préserver `WallInscription`. |
| `DA_Trigger_Cell` | `Trigger_Cell` | `DA_Trigger_Floor` ou `DA_Example_Trigger_Cell` | `Trigger_Floor` / `Example_Trigger_Cell` | Décider si c'est un trigger générique ou un test. Corriger aussi le DisplayName `TriggerCelle`. | Moyen | Pas dans la liste minimale demandée, mais présent dans la palette. |
| `DA_A_FloorRuneCircle` | `FloorRuneCircle` | `DA_A_FloorRuneCircle` | `FloorRuneCircle` | Conserver comme décoration. Créer `DA_Teleporter_Rune` séparément. | Faible | Ne pas convertir implicitement en téléporteur. |
| `DA_A_FloorBloodStain` | `FloorBloodStain` | `DA_A_FloorBloodStain` ou `DA_FloorBloodStain` plus tard | `FloorBloodStain` | Conserver pour l'instant. | Faible | Décoration de sol hors migration prioritaire. |
| `DA_A_FloorBones` | `FloorBones` à vérifier | `DA_A_FloorBones` ou `DA_FloorBones` plus tard | `FloorBones` | Vérifier l'ID exact car `FloorBone` et `FloorBones` apparaissent dans l'audit. | Faible | Décoration de sol. |
| `DA_A_FloorCarpet` | `FloorCarpet` | `DA_A_FloorCarpet` ou `DA_FloorCarpet` plus tard | `FloorCarpet` | Conserver pour l'instant. | Faible | Décoration de sol. |
| `DA_A_FloorDebris` | `FloorDebris` | `DA_A_FloorDebris` ou `DA_FloorDebris` plus tard | `FloorDebris` | Conserver pour l'instant. | Faible | Décoration de sol. |
| `DA_A_FloorDust` | `FloorDust` | `DA_A_FloorDust` ou `DA_FloorDust` plus tard | `FloorDust` | Conserver pour l'instant. | Faible | Décoration de sol. |
| `DA_A_FloorMoss` | `FloorMoss` | `DA_A_FloorMoss` ou `DA_FloorMoss` plus tard | `FloorMoss` | Conserver pour l'instant. | Faible | Décoration de sol. |
| `DA_A_FloorRoots` | `FloorRoots` | `DA_A_FloorRoots` ou `DA_FloorRoots` plus tard | `FloorRoots` | Conserver pour l'instant. Corriger éventuellement la description. | Faible | Décoration de sol. |
| `DA_A_FloorRubble` | `FloorRubble` | `DA_A_FloorRubble` ou `DA_FloorRubble` plus tard | `FloorRubble` | Conserver pour l'instant. | Faible | Décoration de sol. |

## 5. Stratégie de migration

### Étape 1 : créer les archétypes génériques manquants

Créer les archétypes canoniques sans supprimer ni renommer les anciens :

- `DA_Button_Normal`
- `DA_Button_Wall`
- `DA_Lever_Standard`
- `DA_PressurePlate_Stone`
- `DA_Receptacle_Alcove`
- `DA_Receptacle_Altar`
- `DA_Receptacle_OfferingBowl`
- `DA_Lock_Keyhole`
- `DA_Teleporter_Rune`
- `DA_Item_Key`
- `DA_Item_Coin`
- `DA_Spawn_Player`

Cette étape permet de rendre la palette conforme sans casser les niveaux existants.

### Étape 2 : mettre à jour la palette

Mettre à jour `DA_ObjectPalette_Default` pour exposer les archétypes génériques canoniques en priorité.

Les assets puzzle/test peuvent rester dans une catégorie séparée ou être retirés de la palette principale.

### Étape 3 : migrer les objets des niveaux de test

Migrer les objets placés vers les nouveaux `ArchetypeId` quand ils représentent un objet générique :

- `Button_ToggleDoor` -> `Button_Normal` ou `Button_Wall`
- `Lever_OpenSecret` -> `Lever_Standard`
- plaque de `DA_Arch_Plate_HoldDoor` -> `PressurePlate_Stone`
- `Receptacle_WallTorchHolder` -> `Receptacle_TorchHolder`
- `Secret_Door_Stone` -> `Door_Secret` ou `Door_Secret_Stone`

Les objets qui servent à documenter une énigme exemple peuvent conserver un archétype `Example_`.

### Étape 4 : renommer les archétypes puzzle/test conservés

Si les assets orientés puzzle restent utiles, les renommer explicitement :

- `DA_Arch_Button_ToggleDoor` -> `DA_Example_Button_ToggleDoor`
- `Button_ToggleDoor` -> `Example_Button_ToggleDoor`
- `DA_Arch_Lever_OpenSecret` -> `DA_Example_Lever_OpenSecret`
- `Lever_OpenSecret` -> `Example_Lever_OpenSecret`
- `DA_Arch_Plate_HoldDoor` -> `DA_Example_PressurePlate_HoldDoor`
- ID correspondant -> `Example_PressurePlate_HoldDoor`

Cette étape doit être faite après migration des niveaux qui ne doivent pas rester liés aux exemples.

### Étape 5 : corriger les redirectors Unreal

Après les renommages d'assets dans Unreal :

- utiliser `Fix Up Redirectors`;
- sauvegarder les packages concernés ;
- vérifier que les palettes, niveaux et Blueprints ne pointent plus vers des redirectors temporaires ;
- committer les `.uasset` modifiés ensemble par famille.

### Étape 6 : supprimer seulement après validation

Ne supprimer les anciens assets qu'après :

- validation palette ;
- validation chargement niveau ;
- validation PIE ;
- recherche de références restantes ;
- commit de migration déjà stable.

## 6. Cas particuliers

### `WallInscription`

`WallInscription` ne doit pas être renommé immédiatement. Les documents de design indiquent explicitement de préserver cet objet existant.

La forme `Readable_WallInscription` peut rester un objectif de normalisation plus tard, mais seulement avec une migration explicite des niveaux et références.

### `Door_Stone`

`Door_Stone` est déjà canonique. Le DataAsset `DA_Door_Stone` respecte déjà la règle `DA_<ArchetypeId>`.

Les corrections éventuelles doivent se limiter à `DisplayName`, meshes, classe runtime ou paramètres, pas au nom.

### `Item_Torch`

`Item_Torch` est déjà canonique. Le DataAsset `DA_Item_Torch` respecte déjà la règle `DA_<ArchetypeId>`.

Avant tout changement, vérifier seulement que `ItemActorClass` est correct et que le behavior ItemSpawn n'est pas configuré inutilement.

### Décorations de sol

Les décorations de sol peuvent conserver leurs IDs actuels :

- `FloorBloodStain`
- `FloorBones`
- `FloorCarpet`
- `FloorDebris`
- `FloorDust`
- `FloorMoss`
- `FloorRoots`
- `FloorRubble`
- `FloorRuneCircle`

Le préfixe de fichier `DA_A_` est moins strict que la règle canonique, mais ces assets ne bloquent pas la normalisation des objets de gameplay. Les traiter plus tard si nécessaire.

### `FloorRuneCircle`

`FloorRuneCircle` reste une `Decoration` tant qu'un archétype séparé `Teleporter_Rune` n'existe pas.

Ne pas convertir l'asset existant en téléporteur sans décision explicite, car cela changerait son rôle dans les niveaux et la palette.

### `Secret_Door_Stone`

`Secret_Door_Stone` doit migrer vers `Door_Secret` ou `Door_Secret_Stone`, mais pas par renommage direct aveugle.

Avant migration :

- vérifier les références dans les niveaux ;
- vérifier la classe runtime ;
- vérifier les meshes fixe/mobile ;
- décider si le canon doit être générique (`Door_Secret`) ou variante pierre (`Door_Secret_Stone`).

## 7. Checklist de validation

Pour chaque archétype renommé ou nouvellement créé :

- apparaît dans la palette ;
- peut être placé dans la grille ;
- la preview est correcte ;
- l'inspecteur affiche la bonne section contextuelle ;
- l'acteur runtime spawn correctement ;
- les connecteurs existants se résolvent toujours ;
- le niveau charge sans warnings d'archétype manquant ;
- le comportement PIE est inchangé ou corrigé volontairement ;
- les `ArchetypeId` référencés dans les behaviors restent valides ;
- les redirectors Unreal ont été corrigés ;
- le commit ne contient que la famille d'assets concernée.

## 8. Recommandation finale

Ne pas renommer tous les assets d'un coup.

Commencer par les boutons, puis les leviers/plaques, puis les portes, puis les réceptacles.

Ordre recommandé :

1. Boutons : créer `Button_Normal`, `Button_Wall`, décider `Button_Secret` vs `Button_Secret_Stone`.
2. Leviers et plaques : créer `Lever_Standard` et `PressurePlate_Stone`.
3. Portes : conserver `Door_Stone`, migrer prudemment `Secret_Door_Stone`.
4. Réceptacles : migrer `Receptacle_WallTorchHolder` vers `Receptacle_TorchHolder`, puis créer les autres réceptacles.
5. Items/spawns : conserver `Item_Torch` et `ItemSpawn_Torch`, créer `Item_Key` et `Item_Coin`.
6. Readable/lumières : préserver `WallInscription`, créer `Teleporter_Rune` séparément de `FloorRuneCircle`.

Faire un commit séparé par famille d'assets, avec validation éditeur et runtime après chaque lot.
