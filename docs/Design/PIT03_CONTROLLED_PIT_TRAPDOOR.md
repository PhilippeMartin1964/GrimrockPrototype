# PIT03 — Controlled Pit Trapdoor

Date : 01.09.2026

## Objectif

PIT03 transforme la fosse PIT01/PIT02 en mécanisme contrôlable par les connecteurs existants.

État runtime autoritaire :

```text
Closed
  = la cellule se comporte comme un plancher
  = le groupe ne tombe pas
  = les World Items restent sur la cellule

Open
  = la cellule se comporte comme une fosse
  = le groupe tombe
  = les World Items utilisent PIT02
```

Aucun nouveau type de commande n'est introduit. Une Pit utilise le contrat standard :

- `Open`
- `Close`
- `Toggle`
- `Activate` = Open
- `Deactivate` = Close

Elle émet :

- `Opened`
- `Closed`

Exemple :

```text
Lever.Activated
    -> Pit.Open

Lever.Deactivated
    -> Pit.Close
```

ou :

```text
Button.Activated
    -> Pit.Toggle
```

## Autorité runtime

`FGridLevelRuntimeState` contient désormais :

```cpp
TMap<FGuid, FGridRuntimePitState> Pits;
```

`FGridRuntimePitState` stocke :

```cpp
ObjectId
bIsOpen
```

Si aucune entrée runtime n'existe encore, `Behavior.Pit.bInitiallyOpen` reste la valeur initiale.

Les APIs sont :

```cpp
bool IsPitOpen(FGuid PitObjectId) const;
bool SetPitOpen(FGuid PitObjectId, bool bOpen, bool bEmitEvent = true);
bool TogglePit(FGuid PitObjectId, bool bEmitEvent = true);
```

Le changement d'état est persistant par niveau et survit aux allers-retours entre floors.

## Ouverture sous le groupe

Lorsqu'une Pit passe de Closed à Open :

1. les World Items déjà posés sur la cellule sont routés par PIT02 ;
2. si le groupe se trouve sur la cellule, `TryBeginPitFallAtCell()` est déclenché ;
3. l'événement `Opened` est émis.

La fermeture ne téléporte rien et n'annule pas une chute déjà commencée.

## Présentation : AGridPitTrapdoorActor

PIT03 ajoute :

`AGridPitTrapdoorActor : AGridMechanismActor`

Contrat visuel :

- `Fixed Mesh` = géométrie permanente de la fosse ouverte ;
- `Moving Mesh` = couvercle/trappe optionnel visible uniquement lorsque la Pit est Closed ;
- Open : Moving Mesh caché + collision désactivée ;
- Closed : Moving Mesh visible + collision activée.

Le jalon PIT03 porte l'état Closed/Open et la commutation visuelle instantanée. L'animation articulée de la trappe sera un jalon distinct.

## DA_Pit_Stone_01 après PIT03

L'ouverture de la palette met automatiquement l'archetype à niveau.

Valeurs attendues :

```text
Gameplay Type             = Pit
Category                  = Hazards
Functional Category       = Mechanism
Placement Kind            = Floor
Blocks Movement           = False
Hide Cell Floor           = True

Default Behavior > Pit
  Initially Open          = True
  Use Same Cell Coordinates = True

Default Behavior > Transition
  Is Transition           = True
  Require Use Action      = False

Visual
  Main Mesh / Preview Mesh = SM_Pit_Stone_01
  Fixed Mesh               = SM_Pit_Stone_01
  Moving Mesh              = None ou mesh de trappe fermé
  Fixed Material           = None
  Moving Material          = matériau optionnel de la trappe

Runtime
  Runtime Actor Class      = GridPitTrapdoorActor
```

Important : si `Moving Mesh=None`, l'état Closed fonctionne logiquement mais aucun couvercle n'est visible. Dès qu'un mesh de trappe sera disponible, l'assigner à `Moving Mesh`.

Les éventuels sons sont configurés par le contrat audio générique de l'archetype :

```text
Audio Events
  Open
  Close
```

Ils sont optionnels.

## Grid Editor

La Pit reste dans :

`Hazards > Stone Pit`

Selected Object > Pit :

```text
Open at Start
Use Same Cell Coordinates
```

Connectors accepte maintenant une Pit comme cible avec :

```text
Open
Close
Toggle
Activate
Deactivate
```

et comme source avec :

```text
Opened
Closed
```

## PIT02 et trappe fermée

`TryDropItemInstanceAtCell()` consulte désormais l'état runtime.

Ainsi :

```text
Closed Pit + pierre
  -> pierre reste sur le couvercle

Open Pit + pierre
  -> pierre tombe au niveau inférieur
```

Si la trappe s'ouvre alors que la pierre est déjà dessus :

```text
pierre sur Closed Pit
  -> commande Open
  -> DropWorldItemsThroughOpenPitAtCell()
  -> PendingInboundItems du niveau inférieur
```

## Persistance

Lors d'un changement de floor :

- l'état Pit reste dans `FGridLevelRuntimeState::Pits` ;
- `RebuildRuntimeObjects()` restaure le bon état visuel ;
- `ApplyCurrentLevelRuntimeState()` resynchronise également le `AGridPitTrapdoorActor`.

## Tests

Filtre :

`Grimrock.Pit.PIT03`

Tests :

- `Grimrock.Pit.PIT03.ControlledStateAndLinks`
- `Grimrock.Pit.PIT03.PresentationActorState`
- `Grimrock.Pit.PIT03.EditorLinkPolicy`

Le test principal couvre :

- Pit initialement Closed ;
- World Item posé sur la trappe ;
- Button.Activated -> Pit.Open ;
- chute automatique du World Item via PIT02 ;
- Pit.Opened -> seconde Pit.Open ;
- Close et Toggle ;
- persistance après aller-retour entre deux niveaux.

## Hors périmètre PIT03

- animation articulée de la trappe ;
- interpolation/reprise en sens inverse ;
- blocage mécanique pendant animation ;
- dégâts de chute ;
- monstres tombant ;
- puits multi-étages en cascade.

Une étape suivante naturelle est :

**PIT03.1 — Trapdoor Opening/Closing Animation**
