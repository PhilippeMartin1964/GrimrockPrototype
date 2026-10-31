# TRAP01.2 — Détection passive des pièges cachés et révélation 3D

**Date :** 31 octobre 2026
**État :** implémentation C++ sur `master`, en attente de validation UE 5.5.4 locale.
**Pré-requis validés :** TRAP01.1 (2/2), régressions ARMEMENT01.16.Trap (3/3).

## Principes validés

Un piège armé est **invisible** tant qu'il n'a pas été détecté ou déclenché.
La détection ne désarme **pas** le piège et n'empêche **pas** un futur déclenchement.
Le joueur ne peut pas provoquer de nouveau jet de perception à chaque clic
ou déplacement aller-retour : une tentative par GUID et distance est mémorisée.

Un piège **détecté**, **déclenché**, **désarmé** ou **désactivé** est un
danger désormais **connu**. Le système peut matérialiser sa géométrie
en jeu, sans changer sa définition ou l'asset du niveau.

## Détection passive

Évaluation **événementielle** : à la fin du mouvement sur grille ou lors
du premier placement stable du groupe, après les événements d'entrée/sortie
existants. Pas de Tick permanent, pas de raccourci « Chercher » requis.

Données : `DungeonTrap.DetectionSense`, `DetectionDifficulty`,
`MaxDetectionRangeCells` et `bInitiallyHidden` (TRAP01.1).

Compétence de production `DA_Skill_Perception` :
- rang `Skill_Perception` et modificateur de **Sagesse** ;
- meilleur personnage **vivant** en score effectif, égalité départagée
  par rang puis index stable du groupe ;
- un seul vrai d20 via `FRPGSkillCheckService` sur la compétence de production ;
- score de sélection = rang + modificateur Sagesse ;
  le résolveur est l'autorité finale sur les éventuels modificateurs de classe ;
- `DC_effective = DetectionDifficulty + 2 × (distance − 1) + pénalité obscurité`.
  La pénalité vaut +2 pour un indice exclusivement visuel sans lumière.

Portée maximale proposée en v0.1 :

| Score de sélection | Avec lumière | Sans lumière, indice visuel |
|---|---:|---:|
| ≤ 1 | 1 | 1 |
| 2–3 | 2 | 1 |
| 4–5 | 3 | 2 |
| ≥ 6 | 4 | 3 |

La portée audible n'a pas de pénalité d'obscurité. Les indices mixtes
(`Both`) bénéficient actuellement de la portée audible. L'obscurité
est déterminée à partir de `UGridPartyIlluminationComponent`,
avec les sources lumineuses d'équipement déjà existantes.

**Topologie v0.1 :** uniquement des axes cardinaux droits, 1 à 4 cases ;
les diagonales, murs pleins et portes bloquantes empêchent un jet.
Utilisation de `FGridMapRevealService::CanRevealAcrossEdge`, identique
à la topologie du reveal de la carte. Pas de détection autour d'un angle.
Le chemin auditif par des portes fermées ou des interstices pourra
être étendu plus tard, sans modifier ce contrat.

**Anti-reroll** : masque `TrapPerceptionAttemptedRangeMask[InstanceGuid]`
par bits 0..3 pour les distances 1..4. Enregistré avant chaque jet de
d20 autorisé. Réexaminer une autre distance déclenche au maximum une
nouvelle tentative. La détection n'est pas relancée à la rotation seule.
Une cible en case 0 (sous les pieds) n'est **pas détectée avant un
déclenchement** ; la résolution offensive arrive avec TRAP01.3.

## Affichage 3D après découverte ou déclenchement

La propriété `DungeonTrap.RevealedOverlayMaterial` est une **référence
de présentation** (optionnelle) ajoutée à la définition.

Après découverte :
1. `TryRecordDungeonTrapDetection` ajoute le GUID à
   `DetectedTriggerTrapIds` ;
2. `RefreshDungeonTrapRevealVisual` crée, si les assets le permettent,
   un `AGridRuntimeObjectActor` standard à la transformation du piège ;
3. le composant mesh utilise Custom Depth et **stencil 252** ;
4. si assigné, le matériau `RevealedOverlayMaterial` crée l'effet
   rouge semi-transparent en **Overlay Material** ;
5. `RestoreDungeonTrapRevealVisuals` restaure les visuels sur
   `RebuildRuntimeObjects` et `ApplyCurrentLevelRuntimeState`.

Les autres objets `Trigger` ne changent pas. Le mécanisme du piège
reste attaché à son GUID d'instance : le mesh ne porte **aucune**
autorité de dégâts, de sauvegarde ni de perception.

### Assets graphiques à fournir localement dans UE

- **Mesh de chaque type de piège** : `SM_Trap_FloorDisarmable_01`
  (ou équivalent Wall/Ceiling plus tard) dans
  `UGridWorldObjectDefinitionAsset::StaticPart.Mesh`.
- **Matériau de révélation** (à créer) :
  `M_Trap_RevealOverlay_Red`, domaine `Surface`, Blend Mode
  `Translucent`, couleur rouge atténuée, faible opacité et absence
  d'éclairage excessif ; à assigner à `DungeonTrap.RevealedOverlayMaterial`.
- **Classe runtime visuelle** :
  `RuntimeActorClass` doit pointer sur une classe dérivée de
  `AGridRuntimeObjectActor` (par exemple `AGridGenericObjectActor`).
  La classe et le Static Mesh sont utilisés **uniquement après révélation**.

**Sans mesh ou RuntimeActorClass**, la découverte demeure effective dans
le gameplay et le flux d'événements, mais l'effet 3D n'apparaît pas.
**Sans overlay**, le mesh révélé est visible avec son matériau d'origine,
avec CustomDepthStencil 252 disponible pour un éventuel postprocess.
Aucun `.uasset` graphique n'est généré automatiquement par le commit distant.

Pour un piège activé, `TryRecordDungeonTrapTriggered` est le point
d'entrée de la révélation 3D et de son événement. TRAP01.3 l'appellera
**après** calcul et engagement des dégâts. On ne simule pas une explosion
sans dégâts dans TRAP01.2.

## Flux d'événements — localisation relative

Une vraie découverte émet `Dungeon.Trap.Detected` **une seule fois** :

> « Observatrice repère un piège sur un mur, à 2 cases, sur votre droite. »

La direction est relative à `PartyPawn->Facing` et non aux axes X/Y
bruts. Surfaces : sol, mur, plafond ; directions : devant, derrière,
gauche, droite ; distance en cases. L'identité de l'observateur vient
du groupe, pas d'une chaîne inventée.

Lors d'un déclenchement enregistré, un piège non découvert est
immédiatement marqué comme **connu** et devient visuellement révélable.
L'événement `Dungeon.Trap.Triggered` annonce le lieu du danger, y
compris `sous le groupe` si le groupe vient de marcher sur la case.

Les pièges encore cachés ne doivent **jamais** apparaître dans le flux
d'événements, ni dans le menu des outils : ARMEMENT01.16.2A/2B reste
sous verrou de découverte.

## Prudence : activation désactivée pendant TRAP01.2

Les `Trigger` explicitement qualifiés comme `DungeonTrap` n'exécutent
plus leur ancien chemin `Activated/Deactivated` générique dans
`GridActivationComponent`. Cette restriction temporaire empêche
qu'un piège se déclenche partiellement (sans dégât) avant TRAP01.3.
Les triggers ordinaires restent inchangés.

## Tests

`Grimrock.TRAP01.2.ObserverAndRange` : meilleur observateur, tie-break,
score, lumière, sens audible, plafond de portée, axes cardinaux et
formulation directionnelle.

`Grimrock.TRAP01.2.RuntimeDetectionAndPersistence` : monde de jeu,
vraie compétence, piège et mage/observateur de test, mur bloquant,
découverte avec d20, anti-reroll, création d'un vrai mesh 3D lors de la
révélation, CustomDepth/stencil, déclenchement explicite, garde
`bOneShot`, conservation des états et tentatives au snapshot.

Commande :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TRAP01.2"
```

Puis régressions :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.TRAP01.1"

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -SkipBuild `
    -AutomationFilter "Grimrock.RPG.ARMEMENT01.16.Trap"
```

**La visualisation définitive en PIE dépend encore de l'attribution
des assets graphiques locaux** et la résolution des dégâts du groupe
reste le jalon TRAP01.3.
