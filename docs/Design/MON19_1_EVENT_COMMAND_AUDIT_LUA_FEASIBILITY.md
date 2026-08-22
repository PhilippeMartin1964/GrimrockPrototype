# MON19.1 — Audit Event/Command et faisabilité de Lua

Statut : **audit terminé — documentation uniquement**  
Date : **22 août 2026**  
Référence `master` avant MON19.1 : `208c5316a2375c276753604ea7faf7a0fc3ecf11` (`Close MON17.8 monster presentation and persistence`)

## 1. Périmètre et méthode

MON19.1 audite l’architecture Event → Command existante avant l’introduction de tout nouveau système de script.

L’audit a couvert :

- `AGENTS.md` et les règles de travail du dépôt ;
- `docs/Design/PROJECT_COMPLETION_ROADMAP.md` ;
- `docs/Design/03_EVENT_COMMAND_LINKS.md` ;
- `FGridObjectLink`, `EGridObjectEvent`, `EGridObjectCommand` et `EGridObjectCondition` ;
- `UGridActivationComponent` et `AGridLevelRuntimeActor` ;
- les spécialisations portes, réceptacles et points d’apparition de monstres ;
- la politique des connecteurs dans l’éditeur, le panneau Slate des connecteurs, la validation portée par l’acteur d’édition et la visualisation dans le viewport ;
- la persistance de l’état d’exécution des niveaux et le versionnement des sauvegardes ;
- les tests automatisés et manuels disponibles concernant les liens et les rencontres MON13 ;
- la documentation historique et les TODO encore présents ;
- Lua officiel, sol2 et UnLua, notamment sous l’angle des licences, des exceptions C++, du packaging et de la sécurité d’exécution.

MON19.1 ne modifie **aucun code C++ de production**, aucun `.uasset` et aucune `.umap`.

Le HEAD `master` visible dans le dépôt GitHub a été vérifié avant la rédaction de cet audit et correspondait encore au commit de référence indiqué ci-dessus. L’environnement utilisé pour cet audit peut consulter et mettre à jour le dépôt GitHub, mais il ne peut pas exécuter `git status` dans le répertoire local `D:\Development\GrimrockPrototype` de la machine de développement ; aucune affirmation n’est donc faite ici concernant d’éventuels fichiers locaux non validés.

---

# A. Architecture Event → Command actuelle

## A.1 Données persistantes du niveau

Les données faisant autorité pour les connecteurs sont stockées dans :

```text
UGridLevelAsset
    Objects : TArray<FGridLevelObjectData>
    Links   : TArray<FGridObjectLink>
```

Un lien n’est plus simplement le quadruplet historique :

```text
SourceObjectId + SourceEvent -> TargetObjectId + Command
```

`FGridObjectLink` contient actuellement :

```text
SourceObjectId
TargetObjectId
SourceEvent
Command
Condition
ConditionItemDefinitionId
ConditionItemTag
ConditionItemType
ConditionCount
ConditionWeight
bInvertCondition
```

Le modèle réel est donc déjà le suivant :

```text
Objet source
    émet SourceEvent
        -> sélection des FGridObjectLink correspondants
        -> évaluation de la condition éventuelle
        -> application de Command à l’objet cible
```

Les conditions existantes sont spécialisées sur l’état d’un réceptacle. Elles permettent :

- l’absence de condition ;
- de tester si un réceptacle est vide ;
- de tester s’il contient au moins un objet ;
- de rechercher une définition d’objet précise ;
- de rechercher un tag d’objet ;
- de rechercher un type d’objet ;
- de tester un nombre minimal d’objets ;
- de tester un poids total minimal ;
- d’inverser le résultat d’une condition valide.

C’est une conclusion importante de MON19.1 : **le projet possède déjà une petite couche de logique conditionnelle**. MON19 doit l’étendre au lieu d’en recréer une autre en parallèle.

## A.2 Dispatcher d’exécution

`UGridActivationComponent` est le coordinateur central de l’exécution Event → Command.

Ses responsabilités sont déjà très proches de l’architecture souhaitable à long terme :

1. indexer les objets du niveau par `ObjectId` ;
2. indexer les liens par `SourceObjectId` ;
3. recevoir les événements produits par les objets ;
4. conserver uniquement les liens dont `SourceEvent` correspond à l’événement reçu ;
5. résoudre l’objet cible et, le cas échéant, son acteur d’exécution ;
6. évaluer la condition éventuelle ;
7. dispatcher la commande selon le type de cible ;
8. mettre à jour l’ensemble central des objets actifs lorsque cela s’applique ;
9. produire les événements consécutifs pour les objets à état qui les prennent en charge ;
10. journaliser les chemins rejetés, absents ou non pris en charge.

Le composant n’a pas de Tick permanent (`PrimaryComponentTick.bCanEverTick = false`). C’est une bonne base pour MON19 : l’exécution de scripts ne justifie pas l’ajout d’un Tick permanent.

## A.3 Chemins d’émission des événements

Les familles d’objets normales émettent actuellement :

```text
Button
    Activated

Lever
    Activated
    Deactivated

PressurePlate
    Activated
    Deactivated

Trigger
    Activated      (le groupe entre)
    Deactivated    (le groupe sort)

Receptacle
    ItemInserted
    ItemRemoved
    ItemChanged

Cycle MonsterSpawn / Encounter
    MonsterDied
    MonsterSpawned
    MonsterDespawned
    MonsterTeleported
    EncounterWaveStarted
    EncounterCompleted
```

À noter : l’entrée et la sortie d’un trigger sont traduites en `Activated` et `Deactivated`. Les valeurs d’enum `Entered` et `Exited` ne constituent donc pas le contrat actif actuel.

`MonsterDied` est émis par `UGridMonsterDeathComponent`, qui transmet l’identifiant stable du point d’apparition du monstre à `AGridLevelRuntimeActor::ExecuteLinksFromRuntimeObject()`.

Les événements du cycle d’apparition et de rencontre de MON13 reviennent eux aussi dans le même chemin Event → Command. Il n’existe pas de bus d’événements ou de système de script séparé pour MON13.

## A.4 Chemins de dispatch des commandes

### Portes

Les commandes de porte réellement opérationnelles sont :

```text
Toggle
Open / Activate
Close / Deactivate
```

Elles utilisent le chemin spécialisé des portes et produisent de véritables effets de gameplay.

### Réceptacles

Les commandes spécialisées réellement opérationnelles sont :

```text
ReceptacleConsumeItem
ReceptacleConsumeAllItems
ReceptacleEnableRemoval
ReceptacleDisableRemoval
```

Les commandes génériques d’état peuvent également atteindre un réceptacle par le chemin d’état central.

### MonsterSpawn

MON13 étend le dispatcher central avec :

```text
Spawn
Despawn
Teleport
Activate   -> alias de Spawn
Enable     -> alias de Spawn
Deactivate -> alias de Despawn
Disable    -> alias de Despawn
Toggle     -> Spawn/Despawn selon la présence actuelle
StartEncounter
```

Il s’agit de comportements d’exécution réels et non de simples valeurs d’enum historiques.

### Chemin générique d’état

Pour plusieurs autres types d’objets, le dispatcher accepte :

```text
Open / Activate    -> active=true
Close / Deactivate -> active=false
Toggle             -> inversion de l’état actif
```

Cependant, **enregistrer avec succès un indicateur d’état actif ne signifie pas que le comportement de gameplay correspondant est réellement implémenté**. Cette distinction est centrale pour MON19.

`ItemSpawn` journalise actuellement que l’état est enregistré, mais que le comportement d’apparition reste TODO. `Teleporter` journalise de la même manière que l’état est enregistré alors que le comportement de téléportation reste TODO. Des mécanismes génériques sans gestionnaire spécialisé peuvent donc eux aussi retourner un succès limité à la seule comptabilité d’état.

## A.5 Chaînage à l’exécution et protection contre les cycles

Lorsqu’une commande de lien change l’état d’un `Lever` ou d’une `PressurePlate`, le nouvel état peut émettre `Activated` ou `Deactivated`. Cela permet déjà de chaîner des connecteurs.

`DispatchingSourceObjectIds` empêche la réentrée d’une source déjà en cours de dispatch. Une chaîne cyclique est donc interrompue lorsqu’elle revient vers une source dont le traitement n’est pas encore terminé.

Ce mécanisme n’est pas une analyse complète de graphe :

- l’éditeur ne calcule pas à l’avance tous les cycles possibles entre connecteurs ;
- une chaîne extrêmement longue de sources toutes différentes n’est pas limitée par un budget d’instructions ;
- une future récursion Lua → Command → Event → Lua devra disposer d’une limite commune de profondeur et/ou d’un budget d’exécution en plus de la protection actuelle contre la réentrée d’une source.

Le runtime actuel n’est néanmoins **pas dépourvu de protection contre les cycles indirects**.

## A.6 Architecture de l’éditeur CONNECTORS

L’éditeur de connecteurs est correctement séparé en plusieurs responsabilités :

```text
GridEditorLinkPolicy
    -> déclare les événements/commandes autorisés selon le type d’objet

SGridEditorLinksPanel
    -> formulaire source/événement/cible/commande et listes de liens

GridLevelEditorActor
    -> création/suppression/validation et mutation persistante du LevelAsset

GridLevelEdMode / GridLevelEdModeToolkit
    -> visualisation des connecteurs et composition de l’outil d’édition
```

`GridEditorLinkPolicy` est particulièrement important. Il constitue déjà la table centrale des capacités de l’éditeur et doit rester l’autorité utilisée pour filtrer l’interface de MON19.

Le panneau Slate expose actuellement uniquement :

```text
Source Object
Source Event
Target Object
Command
```

Il **n’expose pas les champs de condition déjà présents dans `FGridObjectLink`**.

Le mode viewport dessine les connecteurs entrants et sortants ainsi que leurs libellés à partir du même tableau `UGridLevelAsset::Links`. Il ne duplique pas l’exécution du runtime.

## A.7 Incohérence de l’identité des liens lors de leur création/suppression

Le modèle persistant permet aux champs de condition de distinguer plusieurs liens lors de la validation, mais le test de doublon de `CreateLink()` utilise uniquement :

```text
SourceObjectId
TargetObjectId
SourceEvent
Command
```

`RemoveExactLink()` supprime lui aussi selon cette même identité à quatre champs.

Conséquences :

1. un lien conditionnel modifié manuellement peut empêcher la création d’un second lien ayant le même quadruplet mais une condition différente ;
2. la suppression d’un tel connecteur peut supprimer toutes les variantes partageant le même quadruplet ;
3. la validation tient compte de la condition pour identifier un doublon exact, tandis que la création et la suppression n’en tiennent pas compte.

Il s’agit d’une lacune réelle du contrat données/éditeur avant MON19. Cette incohérence doit être corrigée avant d’ajouter une logique plus complexe.

---

# B. Matrice des capacités actuelles

Légende :

- **Oui** : comportement de gameplay réellement implémenté.
- **État uniquement** : le dispatcher peut mémoriser l’état actif, mais aucun effet spécialisé complet n’existe.
- **Non** : ne fait pas partie du contrat de connecteur actuellement pris en charge.
- **Partiel** : une partie de l’état est persistée, mais pas l’ensemble de l’état pertinent pour les commandes.
- **Statique/manuel** : une couverture existe dans la documentation ou via des tests PIE manuels, mais aucun test automatisé dédié au comportement audité n’a été trouvé.

| Type/famille d’objet | Événements réellement émis | Commandes officiellement exposées dans l’éditeur | Implémentation à l’exécution | Exposé dans l’éditeur | Tests trouvés pendant l’audit | Persistance pertinente pour les liens | Échec d’une commande non prise en charge |
|---|---|---|---|---|---|---|---|
| Porte, y compris les archétypes de porte secrète | aucun (`Opened`/`Closed` non émis) | `Open`, `Close`, `Toggle`, `Activate`, `Deactivate` | **Oui** | cible oui, source non | fondation des portes + validation manuelle existante ; aucun nouveau test MON19 exécuté | **Oui** : ouverture et blocage | rejet central propre pour les commandes incompatibles |
| Button | `Activated` | aucune comme cible | source **Oui** ; un chemin générique de cible n’existe que si le lien est créé hors politique | source oui | aucun test automatisé générique de liens trouvé | **Oui** dans l’état interactif | l’éditeur empêche les liens de cible non pris en charge |
| Lever | `Activated`, `Deactivated` | aucune comme cible | source **Oui** ; un changement d’état commandé peut réémettre un événement si le lien est créé manuellement | source oui | aucun test automatisé générique de liens trouvé | **Oui** | l’éditeur empêche les liens de cible non pris en charge |
| PressurePlate | `Activated`, `Deactivated` | aucune comme cible | **Oui**, y compris l’émission lors d’un changement d’état | source oui | aucun test automatisé générique de liens trouvé | **Oui**, puis le rafraîchissement peut recalculer l’état | l’éditeur empêche les liens de cible non pris en charge |
| Trigger | `Activated`, `Deactivated` | aucune comme cible | **Oui** pour la traduction entrée/sortie | source oui | la politique MON13.3 vérifie le Trigger comme source ; aucune suite dédiée aux liens de trigger trouvée | **Oui** seulement si l’état central est utilisé | l’éditeur empêche les liens de cible non pris en charge |
| Réceptacle / famille de serrure murale | `ItemInserted`, `ItemRemoved`, `ItemChanged` | consommer un/tous les objets, autoriser/interdire le retrait | **Oui** | source et cible oui | protocole PIE manuel détaillé et validé ; aucun test automatisé utilisant explicitement `EGridObjectCondition` trouvé | **Partiel** : contenu oui, état actif oui ; `bCanRemoveItem` n’est pas sauvegardé | rejet spécialisé propre |
| MonsterSpawn / Encounter | `MonsterDied`, `MonsterSpawned`, `MonsterDespawned`, `MonsterTeleported`, `EncounterWaveStarted`, `EncounterCompleted` | Spawn/Despawn/Teleport + alias + `StartEncounter` | **Oui** | source et cible oui | **Forte couverture automatisée** : MON13.3/13.4, runtime, politique, persistance et échecs atomiques | **Oui** : état du monstre, placement, présence et rencontres | chemins propres/atomiques couverts par MON13 |
| Light | aucun | `Activate`, `Deactivate`, `Toggle` | **État uniquement**, sauf gestionnaire spécialisé porté par un acteur d’exécution | cible oui | aucun test dédié trouvé | **Non** pour l’indicateur central : Light n’est pas capturé dans `InteractiveObjects` | une commande peut retourner succès alors que seule la comptabilité d’état a eu lieu |
| Teleporter | aucun | `Activate`, `Deactivate`, `Toggle` | **État uniquement ; téléportation TODO** | cible oui | aucun test dédié trouvé | **Non** pour l’indicateur central | une commande peut retourner succès alors que la téléportation de gameplay n’existe pas encore |
| ItemSpawn | aucun | aucune | **État uniquement ; apparition TODO** si créé manuellement | non | aucun test dédié trouvé | **Non** pour l’indicateur central | l’éditeur l’exclut ; un lien créé manuellement peut néanmoins retourner succès pour l’état |
| Decoration / objet lisible | aucun (`Used` non émis) | aucune | l’interaction de lecture existe hors du système de connecteurs ; pas de contrat officiel de cible | non | aucun test dédié de lien trouvé | aucune persistance d’état de connecteur | l’éditeur l’exclut |
| Item | aucun | aucune | le runtime d’objet existe, mais pas comme extrémité officielle d’un connecteur | non | tests séparés du système d’objets | **Oui** pour l’état monde/inventaire, pas pour un état actif de connecteur | l’éditeur l’exclut |

## B.1 Valeurs d’enum déclarées mais non actives dans le contrat générique

Événements présents dans l’enum mais sans émetteur C++ actif dans le système Event → Command audité :

```text
Used
Entered
Exited
Opened
Closed
Enabled
Disabled
```

Commandes déclarées mais non implémentées comme commandes génériques :

```text
Lock
Unlock
ShowMessage
```

`Enable`, `Disable`, `Spawn`, `Despawn` et `Teleport` ne sont **plus globalement inactifs** : MON13 les implémente pour `MonsterSpawn`. Ils ne doivent donc plus être documentés comme universellement inutilisés.

## B.2 Table de politique actuelle de l’éditeur

`GridEditorLinkPolicy` expose actuellement exactement :

```text
Sources
-------
Button          : Activated
Lever           : Activated, Deactivated
PressurePlate   : Activated, Deactivated
Trigger         : Activated, Deactivated
Receptacle      : ItemInserted, ItemRemoved, ItemChanged
MonsterSpawn    : MonsterDied, MonsterSpawned, MonsterDespawned,
                  MonsterTeleported, EncounterWaveStarted,
                  EncounterCompleted

Cibles
------
Door            : Open, Close, Toggle, Activate, Deactivate
Teleporter      : Activate, Deactivate, Toggle
Light           : Activate, Deactivate, Toggle
Receptacle      : ConsumeItem, ConsumeAllItems,
                  EnableRemoval, DisableRemoval
MonsterSpawn    : Spawn, Despawn, Teleport,
                  Activate, Deactivate, Enable, Disable, Toggle,
                  StartEncounter
```

Cette politique doit être **étendue et non remplacée** par MON19.

---

# C. Lacunes fonctionnelles réelles

Les lacunes ci-dessous sont classées selon leur importance architecturale et non selon l’ancienneté des TODO.

## C.1 P0 — absence de variables génériques persistantes pour les énigmes

Il n’existe pas de stockage canonique pour des valeurs telles que :

```text
Crypt.SecretOpened = true
Crypt.RuneCount = 3
Crypt.Stage = 2
```

`FGridLevelRuntimeState` persiste les portes, certains objets interactifs, la présence d’objets, les objets, les réceptacles, les monstres et les rencontres, mais ne contient aucune table générique de variables typées propre au niveau.

C’est le prérequis le plus évident pour une logique d’énigme avancée et pour une persistance Lua sûre.

## C.2 P0 — absence de paramètres génériques pour les commandes

`EGridObjectCommand` est un enum sans charge utile générique.

Cela suffit pour `Door.Open`, mais pas pour de futures opérations comme :

```text
Counter.Add(2)
Variable.SetBool(true)
Variable.SetInt(3)
Message.Show("Crypt.Warning")
Lua.Invoke("OnRuneActivated")
```

MON19 a donc besoin soit d’un petit contrat typé d’arguments de commande, soit d’une représentation de nœud logique portant ces paramètres dans les données du niveau. Il n’a **pas** besoin d’un second bus de commandes.

## C.3 P0 — conditions existantes trop dépendantes de la cible

Toutes les conditions actuelles différentes de `None` exigent que **l’acteur cible soit un réceptacle**.

Le système peut exprimer :

```text
Button.Activated
    -> SecretAltar.ConsumeAllItems
       si SecretAltar contient RedGem
```

Mais il ne peut pas exprimer directement :

```text
Button.Activated
    -> Door.Open
       si AnotherReceptacle contient RedGem
```

car la condition est évaluée sur la cible de la commande, qui est ici la porte.

Un mécanisme de variables logiques ou de nœuds logiques génériques est donc justifié avant même l’introduction de Lua.

## C.4 P0 — interface d’édition des conditions absente

Les données du niveau et le runtime savent déjà gérer les conditions de réceptacle, et la validation connaît tous leurs champs. Pourtant, `SGridEditorLinksPanel` ne permet pas de les modifier.

Les concepteurs sont donc contraints d’éditer directement le tableau `Links` dans les propriétés Unreal génériques pour créer des liens conditionnels avancés déjà pris en charge par le runtime.

MON19.2 doit corriger cette lacune avant d’ajouter une interface de script plus complexe.

## C.5 P0 — l’identité des liens dans l’éditeur ignore les conditions

`CreateLink()` et `RemoveExactLink()` identifient un connecteur uniquement par le quadruplet source/cible/événement/commande, alors que la validation inclut les paramètres de condition lorsqu’elle recherche les doublons exacts.

Avant d’ajouter d’autres champs de condition ou de charge utile, l’identité et les opérations d’édition des liens doivent devenir cohérentes.

## C.6 P0 — succès d’une commande ≠ succès de gameplay

`Light`, `Teleporter` et `ItemSpawn` révèlent un problème sémantique :

- une commande générique d’état peut mettre à jour `ActiveObjectIds` et retourner succès ;
- l’effet de gameplay spécialisé peut cependant être absent ;
- `Teleporter` et `ItemSpawn` journalisent explicitement un comportement encore TODO.

La validation MON19 doit distinguer :

```text
l’état peut être mémorisé
```

de :

```text
ce type de cible implémente réellement cette commande en gameplay
```

Cette distinction est indispensable si Lua reçoit ultérieurement le résultat succès/échec de `grid.command()`.

## C.7 P0 — persistance incomplète pour certains états modifiés par commande

Exemples relevés pendant l’audit :

1. `FGridRuntimeReceptacleState` persiste les objets contenus mais pas `bCanRemoveItem`. Les commandes `ReceptacleDisableRemoval` et `ReceptacleEnableRemoval` ne survivent donc pas à une sauvegarde/recharge.
2. `CaptureCurrentLevelRuntimeState()` capture l’état d’activation uniquement pour `Button`, `Lever`, `PressurePlate`, `Receptacle` et `Trigger`. Les indicateurs actifs génériques de `Light`, `Teleporter`, `ItemSpawn`, `Decoration` et `Item` ne sont pas capturés.
3. Une commande générique ayant retourné succès peut donc être perdue lors d’un Save/Load.

MON19 ne doit pas construire des énigmes persistantes sur cette ambiguïté.

## C.8 P1 — couverture automatisée faible pour les liens et conditions génériques

L’extension MonsterSpawn/Encounter de MON13 possède une bonne couverture automatisée, notamment :

```text
MON13.3 DeferredSpawnLinks
MON13.3 LifecyclePersistence
MON13.3 AtomicCommands
MON13.3 EditorLinkPolicy
MON13.4 EncounterWaves
MON13.4 AtomicWaveFailure
MON13.4 Validation
MON13.4 EditorLinkPolicy
```

Les réceptacles disposent de leur côté d’une suite PIE manuelle détaillée et de résultats déjà validés.

En revanche, la recherche dans le dépôt n’a pas révélé de tests automatisés référant directement les valeurs de `EGridObjectCondition`, ni de suite générique exhaustive couvrant chaque combinaison source/événement/cible/commande.

MON19.2 doit introduire des tests contractuels pilotés par table avant de relier Lua au système.

## C.9 P1 — risque de divergence entre capacités éditeur et runtime

`GridEditorLinkPolicy` constitue une bonne autorité côté éditeur, mais `GridLevelEditorActor` contient également des fonctions auxiliaires de validation du support runtime, tandis que `UGridActivationComponent` porte le véritable dispatcher.

L’audit montre déjà des cas où un type est considéré compatible avec une commande d’état alors que son effet de gameplay spécialisé n’existe pas.

MON19 doit éviter d’introduire une troisième table de capacités maintenue séparément. Il faut privilégier des fonctions déclaratives partagées ou, à défaut, des tests vérifiant explicitement la politique de l’éditeur contre les capacités réelles du runtime.

## C.10 P1 — absence de budget général d’exécution

La protection actuelle contre la réentrée d’une source est utile, mais Lua introduit une seconde classe de risques :

```lua
while true do
end
```

ainsi que des chaînes inter-systèmes telles que :

```text
Lua -> Command -> Event -> Lua -> Command -> ...
```

Un budget d’instructions Lua ainsi qu’une limite commune de profondeur/budget pour le dispatch MON19 seront indispensables avant d’autoriser les scripts de niveaux communautaires.

---

# D. Dérive historique de la documentation

## D.1 `docs/Design/03_EVENT_COMMAND_LINKS.md`

Ce document reste utile comme intention de conception historique, mais il n’est plus la source d’autorité pour le contrat exact des enums et du runtime.

Exemples de dérive :

- anciens noms d’événements tels que `OnActivate`/`OnDeactivate` au lieu des valeurs actuelles `Activated`/`Deactivated` ;
- anciennes commandes de timer, `Destroy`, commandes d’animation ou de son, etc., absentes de l’enum actuel ;
- anciennes affirmations sur l’apparition et la téléportation antérieures à MON13 ;
- listes de sources/cibles qui ne décrivent plus l’extension complète MonsterSpawn/Encounter.

La documentation MON19 doit considérer le code actuel et `GridEditorLinkPolicy` comme sources d’autorité.

## D.2 `docs/Architecture/LINK_EVENT_COMMAND_FOUNDATION.md`

Ce document est beaucoup plus proche du code actuel, mais comporte lui aussi une dérive liée à MON13 et à la persistance :

- il classe `Spawn`, `Despawn` et `Teleport` parmi les valeurs non dispatchées alors que MON13 les implémente désormais pour `MonsterSpawn` ;
- son résumé des commandes est antérieur à `StartEncounter` ;
- l’affirmation selon laquelle aucun état runtime de lien n’est sauvegardé est devenue trop générale : plusieurs états cibles sont désormais persistés, bien que de manière incomplète ;
- la formule « les cycles indirects ne sont pas détectés » doit distinguer l’absence d’analyse de graphe dans l’éditeur de la protection runtime existante contre la réentrée, qui interrompt une chaîne lorsqu’elle revient sur une source déjà en cours de dispatch.

## D.3 Documentation des réceptacles

La documentation de test des réceptacles décrit correctement une limitation toujours actuelle : les champs de condition existent dans `FGridObjectLink`, mais ne sont pas exposés dans le formulaire Slate de création des connecteurs.

Ce point reste actuel et doit être traité dans MON19.

## D.4 Conséquence pour la feuille de route

La feuille de route faisant autorité précise à juste titre qu’un langage de script ne doit être introduit que si Event → Command s’avère insuffisant.

MON19.1 confirme que :

- Event → Command suffit aux énigmes simples et à une part importante des mécanismes chaînés ;
- les premières pièces réellement manquantes sont des valeurs génériques persistantes et une logique paramétrée ;
- Lua n’est justifié que lorsque la logique devient maladroite, combinatoire ou difficile à représenter uniquement en données ;
- la création d’un langage propriétaire n’est pas justifiée.

---

# E. Étude de faisabilité de Lua

## E.1 État des versions au 22 août 2026

Les informations officielles de Lua indiquent :

- Lua 5.5 publié le 22 décembre 2025 ;
- version Lua courante : **5.5.1**, publiée le 3 août 2026 ;
- dernière version de maintenance stable de la branche 5.4 : **5.4.8**, publiée le 4 juin 2025 ;
- Lua 5.4.9 est encore en phase de release candidate pendant août 2026 et ne doit donc pas servir immédiatement de base de production.

Lua est distribué sous licence MIT et conçu explicitement pour être embarqué dans des applications C/C++.

La documentation de version de Lua rappelle également que la compatibilité du bytecode et de la VM n’est pas garantie entre versions. Cela renforce la décision de stocker les scripts sous forme de texte source et non de bytecode compilé persisté.

Références officielles :

- https://www.lua.org/versions.html
- https://www.lua.org/download.html
- https://www.lua.org/manual/5.4/
- https://www.lua.org/license.html

## E.2 Version recommandée : Lua 5.4.8

**Recommandation pour MON19 : Lua 5.4.8.**

Raisons :

1. il s’agit d’une version de maintenance stable et éprouvée ;
2. l’intégration C de la branche 5.4 est bien établie ;
3. sol2 v3 contient explicitement des correctifs pour Lua 5.4 ;
4. le suivi des problèmes de sol2 en 2026 comporte encore un ticket ouvert concernant Lua 5.5 ;
5. Lua 5.5.1 n’a que quelques semaines à la date de cet audit ;
6. MON19 ne nécessite aucune fonctionnalité propre à Lua 5.5 ;
7. le stockage des scripts en texte source rend une migration ultérieure vers 5.5 raisonnablement simple à envisager.

Il ne faut pas utiliser Lua 5.4.9 RC en production. Une réévaluation pourra avoir lieu lorsque 5.4.9 sera final ou lorsqu’une migration vers 5.5 apportera un bénéfice concret et aura été testée.

## E.3 API C directe de Lua

### Avantages

- une seule dépendance tierce : Lua officiel ;
- surface d’attaque minimale ;
- contrôle exact des fonctions et tables introduites dans la VM ;
- aucune liaison automatique avec la réflexion Unreal ;
- pas de couche de templates C++ lourde ;
- aucune nécessité d’activer les exceptions C++ dans `GrimrockPrototype` ;
- parfaitement adaptée à une API volontairement très petite et explicitement autorisée ;
- plus facile à auditer en vue du futur chargement de contenus joueurs non fiables.

### Coûts

- manipulation de pile plus verbeuse ;
- nécessité d’une discipline stricte sur le contrôle des types ;
- obligation de respecter le modèle d’erreur C de Lua et son usage de `longjmp` ;
- les petits wrappers C++ devront rester simples et fortement testés.

Dans ce projet, la petitesse de la surface d’API est un avantage recherché et non une limitation.

## E.4 sol2

sol2 est :

- sous licence MIT ;
- header-only ;
- conçu comme couche de liaison C++ ↔ Lua ;
- annoncé compatible avec Lua 5.1+ et notamment Lua 5.4.

La version stable indiquée en amont est `v3.3.0`.

Faits pertinents pour GrimrockPrototype :

1. les `Build.cs` du projet n’activent actuellement pas `bEnableExceptions=true` ;
2. Unreal Build Tool fournit bien `bEnableExceptions`, mais le projet n’a pas choisi cette option ;
3. sol2 prend en charge `SOL_NO_EXCEPTIONS`, mais sa documentation précise que ce mode modifie le comportement par défaut des paniques et demande une gestion volontaire ;
4. la documentation d’erreur de sol2 attire l’attention sur le comportement `setjmp`/`longjmp` de Lua et les risques pour les destructeurs C++ ;
5. le suivi des problèmes de sol2 contient en 2026 des tickets concernant le mode sans exceptions et Lua 5.5 ;
6. une grande partie de l’intérêt de sol2 — liaison automatique de types utilisateurs et ergonomie de binding — est précisément ce que le futur bac à sable ne doit **pas** offrir.

Références :

- https://github.com/ThePhD/sol2
- https://sol2.readthedocs.io/en/latest/exceptions.html
- https://sol2.readthedocs.io/en/latest/safety.html

### Conclusion concernant sol2

**Ne pas faire de sol2 une dépendance de production de MON19.**

Un petit essai de compilation isolé peut rester envisageable à titre expérimental, mais la recommandation de production est l’API C directe de Lua 5.4.

Ce choix ne signifie pas que sol2 serait une mauvaise bibliothèque. Il signifie que GrimrockPrototype cherche volontairement une API minuscule, maîtrisable du point de vue de la sécurité, tandis que le choix actuel du projet de ne pas activer les exceptions C++ réduit l’intérêt de la couche de confort de sol2 et ajoute une surface supplémentaire de compatibilité.

Il ne faut pas activer globalement ou à l’échelle du module les exceptions C++ uniquement pour satisfaire sol2.

## E.5 UnLua

UnLua est sous licence MIT et prend en charge Unreal Engine 5.x.

Ses fonctionnalités principales comprennent volontairement un accès direct à :

```text
UCLASS
UPROPERTY
UFUNCTION
USTRUCT
UENUM
Blueprint events
conteneurs UE natifs
fonctions latentes/coroutines
```

C’est presque l’opposé de la frontière de confiance souhaitée :

```text
Lua
  -> API Grimrock contrôlée
  -> services Event/Command et runtime existants
```

Par conséquent :

**UnLua est rejeté pour MON19**, malgré son intégration UE valide et sa licence MIT.

Référence :

- https://github.com/Tencent/UnLua

## E.6 Intégration UE5.5.4 et packaging

Le modèle de dépendance tierce documenté par Epic utilise un module externe `.Build.cs` (`ModuleType.External`) afin d’exposer les includes, définitions et bibliothèques statiques/importées.

Une organisation Windows-first raisonnable serait :

```text
Source/
  ThirdParty/
    Lua54/
      Lua54.Build.cs
      include/
        lua.h
        lauxlib.h
        lualib.h
        luaconf.h
      lib/
        Win64/
          lua54.lib
      LICENSE.txt
```

Puis :

```text
GrimrockPrototype.Build.cs
    -> dépendance vers Lua54
```

Pour la première intégration, il est préférable d’utiliser une **bibliothèque statique** construite à partir des sources C officielles de Lua, en excluant les programmes autonomes `lua.c` et `luac.c`. Une bibliothèque statique évite le staging de DLL et les problèmes de recherche de bibliothèques dynamiques en Development et Shipping.

La configuration exacte MSVC/UBT devra être démontrée dans MON19.3 avec :

```text
Editor Development
Game Development
packaged Development
Shipping
```

MON19.1 ne prétend pas que ces configurations ont déjà été compilées.

Référence Epic :

- https://dev.epicgames.com/documentation/unreal-engine/integrating-third-party-libraries-into-unreal-engine

## E.7 Modèle d’exceptions

Les fichiers actuels :

```text
Source/GrimrockPrototype/GrimrockPrototype.Build.cs
Source/GrimrockPrototypeEditor/GrimrockPrototypeEditor.Build.cs
```

n’activent pas les exceptions C++.

Recommandations :

- conserver la politique actuelle du module principal ;
- compiler Lua officiel en C dans la bibliothèque tierce ;
- exécuter toutes les fonctions de rappel utilisateur à travers des appels Lua protégés ;
- veiller à ce que les trampolines C/C++ exposés à Lua ne reposent pas sur des exceptions C++ ;
- éviter toute conception dans laquelle un `longjmp` Lua pourrait contourner une destruction RAII importante ;
- ne jamais utiliser une panique Lua comme mécanisme normal de contrôle de flux.

---

# F. Recommandation architecturale ferme

## F.1 Faut-il utiliser Lua ?

**Oui.**

Mais uniquement pour les logiques complexes dont l’expression devient réellement plus claire sous forme de script.

Les énigmes simples doivent rester :

```text
Event -> Command
```

sans appel à la VM Lua.

## F.2 Faut-il utiliser sol2 ?

**Non pour la base de production de MON19.**

Utiliser l’API C officielle de Lua 5.4 derrière un wrapper très réduit appartenant au projet.

sol2 ne devra être réévalué que si un besoin mesuré d’une liaison C++ plus riche apparaît ultérieurement et si son mode sans exceptions a été démontré avec la chaîne d’outils UE exacte du projet.

## F.3 Version de Lua

**Lua 5.4.8** pour MON19.

## F.4 Propriété et durée de vie du runtime Lua

Il est préférable d’utiliser un composant sans Tick appartenant à l’acteur d’exécution du niveau, par exemple :

```text
AGridLevelRuntimeActor
  + UGridActivationComponent
  + UGridLuaRuntimeComponent   // proposé, sans Tick
```

plutôt qu’une VM globale singleton.

Raisons :

- sa durée de vie suit celle du niveau chargé ;
- la remise à zéro lors d’un rebuild ou d’une transition de niveau est simple ;
- aucun état global de script ne fuit d’un niveau à l’autre ;
- le point d’intégration se trouve naturellement à côté du composant d’activation existant ;
- les tests peuvent instancier un acteur runtime et son composant Lua en isolation.

Le composant devrait posséder un unique `lua_State*` pour le niveau actif et le détruire lors du teardown ou du rechargement.

## F.5 Aucune exposition directe d’Unreal

Lua ne doit jamais recevoir :

```text
UWorld*
AActor*
UObject*
UClass*
UFunction*
accès direct à un chemin du système de fichiers
outils de réflexion
exécution de console/processus
```

La surface exposée doit se limiter à des fonctions/tables scalaires définies par le projet, par exemple l’équivalent futur de :

```text
grid.command(id, command)
grid.getBool(name)
grid.setBool(name, value)
grid.getInt(name)
grid.setInt(name, value)
grid.addInt(name, delta)
grid.log(message)        // limité en fréquence / adapté au mode Development
```

Les noms exacts relèveront de l’implémentation MON19.3/MON19.4 ; ils ne sont pas figés par cet audit.

## F.6 Lua → Command doit réutiliser le dispatcher central

Lua ne doit jamais résoudre directement un Actor puis appeler `OpenDoor()`.

Le chemin obligatoire est :

```text
Fonction Lua
    -> liaison Grimrock explicitement autorisée
        -> résolution ObjectId / identifiant logique validé
            -> entrée de commande UGridActivationComponent / AGridLevelRuntimeActor
                -> implémentation existante propre au type de cible
```

Cela demande d’extraire ou d’exposer un **point d’entrée sûr permettant d’exécuter une commande unique** dans le dispatcher central actuel, et non de créer une seconde implémentation des commandes réservée à Lua.

## F.7 Le pont Event → Lua doit rester intégré au graphe de connecteurs

Le modèle conceptuel le moins perturbateur consiste à introduire une extrémité logique/script sans gameplay propre, participant au graphe de liens existant.

Exemple :

```text
Lever_A.Activated
    -> ScriptLogic_OnLever.Invoke
```

Cette extrémité de script ne nécessite pas d’acteur de gameplay dans le monde. Elle résout une fonction de rappel configurée dans l’environnement Lua du niveau.

Cette fonction peut ensuite demander des commandes normales :

```text
Lua
    -> grid.command(Door_Secret, Open)
    -> grid.command(Teleporter_Exit, Activate)
```

La représentation exacte — sous-type d’objet logique ou petit enregistrement de nœud logique dédié — devra être finalisée dans MON19.2 après la conception des primitives logiques génériques. Il ne faut **pas** simuler l’appel Lua en détournant une cible sans rapport telle qu’une porte ou un trigger.

## F.8 Identifiants destinés aux concepteurs

L’exécution interne doit continuer à utiliser `FGuid ObjectId` comme identité canonique des objets.

Pour l’écriture de scripts Lua, des GUID bruts sont trop fragiles à saisir manuellement. Un objet exposé aux scripts devra donc disposer à terme d’un alias `FName` stable et optionnel — par exemple `LogicId` ou `ScriptId` — répondant aux règles suivantes :

- unique dans le niveau ;
- validé par l’éditeur ;
- résolu une seule fois vers `ObjectId` ;
- jamais utilisé pour contourner l’index central des objets.

Il ne faut pas recycler un `Tag` arbitraire potentiellement non unique sans ajouter une validation explicite d’unicité.

---

# G. Proposition finale MON19.2 → MON19.8

## MON19.2 — Durcissement Event/Command et primitives logiques

### Objectif

Couvrir les cas courants d’énigmes sans Lua et corriger les incohérences découvertes par MON19.1.

### Travaux

1. rendre l’identité des connecteurs cohérente avec les champs de condition et de charge utile ;
2. exposer les conditions de réceptacle existantes dans `SGridEditorLinksPanel` ;
3. ajouter des tests pilotés par table pour chaque paire événement/commande prise en charge par l’éditeur ;
4. distinguer dans la validation la simple « mémorisation d’état » d’une véritable implémentation de commande ;
5. ajouter un petit modèle générique de variables typées de niveau :
   - Bool ;
   - Int32 ;
   - éventuellement FName uniquement si un besoin de production immédiat existe ;
6. ajouter quelques primitives logiques data-driven, de préférence sans créer une classe Actor par primitive :
   - définir/inverser un Bool ;
   - définir/ajouter/soustraire/réinitialiser un Int ;
   - seuil/comparaison ;
   - verrou à déclenchement unique ;
   - relais ;
7. permettre aux nœuds logiques d’émettre `Activated` / `Deactivated`, ou un autre ensemble volontairement réduit d’événements, dans le graphe de connecteurs existant ;
8. corriger les trous de persistance nécessaires aux énigmes de production, notamment l’état d’autorisation de retrait des réceptacles s’il reste une commande persistante.

### Explicitement hors périmètre

- VM Lua ;
- langage de script propriétaire ;
- IDE intégré ;
- moteur d’expressions arbitraires.

### Critère de sortie

Au moins deux énigmes non triviales utilisant variables et compteurs doivent fonctionner avec **zéro Lua**.

## MON19.3 — Fondation du runtime Lua 5.4

### Objectif

Embarquer Lua 5.4.8 officiel avec la plus petite surface d’exécution sûre possible.

### Travaux

1. ajouter la licence et les sources Lua 5.4.8 vendues avec le projet, ou des entrées de build tierces reproductibles ;
2. créer le module UBT externe `Lua54` et l’intégration de la bibliothèque statique Win64 ;
3. ajouter le composant/service Lua sans Tick ;
4. créer/détruire une VM par niveau actif ;
5. charger uniquement du texte source ;
6. exécuter uniquement des appels protégés ;
7. ouvrir uniquement les bibliothèques explicitement approuvées ;
8. fournir des diagnostics d’erreur déterministes ;
9. intégrer dès le départ un allocateur mémoire avec comptabilisation, même si les limites initiales sont généreuses ;
10. intégrer dès le départ l’infrastructure de hook de comptage d’instructions.

### Critère de sortie

Une fonction Lua triviale peut s’exécuter en Editor Development sans accès aux objets Unreal, au système de fichiers, au système d’exploitation ou au chargement de packages.

Aucune dépendance sol2 n’est nécessaire.

## MON19.4 — Pont Event → Lua → Command

### Objectif

Relier Lua au graphe de connecteurs existant sans créer un second dispatcher de gameplay.

### Travaux

1. représenter une fonction de script comme cible de connecteur ou extrémité logique valide ;
2. ajouter un contrat unique d’invocation de script ;
3. résoudre les alias d’objets destinés aux concepteurs vers les `ObjectId` canoniques ;
4. exposer `grid.command()` à travers le chemin central d’exécution des commandes ;
5. retourner des résultats succès/erreur contrôlés, jamais des pointeurs d’Actor ;
6. partager un contexte de profondeur/budget de dispatch entre les chaînes Event → Lua → Command → Event ;
7. garantir qu’une erreur Lua échoue uniquement l’invocation concernée sans provoquer de crash ni corrompre l’état du niveau.

### Critère de sortie

Le scénario :

```text
Button.Activated
    -> fonction Lua
        -> Door.Open
```

fonctionne, tandis que le chemin direct :

```text
Button.Activated
    -> Door.Open
```

continue à fonctionner sans aucune intervention de Lua.

## MON19.5 — Contrat de persistance Lua et version de sauvegarde

### Objectif

Ne persister que les résultats canoniques des énigmes.

### Travaux

1. exposer à Lua le stockage de variables typées de MON19.2 ;
2. ne jamais sérialiser `lua_State`, pile, closures, coroutines, userdata ou chunks compilés ;
3. capturer les variables typées dans `FGridLevelRuntimeState` ;
4. restaurer les variables avant toute fonction de script qui pourrait en dépendre ;
5. ajouter la migration et la validation du SaveGame ;
6. ajouter des tests Save/Load au milieu d’une énigme partiellement résolue ;
7. garantir qu’un changement de version/source du script n’exige jamais de migration de bytecode.

### Critère de sortie

Une sauvegarde réalisée après modification des variables d’une énigme se recharge dans le même état logique avec une VM Lua entièrement recréée.

## MON19.6 — Intégration et validation dans l’éditeur

### Objectif

Rendre la logique avancée éditable sans transformer l’éditeur de donjon en IDE Lua.

### Travaux

1. éditer directement les conditions des liens dans CONNECTORS ;
2. créer et modifier les variables/nœuds logiques ;
3. associer une source de script de niveau et un nom de fonction de rappel ;
4. valider les noms de fonctions lorsque la source est disponible ;
5. valider l’unicité des alias d’objets visibles par les scripts ;
6. afficher les arêtes Event → Script dans la même visualisation des connecteurs ;
7. ajouter des catégories de diagnostic pour :
   - script absent ;
   - erreur de syntaxe ;
   - fonction de rappel absente ;
   - alias d’objet invalide ;
   - commande non prise en charge ;
   - violation de la politique de sécurité ;
8. fournir, si utile, un petit inspecteur en lecture seule des variables runtime pendant PIE.

### Explicitement hors périmètre

- débogueur ;
- points d’arrêt ;
- complétion de code ;
- éditeur de source complet.

## MON19.7 — Bac à sable, limites d’exécution et packaging

### Objectif

Démontrer qu’un script provenant d’un niveau communautaire peut être traité comme une entrée non fiable à l’intérieur du bac à sable défini par le jeu.

### Travaux

1. ne jamais appeler `luaL_openlibs()` globalement ;
2. ouvrir explicitement uniquement les bibliothèques approuvées ;
3. ne jamais exposer `io`, `os`, `package`, `debug` ;
4. ne pas exposer `require`, `dofile`, `loadfile` ni de chargeur de module arbitraire ;
5. restreindre le chargement de code dynamique et utiliser un mode texte uniquement ;
6. utiliser un hook de comptage d’instructions pour interrompre les boucles infinies ;
7. imposer un quota mémoire par VM grâce à l’allocateur personnalisé de `lua_newstate` ;
8. imposer une taille maximale de script ;
9. imposer une profondeur maximale des fonctions de rappel/dispatch ;
10. garantir que chaque fonction C++ exposée reste bornée et non bloquante ;
11. limiter la fréquence des logs et erreurs issus des scripts ;
12. tester des scripts mal formés et des boucles infinies intentionnelles ;
13. tester un package Development ;
14. tester Shipping ;
15. vérifier la politique de staging et de chargement des sources de scripts packagées ;
16. vérifier qu’aucun accès direct à UE, à la réflexion, au système de fichiers ou aux processus n’existe.

### Critère de sortie

Un script de test volontairement hostile peut épuiser son propre budget puis être interrompu sans bloquer ni faire planter le thread de jeu.

## MON19.8 — Suite d’énigmes de production, régression et clôture

Implémenter et valider :

### Énigme A — data-driven directe

```text
Lever -> Door
```

Sans Lua.

### Énigme B — variables/compteur

Plusieurs interrupteurs, compteur et seuil, uniquement avec MON19.2.

### Énigme C — Lua conditionnel

Une fonction Lua lit des variables persistantes puis demande des commandes normales.

### Énigme D — pont avec une rencontre

```text
EncounterCompleted
    -> Lua
        -> Door.Open
        -> Teleporter/Message ou autre commande de production complète
```

Si `Teleporter` ou `ShowMessage` sont encore incomplets à ce stade, leur comportement doit être terminé ou le test doit utiliser une cible dont l’effet de gameplay est réel. Un simple succès « état uniquement » ne doit pas être considéré comme une énigme réussie.

### Énigme E — Save/Load au milieu d’une résolution

Persister les variables et les états canoniques des cibles, recréer la VM Lua, puis poursuivre l’énigme.

### Énigme F — Lua défectueux ou hostile

Erreur de syntaxe, erreur runtime et boucle infinie sont contenues et diagnostiquées.

Puis :

- exécuter les tests automatisés ciblés ;
- exécuter les régressions pertinentes ;
- effectuer les validations PIE fournies/confirmées par le propriétaire du projet ;
- vérifier les packages Development/Shipping selon le périmètre retenu ;
- mettre à jour la documentation d’architecture faisant autorité ;
- clôturer MON19.

---

# H. Impact sur SaveGame et le versionnement

## H.1 État actuel

`UGrimrockPartySaveGame::CurrentSaveVersion` vaut actuellement :

```text
6
```

La version 6 a été introduite par MON18.8 pour la persistance du Spellbook.

`FGridDungeonRuntimeState` contient les snapshots `FGridLevelRuntimeState` de chaque niveau.

## H.2 Données MON19 proposées

Ajouter dans `FGridLevelRuntimeState` un stockage logique typé persistant, conceptuellement :

```text
LevelVariables
    Crypt.SecretOpened : Bool=true
    Crypt.RuneCount    : Int=3
```

Utiliser une `USTRUCT` typée, jamais une valeur Lua ni une table Lua sérialisée.

La première version de production doit prendre en charge uniquement les types dont l’usage gameplay est démontré. Base recommandée :

```text
Bool
Int32
```

Ajouter `FName` ou `String` plus tard uniquement si un besoin réel l’impose.

## H.3 Incrément de version

Lors de l’introduction du stockage persistant MON19 :

```text
CurrentSaveVersion 6 -> 7
```

Le projet utilise des migrations explicites de version même pour les domaines ajoutés de manière additive. Il est préférable de conserver cette discipline plutôt que de modifier silencieusement la sémantique de la version 6.

## H.4 Détail critique de migration

`FRPGSaveMigrationService::PrepareLoadedSave()` possède actuellement des traitements spécifiques pour v5 et v4, puis un chemin de migration pour les versions plus anciennes.

Lorsque `CurrentSaveVersion` passera à 7, **v6 devra disposer de son propre chemin de migration explicite avant la branche v5 actuelle**.

Sans cela, une sauvegarde v6 parfaitement légitime tomberait dans une logique de reconstruction ancienne qui n’a pas été conçue pour elle.

Migration v6 → v7 recommandée :

1. valider les domaines v6 existants exactement comme aujourd’hui ;
2. laisser vide le snapshot des variables MON19 ;
3. définir `SaveVersion=7` ;
4. lors de la restauration du niveau, un snapshot de variables absent est initialisé à partir des valeurs par défaut actuelles du `UGridLevelAsset` ;
5. valider ensuite le stockage typé obtenu.

Le service de migration SaveGame ne doit pas lui-même instancier les valeurs par défaut spécifiques aux énigmes de chaque niveau.

## H.5 Trous de persistance à trancher avant la clôture de MON19

- `bCanRemoveItem` des réceptacles, si les commandes de retrait doivent survivre à Save/Load ;
- état actif central de `Light`, `Teleporter` et `ItemSpawn` s’ils deviennent de véritables cibles de production ;
- tout état de verrou ou de déclenchement unique introduit par MON19.2.

## H.6 Règle SaveGame propre à Lua

Ne jamais sauvegarder :

```text
lua_State
pile Lua
closures
coroutines
userdata
registry
compteurs d’instructions
itérateurs ouverts
timers Lua
bytecode compilé
```

Seul l’état canonique du jeu doit être persisté.

---

# I. Stratégie de bac à sable pour les futurs contenus joueurs

## I.1 Politique des bibliothèques

Ne **pas** appeler `luaL_openlibs()` dans la VM destinée aux niveaux joueurs.

Ouvrir explicitement les bibliothèques sûres, une par une, avec l’API C de Lua.

Liste blanche initiale envisageable :

```text
sous-ensemble des fonctions de base
math
string
table
utf8 (optionnel, risque faible)
```

Liste noire initiale :

```text
io
os
package
debug
coroutine (à différer tant qu’un besoin concret ne l’exige pas)
```

Ne jamais exposer non plus :

```text
require
dofile
loadfile
chargement arbitraire de modules depuis le système de fichiers
exécution de processus
```

`load` doit être omis dans un premier temps. Un script de niveau ne doit pas pouvoir compiler dynamiquement des fragments secondaires arbitraires tant qu’aucune fonctionnalité ne justifie ce besoin.

## I.2 Scripts texte uniquement

Utiliser le mode texte de Lua (`luaL_loadbufferx(..., "t")` ou équivalent) afin de rejeter les chunks binaires.

Les scripts créés par les joueurs doivent rester des fichiers source `.lua` dans la frontière du futur package de niveau.

## I.3 Budget d’instructions

Utiliser `lua_sethook()` avec `LUA_MASKCOUNT` pour décrémenter un budget d’instructions à chaque invocation protégée.

Le budget doit être déterministe et réinitialisé à chaque appel de script de premier niveau.

Un diagnostic complémentaire basé sur le temps réel peut exister, mais un simple timeout temporel ne suffit pas car :

- il n’est pas déterministe ;
- il ne peut pas interrompre proprement n’importe quel travail C++ ;
- chaque fonction C++ exposée doit déjà être bornée par conception.

## I.4 Budget mémoire

Créer la VM avec `lua_newstate(customAllocator, context)`.

Suivre le nombre d’octets alloués pour cette VM et refuser les allocations au-delà d’un quota configuré pour le script du niveau.

Cela fournit une véritable limite contre les scripts construisant des tables ou chaînes sans borne.

## I.5 Profondeur d’exécution

Maintenir un contexte d’exécution MON19 contenant au minimum :

```text
budget d’instructions Lua restant
profondeur d’imbrication Event/Command/Lua
identité du script et de la fonction actuellement exécutés
identité du niveau courant
```

`DispatchingSourceObjectIds` reste utile, mais la nouvelle limite de profondeur protège les chaînes longues utilisant des sources différentes et des appels Lua.

## I.6 Liste blanche de l’API

Les liaisons doivent manipuler uniquement des identifiants et des valeurs scalaires stables.

Bonne frontière :

```text
FName / chaîne validée servant d’identifiant
bool
int32
petit enum de résultat/erreur
```

Mauvaise frontière :

```text
UObject*
AActor*
UWorld*
TSubclassOf
accès à la réflexion
pointeurs bruts
chargement arbitraire d’assets
```

## I.7 Isolation des erreurs

Chaque chargement de script et chaque appel de fonction doit être protégé.

En cas d’échec :

1. enregistrer une erreur concise indiquant niveau/script/fonction ;
2. interrompre cette invocation ;
3. laisser l’état canonique du runtime cohérent ;
4. ne pas poursuivre avec une pile Lua corrompue ;
5. si la VM elle-même atteint un état de panique irrécupérable, la détruire et la recréer plutôt que de prétendre qu’elle reste sûre.

## I.8 Frontière des fichiers communautaires

Les futurs niveaux externes doivent charger Lua uniquement depuis le package ou la racine du niveau sélectionné. L’API Lua ne doit jamais accepter un chemin arbitraire vers le système de fichiers de la machine hôte.

Le chargeur du package de niveau doit résoudre lui-même les noms de scripts et transmettre les octets source au runtime Lua.

---

# J. Fichiers probablement concernés par MON19.2 / MON19.3

Aucun fichier listé ci-dessous n’est modifié par MON19.1, à l’exception du présent document d’audit. Il s’agit de la surface d’implémentation probable d’après les responsabilités actuelles.

## J.1 Fichiers runtime/core existants — probablement MON19.2

```text
Source/GrimrockPrototype/Public/Core/GridTypes.h
Source/GrimrockPrototype/Public/Core/GridLevelAsset.h
Source/GrimrockPrototype/Private/Core/GridLevelAsset.cpp

Source/GrimrockPrototype/Public/Runtime/GridActivationComponent.h
Source/GrimrockPrototype/Private/Runtime/GridActivationComponent.cpp

Source/GrimrockPrototype/Public/Runtime/GridLevelRuntimeActor.h
Source/GrimrockPrototype/Private/Runtime/GridLevelRuntimeActor.cpp

Source/GrimrockPrototype/Public/Runtime/GridDungeonRuntimeState.h
```

Responsabilités probables :

- types de valeurs/nœuds logiques ;
- charge utile de commande ou paramètres équivalents des nœuds logiques ;
- point d’entrée sûr pour l’exécution d’une commande unique ;
- état runtime des variables ;
- capture/restauration ;
- vérification des capacités.

## J.2 Fichiers de sauvegarde — lorsque les variables persistantes MON19 seront introduites

```text
Source/GrimrockPrototype/Public/Save/GrimrockPartySaveGame.h
Source/GrimrockPrototype/Public/RPG/RPGSaveMigrationService.h   // uniquement si le contrat public change
Source/GrimrockPrototype/Private/RPG/RPGSaveMigrationService.cpp
```

Ainsi que les tests automatisés existants de sauvegarde/migration concernés par le passage v6 → v7.

## J.3 Fichiers d’éditeur existants — probablement MON19.2 / MON19.6

```text
Source/GrimrockPrototypeEditor/Public/EditorTools/GridEditorLinkPolicy.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridEditorLinkPolicy.cpp

Source/GrimrockPrototypeEditor/Public/EditorTools/GridLevelEditorActor.h
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEditorActor.cpp

Source/GrimrockPrototypeEditor/Public/EditorTools/Widgets/SGridEditorLinksPanel.h
Source/GrimrockPrototypeEditor/Private/EditorTools/Widgets/SGridEditorLinksPanel.cpp

Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdMode.cpp
Source/GrimrockPrototypeEditor/Private/EditorTools/GridLevelEdModeToolkit.cpp
```

`SGridEditorObjectInspectorPanel` pourra également être concerné si les propriétés logiques/scripts y sont éditées, mais il ne doit pas être modifié uniquement pour satisfaire une convention de nommage MON19.

## J.4 Nouveaux fichiers runtime Lua proposés — MON19.3

Forme minimale :

```text
Source/GrimrockPrototype/Public/Runtime/Scripting/GridLuaRuntimeComponent.h
Source/GrimrockPrototype/Private/Runtime/Scripting/GridLuaRuntimeComponent.cpp
```

De petits fichiers auxiliaires ne devront être ajoutés que si le composant devient réellement trop large, par exemple :

```text
GridLuaSandboxConfig.h
GridLuaBindings.cpp
```

Il ne faut pas commencer par une hiérarchie volumineuse de framework de script.

## J.5 Fichiers tiers — MON19.3

```text
Source/ThirdParty/Lua54/Lua54.Build.cs
Source/ThirdParty/Lua54/include/...
Source/ThirdParty/Lua54/lib/Win64/lua54.lib
Source/ThirdParty/Lua54/LICENSE.txt
```

`Source/GrimrockPrototype/GrimrockPrototype.Build.cs` ajoutera ensuite la dépendance.

Une dépendance du module éditeur vers Lua n’est pas nécessaire simplement pour exécuter des scripts ; la validation syntaxique dans l’éditeur pourra appeler plus tard un service de validation sûr partagé avec le runtime si cela devient utile.

## J.6 Tests à ajouter

Proposition :

```text
Source/GrimrockPrototype/Private/Tests/GridMON19LogicTests.cpp
Source/GrimrockPrototype/Private/Tests/GridMON19LuaTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON19LinkPolicyTests.cpp
Source/GrimrockPrototypeEditor/Private/Tests/GridEditorMON19ValidationTests.cpp
```

Ne pas fragmenter ces tests dans un fichier par minuscule primitive tant que la taille réelle des fichiers ne le justifie pas.

---

# Décision finale

La réponse à la question centrale de MON19.1 est la suivante :

> **Event → Command sait déjà faire nettement plus que ce que laisse penser l’ancien document de conception : événements typés par objet, dispatch central, chaînage, conditions de réceptacle, cycle Monster/Encounter de MON13, filtrage des capacités dans l’éditeur, validation et persistance partielle. Ce système doit rester l’ossature faisant autorité pour la logique de gameplay.**

La plus petite couche manquante n’est pas d’abord Lua. Elle consiste à :

```text
1. durcir l’édition et la validation des connecteurs existants ;
2. ajouter des variables de niveau génériques, typées et persistantes,
   ainsi que quelques primitives logiques data-driven ;
3. corriger les lacunes de persistance pour les états réellement modifiés par commande ;
4. puis seulement embarquer une petite VM Lua pour les logiques réellement complexes.
```

Pour le script :

```text
Utiliser Lua             : OUI
Version                  : Lua 5.4.8
sol2 en production       : NON pour la base MON19
UnLua                    : NON
Liaison                  : API C Lua directe, liste blanche appartenant au projet
Exposition d’Unreal      : AUCUNE
Lua -> gameplay          : via le chemin Command central existant
Persistance de la VM     : JAMAIS
Données persistantes     : uniquement variables canoniques typées du niveau
```

Cette architecture maintient le projet simple, orienté données et compatible avec l’objectif à long terme : charger en sécurité des niveaux créés par les joueurs sans donner aux scripts un accès libre aux mécanismes internes d’Unreal Engine.
