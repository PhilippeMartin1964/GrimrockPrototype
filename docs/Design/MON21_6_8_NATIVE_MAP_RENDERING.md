# MON21.6.8 — Existing WBP + Native Map Rendering

Date : **28 septembre 2026**  
Statut : **AUTOMATION 4/4 VALIDÉE — INTÉGRATION UMG/PIE CONFIRMÉE — CORRECTIF ORIENTATION E/O À REVALIDER EN PIE**

## 1. Objectif

MON21.6.8 branche le read model Map validé sur la surface existante `WBP_GridMap`, sans créer de second menu et sans allouer un widget par cellule.

Architecture :

```text
AGrimrockPartyPawn
    -> AGridLevelRuntimeActor
       -> DungeonAsset
       -> DungeonRuntimeState
       -> DoorSystem vivant
    -> UGridMapWidget::RefreshMap()
       -> FGridMapReadModelBuilder::BuildFloorView()
       -> FGridMapFloorView transient
       -> UGridMapWidget::NativePaint()
       -> Slate draw elements
```

## 2. Classe native

Nouvelle classe :

```text
UGridMapWidget : UGrimrockDesignSurfaceWidget
```

API :

```text
InitializeMapWidget(PartyPawn)
RefreshMap()
HasRenderableMap()
GetFloorView() [C++]
```

`FloorView` est `Transient` et n’est jamais une autorité persistante.

Le SaveGame reste **v23**.

## 3. Intégration au shell existant

`UGrimrockMenuWidget` conserve son binding historique :

```cpp
TObjectPtr<UWidget> Page_Map;
```

Il n’est volontairement **pas** remplacé par un `BindWidget UGridMapWidget` avant la modification binaire de l’asset.

Le shell ajoute uniquement :

```text
GetMapWidget()
RefreshMap()
```

`InitializeMenuWidget()` initialise la Map lorsque `Page_Map` est déjà un `UGridMapWidget`.

`SetActiveTopTab(Map)` appelle `RefreshMap()`.

Ainsi le code C++ reste compatible avec l’asset UMG actuel avant reparent.

## 4. Rendering natif

`NativePaint()` produit directement des primitives Slate.

Aucun :

- `UWidget` par cellule ;
- Canvas child par primitive ;
- Actor Map ;
- Tick de reconstruction Map ;
- scan d’Actors monde.

Le seul snapshot utilisé pour dessiner est `FGridMapFloorView`.

### Cellules

Chaque cellule explorée reçoit un fond fonctionnel discret.

Les cellules inconnues sont absentes du snapshot et ne peuvent donc pas être peintes.

### Murs

`Wall` est dessiné comme un segment plein.

### Portes

`Door` est distingué du mur.

État :

```text
closed -> segment central présent
open   -> ouverture centrale visible
```

### Secrets

`SecretDoor` n’existe dans le snapshot qu’après découverte MON21.6.4/6.6.

Avant découverte, le renderer reçoit uniquement `Wall`.

Une porte secrète découverte reçoit un petit repère fonctionnel supplémentaire ; le style final sera repris en MON21.6.11.

### Groupe

Le groupe est rendu comme une flèche/triangle orienté :

```text
North -> haut écran
East  -> gauche
South -> bas
West  -> droite
```

La projection écran suit la convention visuelle déjà utilisée par l’Overview Map de l’éditeur :

```text
Y+ / North -> haut écran
X+ / East  -> gauche écran
```

Les coordonnées autoritaires restent inchangées (`East = X+`, `West = X-`). Seule la présentation écran est miroir sur X.

## 5. Fit automatique

Le renderer calcule les bornes des seules cellules connues, puis centre le plan dans la surface disponible.

`MaxCellPixels` limite l’agrandissement d’une petite zone explorée.

`MapDrawPadding` réserve une marge fonctionnelle pour le chrome existant du WBP.

Ce mécanisme n’est ni zoom ni pan : MON21.6.10 reste responsable de ces interactions.

## 6. Rafraîchissement

Le snapshot Map est reconstruit :

- à l’initialisation du widget ;
- à chaque activation de la page Map dans le shell.

Il n’est pas reconstruit à chaque frame.

Le `NativeTick` hérité du `UGrimrockDesignSurfaceWidget` reste l’infrastructure UI générale de scaling 1920×1080 ; il ne reconstruit jamais la Map.

## 7. Validation MON21.6.7 actée

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_7
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-092245
```

MON21.6.7 est **VALIDÉ**.

## 8. Automation C++

Filtre :

```text
Grimrock.Map.MON21_6_8
```

Tests :

```text
NativeRendering.WidgetContract
NativeRendering.RefreshBuildsCurrentFloor
NativeRendering.RefreshFailsClosed
NativeRendering.ExistingShellHook
```

Validation utilisateur du 28 septembre 2026 :

```text
Filter                  : Grimrock.Map.MON21_6_8
Succeeded               : 4
Succeeded with warnings : 0
Failed                  : 0
Not run                 : 0
Process exit code       : 0
Report                  : TD04-20260928-093526
```

L’intégration UMG et l’ouverture PIE par `M` sont également confirmées par capture.

Le smoke PIE a toutefois révélé une inversion Est/Ouest du premier renderer. Le correctif conserve les coordonnées Map canoniques et inverse uniquement la projection écran X, ainsi que les bords East/West et l’orientation visuelle du marqueur.

Les tests Automation ne remplacent donc toujours pas la revalidation visuelle de cette correction.

## 9. Modification UMG manuelle obligatoire

L’asset doit être modifié uniquement dans Unreal Editor :

```text
Content/GrimrockPrototype/Blueprints/UI/InGameMenu/WBP_GridMap
```

Action minimale :

1. ouvrir `WBP_GridMap` ;
2. `File -> Reparent Blueprint` ou `Class Settings -> Parent Class` selon l’UI UE ;
3. choisir `GridMapWidget` / `UGridMapWidget` ;
4. Compile ;
5. Save ;
6. ouvrir `WBP_GrimrockMenu` ;
7. Compile ;
8. Save All ;
9. lancer PIE ;
10. presser `M`.

Aucun nouveau widget enfant nommé n’est requis.

Ne pas modifier `Page_Map` dans `WBP_GrimrockMenu` : il doit continuer à référencer l’instance existante `WBP_GridMap`.

## 10. Critères visuels PIE

À l’ouverture par `M` :

- la page Map existante s’ouvre ;
- seules les cellules déjà explorées apparaissent ;
- les murs connus apparaissent ;
- les portes sont distinctes ;
- un secret non découvert reste visuellement un mur normal ;
- un secret découvert peut être distingué ;
- le marqueur du groupe apparaît sur la cellule courante avec la bonne orientation ;
- aucune couture purement technique entre LevelAssets n’apparaît.

Si le contenu placeholder historique du WBP masque ou chevauche le dessin, il sera retiré manuellement après capture d’écran ; aucune suppression aveugle n’est faite côté repository.

## 11. Stop condition

MON21.6.8 ne sera déclaré **VALIDÉ** qu’après :

1. build + Automation `Grimrock.Map.MON21_6_8` verts — **acquis 4/4** ;
2. reparent de `WBP_GridMap` dans UE — **effectué** ;
3. compilation des deux WBP — **effectuée** ;
4. smoke PIE `M` — **effectué** ;
5. revalidation visuelle après correctif Est/Ouest — **requise**.

Prochaine tranche après validation : **MON21.6.9 — Floor Navigation**.
