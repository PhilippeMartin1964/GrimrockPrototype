# PUZZLE01-LUA01 — Gardien aux deux gemmes, architecture Lua-first

Date : 16 septembre 2026  
Statut : **fondation générique conservée — intégration de production transférée à PUZZLE01-LUA02**

## Décision architecturale

PUZZLE01-LUA01 a remplacé l'ancienne approche spécialisée fondée sur un acteur C++ propre au Gardien.

La règle finale est :

```text
C++         = primitives génériques et sûres du moteur
DataAsset   = ressources et paramètres réutilisables
Lua         = logique particulière d'une énigme
Compilateur = validation statique de l'authoring Lua
```

Le moteur ne connaît pas les notions « première gemme », « deuxième gemme », « œil gauche », « œil droit » ou « ouvrir la porte du Gardien ». Ces choix appartiennent au script du niveau.

Le script Lua doit rester direct. Les contrôles de validité d'un `LogicId`, d'une commande, d'un slot ou d'un alias statique appartiennent au compilateur Grimrock Lua, pas au script du level designer.

## Primitive générique conservée

La capacité de présentation introduite pour ce puzzle est générique :

```lua
grid.visual.set_material(target, material_slot, material_alias)
```

`target` accepte un `ObjectId` interne ou un `LogicId` lisible. Lua n'obtient jamais de `UObject`, `AActor`, `UWorld`, chemin de package ou pointeur Unreal.

`UGridWorldObjectDefinitionAsset` expose un vocabulaire de matériaux runtime :

```text
Runtime Material Aliases
    Alias -> UMaterialInterface
```

Le script manipule uniquement l'alias sémantique, par exemple :

```text
BlueGem -> MI_Gem_Blue
```

Le compilateur valide les références statiques ; le runtime garde uniquement ses protections internes de sûreté.

## Persistance générique

Les changements de matériaux runtime sont persistés dans :

```text
FGridLevelRuntimeState
    ObjectVisuals
        ObjectId
            MaterialAliasesBySlot
                Slot -> Alias
```

Aucun chemin d'asset n'est sérialisé. Lorsqu'un runtime object est recréé, `AGridRuntimeObjectActor` réapplique les aliases persistés depuis sa définition.

Cette capacité est générique et reste disponible pour les futurs puzzles.

## Logique du Gardien

La progression reste dans `puzzle1_lvl1` via une variable Lua persistante :

```lua
persistent = {
    GuardianGemCount = 0
}
```

La logique de production exprime directement :

```text
1re gemme
    -> consommation
    -> GuardianGemCount = 1
    -> EyesLeft = BlueGem

2e gemme
    -> consommation
    -> GuardianGemCount = 2
    -> EyesRight = BlueGem
    -> GuardianDoor.Open
    -> ReceptacleDisableInsertion
```

Aucun `RequiredGemCount`, `ProgressCount`, `LeftEye` ou `RightEye` n'existe dans le C++.

## Configuration de données attendue

Le Gardien reste un réceptacle générique :

```text
Gameplay Type        = Receptacle
Runtime Actor Class  = GridReceptacleActor
Runtime Interactable = true
Accept Any Item      = false
Accepted Items       = gemme bleue compatible
Max Contained Items  = 1
Runtime Material Aliases
    BlueGem = MI_Gem_Blue
```

Les slots de matériaux utilisés par le script sont :

```text
EyesLeft
EyesRight
```

La porte porte le `LogicId` :

```text
GuardianDoor
```

## Tests LUA01 conservés

Après PUZZLE01-CLEAN02, LUA01 ne porte plus le test d'intégration complet du Gardien.

Les tests conservés protègent uniquement les primitives génériques :

```text
Grimrock.PUZZLE01.LUA01.LuaVisualApi
Grimrock.PUZZLE01.LUA01.LuaVisualApiFailureIsData
Grimrock.PUZZLE01.LUA01.RuntimeMaterialAliasPersistence
```

Ils vérifient notamment :

- l'existence de `grid.visual.set_material` dans le sandbox ;
- le passage de `LogicId`, slot et alias comme données simples ;
- les retours runtime contrôlés en cas de host indisponible ;
- la résolution d'un alias depuis une `WorldObjectDefinition` ;
- le remplacement par nom de Material Slot ;
- la persistance et la réapplication des overrides.

Les tests internes peuvent inspecter `(ok, err)` pour éprouver les gardes runtime. Cette convention de test ne s'applique pas aux scripts de niveau ordinaires.

## Intégration de production

L'ancien test :

```text
Grimrock.PUZZLE01.LUA01.GuardianPuzzleIntegration
```

est supprimé par PUZZLE01-CLEAN02 pour éviter une seconde autorité d'intégration et des assertions textuelles `Source.Contains(...)`.

L'autorité de production est désormais :

```text
Grimrock.PUZZLE01.LUA02.ProductionAuthoringAudit
Grimrock.PUZZLE01.LUA02.RuntimeGuardianDoorCompletion
```

Voir :

```text
docs/Design/PUZZLE01_LUA02_PRODUCTION_AUDIT.md
docs/Design/PUZZLE01_CLEAN02_FINAL_CONSOLIDATION.md
```
