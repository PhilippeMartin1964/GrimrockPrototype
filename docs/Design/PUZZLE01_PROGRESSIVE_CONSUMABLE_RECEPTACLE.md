# PUZZLE01 — Réceptacle consommable à progression

Statut : **implémentation C++ proposée — validation UE5.5.4 à effectuer**  
Date : **10 septembre 2026**

## 1. Objectif

PUZZLE01 ajoute un mécanisme générique permettant de construire des énigmes dans lesquelles un objet du niveau reçoit plusieurs items compatibles, les retire définitivement de la possession du joueur, matérialise visuellement la progression sur son propre mesh et émet `Activated` lorsque le nombre requis est atteint.

Cas initial : une tête de gardien possède deux orbites vides. Deux exemplaires quelconques de `DA_Item_BlueGem` doivent être trouvés et insérés. La première gemme allume un œil, la seconde allume l'autre puis le gardien émet `Activated`, qui peut être relié directement à `Door.Open`.

Aucun script Lua n'est requis.

## 2. Architecture

La classe runtime est :

```text
AGridProgressiveReceptacleActor
    -> AGridReceptacleActor
```

Le système réutilise intégralement le contrat du réceptacle existant :

- `bAcceptAnyItem` / `AcceptedItems` pour la compatibilité ;
- `MaxContainedItems` comme seuil de complétion ;
- le transfert cursor/inventaire existant pour retirer l'item de sa source ;
- `FGridRuntimeReceptacleState::ContainedItems` pour la persistance ;
- `ItemInserted` et `ItemChanged` à chaque insertion ;
- `Activated` lorsque l'insertion réussie atteint `MaxContainedItems`.

PUZZLE01 n'ajoute donc ni compteur parallèle, ni variable de niveau obligatoire, ni champ SaveGame supplémentaire.

## 3. Sens de « consommé »

Une gemme insérée n'est plus un item récupérable par le joueur et aucun `AGridItemActor` n'est conservé dans la scène. Le runtime garde toutefois l'entrée logique du contenu du réceptacle comme **charge de progression**.

Cette charge interne permet de réutiliser sans duplication la persistance déjà existante des réceptacles. Elle conserve l'identité canonique de l'item (`ItemDefinitionId`) mais sa représentation visuelle individuelle est supprimée.

Pour un réceptacle progressif :

```text
Inventory / Cursor
        |
        v
Accepted item
        |
        +--> logical contained charge (persisted)
        |
        +--> individual ItemActor destroyed
        |
        +--> material progress updated
```

Le retrait joueur est automatiquement désactivé (`bCanRemoveItem = false`).

## 4. Données de définition

`FGridReceptacleBehaviorParams` contient désormais :

```text
Progressive Consume
    Enabled
    Material Steps[]
        Material Slot Name
        Material
```

`Max Contained Items` reste l'unique seuil de complétion. Il ne faut pas ajouter un second champ `RequiredCount`.

La correspondance est volontairement directe :

```text
0 item  -> aucun Material Step appliqué
1 item  -> Material Steps[0]
2 items -> Material Steps[0] + Material Steps[1]
...
```

Lorsqu'une insertion fait atteindre `Max Contained Items`, `AGridProgressiveReceptacleActor` émet `Activated` une seule fois pour cette transition de progression.

## 5. Configuration du gardien

### 5.1 Static Mesh

`SM_Guardian_Face_01` doit exposer deux slots de matériau distincts et nommés de façon stable :

```text
Eye_Left
Eye_Right
```

Les deux slots utilisent au repos le matériau correspondant à une orbite vide/sombre.

Il est préférable d'utiliser les noms de slots plutôt que des indices numériques : une réorganisation des Material Slots du Static Mesh ne casse ainsi pas l'énigme.

### 5.2 Item

Un seul item est nécessaire :

```text
DA_Item_BlueGem
    ItemDefinitionId = Gem_Blue
    ItemType          = Gem
    Stackable         = selon le design global
    World Mesh        = mesh de la gemme bleue
```

Il ne faut pas créer `LeftEye` et `RightEye` comme deux identités d'item distinctes pour cette énigme.

### 5.3 DA_Guardian

Configuration cible :

```text
Definition
    Gameplay Type = Receptacle

Default Behavior
    Receptacle
        Accept Any Item = false
        Accepted Items
            [0] DA_Item_BlueGem

        Max Contained Items = 2

        Progressive Consume
            Enabled = true
            Material Steps
                [0]
                    Material Slot Name = Eye_Left
                    Material          = MI_Gem_Blue
                [1]
                    Material Slot Name = Eye_Right
                    Material          = MI_Gem_Blue

        Visual Placement Mode = Attached Socket
        Simulate Physics When Placed = false

Runtime
    Runtime Actor Class = GridProgressiveReceptacleActor
```

`Initial Content` reste vide pour le puzzle normal.

Le choix gauche puis droite est uniquement une présentation de progression. Les deux items acceptés sont exactement le même `DA_Item_BlueGem` et peuvent être trouvés dans n'importe quel ordre.

## 6. Connexion à la porte

Le Grid Editor autorise désormais `Activated` comme événement source d'un `Receptacle`.

Le puzzle complet n'a besoin que d'un lien :

```text
Guardian.Activated
    -> Door.Open
```

Il n'est pas nécessaire d'ajouter :

- `ReceptacleConsumeItem` ;
- un `AddInt` ;
- un `CompareInt` ;
- une variable `GuardianEyeCount` ;
- un callback Lua.

La progression est portée directement par le contenu logique non récupérable du réceptacle.

## 7. Événements

Chaque gemme acceptée conserve les événements historiques du réceptacle :

```text
ItemInserted
ItemChanged
```

La seconde gemme, lorsque `MaxContainedItems = 2`, ajoute ensuite :

```text
Activated
```

Ordre cible :

```text
2e insertion
    -> ItemInserted
    -> ItemChanged
    -> mise à jour de présentation progressive
    -> Activated
```

Les liens `ItemInserted` restent donc disponibles pour d'autres effets secondaires, mais l'ouverture de la porte doit utiliser `Activated` afin de ne se produire qu'à la complétion.

## 8. Persistance

Le mécanisme réutilise `FGridRuntimeReceptacleState::ContainedItems`.

Exemple après une gemme :

```text
ContainedItems.Num() = 1
ItemActor             = null au runtime progressif
Material Eye_Left     = MI_Gem_Blue
Material Eye_Right    = matériau initial
```

Après sauvegarde et chargement, l'entrée logique est restaurée par le chemin normal du réceptacle. `AGridProgressiveReceptacleActor` supprime immédiatement le visuel d'item restauré puis recalcule les Material Steps à partir du nombre d'entrées.

Ainsi :

```text
0 charge -> 0 œil allumé
1 charge -> 1 œil allumé
2 charges -> 2 yeux allumés
```

Le restore ne réémet pas `Activated`; il restaure seulement l'état déjà atteint.

## 9. Limites volontaires de PUZZLE01

PUZZLE01 ne cherche pas à devenir un système général de timeline visuelle. Chaque palier applique un remplacement de matériau sur un slot nommé du `MeshComponent` principal du réceptacle.

Ne sont pas ajoutés dans cette tranche :

- animations de transition de l'œil ;
- Niagara/VFX lors de l'insertion ;
- son spécifique de gemme ;
- changement de mesh par palier ;
- callbacks Lua ;
- plusieurs changements de matériaux dans un même palier.

Ces effets pourront être ajoutés ultérieurement sans modifier le contrat de progression de base.

## 10. Validation automatisée

Filtre ajouté :

```text
Grimrock.PUZZLE01.ProgressiveConsumableReceptacle
```

Le test vérifie notamment :

- rejet d'un item non accepté ;
- première gemme -> progression 1/2 ;
- premier Material Step appliqué ;
- suppression de l'`ItemActor` consommé ;
- seconde gemme -> progression 2/2 ;
- second Material Step appliqué ;
- état `Full` à la complétion ;
- rejet d'une troisième gemme ;
- capture des deux charges par la persistance existante ;
- restauration des deux charges ;
- restauration des deux matériaux ;
- maintien de `bCanRemoveItem=false`.

Le test de contrat éditeur `Grimrock.MON19.2.Editor.LinkPolicyMatrix` est également adapté pour reconnaître `Activated` comme quatrième événement possible d'un réceptacle.
