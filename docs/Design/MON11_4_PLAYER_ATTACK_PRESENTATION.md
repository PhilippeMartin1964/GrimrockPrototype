# MON11.4 — Présentation des attaques du groupe

## Séparation gameplay et présentation

MON11.4 observe le pipeline déterministe de MON11.3 sans le modifier.
`FGridCombatResolver` reste l’unique propriétaire des formules et le
`UGridTurnManagerComponent` conserve l’ordre synchrone :

1. résolution gameplay ;
2. diffusion `OnPlayerAttackRequested` ;
3. journal `AttackHit` ou `AttackMiss` ;
4. `ApplyAttackResult()` et réaction existante du monstre ;
5. diffusion `OnPlayerAttackResolved` ;
6. victoire éventuelle.

Le composant de présentation ne tire aucun nombre du flux de combat, ne
modifie ni dégâts ni phase, et ne déclenche jamais directement Hurt, Death,
MonsterDefeated ou Victory. MON11.4.1 ajoute une exception limitée à
l’inventaire : une arme réellement jetée est transférée de l’équipement vers
un objet récupérable dans le monde. Cette mutation ne participe jamais à la
résolution du hit ou des dégâts.

## Composant natif

`UGridPlayerAttackPresentationComponent` est un sous-objet natif de
`AGridLevelRuntimeActor`. Aucun ajout dans `BP_GridLevelRuntimeActor` n’est
requis. Il recherche le gestionnaire de tours du même Actor et écoute :

- `OnPlayerAttackRequested` ;
- `OnPlayerAttackResolved` ;
- `OnPlayerAttackRejected` ;
- `OnCombatEnded`.

Il diffuse une requête `Attack` à la demande acceptée, puis exactement une
requête `ImpactHit` ou `ImpactMiss` au résultat. Le profil est mémorisé par
`RequestId` entre les deux événements.

## Profil orienté données

`FGridPlayerAttackPresentationProfile` contient :

- le style `None`, `Swing`, `Thrust`, `Throw` ou `Cast` ;
- l’activation et la durée du mouvement de l’objet tenu ;
- les offsets de position et rotation au sommet ;
- trois définitions audio facultatives ;
- trois définitions Niagara facultatives ;
- la durée du feedback.

Une définition audio contient des variantes, un volume et un intervalle de
pitch. Une définition VFX contient des variantes, une règle d’attachement, un
socket et un transform. Les listes vides sont valides.

La sélection de variante et de pitch utilise un `FRandomStream` local dérivé
du personnage, de l’attaque, de l’événement et de son occurrence. Elle est
déterministe et indépendante de `CombatRandomStream`. Seule la variante
retenue est chargée.

## Intégration par ItemDefinitionId

`UGridItemDefinitionAsset` expose
`bProvidesAttackPresentation` et `PlayerAttackPresentationProfile`.
La présentation est retrouvée avec
`FGridPlayerAttackRequest::OffensiveItemDefinitionId`. Elle ne fait pas partie
de `FGridItemInstance` et n’est pas sauvegardée.

Un profil absent ou invalide n’empêche jamais l’attaque gameplay. Pour
`Attack_Unarmed`, le composant utilise un profil interne `Thrust`, durée
0,15 seconde, sans mouvement d’objet ni média obligatoire.

## Audio et VFX

L’audio source est positionné à la caméra du groupe, avec repli sur le Pawn.
L’audio d’impact est positionné sur le monstre. Le rendu natif utilise
`UGameplayStatics::PlaySoundAtLocation` et peut être désactivé sans supprimer
les requêtes.

Le VFX d’attaque s’attache au visuel tenu lorsqu’il correspond à
`OffensiveItemDefinitionId`, sinon à la caméra puis au RootComponent du
groupe. Les impacts sont produits dans le monde, à la position du monstre et
orientés du groupe vers la cible. La collision du projectile d’objet de
MON11.4.1 décide uniquement où l’objet récupérable termine sa course. Elle
n’appelle ni `FGridCombatResolver`, ni `ApplyAttackResult()`.

Les composants Niagara sont auto-détruits et conservés uniquement par
références faibles. Ils sont arrêtés à la fin du combat et pendant le
nettoyage runtime.

## Mouvement de l’objet tenu

Pour `Swing`, `Thrust` et `Cast`, le Tick du composant est désactivé par
défaut et actif uniquement pendant un mouvement. Le transform relatif initial
de `HeldItemActor` est mémorisé. L’interpolation suit :

`MotionAlpha = sin(PI × Alpha)`

La position et la rotation vont du transform initial au pic, puis reviennent
exactement au transform initial. Le mouvement est refusé si l’item ne
correspond pas, si l’attaquant n’est pas sélectionné ou pour
`Attack_Unarmed`.

Depuis MON11.4.1, `Throw` n’utilise jamais cette interpolation et ne revient
donc jamais comme un boomerang. Une unité équipée est portée par un
`AGridThrownItemActor`. Sur un hit, le projectile s’arrête à la cible et tombe
dans sa cellule ; sur un miss, il poursuit sa trajectoire jusqu’au décor, au
sol ou à l’expiration. Il devient alors un objet récupérable.

`EquipHeldItem()` résout désormais la définition réelle et utilise
`EquippedMesh`, avec repli sur `WorldMesh`. `HeldItemActor` reste un visuel :
la propriété réelle demeure dans `UGridPartyInventoryComponent`.

## Feedback accepté et refusé

Les résultats produisent les outcomes `Miss`, `Hit`, `CriticalHit` ou
`TargetDefeated`. Le texte localisable indique l’attaquant, la cible et les
dégâts. Le détail n’affiche que les valeurs non nulles d’armure physique,
d’armure magique et de points de vie, lues directement depuis
`FGridAttackResult`.

Chaque refus diffuse `OnPlayerAttackRejected` une seule fois et produit un
feedback français `Rejected`. Il ne produit aucun événement Attack/Impact,
son, Niagara ou mouvement. Le canal visuel de
`AGridLevelRuntimeActor` possède son widget et son timer propres, distincts
des messages lisibles et du feedback d’interaction. Il réutilise
`UReadableMessageWidget` avec l’ordre de classe :

1. `CombatFeedbackWidgetClass` ;
2. `InteractionFeedbackWidgetClass` ;
3. `ReadableMessageWidgetClass`.

L’absence de classe ne bloque ni delegate, ni compteur, ni journal.

## Réactions de la cible

`ApplyAttackResult()` continue à déclencher la réaction Hurt existante sur un
hit non mortel. Une mort déclenche une seule fois DeathAudio et DeathVFX,
sans Hurt supplémentaire. Un miss ne déclenche ni Hurt ni Death. La
présentation du groupe ajoute seulement l’impact joueur et le feedback.

## Configuration du shuriken

`DA_Weapon_Shuriken` conserve intégralement son profil MON11.3 et ajoute :

| Donnée | Valeur |
| --- | --- |
| `bProvidesAttackPresentation` | `true` |
| `MotionStyle` | `Throw` |
| `bAnimateHeldItem` | `true` |
| `MotionDurationSeconds` | `0.22` |
| `PeakLocationOffset` | `(30, 0, -4)` |
| `PeakRotationOffset` | `(-20, 0, 135)` |
| `FeedbackDurationSeconds` | `1.35` |

Aucun son ou Niagara factice n’est assigné. Les listes média restent vides si
aucun asset spécifiquement approprié au shuriken n’est disponible.

## Tests

Sept tests transitoires, sans chargement d’asset Content, couvrent :

- validation du profil et indépendance du gameplay ;
- ordre Attack/Impact/feedback ;
- séparation des médias Attack, hit et miss ;
- mouvement et restauration de l’objet tenu ;
- textes de feedback et refus atomiques ;
- exclusivité des réactions Hurt/Death et absence de doublon.
- préférence `EquippedMesh`, transfert atomique d’une unité, absence de
  mouvement boomerang, création du projectile et conversion en pickup.

## Procédure PIE

1. Équiper le shuriken existant en `MainHand`.
2. Placer le Rat géant à deux ou trois cellules dans l’axe.
3. Démarrer le combat et attendre `PlayerPhase`.
4. Appuyer sur NumPad 7 et vérifier `Attack_Shuriken`, `Throw`, Attack puis
   Impact, le départ réel du shuriken et le texte français.
5. Vérifier `ThrownItemLaunchRequests=1`,
   `ThrownItemLaunchStarted=true`, `ThrownItemLaunchCount=1` et
   `HeldItemMotionStarted=false`.
6. Vérifier l’armure puis les PV sur hit, et l’absence de Hurt/Death sur miss.
7. Vérifier que la pile équipée diminue d’une unité et que le shuriken devient
   récupérable dans le monde.
8. Tuer le Rat et vérifier une seule réaction Death avant Victory.
9. Réessayer avec le même personnage et vérifier
   `AttackerAlreadyActed`.
10. Interposer un mur et vérifier `PassageBlocked` sans média, projectile ni
    mutation d’inventaire.
11. Retirer le shuriken et vérifier `Attack_Unarmed` adjacent sans mouvement.

Si les listes média sont vides, les diagnostics doivent indiquer
`Sound=None` et `Niagara=None` ; mouvement et feedback restent fonctionnels.

## Limites différées

MON11.4.1 ajoute un projectile d’objet et la consommation intrinsèque d’une
unité de l’arme jetée. Ce projectile n’est jamais autorité de combat.

Restent absents : projectile de dégâts, seconde résolution, munition séparée,
dual wield, seconde attaque, Montage de personnage, délai avant dégâts, hit
stop, camera shake, HUD complet, musique, voix, InputAction, chargement
asynchrone ou donnée de présentation sauvegardée.
