# MON19.2.3 — Primitives logiques data-driven

## Statut

**VALIDÉ sous Unreal Engine 5.5.4.**

Validation fournie le 22.08.2026 :

- compilation UE5.5.4 : OK ;
- runtime Logic : 5/5 tests `Success` ;
- policy éditeur Logic : 2/2 tests `Success` ;
- total MON19.2.3 : **7/7 tests `Success`**.

Le warning `cyclic logic target dispatch` observé dans `EventCommandChain` est attendu : le test crée volontairement un self-loop afin de vérifier qu'un nœud Logic ne peut pas être réexécuté pendant sa propre propagation et qu'aucune seconde mutation n'est appliquée.

## Objectif

MON19.2.3 construit les premières briques logiques au-dessus des variables persistantes validées en MON19.2.2.

Les primitives restent intégrées au graphe existant :

```text
Event
  ↓
FGridObjectLink
  ↓
Command
  ↓
Logic node
  ↓
GridLevelVariableStore
  ↓
Activated / Deactivated
  ↓
FGridObjectLink suivant
```

Il n'existe donc ni second moteur d'événements, ni Actor par primitive, ni Tick, ni Lua dans cette étape.

## Représentation

`EGridLevelObjectType::Logic` est ajouté à la fin de l'enum existant afin de ne pas déplacer les valeurs historiques.

Un objet `Logic` utilise `FGridLogicNodeParams` :

- `NodeType` ;
- `VariableId` ;
- valeur Bool éventuelle ;
- valeur/opérande Int32 éventuel ;
- comparateur Int32 éventuel.

Les nœuds sont des entrées data-only de `UGridLevelAsset::Objects`. Ils possèdent donc déjà :

- un `ObjectId` stable ;
- la capacité d'être source d'un lien ;
- la capacité d'être cible d'un lien ;
- la sérialisation native du LevelAsset.

Ils ne nécessitent aucun Actor runtime. `ArchetypeId` doit rester `None` : `GridLogicRuntime::ValidateNode()` refuse explicitement un archetype afin de préserver le contrat data-only. Le pipeline runtime existant est déjà sur liste blanche et ne spawn donc pas un nœud Logic sans archetype.

## Commandes

Deux nouvelles commandes sont ajoutées avec des valeurs explicites après le contrat MON13 :

```text
LogicExecute = 20
LogicReset   = 21
```

`LogicExecute` exécute la primitive.

`LogicReset` est volontairement limité au `Latch`.

Le SaveVersion reste `7` : aucune nouvelle donnée runtime persistante n'est ajoutée par MON19.2.3.

## Primitives

### Relay

Entrée :

```text
LogicExecute
```

Sortie :

```text
Activated
```

Aucun état n'est modifié.

### SetBool

Écrit `bBoolValue` dans la variable Bool ciblée puis émet `Activated`.

### ToggleBool

Inverse la variable Bool ciblée puis émet `Activated`.

### SetInt

Écrit `IntValue` dans la variable Int32 ciblée puis émet `Activated`.

### AddInt

Ajoute `IntValue` à la variable Int32 ciblée puis émet `Activated`.

Le calcul est effectué en `int64` puis validé avant écriture. Un dépassement de `int32` échoue atomiquement et n'émet aucun événement.

### SubtractInt

Soustrait `IntValue` avec la même protection contre les dépassements.

### ResetVariable

Restaure uniquement la variable ciblée à la valeur par défaut déclarée dans `UGridLevelAsset`.

Les autres variables du niveau ne sont pas modifiées.

### CompareBool

Compare la valeur courante au `bBoolValue` configuré :

```text
égalité     → Activated
différence  → Deactivated
```

### CompareInt

Comparateurs supportés :

- Equal ;
- NotEqual ;
- Less ;
- LessOrEqual ;
- Greater ;
- GreaterOrEqual.

Résultat :

```text
vrai   → Activated
faux   → Deactivated
```

### Latch

Le latch s'appuie sur une variable Bool persistante déclarée dans le niveau.

`LogicExecute` :

```text
false → true + Activated
true  → succès silencieux
```

`LogicReset` :

```text
true  → false + Deactivated
false → succès silencieux
```

Le one-shot survit donc naturellement aux sauvegardes et changements de niveau via le stockage MON19.2.2.

## Runtime central

`Runtime/GridLogicRuntime.h/.cpp` fournit :

```text
ValidateNode(...)
ExecuteNode(...)
```

Le runtime :

- valide le type de variable attendu ;
- refuse une variable absente ;
- refuse les commandes incompatibles ;
- exécute les mutations via `GridLevelVariableStore` ;
- produit éventuellement un `Activated` ou `Deactivated`.

`UGridActivationComponent::ApplyLinkCommand()` détecte une cible `Logic`, récupère le `FGridLevelRuntimeState` courant, appelle `GridLogicRuntime`, puis redispatche l'événement produit dans le graphe Event→Command existant.

La protection cyclique déjà présente dans `UGridActivationComponent` reste donc utilisée pour les chaînes logiques. MON19.2.3 ajoute en plus un garde ciblé : un nœud Logic déjà en cours d'émission ne peut pas redevenir cible avant la fin de cette propagation. La commande est rejetée avant mutation, ce qui évite qu'un cycle applique deux fois `AddInt`, `ToggleBool`, etc.

## Éditeur — politique de connecteurs

`GridEditorLinkPolicy` reconnaît désormais les nœuds Logic.

Sources :

```text
Relay / mutations / ResetVariable
    → Activated

CompareBool / CompareInt
    → Activated, Deactivated

Latch
    → Activated, Deactivated
```

Cibles :

```text
tous les nœuds Logic
    → LogicExecute

Latch
    → LogicExecute, LogicReset
```

Ces commandes sont classées `Gameplay`, car elles ont un effet runtime réel.

## Tests ajoutés

Runtime :

```text
Grimrock.MON19.2.Runtime.Logic.MutationPrimitives
Grimrock.MON19.2.Runtime.Logic.ComparisonPrimitives
Grimrock.MON19.2.Runtime.Logic.LatchAndRelay
Grimrock.MON19.2.Runtime.Logic.ValidationAndOverflow
Grimrock.MON19.2.Runtime.Logic.EventCommandChain
```

Le dernier test construit une vraie chaîne :

```text
Trigger.Activated
  → SetBool(Gate=true).LogicExecute
      → Activated
          → AddInt(Count,+3).LogicExecute
```

sans Actor pour les deux nœuds Logic.

Éditeur :

```text
Grimrock.MON19.2.Editor.LogicPolicy
Grimrock.MON19.2.Editor.LogicRuntimeSupport
```

## Validation UE5.5.4

Résultats observés :

```text
Grimrock.MON19.2.Runtime.Logic.ComparisonPrimitives   Success
Grimrock.MON19.2.Runtime.Logic.EventCommandChain     Success
Grimrock.MON19.2.Runtime.Logic.LatchAndRelay         Success
Grimrock.MON19.2.Runtime.Logic.MutationPrimitives    Success
Grimrock.MON19.2.Runtime.Logic.ValidationAndOverflow Success

Grimrock.MON19.2.Editor.LogicPolicy                  Success
Grimrock.MON19.2.Editor.LogicRuntimeSupport          Success
```

Le log attendu du test cyclique est :

```text
Grid link failed ... Command=LogicExecute Reason=cyclic logic target dispatch
```

Cette ligne confirme le comportement testé ; elle n'indique pas un échec du jalon. `EventCommandChain` se termine par `Result={Success}` et la chaîne précédente applique correctement chaque mutation une seule fois.

## Périmètre volontairement différé

MON19.2.3 ne fournit pas encore :

- panneau dédié de création/édition des variables et nœuds Logic dans le Grid Editor ;
- visualisation graphique spécialisée des nœuds ;
- adaptation complète du panneau général `ValidateCurrentLevel()` aux nœuds Logic data-only (le chantier de validation/édition dédié reste MON19.6) ;
- conditions de lien basées directement sur une variable ;
- opérations Bool AND/OR/XOR ;
- branches multi-conditions ;
- timers/delays ;
- Lua ;
- API de debug PIE dédiée aux variables.

L'édition ergonomique complète est prévue dans le chantier éditeur MON19.6. À ce stade, les primitives constituent le contrat runtime/data-driven stable utilisé par les étapes suivantes.
