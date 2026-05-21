# Plan de normalisation des noms d'archétypes GridObject

Plan de migration des noms de DataAssets `UGridObjectArchetypeAsset` et de leurs `ArchetypeId`, mis à jour après validation UE5 de la Phase 5A.

Ce document est un plan de documentation. Il ne modifie pas le code C++, les `.uasset`, les Blueprints, les enums, les liens, les niveaux ou la sérialisation.

## 1. Objectif

Le projet contenait un mélange de :

- archétypes génériques ;
- variantes visuelles ;
- archétypes orientés puzzle/test ;
- objets historiques à préserver.

La Phase 5A a validé une première normalisation directe dans UE5. Les anciens noms orientés puzzle/test ont été remplacés par des noms génériques pour les objets principaux.

Les noms canoniques actuels sont :

- `Button_Normal`
- `Button_Secret`
- `Button_Wall` à créer
- `Lever`
- `PressurePlate`
- `Trigger`
- `Door_Stone`
- `Door_Secret`
- `Receptacle_TorchHolder`
- `Item_Torch`
- `ItemSpawn_Torch`
- `WallInscription`

## 2. Règles de nommage

- Le fichier DataAsset doit suivre la forme `DA_<ArchetypeId>`.
- `ArchetypeId` doit être stable, explicite et orienté gameplay.
- `DisplayName` doit être lisible par un humain et utilisable dans l'éditeur.
- Les archétypes génériques ne doivent pas décrire une énigme précise.
- Les archétypes puzzle/test conservés doivent être clairement préfixés avec `Test_` ou `Example_`.
- Ne jamais renommer un `ArchetypeId` sans migrer toutes ses références.
- Les formes courtes sont autorisées quand il n'existe pas encore plusieurs variantes : `Lever`, `PressurePlate`, `Trigger`.
- Si plusieurs variantes apparaissent plus tard, créer des archétypes explicites sans casser les IDs existants. Exemple : conserver `Lever`, puis ajouter `Lever_WallRusty` seulement si nécessaire.

## 3. Table canonique des archétypes

| Canonical ArchetypeId | DataAsset Name | DisplayName | SupportedType | ObjectCategory | PlacementKind | Runtime Class | Status |
|---|---|---|---|---|---|---|---|
| `Button_Normal` | `DA_Button_Normal` | `Button` | Button | Mechanism | Wall | `AGridButtonActor` / BP dérivé | Présent / validé UE5 |
| `Button_Secret` | `DA_Button_Secret` | `Secret Button` | Button | Mechanism | Wall | `AGridButtonActor` / BP dérivé | Présent / validé UE5 |
| `Button_Wall` | `DA_Button_Wall` | `Wall Button` | Button | Mechanism | Wall | `AGridButtonActor` / BP dérivé | À créer |
| `Lever` | `DA_Lever` | `Lever` | Lever | Mechanism | Wall | `AGridLeverActor` / `BP_GridLeverActor_C` | Présent / validé UE5 |
| `PressurePlate` | `DA_PressurePlate` | `Pressure Plate` | PressurePlate | Mechanism | Floor ou Center | `AGridPressurePlateActor` / `BP_GridPressurePlateActor_C` | Présent / validé UE5 |
| `Trigger` | `DA_Trigger` | `Trigger` | Trigger | Mechanism | Floor ou Center | `AGridTriggerActor` / `BP_GridTriggerActor_C` | Présent / validé UE5 |
| `Door_Stone` | `DA_Door_Stone` | `Stone Door` | Door | Mechanism | Edge | `AGridDoorActor` / BP dérivé | Présent / déjà canonique |
| `Door_Secret` | `DA_Door_Secret` | `Secret Door` | Door | Mechanism | Edge ou Wall | `AGridSecretDoorActor` / BP dérivé de `AGridDoorActor` | Présent / validé UE5 |
| `Receptacle_TorchHolder` | `DA_Receptacle_TorchHolder` | `Torch Holder` | Receptacle | Receptacle | Wall | `AGridReceptacleActor` / BP dérivé | Présent / validé UE5 |
| `Receptacle_Alcove` | `DA_Receptacle_Alcove` | `Alcove` | Receptacle | Receptacle | Wall | `AGridReceptacleActor` / BP dérivé | À créer |
| `Receptacle_Altar` | `DA_Receptacle_Altar` | `Altar` | Receptacle | Receptacle | Floor ou Center | `AGridReceptacleActor` / BP dérivé | À créer |
| `Receptacle_OfferingBowl` | `DA_Receptacle_OfferingBowl` | `Offering Bowl` | Receptacle | Receptacle | Floor ou Center | `AGridReceptacleActor` / BP dérivé | À créer |
| `Lock_Keyhole` | `DA_Lock_Keyhole` | `Keyhole` | Receptacle | Receptacle | Wall | `AGridReceptacleActor` ou futur `AGridLockActor` | À créer |
| `Teleporter_Rune` | `DA_Teleporter_Rune` | `Rune Teleporter` | Teleporter | Teleporter | Floor ou Center | `AGridTeleporterActor` / BP dérivé | À créer |
| `Item_Torch` | `DA_Item_Torch` | `Torch` | Item | Item | Floor ou Center | `AGridItemActor` / BP dérivé via `ItemActorClass` | Présent / déjà canonique |
| `Item_Key` | `DA_Item_Key` | `Key` | Item | Item | Floor ou Center | `AGridItemActor` / BP dérivé | À créer |
| `Item_Coin` | `DA_Item_Coin` | `Coin` | Item | Item | Floor ou Center | `AGridItemActor` / BP dérivé | À créer |
| `ItemSpawn_Torch` | `DA_ItemSpawn_Torch` | `Torch Spawn` | ItemSpawn | Spawn | Floor ou Center | Optionnel ; spawn via behavior | Présent / déjà canonique |
| `WallInscription` | `DA_WallInscription` | `Wall Inscription` | Decoration | Readable | Wall | Système readable existant | Présent / conservé |
| `Spawn_Player` | `DA_Spawn_Player` | `Player Spawn` | MonsterSpawn ou futur type dédié à confirmer | Spawn | Floor ou Center | Marker/donnée | À créer |

## 4. Mapping des anciens noms vers les noms validés

| Ancien asset / ID | Nom validé | Action Phase 5A | Risque résiduel | Notes |
|---|---|---|---|---|
| `DA_Arch_Button_ToggleDoor` / `Button_ToggleDoor` | `DA_Button_Normal` / `Button_Normal` | Renommé dans UE5 | Faible si les niveaux ont été sauvegardés après migration | L'ancien nom décrivait une énigme. |
| `DA_Button_Secret_Stone` / `Button_Secret_Stone` | `DA_Button_Secret` / `Button_Secret` | Renommé dans UE5 | Faible à moyen | La variante pierre n'est pas distinguée tant qu'il n'existe pas plusieurs variantes. |
| `DA_Arch_Lever_OpenSecret` / `Lever_OpenSecret` | `DA_Lever` / `Lever` | Renommé dans UE5 | Faible si les références ont été resauvegardées | Forme courte validée. |
| `DA_Arch_Plate_HoldDoor` | `DA_PressurePlate` / `PressurePlate` | Renommé dans UE5 | Faible si les références ont été resauvegardées | Forme courte validée. |
| `DA_Trigger_Cell` / `Trigger_Cell` | `DA_Trigger` / `Trigger` | Renommé dans UE5 | Faible si les références ont été resauvegardées | Corrige aussi le DisplayName temporaire. |
| `DA_Door_Stone` / `Door_Stone` | `DA_Door_Stone` / `Door_Stone` | Inchangé | Faible | Déjà canonique. |
| `DA_SecretDoor_Stone1` / `Secret_Door_Stone` | `DA_Door_Secret` / `Door_Secret` | Renommé dans UE5 | Moyen | Vérifier les niveaux et connecteurs qui ciblaient la porte secrète. |
| `DA_Receptacle_WallTorchHolder` / `Receptacle_WallTorchHolder` | `DA_Receptacle_TorchHolder` / `Receptacle_TorchHolder` | Renommé dans UE5 | Moyen | Vérifier l'acceptation de `Item_Torch`. |
| `DA_Item_Torch` / `Item_Torch` | `DA_Item_Torch` / `Item_Torch` | Inchangé | Faible | Déjà canonique. |
| `DA_ItemSpawn_Torch` / `ItemSpawn_Torch` | `DA_ItemSpawn_Torch` / `ItemSpawn_Torch` | Inchangé | Faible | Déjà canonique. |
| `DA_WallInscription` / `WallInscription` | `DA_WallInscription` / `WallInscription` | Inchangé | Faible | Objet historique explicitement conservé. |

## 5. Stratégie post-normalisation

### Étape 1 : vérifier les références migrées

Après renommage dans UE5 :

- ouvrir la palette ;
- ouvrir les niveaux de test ;
- vérifier les objets placés ;
- vérifier les connecteurs ;
- sauvegarder les packages concernés ;
- exécuter `Fix Up Redirectors`.

### Étape 2 : créer les archétypes manquants

Créer ensuite, sans renommer les archétypes validés :

- `DA_Button_Wall`
- `DA_Receptacle_Alcove`
- `DA_Receptacle_Altar`
- `DA_Receptacle_OfferingBowl`
- `DA_Lock_Keyhole`
- `DA_Teleporter_Rune`
- `DA_Item_Key`
- `DA_Item_Coin`
- `DA_Spawn_Player`

### Étape 3 : ne conserver les exemples que s'ils servent vraiment

Si un ancien puzzle/test doit rester comme exemple, créer un asset dédié avec préfixe `Example_`.

Ne pas réutiliser les archétypes génériques pour encoder une énigme spécifique.

## 6. Cas particuliers

### `WallInscription`

`WallInscription` reste sous son nom historique. Ne pas le renommer en `Readable_WallInscription` sans migration explicite.

### `Door_Stone`

`Door_Stone` est déjà canonique et ne doit pas être modifié côté nom.

### `Item_Torch`

`Item_Torch` est déjà canonique et ne doit pas être modifié côté nom.

### Décorations de sol

Les décorations de sol peuvent conserver leurs IDs actuels et leur préfixe de fichier `DA_A_` pour l'instant. Elles ne sont pas prioritaires dans la normalisation gameplay.

### `FloorRuneCircle`

`FloorRuneCircle` reste une `Decoration`. Créer `Teleporter_Rune` séparément quand le téléporteur sera authoré.

## 7. Checklist de validation

Pour chaque archétype renommé ou créé :

- apparaît dans la palette ;
- peut être placé dans la grille ;
- la preview est correcte ;
- l'inspecteur affiche la bonne section contextuelle ;
- l'acteur runtime spawn correctement ;
- les connecteurs existants se résolvent toujours ;
- le niveau charge sans warnings d'archétype manquant ;
- le comportement PIE est inchangé ou corrigé volontairement ;
- les redirectors Unreal ont été corrigés ;
- le commit ne contient que la famille d'assets concernée.

## 8. Recommandation finale

La normalisation de base est validée. Ne pas réintroduire les anciens noms orientés puzzle/test dans la documentation ou la palette.

Pour la suite, créer les manquants en petits lots :

1. `Button_Wall`
2. réceptacles manquants ;
3. items manquants ;
4. téléporteur ;
5. spawn joueur.

Committer chaque famille séparément après validation UE5.
