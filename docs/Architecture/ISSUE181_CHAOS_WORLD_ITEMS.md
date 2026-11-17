# Issue #181 — réouverture après échec PIE de f6b951db

**Issue ouverte jusqu'à confirmation PIE du propriétaire.** Les résultats de
la première correction ci-dessous sont historiques : ils ne prouvaient ni
une chute physique après ouverture ni le parcours animé du groupe.
[Retour PIE et mission de reprise](https://github.com/PhilippeMartin1964/GrimrockPrototype/issues/181#issuecomment-6942856629).

## Audit de reprise, avant modification comportementale

Départ : `master`, dépôt propre, `f6b951db1962d68f29635047c5730fefdb3d9516`.
Deux reproductions avec **D3D12, monde PIE et Chaos**, carte temporaire non
sauvegardée, géométrie de `L_Dungeon`, huit placements réels de
`DA_GridLevel_00`, sans slot de sauvegarde personnel. Les entrées de mouvement
passent par l'injection Enhanced Input dans les actions de déplacement ;
aucun appel direct à `TryBeginPitFallAtCell` ne remplace cette marche.

Preuves conservées localement dans `Grimrock-Issue181-Reopened`, journaux
`repro-red.log` et `repro-pie-red3.log` :

| Observation sur f6b951db | Résultat mesuré |
|---|---|
| Marche North, fosse déjà ouverte | `(27,23)` → `(27,24)`, Z=110, `IsPitFalling=false`, génération runtime=1 |
| Hache réelle après basculement naturel | GUID `6D9744564FDCB7EC69B465961254F723`, pivot X=5634, centre X=5556 avant ouverture |
| Quatre secondes après ouverture | centre Z=−7409 cm, vitesse Z=−3753 cm/s, aucun transfert |
| Huit secondes après ouverture | centre Z=−29696 cm, objet toujours au-dessous du monde, pas d'arrivée accessible |

Causes distinctes, lignes sur **f6b951db** :

1. `GridLevelRuntimeActor.cpp:1629`, `TryBeginPitFallAtCell` : l'option
   `bEnableSameLevelPitRelocation=false` interdit la chute. Avec `true`,
   `RelocatePartyWithinCurrentLevel` court-circuite `BeginPitFall` et son
   animation. `GrimrockPartyPawnMovement.cpp:619`, `UpdatePitFall`, ne
   prévoit ensuite qu'un voyage inter-niveaux pour terminer une vraie chute.
2. `GridLevelRuntimeActor.cpp:1485` appelle `DropWorldItemsThroughOpenPitAtCell`
   une seule fois, avant suppression du plancher. `WorldItems.cpp:670`
   sélectionne par cellule persistée ou centre des bounds. Un corps arrivant
   ensuite n'est jamais revu. Les acteurs runtime/items n'ont pas de Tick.
   Activer l'ancienne option une seconde après ouverture ne reprend aucun corps.
3. Le transfert local (`WorldItems.cpp:419`) applique un delta Z nul : un
   corps capturé après descente arriverait sous le plancher cible.
4. Le projectile a son propre Tick. `GridThrownItemActor.cpp`,
   `ConvertToWorldPickupAtImpact`, rejoint le dépôt à l'impact/expiration,
   puis détruit le projectile ; aucun callback ne suit ensuite son pickup
   Chaos. Ce chemin ne couvre pas non plus un projectile passant dans le
   trou avant son expiration.

Le premier audit du parent avait déjà établi l'autre filtre fautif : les
huit entrées existent, `Edge=None`, acteurs et identités valides, mais les
pivots restent en `(28,24)`. Le correctif du ramassage par proximité de la
collision reste pertinent ; remplacer un pivot par un centre ne suffit pas
à détecter le franchissement d'une fosse.

## Correction ciblée et preuves

La destination authorée explicite `Into_The_Dark (27,24) → Into_The_Dark
(27,23)` définit le cas local, sans option globale cachée. Une destination
locale identique à la source, bloquée ou contenant une fosse ouverte reste
refusée. Le groupe conserve `BeginPitFall → UpdatePitFall`, puis atterrit
par l'autorité de relocation locale existante, sans reconstruction ; son
impact caméra et sonore suit l'atterrissage. La chute inter-niveaux garde
son voyage habituel.

Les objets physiques doivent franchir une zone **sous** le plancher avec
une descente effective. Le centre/pivot n'est pas un test d'emprise. Le
suivi doit être limité aux fosses ouvertes, porter sur les corps de collision
et conserver l'identité canonique avant toute suppression. À destination
locale, la forme doit être remontée au-dessus du sol. Aucune file locale
orpheline, migration ou modification de `.uasset/.umap/.sav` n'est requise.

Régressions rouges ajoutées avant le correctif comportemental :
`Grimrock.Pit.Issue181.LocalWalkFallLifecycle` et
`Grimrock.Pit.Issue181.AfterOpenChaosPassage`. Elles utilisent le chemin des
placements réels ; la seconde fait tourner plusieurs frames Chaos après
l'ouverture. Les tests précédents doivent attendre la descente physique au
lieu d'exiger un transfert instantané d'un corps qui n'est pas encore tombé.

La validation technique, le scénario automatisé avec rendu et la recette
PIE du propriétaire restent trois preuves distinctes. Ne pas clore l'issue
par un message de commit. Aucun nouveau texte joueur ni authoring UMG.

## Mécanisme retenu

`CheckOpenPitItemPassages` interroge la collision réelle (narrow phase)
dans un volume sous chaque fosse ouverte, toutes les 50 ms : sommet à
−20 cm, profondeur au moins 800 cm, vitesse verticale du corps < −10 cm/s.
Un bout d'arme au-dessus du bord et un corps soutenu au repos ne suffisent
pas. Ni pivot ni centre XY ne filtrent cette détection. Il n'y a pas de
Tick ajouté aux objets ni de balayage de toutes les armes chaque frame.
Le timer s'arrête sans fosse ouverte et est réinitialisé à la reconstruction,
à la restauration d'état et à EndPlay. Le niveau courant, l'acteur inscrit
et son GUID sont contrôlés avant transfert.

`TransferWorldItemThroughPit` extrait le transfert déjà existant, avec sa
résolution canonique et ses états mutables. L'acteur source n'est supprimé
qu'après succès du routage inter-niveaux. Le transfert local conserve le
même acteur et place le bas de sa forme au-dessus du plancher cible ; il
n'utilise jamais `PendingInboundItems`. Le Tick existant du projectile
couvre aussi le passage dans le trou avant impact/expiration, puis rejoint
cette même autorité de routage. Les dépôts directs conservent leur chemin.

L'ancien `bEnableSameLevelPitRelocation` est supprimé. La destination
explicitement authorée dans le niveau courant est suffisante, avec les
gardes de praticabilité, de boucle et de fosse ouverte à destination.
Le contrôle TRAP-PIT01 utilise déjà cette autorité : aucune seconde
interprétation du même réglage n'est ajoutée au piège.

## Validation de reprise

Le passage rouge est conservé dans
`red-physics-frames/TD04-20261117-101942` : anciens trois tests verts,
**deux nouveaux tests rouges**. L'arme termine à Z=−3308 cm, vitesse
Z=−2528 cm/s ; les deux modes de l'ancienne option échouent pour le groupe
(blocage ou téléportation sans animation). Pour les frames physiques
successives, le fixture avance/restaure `GFrameCounter`, comme le
`FTestWorldWrapper` d'UE ; le Pawn démarre après effacement de son slot de
sauvegarde. L'ancienne préparation ne prouvait pas ce déroulement temporel.

Validation locale finale UE 5.5.4 / v143 14.44.35207 : compilation réussie,
`Grimrock.Pit` : 15 succès (dont 7 avec avertissements), `Grimrock.Trap` :
24 succès, suite `Grimrock` : **1968 tests, zéro échec** (1945 succès et
23 succès avec avertissements). Les sept fixtures Issue181 démarrent un
Pawn sans contrôleur/UI : leur avertissement `NoPlayerController` est
identifié, distinct des assertions de physique et de mouvement. Rapports
`pit-final/TD04-20261117-130343`, `trap-final/TD04-20261117-130343` et
`full-final/TD04-20261117-130213`. Les deux statuts Windows doivent encore
être relevés sur le SHA exact publié après le commit atomique.
Les sept tests `Grimrock.Pit.Issue181` couvrent :

- création par les placements réels, collision/ramassage au mesh final ;
- ouverture par désamorçage raté sous le groupe, début de chute unique ;
- marche sur fosse déjà ouverte et ouverture par piège à l'entrée ;
- descente intermédiaire, arrivée, impact/récupération caméra, pas de rebuild ;
- corps arrivant après ouverture, pointe soutenue sans transfert ;
- forme longue franchissant le volume avec pivot/centre initial voisins ;
- projectile réel passant dans le trou avant expiration ;
- identités N1/N20, file inter-niveaux séparée, SaveGame en mémoire.

Les scènes **PIE D3D12** `green-manual.log`, `green-step.log` et
`green-disarm.log` ont chacune vérifié la chute animée du groupe vers
`(27,23)`, la hache basculée naturellement depuis les placements, une dague
mise en mouvement physiquement **deux secondes après ouverture**, son
arrivée et son ramassage, huit GUID uniques, génération runtime restée à 1.
Le désamorçage utilise la vraie compétence, les outils et l'interaction du
piège (`Total=12`, `DC=30`, `Triggered=true` dans la scène conservée).
`green-pickup.log` vérifie aussi un rayon Visibility complexe jusqu'à la
collision finale, `CanInteract=true`, puis un seul GUID/N1/quantité=1 dans
l'inventaire. Ce sont des entrées et interactions automatisées en PIE ;
aucun clic souris manuel ni validation audible des sons n'est revendiqué.

La simulation Chaos n'est pas déterministe : les poses et le nombre
exact d'armes basculant dans le trou varient. Les tests à pose contrôlée
isolent la géométrie ; les scènes avec basculement naturel sont une preuve
complémentaire. Le volume échantillonné n'est pas un détecteur continu de
trajectoires arbitrairement rapides : les vitesses de gameplay observées
sont couvertes, aucune preuve pour des corps traversant ses 800 cm entre
deux échantillons n'est revendiquée.

Aucun `.uasset`, `.umap` ou `.sav` modifié. La destination volontaire
Into_The_Dark `(27,24) → (27,23)` est conservée. Aucun nouveau texte joueur
ni authoring UMG : UI.csv → ST_UI → WBP → Game/fr/en inchangé, message de
relocation existant réutilisé. **La recette PIE du propriétaire reste
requise et l'issue #181 doit rester ouverte.**