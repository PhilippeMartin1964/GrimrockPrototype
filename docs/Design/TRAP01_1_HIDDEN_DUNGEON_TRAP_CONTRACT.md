# TRAP01.1 — Contrat des pièges cachés du donjon

Date : 31 octobre 2026
Statut : **code et tests publiés ; validation UE 5.5.4 locale à réaliser**.
Référence : spécification TRAP01 approuvée le 31.10.2026.
Portée : **données, contrôles de validité, invisibilité initiale et sauvegarde de l'état**, aucun nouveau déclenchement de dégâts.

## 1. Règles impératives

1. Un piège de donjon **est caché par défaut**. Sa présence n'est révélée ni par une dalle visible ni par le menu des outils.
2. La détection est explicite, distincte de la simple proximité et de la découverte d'une cellule sur la carte.
3. Le désamorçage exige la découverte préalable et un outil réel ; la difficulté ne doit pas être disponible dans l'action d'inventaire d'un piège caché.
4. Les pièges de donjon sont des instances `FGridWorldObjectInstance` de type `Trigger` avec `InstanceId` stable ; les autres `Trigger` conservent leur comportement.
5. Les pièges de donjon ne sont pas les pièges temporaires de combat `FGridCombatTrapState` stockés par cellule.
6. Le niveau reste l'unique asset de placement. Pas de nouvel acteur sauvegardé par piège ni de map parallèle.
7. Les risques et leur état persistent par **niveau et GUID**, à travers le même pipeline `FGridDungeonRuntimeState`.

## 2. Définition unique, réutilisable

Nouvel en-tête : `Public/Core/GridDungeonTrapTypes.h`, contenant `FGridDungeonTrapDefinitionParams`.
Le `UGridWorldObjectDefinitionAsset` possède un champ `DungeonTrap`, opt-in `bIsDungeonTrap=false`.
Ce champ n'affecte aucun asset historique.

| Champ | Valeur par défaut | Validation |
|---|---|---|
| `bIsDungeonTrap` | `false` | si `true`, `SupportedType=Trigger` |
| `bInitiallyHidden` | `true` | doit rester `true` pour TRAP01 |
| `DetectionSense` | `Visual` | Visual / Audible / Both |
| `DetectionDifficulty` | 10 | 1 à 40 |
| `MaxDetectionRangeCells` | 4 | 1 à 4 ; plafond, pas portée garantie |
| `ActivationPolicy` | `OnEnterCell` | OnEnterCell / OnCrossBoundary / OnInteract |
| `bOneShot` | `true` | réarmement ultérieur hors TRAP01.1 |
| `DamageType` | `Physical` | enum `EGridDamageType` existant |
| `PhysicalSubtype` | `Piercing` | enum `EGridPhysicalDamageSubtype` existant |
| `MinimumDamage` / `MaximumDamage` | 4 / 8 | entiers non négatifs, minimum ≤ maximum |
| `bAffectsEntireParty` | `true` | traitement individuel en TRAP01.3 |
| `RequiredToolTag` | `Tool.Disarm` | non vide si `bCanBeDisarmed` |

Les champs **déjà existants** `bCanBeDisarmed` et `DisarmDifficulty`
restent les références du désamorçage ARMEMENT01.16 en attendant un audit/migration dédié.
Pour être désamorçable, une définition doit également avoir `DungeonTrap.bIsDungeonTrap=true`.

La surface d'attache ne se duplique pas : `PlacementSurface` du même
`UGridWorldObjectDefinitionAsset` est déjà un `EGridObjectPlacementKind` (Floor/Wall/Ceiling).
L'orientation est portée par `FGridWorldObjectInstance::WallSide`, avec les overrides de placement existants.

Le type de dégâts `Physical` regroupe aujourd'hui Slashing/Piercing/Bludgeoning sous
une résistance physique commune. **Ne pas inventer des résistances individuelles
à ces sous-types**. Les dégâts n'exécutent encore aucun effet à TRAP01.1.

## 3. Visibilité et rendu

`AGridLevelRuntimeActor::IsRuntimeSpawnableObject` exclut les définitions
`DungeonTrap.bIsDungeonTrap && bInitiallyHidden` de l'apparition d'un acteur
visuel au rebuild du niveau. Le piège reste **un objet logique indexé dans
l'asset de niveau**. Les objets `Trigger` ordinaires ne sont pas exclus.

Les meshes, parties mobiles, particules et sons des pièges déclenchés ne sont
pas instanciés par TRAP01.1. TRAP01.6 définira leur cycle de vie visuel et
destruction. Pour une première recette PIE, aucune dalle visible n'est requise.

## 4. État SaveGame du niveau

`FGridLevelRuntimeState` conserve les collections existantes :
- `DetectedTriggerTrapIds` (connaissance du groupe ; ARMEMENT01.16 durci).
- `DisarmedTriggerTrapIds` (neutralisation réussie et persistée).

Et ajoute :
- `TriggeredTriggerTrapIds` (activation observée ; non consommé par TRAP01.1).
- `DisabledTriggerTrapIds` (désactivation par événement/énigme).
- `TrapPerceptionAttemptedRangeMask` (GUID → `uint8` ; distances 1–4 déjà testées, exploité en TRAP01.2).

`EGridDungeonTrapState` fournit le modèle de lecture `Unknown / Hidden /
Detected / Triggered / Disarmed / Disabled`. Précédence si plusieurs marqueurs
existent : **Disabled > Disarmed > Triggered > Detected > Hidden**.
Il s'agit d'un état **dérivé** des collections sauvegardées, sans duplicata
d'autorité.

API d'accès C++ :
- `GetDungeonTrapState(InstanceGuid)` : interroge le type, l'opt-in,
  la définition et les marqueurs persistants.
- `TryRecordDungeonTrapDetection(InstanceGuid)` : seule écriture
  d'une nouvelle découverte, refuse les GUID inexistants et les
  doublons. **TRAP01.2 en sera le seul caller gameplay**.
- `IsTriggerTrapDetected`, `GetTriggerTrapDisarmDifficulty` et
  `FindDisarmableTriggerTrapForParty` sont déjà présents et ne
  divulguent pas un piège inconnu dans l'interface.

**Attention aux anciens saves** : les collections ajoutées seront vides.
Un trigger historique reste non piégé. Le nouveau modèle est append-only.

## 5. Limites assumées de TRAP01.1

- **Pas encore de perception passive** : la découverte doit être injectée
  par les tests. Ne pas en déduire que les personnages découvrent déjà un piège.
- **Pas encore d'activation dommageable** sur `OnEnterCell`. Les champs
  DamageType, Min/Max et bOneShot sont uniquement des définitions.
- **Pas encore de résolution d'esquive ni de résistances du groupe** :
  TRAP01.3 réutilisera `FGridCombatResolver::ResolveDirectDamage`.
- **Pas encore de sélection du meilleur personnage** ni changement de
  la caractéristique de `Skill_Traps` (actuellement Intelligence) : TRAP01.4
  adoptera la Dextérité après tests RPG transversaux.
- **Pas encore de test complet Save/Continue sur fichier** : les tests
  couvrent les propriétés `SaveGame` et un snapshot restauré en mémoire.
- **Pas encore de réglage dans l'inspecteur personnalisé Grid Editor** :
  les nouveaux champs existent sur le DataAsset et seront intégrés à
  l'interface d'édition en TRAP01.5.

## 6. Plan de tests / validation

Commande :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.TRAP01.1"
```

Deux tests :
- `Grimrock.TRAP01.1.DefinitionContract` : opt-in, invisibilité, surface,
  détection, bornes des dégâts, type physique.
- `Grimrock.TRAP01.1.PersistentInstanceStates` : visibilité initiale,
  découverte unique, protections du désamorçage, toutes les étapes
  d'état, flags `SaveGame`, copie/restauration par niveau.

Et regressions précédentes :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ARMEMENT01.16.Trap"
```

Ces filtres doivent être validés localement avant de commencer TRAP01.2.
Aucun fichier binaire `.uasset`, objet de palette ou map `L_Dungeon`
n'est modifié automatiquement par ce jalon.

## 7. Correctif de régression du 31.10.2026

La validation locale de `Grimrock.TRAP01.1` a réussi (2/2, 0 avertissement).
Le filtre `Grimrock.RPG.ARMEMENT01.16.Trap` a échoué dans
`ContextAndTargeting` : le test d'ambiguïté ajoutait un second piège dans
la même case sans enregistrer sa découverte. Avec le verrou de confidentialité
introduit dans ARMEMENT01.16.2A/2B, **un piège inconnu ne fait pas partie des
cibles interactives**. Le runtime sélectionnait correctement le seul piège connu.

Le test corrigé couvre désormais explicitement les deux situations :

- Un piège détecté et un piège caché dans la même case : seul le piège
  détecté est sélectionnable ; le second reste inconnu.
- Deux pièges **détectés** dans la même case : la sélection est ambiguë,
  l'identifiant de cible est invalidé et l'action de désamorçage n'est pas proposée.

Aucune logique de runtime ni aucune règle de gameplay n'est modifiée par
ce correctif ; seul le test de régression est adapté au contrat approuvé.
Relancer les filtres ARMEMENT01.16.Trap sans recompiler les assets.
