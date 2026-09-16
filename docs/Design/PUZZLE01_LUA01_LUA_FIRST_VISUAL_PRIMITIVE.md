# PUZZLE01-LUA01 — Gardien aux deux gemmes, architecture Lua-first

Statut : **implémentation C++ proposée — validation UE5.5.4 à effectuer**  
Date : **16 septembre 2026**  
Base : `bb2629dfadc594f71560e46f8dd6fcdbea5779c4`

## 1. Décision architecturale

PUZZLE01-LUA01 remplace volontairement la première implémentation PUZZLE01 fondée sur un `AGridProgressiveReceptacleActor` spécialisé.

La règle retenue est désormais :

```text
C++         = primitives génériques et sûres du moteur
DataAsset   = ressources et paramètres réutilisables
Lua         = logique particulière d'une énigme
Compilateur = validation statique de l'authoring Lua
```

Le moteur ne doit pas connaître les notions « première gemme », « deuxième gemme », « œil gauche », « œil droit » ou « ouvrir la porte du Gardien ».

Ces choix appartiennent au script du niveau.

Le script Lua doit rester **le plus simple possible et aller droit au but**. Les contrôles de validité d'un `LogicId`, d'une commande, d'un slot ou d'un alias statique appartiennent au compilateur Grimrock Lua, pas au script du level designer.

## 2. Rollback de l'ancien PUZZLE01

Le commit spécialisé `a69489c8912639bbe95b25ef8e5050a6c766785c` a été retiré de `master` avant PUZZLE01-LUA01.

Sont donc abandonnés :

```text
AGridProgressiveReceptacleActor
FGridReceptacleProgressiveConsumeParams
FGridReceptacleProgressMaterialStep
Receptacle.Activated comme événement spécial de complétion
```

Le Gardien redevient un `AGridReceptacleActor` ordinaire.

## 3. Primitive générique ajoutée

La seule nouvelle capacité de présentation nécessaire au puzzle est :

```lua
grid.visual.set_material(target, material_slot, material_alias)
```

Exemple d'authoring cible :

```lua
grid.visual.set_material(
    "Guardian",
    "EyesLeft",
    "BlueGem")
```

Aucun `must(...)`, `assert(...)` ni traitement manuel de `(ok, err)` n'est nécessaire pour cet appel statiquement vérifiable.

`target` accepte le même contrat que `grid.command` :

```text
ObjectId interne
ou
LogicId lisible
```

Lua n'obtient jamais de `UObject`, `AActor`, `UWorld`, chemin de package ou pointeur Unreal.

## 4. Material Alias

`UGridWorldObjectDefinitionAsset` expose désormais :

```text
Visual
└── Runtime Materials
    └── Runtime Material Aliases
```

C'est une map :

```text
Alias -> UMaterialInterface
```

Pour le Gardien :

```text
BlueGem -> MI_Gem_Blue
```

Le script utilise seulement `BlueGem`.

Le compilateur doit vérifier avant PlayTest :

1. que `Guardian` résout exactement un objet ;
2. que la définition visuelle existe ;
3. que le slot `EyesLeft` ou `EyesRight` existe sur le mesh ;
4. que l'alias `BlueGem` est déclaré et non nul.

Le runtime conserve ses propres gardes internes en cas de données incohérentes, mais le level designer ne doit pas recopier ces contrôles dans son script.

## 5. Persistance générique de la présentation

Un changement de matériau fait partie de l'état runtime du niveau.

`FGridLevelRuntimeState` contient désormais :

```text
ObjectVisuals
    ObjectId
        MaterialAliasesBySlot
            Slot -> Alias
```

Exemple :

```text
Guardian
    EyesLeft  -> BlueGem
    EyesRight -> BlueGem
```

Seuls les noms sémantiques sont sauvegardés. Aucun pointeur de matériau n'est sérialisé.

Lorsqu'un runtime object est recréé après un changement de niveau ou un chargement de sauvegarde, `AGridRuntimeObjectActor` réapplique les alias sauvegardés depuis sa définition.

L'ajout de cette propriété SaveGame est rétrocompatible avec les sauvegardes précédentes : l'absence de `ObjectVisuals` signifie simplement qu'aucun override visuel n'existe. `CurrentSaveVersion` reste inchangée.

## 6. Pourquoi le compteur reste en Lua

Dans PUZZLE01-LUA01, les gemmes sont consommées immédiatement par la commande générique déjà existante :

```lua
grid.command("Guardian", "ReceptacleConsumeItem")
```

Le réceptacle ne conserve donc pas les deux gemmes comme état logique.

Le nombre de gemmes données au Gardien appartient à l'énigme et est déclaré dans le script :

```lua
persistent = {
    GuardianGemCount = 0
}
```

Le mécanisme MON19.7.1 synchronise automatiquement cette déclaration avec les `LevelVariables`, puis le `FGridLevelRuntimeState` et le SaveGame.

Ainsi le C++ ne possède aucun `RequiredGemCount`, `ProgressCount`, `LeftEye` ou `RightEye`.

## 7. Script cible du Gardien

Le script cible exprime uniquement la logique du puzzle :

```lua
persistent = {
    GuardianGemCount = 0
}

function on_gem_inserted(event)
    if persistent.GuardianGemCount >= 2 then
        return
    end

    grid.command("Guardian", "ReceptacleConsumeItem")

    local next_count = persistent.GuardianGemCount + 1

    if next_count == 1 then
        grid.visual.set_material(
            "Guardian",
            "EyesLeft",
            "BlueGem")
    else
        grid.visual.set_material(
            "Guardian",
            "EyesRight",
            "BlueGem")

        grid.command("GuardianDoor", "Open")
        grid.command("Guardian", "ReceptacleDisableInsertion")
    end

    persistent.GuardianGemCount = next_count
end
```

Le nombre `2`, l'ordre gauche/droite et l'ouverture de la porte sont clairement visibles et modifiables dans le script du puzzle.

La validité des appels `grid.command(...)` et `grid.visual.set_material(...)` est une responsabilité de **Compile Lua**. Si une primitive statiquement vérifiable n'est pas encore couverte, il faut étendre le compilateur plutôt qu'ajouter de la plomberie de validation dans le script.

## 8. Configuration manuelle de `DA_Guardian`

Après récupération de PUZZLE01-LUA01, remettre le Gardien sur le runtime générique :

```text
Gameplay Type        = Receptacle
Runtime Actor Class  = GridReceptacleActor
Runtime Interactable = true
```

Réceptacle :

```text
Accept Any Item     = false
Accepted Items[0]   = DA_Item_BlueGem
Max Contained Items = 1
Initial Content     = vide
```

`Max Contained Items = 1` est intentionnel : le réceptacle sert de point d'entrée temporaire. Chaque gemme acceptée est consommée immédiatement par Lua ; la progression `0/1/2` appartient au script.

Présentation :

```text
Static Mesh = SM_Guardian_Face_01

Runtime Material Aliases
    BlueGem = MI_Gem_Blue
```

Le Static Mesh réel utilise déjà les slots :

```text
EyesLeft
EyesRight
```

Le script doit conserver exactement ces noms.

## 9. LogicId et binding

Attribuer au Gardien :

```text
Logic Id = Guardian
```

Attribuer à la porte :

```text
Logic Id = GuardianDoor
```

Créer ensuite le binding Lua :

```text
Source   : Guardian
Event    : Item Inserted
Script   : GuardianGemDoor
Callback : on_gem_inserted
```

Le lien objet direct utilisé pendant le premier PUZZLE01 doit disparaître :

```text
Guardian.Activated -> Door.Open   [SUPPRIMÉ]
```

Le seul déclencheur du puzzle est désormais :

```text
Guardian.ItemInserted
    -> Lua GuardianGemDoor.on_gem_inserted
```

## 10. Flux runtime

Première gemme :

```text
DA_Item_BlueGem accepté
    -> ItemInserted
    -> Lua
        -> ReceptacleConsumeItem
        -> GuardianGemCount = 1
        -> EyesLeft = BlueGem
```

Deuxième gemme :

```text
DA_Item_BlueGem accepté
    -> ItemInserted
    -> Lua
        -> ReceptacleConsumeItem
        -> GuardianGemCount = 2
        -> EyesRight = BlueGem
        -> GuardianDoor.Open
        -> ReceptacleDisableInsertion
```

Le changement de matériau n'est qu'une primitive de présentation. Sa signification « une gemme occupe cet œil » existe uniquement dans le script.

## 11. Périmètre volontaire de LUA01

PUZZLE01-LUA01 n'essaie pas encore de reproduire toute l'API objet de Legend of Grimrock.

Ne sont pas encore ajoutés :

```text
grid.receptacle.contents(...)
grid.item.enable/disable(...)
grid.visual.set_mesh(...)
grid.visual.set_visibility(...)
grid.visual.play_vfx(...)
```

Ces primitives pourront être ajoutées lorsqu'un puzzle réel en aura besoin. La règle est désormais de compléter le moteur avec la primitive générique minimale, puis d'écrire la logique de puzzle en Lua.

## 12. Tests ajoutés

Filtres PUZZLE01-LUA01 :

```text
Grimrock.PUZZLE01.LUA01
```

Couverture :

```text
LuaVisualApi
LuaVisualApiFailureIsData
RuntimeMaterialAliasPersistence
GuardianPuzzleIntegration
```

Les tests vérifient :

- l'existence de `grid.visual.set_material` dans le sandbox hébergé ;
- le passage de `LogicId`, slot et alias comme simples données ;
- le retour contrôlé `false, error` au niveau runtime si le host visuel n'est pas disponible ;
- la résolution d'un alias depuis une `WorldObjectDefinition` ;
- le remplacement par nom de Material Slot ;
- la persistance Slot -> Alias ;
- la réapplication sur un runtime actor fraîchement recréé ;
- un Gardien réel basé sur `AGridReceptacleActor` ;
- deux insertions successives pilotées par Lua ;
- la consommation des gemmes par la commande existante ;
- `EyesLeft`, puis `EyesRight` ;
- le compteur de puzzle persistant porté par Lua.

Les tests internes peuvent volontairement inspecter les retours techniques `(ok, err)` pour éprouver les gardes du runtime. Cela ne change pas la convention d'authoring : les scripts de niveau ordinaires restent directs et sans wrapper de validation.

Validation à effectuer localement sous UE5.5.4 avant de considérer PUZZLE01-LUA01 comme clos.
