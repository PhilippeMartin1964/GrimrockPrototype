# Journal de validation terrain — Donjon de référence du Grid Editor

**Début :** 29 août 2026  
**Branche :** `master`  
**Référence initiale :** `8b63c86cd22a5ceaad39cdc853ce7922784753a3`  
**Contexte :** construction d'un niveau de donjon complet avec le Grimrock Grid Editor après clôture de GEUI01–GEUI10.

Ce document consigne uniquement les problèmes réellement rencontrés pendant l'authoring du donjon de référence et les corrections motivées par ces problèmes.

---

## Incident 001 — MON13.5 dépendait des monstres historiques du niveau de travail

### Observation

Après nettoyage du niveau de test afin de repartir d'un niveau presque vierge, le test :

`Grimrock.Monsters.MON13.5.RealPIEIntegration`

échouait avant même le lancement PIE avec :

- `The real encounter anchor exists` ;
- `The second wave-zero Rat exists` ;
- `The wave-one Rat exists`.

### Cause

`GridEditorMON135RealPIETests.cpp` utilisait directement trois `MonsterSpawn` historiques enregistrés dans le `UGridLevelAsset` courant de `L_GrimrockEditor`.

Le test imposait notamment :

- trois GUID de placement précis ;
- les cellules `(29,25)`, `(28,26)` et `(27,25)` ;
- le groupe `Encounter_Rats_01` ;
- un trigger en `(27,24)` ;
- un lien `Trigger.Activated -> StartEncounter`.

Ces données n'étaient donc pas seulement des données de démonstration : elles constituaient implicitement le fixture du test automatisé.

Le nettoyage légitime du niveau d'auteur supprimait ce fixture et cassait le test sans qu'aucune régression runtime ne soit présente.

### Classification

- **Type :** dette de test / couplage Editor-Content ;
- **Impact authoring :** élevé, car le niveau de travail ne pouvait pas être nettoyé librement ;
- **Régression runtime :** aucune identifiée ;
- **Correction du contenu utilisateur :** aucune nécessaire.

### Décision

Ne pas remettre les Rats historiques dans le niveau.

`FGridEditorMON135RealPIEIntegrationTest` doit rester un vrai test PIE, mais son scénario MON13.5 est désormais construit dans un `UGridLevelAsset` transient appartenant au test.

Le fixture :

- utilise toujours le vrai `DA_MON_RatGiant` ;
- utilise donc toujours le vrai `BP_MON_RatGiant` référencé par cette définition ;
- conserve les trois vagues/placements historiques du scénario MON13.5 ;
- conserve le vrai chemin Trigger -> `StartEncounter` ;
- conserve les deux sessions PIE Fresh puis Continue ;
- conserve le vrai chemin SaveGame ;
- ne dépend plus des objets actuellement placés dans le niveau en cours d'authoring.

Si un `UGridDungeonAsset` est assigné, le test en crée également une copie transient et rebinde uniquement le niveau courant vers le fixture transient. Cela maintient la vérification de cohérence effectuée par `PreparePIETestFromStartInternal` sans modifier le DungeonAsset persistant.

À la fin du test, les références originales de l'EditorActor et du PreviewRuntimeActor sont restaurées.

### Garanties

La correction ne modifie aucun :

- `.uasset` ;
- `.umap` ;
- placement du niveau de travail ;
- DungeonAsset persistant.

Le niveau de référence reste donc libre d'évoluer et d'être nettoyé sans casser MON13.5.

### Validation UE5.5.4 demandée

Validation ciblée :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.5.RealPIEIntegration"
~~~

Puis validation du jalon MON13.5 :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON13.5"
~~~

Après succès, poursuivre l'authoring du niveau presque vierge sans réintroduire les Rats historiques.

---

## Incident 002 — Feedback visuel lors d'un déplacement bloqué

### Observation

En PIE, une tentative de déplacement contre un mur, une porte fermée ou un obstacle était simplement refusée. La position restait correcte, mais le joueur ne recevait aucun retour spatial permettant de distinguer clairement « commande ignorée » de « passage bloqué ».

### Classification

- **Type :** UX / Runtime ;
- **Sous-système :** déplacement du groupe ;
- **Impact gameplay :** aucun changement de règle de déplacement ;
- **Impact sauvegarde / triggers / occupancy :** aucun.

### Décision

Ajouter dans `AGrimrockPartyPawn` un feedback de déplacement bloqué purement visuel :

1. la translation logique reste refusée ;
2. `CurrentCellX` / `CurrentCellY` ne changent pas ;
3. le Pawn avance brièvement de `BlockedMoveDistance` dans la direction demandée ;
4. il revient exactement au centre de sa cellule logique ;
5. aucun `HandlePartyCellChanged`, trigger de cellule, transition ou coût de déplacement supplémentaire n'est produit.

Valeurs par défaut :

- `bEnableBlockedMoveFeedback = true` ;
- `BlockedMoveDistance = 15 cm` ;
- `BlockedMoveForwardDuration = 0.08 s` ;
- `BlockedMoveReturnDuration = 0.10 s`.

Le feedback participe à `IsBusy()`. Les nouvelles commandes reçues pendant ces ~0,18 s ne sont pas bufferisées afin d'éviter l'empilement de chocs visuels.

En combat, le feedback n'est lancé que pour les refus spatiaux :

- `TargetCellUnavailable` ;
- `PassageBlocked` ;
- `TargetCellOccupied`.

Les refus liés au tour actif, aux PA/PAM ou à l'état du groupe restent sans déplacement visuel.

### Test automatisé

Nouveau test :

`Grimrock.Runtime.PartyMovement.BlockedFeedback`

Il vérifie notamment :

- qu'un mur refuse toujours la translation logique ;
- que la cellule logique ne change ni à l'aller ni au retour ;
- que le Pawn atteint la distance visuelle configurée ;
- qu'il revient exactement au centre de sa cellule ;
- qu'aucune entrée supplémentaire n'est bufferisée pendant le choc ;
- que désactiver le feedback ne change pas la règle de collision ;
- qu'un passage ouvert utilise toujours le mouvement de grille normal.

### Validation UE5.5.4 demandée

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.PartyMovement.BlockedFeedback"
~~~

Puis, pour vérifier les contrats de déplacement déjà existants :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TechnicalDebt.TD02_9.PartyMovementFacade"
~~~

Enfin, valider en PIE le ressenti des valeurs par défaut et ajuster uniquement les quatre paramètres `Movement|Blocked Feedback` si nécessaire.

---

## Incident 003 — Audio de déplacement du groupe

### Observation

Le feedback visuel d'un déplacement bloqué est désormais lisible, mais l'absence de son réduit encore la sensation de contact. Les déplacements réussis restent également silencieux.

### Classification

- **Type :** UX / Runtime / Audio ;
- **Sous-système :** déplacement du groupe ;
- **Autorité gameplay :** inchangée ;
- **Lecture :** 2D, attachée à l'expérience du joueur et non spatialisée dans le monde.

### Décision

Ajouter deux familles de sons configurables sur `AGrimrockPartyPawn` :

- `FootstepSounds` : une variante jouée uniquement lorsqu'une translation de grille est acceptée ;
- `BlockedMoveSounds` : une variante jouée une seule fois lorsque le feedback bloqué atteint sa distance maximale.

Le Pawn expose également :

- `bMovementAudioEnabled` ;
- `bNativeMovementAudioPlaybackEnabled` ;
- `FootstepVolume` ;
- `BlockedMoveVolume` ;
- `MovementAudioPitchVariation`.

La variation de pitch est présentationnelle et déterministe. Elle utilise son propre `FRandomStream` local et ne touche jamais au hasard du combat.

Valeurs par défaut :

- `FootstepVolume = 0.75` ;
- `BlockedMoveVolume = 0.85` ;
- `MovementAudioPitchVariation = 0.04`, soit environ `0.96..1.04`.

Les sons sont joués avec `UGameplayStatics::PlaySound2D`. Le free-look ne modifie donc pas la perception des propres pas du groupe.

### Convention de contenu

Le projet ne possédait pas encore de dossier audio racine sous `Content/GrimrockPrototype`. Créer :

~~~text
Content/GrimrockPrototype/Audio/Party/Movement/
~~~

Convention recommandée pour des sons de pierre :

~~~text
S_Party_Footstep_Stone_01
S_Party_Footstep_Stone_02
S_Party_Footstep_Stone_03
S_Party_Footstep_Stone_04

S_Party_BlockedImpact_Stone_01
S_Party_BlockedImpact_Stone_02
~~~

Le préfixe `S_` reste cohérent avec MON10. La structure est :

~~~text
S_<Source>_<Event>_<SurfaceOptionnelle>_<Variante>
~~~

Le système courant ne choisit pas encore automatiquement la surface. Le suffixe `Stone` est donc informatif et prépare une future extension matériau-sol/mur.

### Configuration dans Unreal Editor

1. Créer `Content/GrimrockPrototype/Audio/Party/Movement/`.
2. Importer les fichiers WAV.
3. Renommer les SoundWave selon la convention ci-dessus.
4. Ouvrir `Content/GrimrockPrototype/Blueprints/Runtime/BP_GrimrockPartyPawn`.
5. Dans **Class Defaults**, ouvrir **Movement > Audio**.
6. Laisser `Movement Audio Enabled = true`.
7. Laisser `Native Movement Audio Playback Enabled = true`.
8. Ajouter les variantes de pas dans `Footstep Sounds`.
9. Ajouter les variantes d'impact dans `Blocked Move Sounds`.
10. Commencer avec les volumes et la variation de pitch par défaut, puis ajuster en PIE.

Aucun SoundCue ni MetaSound n'est requis : les `SoundWave` importées peuvent être assignées directement.

### Tests automatisés

Nouveau test :

`Grimrock.Runtime.PartyMovement.AudioFeedback`

Il vérifie sans périphérique audio que :

- un mur ne joue pas l'impact au moment de la touche ;
- l'impact est demandé exactement au maximum du nudge ;
- le retour ne rejoue pas l'impact ;
- une translation acceptée demande exactement un pas ;
- un mouvement réussi n'ajoute aucun impact bloqué.

### Validation UE5.5.4 demandée

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.PartyMovement"
~~~

Puis validation manuelle en PIE avec les SoundWave réellement assignées dans `BP_GrimrockPartyPawn`.

---

## Incident 004 — Une porte en ouverture libérait le passage trop tôt

### Observation

En PIE, le groupe pouvait engager le déplacement vers la cellule suivante dès la commande d'ouverture d'une porte, alors que le panneau de porte était encore en mouvement.

Le problème concernait le contrat commun des portes et s'appliquait donc aussi aux portes secrètes dérivées de `AGridDoorActor`.

### Cause

`UGridDoorSystemComponent::OpenDoorOnEdge()` exécutait dans cet ordre :

~~~text
SetDoorPassageBlocked(..., false)
DoorActor->OpenDoor()
~~~

Le passage logique devenait donc libre au début de l'animation.

### Correction

L'ouverture suit désormais cette règle :

~~~text
commande Open
    -> lancer l'animation
    -> passage bloqué tant que IsFullyOpen() == false
    -> HandleDoorAnimationFinished()
    -> déblocage seulement si IsFullyOpen() == true
~~~

`AGridLevelRuntimeActor::CanMove()` continue d'utiliser `IsDoorPassageBlocked()` comme autorité. Aucune règle spéciale n'est ajoutée au Pawn.

La fermeture conserve son comportement sûr existant : le passage est bloqué immédiatement dès la commande de fermeture.

La correction s'applique aux :

- portes normales ;
- portes secrètes ;
- autres variantes runtime dérivées de `AGridDoorActor` et enregistrées comme `Door`.

### Non-régression

Nouveau test :

`Grimrock.Runtime.Doors.PassageBlockedUntilFullyOpen`

Il vérifie pour une porte normale puis une porte secrète :

- fermée : passage bloqué ;
- juste après la commande Open : animation active et passage encore bloqué ;
- à mi-animation : passage encore bloqué ;
- animation terminée : `IsFullyOpen() == true` ;
- seulement alors : `CanMove()` devient vrai.

### Validation UE5.5.4

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Runtime.Doors.PassageBlockedUntilFullyOpen"
~~~

Puis vérifier en PIE qu'une tentative d'avancer pendant l'ouverture produit le feedback de déplacement bloqué et que le franchissement devient possible uniquement à la fin complète de l'ouverture.

---

## Incident 005 — Selected Object / MonsterSpawn trop technique et incomplet

### Observation terrain

Lors de l'authoring d'un Rat géant en embuscade, la fenêtre **Selected Object** affichait principalement des métadonnées génériques et debug :

- catégories de palette ;
- indicateurs Interactable / Readable / Light Source ;
- classes runtime génériques ;
- meshes d'archétype vides ;
- ObjectId dupliqué.

En revanche, les informations indispensables au gameplay du `MonsterSpawn` n'étaient pas exposées correctement.

Le défaut le plus trompeur était :

~~~text
Initial State = Enabled
~~~

Cette ligne lisait en réalité `bInitiallyEnabled` et ne montrait jamais `InitialMonsterState`.

### Audit du contrat existant

Le modèle possède déjà deux niveaux distincts.

**Placement MonsterSpawn :**

~~~text
bInitiallyEnabled
InitialMonsterState = Idle | Dormant
InitialFacing
EncounterGroupId
EncounterWaveIndex
PatrolMode
PatrolWaypoints
~~~

**MonsterDefinition partagée :**

~~~text
SightRangeCells
HearingRangeCells
PrimaryAIProfile
bSharesAggroWithGroup
AggroPropagationRange
...
~~~

La perception reste volontairement propriété de la Definition. Le Selected Object affiche donc ses valeurs effectives en lecture seule au lieu de créer implicitement un override par occurrence.

### Consolidation de l'inspecteur

Pour un `MonsterSpawn`, la fenêtre est désormais organisée autour des besoins d'authoring :

~~~text
Spawn Presence
  Present at Start

Monster Spawn
  Definition
    Monster Definition
    Definition Id

  Spawn
    Initial State       Idle / Dormant
    Initial Facing

  Perception — from Monster Definition
    Sight Range
    Hearing Range
    Primary AI Profile
    Shares Aggro With Group
    Aggro Propagation Range

  Patrol
    Patrol Mode
    Waypoints
    Edit Patrol Route
    Clear Route

  Encounter — optional
    Encounter Group
    Wave Index
~~~

`Active at Start` est masqué pour les MonsterSpawns : il s'agit de l'état générique d'activation des objets et non de l'état d'exploration d'un monstre.

Les lignes génériques d'archétype sans valeur (`Runtime Actor Class=None`, meshes `None`, etc.) sont également retirées du premier plan pour ce type d'objet.

### État initial

Une nouvelle action d'éditeur :

~~~cpp
SetSelectedObjectInitialMonsterState(...)
~~~

autorise uniquement :

- `Idle` ;
- `Dormant`.

Les états runtime `Alert`, `Pursuing`, `Attacking`, `Hurt`, `Dead`, etc. restent interdits comme état de fresh spawn.

La sémantique est maintenant visible sans ambiguïté :

~~~text
Present at Start = false
    => Actor absent

Present at Start = true + Initial State = Idle
    => Actor présent et actif en exploration

Present at Start = true + Initial State = Dormant
    => Actor présent mais dormant
~~~

### Perception et embuscade

Le panneau affiche directement les valeurs effectives de la Definition.

Exemple pour une embuscade à moins de trois cases :

~~~text
Present at Start     = true
Initial State        = Dormant
Initial Facing       = direction du couloir
Sight Range          = 2
Hearing Range        = 0
Patrol Mode          = None
Encounter Group      = None
~~~

Le panneau rappelle que la vue est directionnelle et l'ouïe omnidirectionnelle.

Un avertissement apparaît également lorsqu'un monstre est `Dormant` alors que `SightRangeCells == 0` et `HearingRangeCells == 0`, car il ne peut alors pas se réveiller par perception.

### Patrouille

Les API d'authoring de route existaient déjà mais étaient pratiquement invisibles dans Selected Object.

Le panneau expose désormais le mode, le nombre de waypoints et le bouton **Edit Patrol Route**. Pendant l'édition, il rappelle les contrôles viewport existants :

~~~text
clic gauche      ajouter / sélectionner un waypoint
F                changer son Facing
+ / -            modifier l'attente
Delete           supprimer
PageUp/PageDown  réordonner
P                terminer l'édition
~~~

### Contrat runtime MON14.2 vérifié

Le contrat runtime existant était déjà implémenté dans le fichier dédié
`GridMonsterSpawnConfiguration.cpp`. L'audit initial avait manqué cette unité de traduction séparée.

Le correctif Selected Object ne modifie donc pas la logique MON14.2. Il s'appuie sur le contrat existant :

- un fresh spawn lit son `InitialMonsterState` ;
- `Dormant` atteint réellement l'Actor runtime ;
- `EncounterGroupId`, `PatrolMode` et `PatrolWaypoints` sont recopiés depuis le placement ;
- un état sauvegardé reste ensuite autoritaire lors d'un Continue.

Le doublon accidentellement ajouté dans `GridMonsterActor.cpp` a été retiré afin que
`ApplySpawnPlacementConfiguration()` ne possède qu'une seule définition native.

### Validation

Nouveau test éditeur :

~~~text
Grimrock.Editor.MonsterSpawn.InspectorAuthoringContract
~~~

Régression runtime existante à rejouer :

~~~text
Grimrock.Monsters.MON14.2.FreshSpawnConfiguration
~~~

Validation recommandée :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Editor.MonsterSpawn.InspectorAuthoringContract"
~~~

puis :

~~~powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.Monsters.MON14.2.FreshSpawnConfiguration"
~~~

Enfin, vérifier en PIE un MonsterSpawn `Present at Start=true`, `Initial State=Dormant`, orienté vers le groupe et utilisant une Definition avec `SightRangeCells=2`, `HearingRangeCells=0`.
