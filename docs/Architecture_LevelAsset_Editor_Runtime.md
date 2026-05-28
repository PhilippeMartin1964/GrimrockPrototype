# Architecture LevelAsset / Editor / Runtime

Statut : reference d'architecture avant le passage au multi-niveaux.

## Principe central

La source de verite d'un niveau Grimrock est le DataAsset, pas une map Unreal.

`UGridLevelAsset` contient les donnees du niveau :

- taille de grille, cellules et murs ;
- objets places ;
- liens entre objets et commandes runtime.

Les maps Unreal servent a heberger des acteurs, des cameras, de la lumiere, des volumes et des configurations de test. Elles ne doivent pas devenir la source de verite du donjon.

## Roles actuels

### `UGridLevelAsset`

`UGridLevelAsset` est le format persistant d'un niveau.

Il doit rester dans le module runtime parce qu'il est lu par le jeu et modifie par les outils editor. Il ne doit pas dependre du module editor.

Responsabilites :

- stocker `Width`, `Height`, `CellSize` ;
- stocker `Cells` ;
- stocker `Objects` ;
- stocker `Links` ;
- fournir les helpers de coordonnees et de maintenance des donnees.

### `AGridLevelRuntimeActor` / `BP_GridLevelRuntimeActor`

`AGridLevelRuntimeActor` lit un `UGridLevelAsset` et reconstruit le niveau visible et jouable.

Responsabilites :

- generer sols, murs et plafonds depuis les cellules ;
- generer les objets runtime depuis les objets du DataAsset ;
- initialiser les composants d'activation et de portes ;
- fournir les helpers de deplacement et d'interaction.

Point important : chaque instance de `BP_GridLevelRuntimeActor` a sa propre reference `LevelAsset` et sa propre configuration de meshes/archetypes. Deux instances placees dans deux maps differentes peuvent donc diverger.

### `AGridLevelEditorActor` / `BP_GridLevelEditorActor`

`AGridLevelEditorActor` modifie un `UGridLevelAsset`.

Responsabilites :

- peindre les cellules et les murs ;
- placer, selectionner, deplacer et editer les objets ;
- creer et supprimer les liens ;
- valider les donnees du niveau ;
- pousser le `LevelAsset` vers un runtime actor de preview.

Il appartient au module `GrimrockPrototypeEditor`. Le runtime ne doit pas dependre de lui.

### `L_GrimrockEditor`

`L_GrimrockEditor` est une map de travail pour l'edition.

Elle peut contenir :

- un `BP_GridLevelEditorActor` ;
- un `BP_GridLevelRuntimeActor` utilise comme preview ;
- des lumieres, cameras et aides editor.

Elle ne doit pas etre consideree comme le niveau lui-meme. Si cette map pointe vers le mauvais `UGridLevelAsset`, l'editeur modifie le mauvais niveau.

### `L_GrimrockRuntime`

`L_GrimrockRuntime` est une map de test ou de jeu.

Elle peut contenir :

- un `BP_GridLevelRuntimeActor` ;
- le pawn, le controller, les lumieres et les elements propres a l'execution.

Elle reconstruit le niveau depuis le `LevelAsset` reference par son runtime actor. Elle ne synchronise pas automatiquement sa reference avec celle de `L_GrimrockEditor`.

## Probleme actuel

`BP_GridLevelRuntimeActor` existe dans `L_GrimrockEditor` et dans `L_GrimrockRuntime`, mais ce sont deux instances differentes.

Consequences :

- l'instance editor peut pointer vers `DA_Level_A` ;
- l'instance runtime peut pointer vers `DA_Level_B` ;
- les deux instances peuvent avoir des meshes ou archetypes differents ;
- un test PIE dans une map peut ne pas representer le niveau edite dans l'autre map.

Ce n'est pas un bug Unreal : c'est la consequence normale d'une configuration portee par des instances d'acteurs placees dans des maps differentes.

## Diagnostics ajoutes

`AGridLevelRuntimeActor` expose maintenant :

```cpp
UFUNCTION(CallInEditor, BlueprintCallable, Category = "Level|Diagnostics")
void LogLevelAssetDiagnostics() const;

UFUNCTION(BlueprintCallable, Category = "Level|Diagnostics")
FString GetLevelAssetDiagnostics() const;
```

Utilisation recommandee :

1. Ouvrir `L_GrimrockEditor`.
2. Selectionner l'instance `BP_GridLevelRuntimeActor`.
3. Executer `LogLevelAssetDiagnostics`.
4. Ouvrir `L_GrimrockRuntime`.
5. Repeter sur son instance `BP_GridLevelRuntimeActor`.
6. Comparer `RuntimeActor`, `Map`, `LevelAsset`, `GridSize`, `Objects`, `Links` et les meshes.

La sortie indique notamment :

- l'identite de l'acteur runtime ;
- la map et le type de world ;
- le chemin complet du `LevelAsset` ;
- la taille de grille ;
- le nombre de cellules attendues et presentes ;
- le nombre d'objets et de liens ;
- les meshes principaux configures sur l'acteur runtime.

## Regle de configuration

Pour un niveau donne, les instances runtime et editor doivent pointer vers le meme `UGridLevelAsset`.

Configuration attendue pour un niveau simple :

```text
L_GrimrockEditor
  BP_GridLevelEditorActor.LevelAsset  -> DA_Level_01
  BP_GridLevelRuntimeActor.LevelAsset -> DA_Level_01

L_GrimrockRuntime
  BP_GridLevelRuntimeActor.LevelAsset -> DA_Level_01
```

Si ces references divergent, le DataAsset reste la verite, mais les maps ne regardent pas la meme verite.

## Direction multi-niveaux

Le futur `UGridDungeonAsset` devra devenir l'asset racine du donjon.

Role cible probable :

- lister les `UGridLevelAsset` qui composent le donjon ;
- definir l'ordre ou les identifiants de niveaux ;
- stocker les transitions entre niveaux ;
- porter les metadonnees globales du donjon.

Relation cible :

```text
UGridDungeonAsset
  LevelEntries[0] -> UGridLevelAsset
  LevelEntries[1] -> UGridLevelAsset
  LevelEntries[2] -> UGridLevelAsset
```

`AGridLevelRuntimeActor` peut rester responsable de reconstruire un seul `UGridLevelAsset`. Un futur acteur ou subsystem de donjon pourra choisir quel `UGridLevelAsset` charger dans l'acteur runtime.

## Regles a conserver

- Le DataAsset est la source de verite.
- Les maps ne stockent pas la topologie du donjon.
- Le runtime lit les assets, il ne depend pas du module editor.
- L'editor modifie les assets, puis demande une reconstruction de preview.
- Les references placees dans les maps doivent etre verifiees explicitement avec les diagnostics.
- Le passage au multi-niveaux doit ajouter une couche `UGridDungeonAsset`, pas transformer les maps en base de donnees de niveaux.
