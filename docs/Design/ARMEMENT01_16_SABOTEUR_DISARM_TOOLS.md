# ARMEMENT01.16 — Outils de désamorçage du Voleur/Saboteur

Date : 30 octobre 2026
Statut au 31.10.2026 : **ARMEMENT01.16.1, 16.2A et 16.2B compilés et tests validés localement ; verrou de découverte explicite publié, TRAP01.1 en cours de validation.** L'action réelle n'est pas utilisable sans découverte, et la perception passive reste à implémenter.

## Source d'autorité et objectif

La recette approuvée dans `ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv`
prévoit pour `Kit_Rogue_Saboteur` l'arme ARM003 N1 (`DA_Weapon_Stiletto_01`)
et le complément `DisarmToolToAuthor`.

`ARMEMENT01_9_7_FINAL_KIT_APPROVAL.md` exige un outil **avec usage N1 testé**.
Ce document interdit d'attribuer un objet inutilisable sous prétexte de
compléter le kit. Les premiers talents du Saboteur sont acquis au niveau 2,
pas à la création.

## Audit de l'existant

- Les pièges temporaires de combat utilisent
  `FGridCombatTrapState` et `AGridLevelRuntimeActor::FindCombatTrapAtCell`,
  `ConsumeCombatTrapAtCell` dans `GridLevelRuntimeActorTraps.cpp`.
- **Ces pièges de combat ne constituent pas encore un système complet de
  pièges mécaniques de donjon** : pas d'identité d'objet de monde ni de
  difficulté de désamorçage que l'item de départ pourrait utiliser.
- `EGridItemActionType::UseOnTarget` existe dans l'interface mais ne possède
  pas de chemin d'exécution générique pour les outils et les pièges.
- `Skill_Traps` est défini dans le catalogue RPG, mais aucun pipeline
  de désamorçage d'un piège de donjon ne le relie actuellement à
  une cible, un test de compétence et un événement de niveau.
- Le document `GRIMROCK_LOCK_SYSTEM.md` est **prospectif** ; son
  contrat de liens `SourceObject+SourceEvent -> TargetObject+TargetCommand`
  doit être respecté par la future implémentation.

## ARMEMENT01.16.1 — Objet canonique non consommable

`UGridItemDefinitionAsset` :

| Champ | Valeur |
|---|---|
| Package | `/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_DisarmTools_01` |
| `ItemDefinitionId` | `Item_DisarmTools_01` |
| `ItemType` | `Misc` (outil, pas une potion) |
| `HandUsage` | `NotHandHeld` |
| Poids | 0,5 kg **provisoire** |
| Stack | Non, 1 exemplaire par unité |
| `ItemTags` | `Tool.Disarm` |
| QuickItem / action de combat | **Aucun** |
| Effets de talent/compétence | **Aucun bonus automatique** |
| Localization | `Item.StartingTool.Disarm01.Name/Description` dans ST_UI |
| Icône / mesh | à authorer et vérifier en PIE avant attribution au kit |

Il ne faut **pas** interpréter le tag `Tool.Disarm` comme un exécuteur :
il est seulement un identifiant pour la future logique dédiée.

### Production

Le mode isolé ajouté au script existant `Scripts/AuthorRPGRogue.ps1`
ne modifie ni les six classes, ni leurs kits, ni le catalogue d'armes :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

.\Scripts\AuthorRPGRogue.ps1 `
    -EngineRoot D:\UE_5.5 `
    -StartingDisarmTool `
    -Apply
```

Cette commande compile UE, synchronise `ST_UI` avec `Localization/Source/UI.csv`,
crée le seul package `DA_Item_DisarmTools_01.uasset` si absent,
refuse tout overwrite incompatible, puis exécute les tests ciblés.
L'asset peut ensuite être enrichi d'une icône et d'un mesh sans réécrire
le contrat gameplay. La commande est idempotente si le DataAsset concorde.

Tests :
`Grimrock.RPG.ARMEMENT01.16.DisarmTool.AuthoringContract` et
`Grimrock.RPG.ARMEMENT01.16.DisarmTool.NoPrematureKitGrant`.

**Le nouvel asset binaire et ST_UI.uasset sont des modifications locales,
non poussées automatiquement par le commit C++ distant.**

## Suite obligatoire avant attribution au kit

**ARMEMENT01.16.2 — Désamorçage N1 fonctionnel**, à implémenter et tester :

1. Une cible de piège de donjon explicitement désamorçable dans le modèle
   de niveau, avec ID stable, difficulté et état actif persisté en SaveGame.
2. Un caller d'interaction unique qui cible ce piège, vérifie la proximité,
   l'outil détenu, l'acteur sélectionné et la disponibilité du Skill_Traps.
3. Résolution du test via `FRPGSkillCheckService` (sans « +2 »
   du talent ExpertDisarm, indisponible au niveau 1).
4. Résultat de succès/échec conforme à la règle décidée et émission
   d'événements via les liens logiques du niveau. Ne pas retirer l'outil
   à moins qu'une règle de durabilité/consommation soit explicitement approuvée.
5. Tests de cible absente, hors de portée, échec du test, réussite,
   absence d'outil, persistance, UI et PIE.
6. **Seulement après ces validations :** ajout de `DA_Item_DisarmTools_01`
   au `Kit_Rogue_Saboteur`, puis vérification du gameplay N1 et de Save/Continue.

Ce découpage protège le principe acté dans ARMEMENT01.9.7 :
**aucun objet décoratif ou action fictive dans un kit approuvé.**

## Prompt Meshy (pour une étape visuelle ultérieure)

```text
A single medieval fantasy trap-disarming toolkit for a dungeon crawler game:
a compact folded leather roll containing small brass lock probes, slender steel
needles, miniature screwdrivers and precise mechanical picks, secured by a
simple leather strap. One isolated object on a neutral gray studio background,
three-quarter view, all parts clearly visible, practical plausible medieval
craftsmanship, subdued dark brown leather and worn steel, game-ready
stylized realism, clean silhouette, symmetry where appropriate, no hands,
no character, no scene, no lettering, no magic effects, no extra props.
```

Le rendu 3D Meshy et l'icône doivent être approuvés dans Unreal, puis
committés ensemble depuis Windows. Ne pas attribuer ce kit prématurément.

## ARMEMENT01.16.2A — Fondations C++ des pièges Trigger (30.10.2026)

Statut : code préparé pour validation UE locale. L'usage par l'interface joueur et l'attribution au kit restent hors périmètre.

### Modèle

- Définition UGridWorldObjectDefinitionAsset : bCanBeDisarmed=false par défaut et DisarmDifficulty (1..40). Opt-in uniquement sur les Trigger ; les Trigger historiques sont inchangés.
- Identité par InstanceId stable sur un FGridWorldObjectInstance de type Trigger. Aucun détournement des CombatTraps temporaires indexés par cellule.
- Le désamorçage réussi persiste le GUID dans FGridLevelRuntimeState::DisarmedTriggerTrapIds (SaveGame). Les événements Activated et Deactivated du Trigger sont alors neutralisés.
- Un événement EGridObjectEvent::Disarmed ajouté en fin d'énumération permet les liens SourceEvent -> TargetCommand et reste émis une seule fois après succès. L'éditeur de liens autorise Disarmed pour Trigger.

### Résolution d'une demande

AGridLevelRuntimeActor::TryDisarmTriggerTrap vérifie : cible existante et autorisée, acteur et niveau, case sous les pieds ou en face immédiatement accessible (CanMove), personnage vivant, possession réelle d'un Item_DisarmTools_01 marqué Tool.Disarm et compétence Skill_Traps. Le jet déterministe est résolu par FRPGSkillCheckService, sans bonus de talent acquis avant le niveau 2.

Un refus n'avance pas le générateur aléatoire et ne retire pas l'outil. Un jet échoué laisse le Trigger armé. Une réussite conserve l'outil, persiste l'état et émet Disarmed ; aucune seconde tentative n'est possible. Les Trigger non concernés restent inchangés.

### Validation locale

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.16.TrapCore"

Tests : DefinitionOptIn (opt-in/règles de type) et SkillAndPersistentState (absence d'outil, refus sans jet, échec, succès, outil conservé, état sauvegardé/repris via snapshot runtime).

Attention : la sérialisation SaveGame complète, la sélection et l'interaction souris, les messages UI et l'attribution du kit n'ont pas encore été testés. Ce sont les objectifs de ARMEMENT01.16.2B ; aucune validation PIE n'est revendiquée pour 2A.

### Publication indépendante des ressources ARMEMENT01.16.1

Le journal utilisateur du 30.10.2026 valide déjà 2/2 tests de 16.1 et la création locale de DA_Item_DisarmTools_01. Publier depuis Windows ce package et ST_UI.uasset, indépendamment du commit C++ de 16.2A. Ne pas attribuer l'outil à Kit_Rogue_Saboteur avant fonctionnement en PIE.


## ARMEMENT01.16.2B — Action d'inventaire « Désamorcer le piège »

Date : 30 octobre 2026. Statut : code préparé, compilation et PIE à valider.

### Parcours joueur

Dans le donjon, sur un personnage ayant physiquement les outils
`Item_DisarmTools_01`, clic droit sur l'objet dans l'inventaire :
le menu C++ offre « Désamorcer le piège » lorsque l'autorité grille
résout un et un seul Trigger désamorçable dans la case courante ou
la case directement devant le groupe, accessible sans mur/porte fermée.
Ces Trigger sont des objets de données et peuvent ne pas posséder
de mesh ou de collider ; il serait incorrect de se fier exclusivement
au line trace des objets visibles.

Les autres items conservent leurs actions existantes.
La sélection du groupe, le slot d'inventaire et le GUID de la cible
sont revérifiés à l'exécution. Le mode combat refuse cette action
d'exploration, sans coût en PA et sans QuickItem artificiel.

L'action charge la compétence de production
`/Game/GrimrockPrototype/Core/DataAssets/RPG/Skills/DA_Skill_Traps`
et passe par le résolveur autoritaire ARMEMENT01.16.2A.
**Important :** le catalogue officiel déclare
`Skill_Traps` gouvernée par **Intelligence**, sans jet non entraîné.
Le personnage doit donc investir au moins un rang de Pièges ;
l'outil ne remplace pas cette formation et ne confère pas ExpertDisarm.

Résultats dans le journal d'événements du donjon :
- succès : « Le piège est désamorcé. » et événement Disarmed ;
- jet échoué : « Le désamorçage a échoué. » ; le piège reste armé ;
- absence de formation / rejet : message localisé, sans consommation ;
- après réussite : l'action disparaît du menu du même outil pour ce piège.

### Contrôles automatiques

Filtre :
`Grimrock.RPG.ARMEMENT01.16.TrapInteraction`.

Le test ContextAndTargeting crée un runtime et un pion réels dans un
monde de test, une définition Trigger opt-in et un outil typé ; il
vérifie la découverte par GUID même sans acteur physique, l'action
du menu, le mur bloquant, l'ambiguïté de deux pièges dans une case
et la disparition de l'action après désamorçage.

Commande locale :

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.16.TrapInteraction"

Ce test ne prouve **pas** encore à lui seul que la création d'un
Trigger et de ses liens dans l'éditeur ainsi que Save/Continue
fonctionnent en PIE ; ces vérifications manuelles restent exigées.

### Plan de vérification PIE

1. Créer ou choisir une définition de type Trigger (Floor), cocher
   Can Be Disarmed et fixer Disarm Difficulty (p. ex. 10).
   Placer une instance dans une cellule accessible devant le groupe.
2. Ajouter un lien Disarmed vers une commande observable
   (p. ex. ouverture d'une porte), et un lien Activated distinct.
3. Donner les outils au personnage et lui accorder un rang réel de
   Pièges dans la création ou progression. Clic droit sur les outils
   dans le sac : Désamorcer le piège.
4. Vérifier en cas d'échec l'absence d'événement Disarmed et la
   conservation de l'outil ; en cas de succès le lien de porte,
   l'absence de réactivation du piège, le message et la disparition
   de l'action.
5. Sauvegarder et charger la partie : le Trigger doit rester désarmé.
   Vérifier aussi un second Trigger et l'absence de régression des
   mécanismes non désamorçables.
6. Réessayer sans rang de Pièges, sans outils, derrière un mur, en
   combat et avec une cible déjà désarmée.

**Le kit de départ Saboteur n'est pas modifié dans 2B.**
La décision de le remplir sera prise après validation PIE et création
ou approbation de son icône/mesh (référence ARMEMENT01.9.7).

## TRAP01.1 — Changement de contrat validé le 31.10.2026

Les pièges du donjon sont **cachés par définition**. La proximité d'un Trigger ne donne ni son identifiant exploitable ni une action de désamorçage au joueur. ARMEMENT01.16.2A/2B requiert désormais une découverte persistée dans `DetectedTriggerTrapIds`. Le nouveau `DungeonTrap.bIsDungeonTrap` doit être actif sur une définition de type `Trigger` pour que l'API de désamorçage accepte l'instance. Les tests historiques injectent explicitement la découverte pour isoler l'exécution de la compétence. La découverte automatique relève de `TRAP01.2`, non d'ARMEMENT01.16.2A/2B.

Le kit Saboteur ne doit pas être attribué automatiquement à ce stade. Le choix de basculer `Skill_Traps` d'Intelligence à Dextérité concerne `TRAP01.4`, après audit de progression et compatibilité RPG. Voir `docs/Design/TRAP01_1_HIDDEN_DUNGEON_TRAP_CONTRACT.md` pour les données, états SaveGame et limitations.
