# MON13.1 — Modèle persistant `MonsterSpawn`

## Objectif

MON13.1 complète les données nécessaires au futur pipeline :

```text
MonsterSpawn → MonsterDefinition → AGridMonsterActor
```

Ce jalon ne crée encore aucun Actor. Il définit et valide la source persistante
dans le `UGridLevelAsset`, tout en préservant l'autorité logique de la grille et
les identités MON8/MON9.

## Décision d'architecture

`MonsterSpawn` reste un `FGridLevelObjectData` dans
`UGridLevelAsset::Objects`. Aucun tableau parallèle et aucun second GUID ne sont
ajoutés.

| Concept MON13 | Donnée persistante |
|---|---|
| `SpawnId` | `FGridLevelObjectData::ObjectId` |
| Définition | `MonsterDefinitionAsset` et `MonsterDefinitionId` |
| Cellule | `CellX`, `CellY` |
| Orientation | `InitialFacing` |
| Rencontre | `EncounterGroupId` |
| État initial | `bInitiallyEnabled` |

`ObjectId` reste donc directement compatible avec `SpawnObjectId` et
`ResolvePersistenceId()` de MON9.

## Définition du monstre

`UGridMonsterDefinitionAsset` porte désormais `MonsterActorClass`. Sa valeur par
défaut est `AGridMonsterActor`, ce qui conserve les définitions existantes et
permet à une définition spécialisée de choisir ultérieurement une sous-classe
Blueprint.

La validation d'une définition refuse une classe d'Actor absente. La classe ne
porte pas l'identité de l'instance : l'identité persistante reste exclusivement
le `SpawnId` du niveau.

## Orientation et compatibilité

`InitialFacing` accepte uniquement `North`, `East`, `South` ou `West` et devient
la source de vérité gameplay.

`LocalYaw` demeure un miroir de compatibilité pour l'aperçu générique existant.
Au chargement des anciens assets, une orientation manquante est déduite du yaw
historique, puis le yaw est normalisé sur l'orientation cardinale. Les nouveaux
placements commencent au nord et l'inspecteur met les deux valeurs en cohérence.

## Palette et inspecteur

Une `FGridObjectPaletteEntry` de type `MonsterSpawn` doit renseigner
`DefaultMonsterDefinition`. Lors du placement, l'éditeur copie :

- le DataAsset de définition ;
- son `MonsterId` dans `MonsterDefinitionId` ;
- l'orientation initiale nord ;
- l'état initial de l'archetype.

L'inspecteur Slate expose :

- `SpawnId / ObjectId` en lecture seule ;
- `MonsterDefinitionAsset` ;
- `MonsterDefinitionId` et sa synchronisation ;
- `EncounterGroupId` ;
- `InitialFacing` via les quatre boutons d'orientation ;
- l'état initial ;
- la classe d'Actor résolue par la définition.

Aucun WBP ni asset binaire n'est modifié par MON13.1. Pour chaque entrée de
palette existante représentant un monstre, il faut assigner manuellement
`DefaultMonsterDefinition`, par exemple `DA_MON_RatGiant`.

## Validation

`UGridLevelAsset::ValidateMonsterSpawns()` vérifie :

- un `ObjectId/SpawnId` valide et unique parmi tous les objets du niveau ;
- une cellule dans les limites, présente, non vide et autorisant l'occupation ;
- `Edge=None`, car le spawn est centré sur une cellule ;
- une orientation cardinale ;
- une définition ou un identifiant résolvable ;
- la validité complète du DataAsset lorsqu'il est directement référencé ;
- l'égalité de `MonsterDefinitionId` et `MonsterDefinitionAsset::MonsterId` ;
- l'absence de deux spawns initialement activés sur la même cellule.

Deux spawns désactivés peuvent partager une cellule dans les données. Le futur
spawn runtime devra néanmoins refuser atomiquement toute activation dont la
destination est occupée.

Le bouton `Refresh Validation` du panneau `VALIDATION` appelle
`ValidateCurrentLevel()` et applique les mêmes règles. Chaque message concernant
un spawn est rattaché au `SpawnId` afin de rendre disponibles les actions
`Select Object` et `Focus Object`.

## Tests Automation

Trois tests sont ajoutés :

```text
Grimrock.Monsters.MON13.1.PersistentModel
Grimrock.Monsters.MON13.1.Validation
Grimrock.Monsters.MON13.1.PaletteContract
```

Ils couvrent la génération du `SpawnId`, la migration du yaw, la synchronisation
de l'identifiant, la classe d'Actor par défaut, les rejets de placement et le
contrat de palette.

Commande UE 5.5.4 :

```bat
D:\UE_5.5\Engine\Binaries\Win64\UnrealEditor-Cmd.exe D:\Development\GrimrockPrototype\GrimrockPrototype.uproject -unattended -nop4 -nosplash -NullRHI -ExecCmds="Automation RunTests Grimrock.Monsters.MON13.1" -TestExit="Automation Test Queue Empty" -ReportOutputPath="D:\Development\GrimrockPrototype\Saved\TestReports\MON13_1"
```

## Checklist éditeur détaillée

### Périmètre et critère de réussite

Cette checklist valide isolément le modèle persistant et les outils d'édition de
MON13.1. Dans la version actuelle, MON13.2 crée désormais l'aperçu squelettique
et l'Actor runtime ; leur validation complète appartient à la checklist
`MON13_2_MONSTER_SPAWN_PIPELINE.md`.

MON13.1 est validé dans l'éditeur si :

- la palette copie la bonne définition dans chaque nouveau spawn ;
- chaque placement reçoit un `SpawnId` valide, unique et stable ;
- la définition, son identifiant, l'orientation, le groupe de rencontre et
  l'état initial survivent à une sauvegarde puis à un rechargement ;
- les quatre orientations cardinales restent synchronisées avec l'aperçu ;
- l'interface empêche un déplacement vers une cellule déjà occupée par un autre
  `MonsterSpawn` ;
- `Refresh Validation` accepte l'état valide et signale les états invalides
  attendus sans modifier silencieusement les données.

Les tests négatifs modifient volontairement des données invalides. Il est
fortement recommandé de les effectuer sur une copie temporaire du
`UGridLevelAsset`, jamais sur le niveau principal sans sauvegarde préalable.

### 0 — Préparation du niveau de test

- [ ] compiler `GrimrockPrototypeEditor` en `Development Editor Win64` ;
- [ ] démarrer Unreal Editor 5.5.4 sans message de chargement de module ;
- [ ] ouvrir une carte contenant un `BP_GridLevelEditorActor` correctement
  configuré ;
- [ ] activer le mode `Grimrock Grid Editor Mode` ;
- [ ] vérifier dans `DUNGEON LEVELS` que le bon `Current LevelAsset` est chargé ;
- [ ] vérifier que le `BP_GridLevelEditorActor` référence la bonne
  `ObjectPalette` ;
- [ ] utiliser de préférence un niveau dédié ou une copie du niveau de test pour
  les scénarios négatifs ;
- [ ] préparer trois cellules distinctes `A`, `B` et `C`, non vides et avec
  `bBlocksOccupancy=false` ;
- [ ] noter leurs coordonnées avant de commencer.

| Cellule | Coordonnées | Usage |
|---|---|---|
| `A` | `(____, ____)` | premier rat |
| `B` | `(____, ____)` | second rat |
| `C` | `(____, ____)` | déplacement et restauration |

### 1 — Vérification de la définition et de la palette

1. Ouvrir `DA_MON_RatGiant` dans le Content Browser.
2. Vérifier les propriétés suivantes :

   - [ ] `MonsterId` n'est pas `None` ;
   - [ ] `MonsterActorClass` contient `AGridMonsterActor` ou une sous-classe
     prévue ;
   - [ ] la définition ne contient aucune erreur de validation existante.

3. Ouvrir le DataAsset `ObjectPalette` référencé par le
   `BP_GridLevelEditorActor`.
4. Retrouver l'entrée correspondant au Rat géant.
5. Vérifier puis sauvegarder :

   - [ ] `DefaultArchetype` est renseigné ;
   - [ ] le `SupportedType` de cet archetype est `MonsterSpawn` ;
   - [ ] `DefaultMonsterDefinition` vaut `DA_MON_RatGiant`.

6. Revenir dans le `Grimrock Grid Editor Mode`, ouvrir `VALIDATION`, puis
   cliquer `Refresh Validation`.

Résultat attendu : aucune erreur
`ObjectPalette: Palette entry '...' requires DefaultMonsterDefinition for MonsterSpawn.`
ne doit être présente.

### 2 — Placement et identité persistante

1. Dans `TOOLS / PALETTE`, sélectionner l'entrée Rat géant.
2. Placer un premier rat sur la cellule `A`.
3. Placer un second rat sur la cellule `B`.
4. Sélectionner chaque rat et relever les valeurs affichées dans
   `SELECTED OBJECT`.

| Champ | Rat `A` | Rat `B` | Résultat attendu |
|---|---|---|---|
| Cellule | `(____, ____)` | `(____, ____)` | cellules `A` et `B` |
| `SpawnId / ObjectId` | `________________` | `________________` | GUID valides et différents |
| `MonsterDefinitionAsset` |  |  | `DA_MON_RatGiant` |
| `MonsterDefinitionId` |  |  | égal au `MonsterId` du DataAsset |
| `InitialFacing` |  |  | `North` au placement |
| `Initial State` |  |  | `Enabled` si `Enabled at Start` était coché |
| `Monster Actor Class` |  |  | classe définie par `DA_MON_RatGiant` |

- [ ] aucun `SpawnId` n'est vide ou composé uniquement de zéros ;
- [ ] les deux `SpawnId` sont différents ;
- [ ] le second placement n'a ni supprimé ni modifié le premier ;
- [ ] le texte d'information confirme que MON13.2 crée désormais l'Actor et que
  les commandes dynamiques commencent en MON13.3.

### 3 — Orientation cardinale et aperçu

1. Sélectionner le rat `A`.
2. Dans `SELECTED OBJECT`, utiliser successivement les quatre boutons de la ligne
   `Orientation`.

| Bouton | `InitialFacing` attendu | Rotation d'aperçu attendue |
|---|---|---|
| `North` | `North` | nord |
| `East` | `East` | quart de tour vers l'est |
| `South` | `South` | demi-tour |
| `West` | `West` | quart de tour vers l'ouest |

Pour chaque orientation :

- [ ] le bouton actif est visuellement sélectionné ;
- [ ] la ligne `InitialFacing` affiche la même direction ;
- [ ] l'aperçu pivote sans changer de cellule ni de `SpawnId` ;
- [ ] sélectionner un autre objet puis revenir sur le rat conserve la direction.

Après le test, choisir l'orientation finale souhaitée et cliquer
`Refresh Validation`.

Résultat attendu : aucun avertissement
`MonsterSpawn LocalYaw preview mirror differs from InitialFacing` ne doit être
présent. Si le mesh est trop symétrique pour voir sa rotation, le bouton actif et
la valeur `InitialFacing` restent les contrôles déterminants.

### 4 — Groupe de rencontre et état initial

1. Sélectionner le rat `A`.
2. Saisir `Encounter_MON13_Rats_A` dans `EncounterGroupId`, puis presser
   `Entrée` pour valider la saisie.
3. Répéter exactement la même valeur sur le rat `B`.
4. Décocher temporairement `Enabled at Start` sur le rat `B`.

- [ ] `Initial State` passe à `Disabled` pour le rat `B` ;
- [ ] le rat `A` reste `Enabled` ;
- [ ] les deux objets conservent leur `SpawnId` ;
- [ ] les deux objets affichent le même `EncounterGroupId`.

Recocher ensuite `Enabled at Start` sur le rat `B` afin de rétablir l'état de
référence avec deux rats activés sur deux cellules différentes.

`Active at Start` n'appartient pas au contrat spécifique MON13.1 et n'a pas
besoin d'être modifié pour cette validation.

### 5 — Sauvegarde et rechargement

1. Noter les deux `SpawnId`, les deux cellules, les orientations et le
   `EncounterGroupId`.
2. Sauvegarder le `UGridLevelAsset`, l'`ObjectPalette` si elle a été modifiée, et
   la carte.
3. Utiliser `Reload Current` dans `DUNGEON LEVELS`, ou fermer puis rouvrir la
   carte après sauvegarde.
4. Resélectionner les deux rats.

- [ ] les deux objets existent encore sur leurs cellules respectives ;
- [ ] chaque `SpawnId` est strictement identique à la valeur relevée avant le
  rechargement ;
- [ ] `MonsterDefinitionAsset` et `MonsterDefinitionId` sont inchangés ;
- [ ] `InitialFacing` et l'aperçu sont inchangés ;
- [ ] `EncounterGroupId` est inchangé ;
- [ ] `Initial State` est inchangé.

Toute régénération d'un GUID au rechargement constitue un échec bloquant de
MON13.1.

### 6 — Validation positive du niveau

Avant de lancer ce scénario, vérifier que les rats `A` et `B` sont activés, sur
deux cellules différentes et praticables, et que leur définition est correcte.

1. Déplier le panneau `VALIDATION`.
2. Cocher les filtres `Errors` et `Warnings`.
3. Cliquer `Refresh Validation`.
4. Utiliser `Copy Summary` pour conserver le résultat si nécessaire.

Résultat attendu : aucun message concernant les deux `MonsterSpawn`, notamment
aucun des messages suivants :

```text
MonsterSpawn requires MonsterDefinitionAsset or MonsterDefinitionId.
MonsterSpawn stores MonsterDefinitionId '...' but its asset resolves to '...'.
MonsterSpawn requires a cardinal InitialFacing.
MonsterSpawn is cell-centered and requires Edge=None.
MonsterSpawn must be placed on a non-empty cell that allows occupancy.
MonsterSpawn shares its initial cell with enabled MonsterSpawn ...
```

Le niveau peut contenir des avertissements historiques sans rapport avec
MON13.1. Ils doivent être notés séparément et ne doivent pas être interprétés
comme un échec du modèle `MonsterSpawn`.

### 7 — Définition absente puis restauration

Ce test peut être effectué depuis l'inspecteur normal.

1. Sélectionner le rat `A`.
2. Effacer `MonsterDefinitionAsset`.
3. Effacer également `MonsterDefinitionId`, puis presser `Entrée`.

Résultats attendus :

- [ ] l'inspecteur affiche
  `Error: MonsterSpawn requires a monster definition.` ;
- [ ] `Refresh Validation` ajoute l'erreur
  `MonsterSpawn requires MonsterDefinitionAsset or MonsterDefinitionId.` ;
- [ ] le message de validation porte l'identifiant du rat `A` ;
- [ ] `Select Object` et `Focus Object` ramènent bien sur ce rat ;
- [ ] aucun autre objet n'est supprimé ou modifié.

Restaurer `DA_MON_RatGiant` dans `MonsterDefinitionAsset`. L'identifiant doit se
resynchroniser automatiquement. Relancer `Refresh Validation` et vérifier la
disparition de l'erreur.

### 8 — Identifiant incohérent puis synchronisation

1. Conserver `DA_MON_RatGiant` sur le rat `A`.
2. Remplacer temporairement `MonsterDefinitionId` par
   `MON13_InvalidDefinitionId`, puis presser `Entrée`.

Résultats attendus :

- [ ] l'inspecteur affiche
  `Error: MonsterDefinitionId differs from the selected asset id.` ;
- [ ] `Refresh Validation` signale l'identifiant stocké et l'identifiant résolu
  par le DataAsset ;
- [ ] la définition et le `SpawnId` ne sont pas modifiés.

Cliquer `Sync Id From Asset`, puis vérifier :

- [ ] `MonsterDefinitionId` redevient égal à `DA_MON_RatGiant.MonsterId` ;
- [ ] l'erreur de l'inspecteur disparaît ;
- [ ] l'erreur de validation disparaît après `Refresh Validation`.

### 9 — Garde-fou de déplacement vers une cellule occupée

Ce scénario vérifie le refus de l'outil d'édition, pas le validateur de données.

1. Sélectionner le rat `B`.
2. Définir la cellule `A` comme cellule courante sans sélectionner le rat `A`.
3. Dans `SELECTED OBJECT`, cliquer `Move To Current Cell`.

Résultats attendus :

- [ ] le déplacement est refusé ;
- [ ] le rat `A` reste sur `A` ;
- [ ] le rat `B` reste sur `B` ;
- [ ] les deux `SpawnId` restent inchangés ;
- [ ] l'Output Log contient
  `cannot move selected object, destination already contains an object of the same type.`

Déplacer ensuite le rat `B` vers la cellule libre `C` avec le même bouton.

- [ ] le déplacement vers `C` réussit ;
- [ ] le `SpawnId` du rat `B` ne change pas ;
- [ ] `Refresh Validation` ne produit aucune erreur MON13.1.

Rétablir enfin le rat `B` sur sa cellule `B` si cette disposition doit être
conservée.

### 10 — Tests négatifs exhaustifs du validateur

Ces tests sont facultatifs pour le contrôle visuel minimal, mais recommandés
pour valider toutes les règles MON13.1. Ils doivent être réalisés uniquement sur
une copie temporaire du `UGridLevelAsset` : ouvrir le DataAsset, développer
`Objects`, repérer les deux entrées `Type=MonsterSpawn`, modifier une seule
donnée à la fois, lancer `Refresh Validation`, puis annuler ou restaurer la
valeur avant le test suivant.

| Mutation temporaire | Résultat attendu |
|---|---|
| copier l'`ObjectId` du rat `A` sur le rat `B` | erreur de `SpawnId` non unique |
| mettre `CellX` ou `CellY` hors des dimensions de la grille | erreur `outside grid bounds` |
| placer le spawn sur une cellule `Empty` | erreur de cellule non praticable |
| mettre `bBlocksOccupancy=true` sur sa cellule | erreur de cellule n'autorisant pas l'occupation |
| mettre `Edge=North` | erreur `requires Edge=None` |
| mettre `InitialFacing=None` | erreur `requires a cardinal InitialFacing` |
| conserver le DataAsset et saisir un autre `MonsterDefinitionId` | erreur d'identifiant incohérent |
| vider le DataAsset et l'identifiant | erreur de définition absente |
| donner la même cellule aux deux rats avec `bInitiallyEnabled=true` | erreur de cellule initiale partagée |
| conserver la même cellule mais désactiver les deux rats | aucune erreur de partage de cellule |

Pour chaque erreur liée à un objet :

- [ ] le message contient ou référence le `SpawnId` concerné ;
- [ ] `Select Object` sélectionne l'objet ;
- [ ] `Focus Object` centre la vue sur l'objet ;
- [ ] la correction de la donnée fait disparaître uniquement l'erreur attendue.

Ne pas sauvegarder les mutations invalides. Si une modification brute rend la
sélection ambiguë, fermer le DataAsset sans sauvegarder ou restaurer sa copie de
test avant de continuer.

### 11 — État final à conserver

- [ ] les deux rats ont des `SpawnId` distincts et stables ;
- [ ] les deux rats référencent `DA_MON_RatGiant` ;
- [ ] les deux `MonsterDefinitionId` correspondent au `MonsterId` du DataAsset ;
- [ ] les orientations sont cardinales ;
- [ ] les cellules sont valides, praticables et distinctes si les deux rats sont
  activés ;
- [ ] `EncounterGroupId` contient la valeur voulue ;
- [ ] `Refresh Validation` ne produit aucune erreur MON13.1 ;
- [ ] tous les assets valides ont été sauvegardés ;
- [ ] aucune mutation négative temporaire n'a été conservée.

### Feuille de résultat

| Scénario | Résultat attendu | Résultat observé | Statut |
|---|---|---|---|
| 1 — Définition et palette | définition copiée, palette valide |  | [ ] OK / [ ] KO |
| 2 — Placement et identité | deux GUID valides et distincts |  | [ ] OK / [ ] KO |
| 3 — Orientations | quatre directions cohérentes |  | [ ] OK / [ ] KO |
| 4 — Groupe et état | valeurs éditables et indépendantes |  | [ ] OK / [ ] KO |
| 5 — Rechargement | aucune donnée persistante modifiée |  | [ ] OK / [ ] KO |
| 6 — Validation positive | aucune erreur MON13.1 |  | [ ] OK / [ ] KO |
| 7 — Définition absente | erreur détectée puis supprimée |  | [ ] OK / [ ] KO |
| 8 — ID incohérent | erreur puis `Sync Id From Asset` |  | [ ] OK / [ ] KO |
| 9 — Cellule occupée | déplacement refusé sans perte |  | [ ] OK / [ ] KO |
| 10 — Validations brutes | erreurs ciblées conformes |  | [ ] OK / [ ] KO / [ ] N/A |

En cas d'échec, relever au minimum le scénario, le `SpawnId`, la cellule, le
texte exact de validation, une capture de `SELECTED OBJECT` et les lignes utiles
de l'Output Log.

## Suite du pipeline

MON13.2 implémente la résolution stricte
`MonsterSpawn → MonsterDefinition → MonsterActorClass`, l'aperçu squelettique,
la création différée de l'Actor gameplay et sa reconstruction avant restauration
MON9. Voir `docs/Design/MON13_2_MONSTER_SPAWN_PIPELINE.md`.

MON13.3 ajoute les commandes runtime `Spawn`, `Despawn` et `Teleport`,
l'activation différée, les événements de cycle de vie et la présence
persistante. Voir `docs/Design/MON13_3_MONSTER_RUNTIME_COMMANDS.md`.

Restent hors périmètre :

- résolution Asset Manager d'un `MonsterDefinitionId` seul ;
- téléportation inter-niveaux ;
- suppression définitive non réversible d'un placement ;
- gestion complète des rencontres.
