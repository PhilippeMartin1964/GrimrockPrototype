# ARM-CLEAN06.4X.16.3 — Corriger l'édition du contenu initial des réceptacles

Parent : X.16.2 `adaf7f6cf6fed2eb4d5cc86feb6d922846739151`,
validé par `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`.

## Défaut avéré

Dans `SGridEditorObjectInspectorPanel::BuildReceptacleBehaviorSection`,
le choix d'un autre `ItemDefinition` dans
`ReceptacleInitialContent` ne changeait pas le
`CanonicalItemId` déjà persisté et réinitialisait
toujours `ItemLevel=0`. Depuis X.9.1/X.9.2, cela
peut produire un couple incohérent et faire
refuser le contenu au démarrage.

## Correctif

Le nouveau helper `FGridEditorCanonicalReceptacleInitialAuthoring`
écrit **ensemble** l'identité de l'item et son niveau
physique lorsqu'on change réellement la définition :
- un objet non-ARM canonique reste au vrai **N0** ;
- un ARM canonique reçoit son
  `DefaultWeaponItemLevel` N1–N20, validé dans
  `UGridItemSubsystem`, sans inventer de N1 ;
- le niveau ARM peut ensuite être édité à N1–N20,
  mais jamais à N0 une fois canonique ;
- les identités inconnues restent explicitement
  legacy-only, sans produire de faux ItemId ;
- changer la source ou l'effacer ne conserve jamais
  l'ancien `CanonicalItemId` ni l'ancien niveau ;
- la quantité reste intacte.

Le changement n'agit que lors d'une modification
réelle de la définition, donc une simple réouverture
de Selected Object ne réinitialise pas un niveau
personnalisé. Le tooltip du niveau distingue
désormais ARM N1–N20 et non-ARM N0.

## Validation

`Grimrock.RPG.ARM.CLEAN06_4X163.PhysicalReceptacleInitialItemAuthoring`
vérifie N0 réel, changement vers ARM N7, édition
N20, refus des N0 ARM/N1 non-ARM, valeurs par défaut
invalides, source inconnue, effacement, quantité et
source contradictoire.

Ne pas relancer la conversion des 18 contenus
initiaux existants : ils sont déjà persistés.
Aucun asset natif (`.uasset`, `.umap`, `.sav`)
ou Blueprint n'est modifié par ce jalon.
