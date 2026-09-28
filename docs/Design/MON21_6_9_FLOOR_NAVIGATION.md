# MON21.6.9 — Floor Navigation

Date : **28 septembre 2026**  
Statut : **AUTOMATION 4/4 + UMG/PIE CONFIRMÉS — AFFICHAGE DisplayName À REVALIDER**

## 1. Objectif

MON21.6.9 rend navigables les étages déjà exposés par MON21.6.7, sans ajouter de nouvelle autorité et sans supposer que les valeurs Z sont contiguës.

Le contrat reste :

```text
Level Up   -> prochain Z activé strictement supérieur
Level Down -> prochain Z activé strictement inférieur
```

Exemple :

```text
AvailableFloorZs = [-3, 0, +2]

depuis +2 :
Level Down -> 0
Level Down -> -3
```

Aucun calcul `Z ± 1` n’est utilisé.

## 2. État transitoire de navigation

`UGridMapWidget` porte maintenant uniquement pour la session UI :

```text
bHasFloorSelection
SelectedFloorZ
```

Ces champs sont `Transient` et ne portent pas `SaveGame`.

Le schéma reste **v23 exact-match**.

La sélection d’un étage n’altère ni :

- `UGridDungeonAsset` ;
- `FGridDungeonRuntimeState` ;
- `FGridMapExplorationState` ;
- le niveau courant réel du groupe.

## 3. Refresh vs ouverture de la Map

Deux intentions sont séparées.

### Refresh événementiel

```text
RefreshMap()
    -> reconstruit SelectedFloorZ
    -> conserve l’étage consulté
```

Cela permet à une future mutation pertinente de rafraîchir la carte sans ramener artificiellement le joueur à son étage réel.

### Ouverture par le shell

`UGrimrockMenuWidget::RefreshMap()`, appelé lors de l’activation de l’onglet Map, appelle maintenant :

```text
UGridMapWidget::SelectPartyFloor()
```

Cela respecte le contrat MON21.6.1 :

```text
à l’ouverture :
SelectedFloorZ = LogicalPosition.Z du CurrentDungeonLevelId
```

Le futur `Recentrer` de MON21.6.10 réutilisera ce retour d’étage et y ajoutera le recentrage spatial.

## 4. API

`UGridMapWidget` expose :

```text
SelectPartyFloor()
NavigateFloorUp()
NavigateFloorDown()
CanNavigateFloorUp()
CanNavigateFloorDown()
GetSelectedFloorZ()
```

Les commandes `Navigate*` reconstruisent immédiatement le `FGridMapFloorView` sélectionné.

Si l’étage sélectionné n’est pas celui du groupe, le comportement MON21.6.7 reste autoritaire :

```text
bHasPartyMarker = false
```

## 5. Boutons désactivés aux bornes

Les Z sont récupérés via :

```text
FGridMapReadModelBuilder::GetAvailableFloorZs()
```

Ils sont déjà distincts et triés.

`CanNavigateFloorUp()` et `CanNavigateFloorDown()` pilotent donc directement l’état Enabled des boutons optionnels.

Exemple :

```text
[-3, 0, +2]

sur +2 :
Level Up   disabled
Level Down enabled

sur -3 :
Level Up   enabled
Level Down disabled
```

Les entrées de donjon désactivées ne participent pas à cette liste.

## 6. Intégration UMG optionnelle

Trois bindings optionnels sont ajoutés à `UGridMapWidget` :

```text
Button_LevelUp
Button_LevelDown
Text_FloorLabel
```

Types :

```text
Button_LevelUp   : UButton
Button_LevelDown : UButton
Text_FloorLabel  : UTextBlock
```

Le C++ :

- bind les deux `OnClicked` dans `NativeConstruct()` ;
- retire les bindings dans `NativeDestruct()` ;
- désactive automatiquement les boutons lorsqu’aucun étage n’existe dans leur direction ;
- affiche le `DisplayName` du niveau lorsque le nom de l’étage sélectionné est non ambigu ;
- fallback `Étage <Z>` si aucun `DisplayName` n’est défini ou si plusieurs dalles du même Z portent des noms différents.

Aucun Graph Blueprint n’est nécessaire.

Les widgets sont `BindWidgetOptional` : le C++ compile et les tests fonctionnent avant modification du `.uasset`.

## 7. Validation MON21.6.8 actée

Validation finale utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_8
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-095036
```

Le contrôle PIE confirme également l’orientation Est/Ouest corrigée.

MON21.6.8 est **VALIDÉ**.

La convention visuelle X miroir reste explicitement différée ; MON21.6.9 ne la modifie pas.

## 8. Automation MON21.6.9

Filtre :

```text
Grimrock.Map.MON21_6_9
```

Tests :

```text
FloorNavigation.SkipsMissingZAndStopsAtBounds
FloorNavigation.OtherFloorProjectionHidesParty
FloorNavigation.RefreshPreservesSelectionOpenResetsToParty
FloorNavigation.DisplayNameLabel
FloorNavigation.OptionalUMGAndTransientState
```

Première validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_9
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-102621
```

Le smoke PIE utilisateur confirme que les boutons Up/Down changent correctement d’étage.

À la suite de cette validation, `Text_FloorLabel` est affiné pour afficher le `DisplayName` plutôt que le Z technique lorsque ce nom est non ambigu. Le filtre MON21.6.9 contient désormais un cinquième test `DisplayNameLabel` et doit être relancé avant clôture définitive.

## 9. Modification UMG après validation C++

Dans `WBP_GridMap`, ajouter manuellement les trois widgets nommés exactement :

```text
Button_LevelUp
Button_LevelDown
Text_FloorLabel
```

Aucun changement de parent class n’est requis : le reparent vers `UGridMapWidget` a déjà été effectué en MON21.6.8.

Le placement visuel et le style peuvent rester fonctionnels à ce stade ; le polish appartient à MON21.6.11.

## 10. Hors périmètre

- Recentrer spatialement sur la position du groupe : MON21.6.10 ;
- molette zoom : MON21.6.10 ;
- clic-glisser pan : MON21.6.10 ;
- style parchemin / manuscrit : MON21.6.11 ;
- symboles supplémentaires : MON21.6.12.

Prochaine tranche après validation : **MON21.6.10 — Zoom / Pan / Recenter**.
