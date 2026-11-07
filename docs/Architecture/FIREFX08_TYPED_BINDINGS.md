# FIREFX08 — Auto-liaison typée des contrôles FireFX

## Diagnostic réellement observé dans L_Dungeon

`NS_FFX_Ground_Fire` expose 29 paramètres utilisateurs, dont
`SpawnBox`, `FlameRate`, `FlameSizeMin/Max`, `SmokeRate`, `Tint`,
`Wind`, `FlameLife`. Ils sont **présents**, mais le rapport initial ne
connaissait pas leurs types. Tous les anciens champs de liaison du
`DA_GroundFire` étaient à `None`.

## Nouveau mécanisme, versionné

`FGridFireNiagaraBindings` vérifie le nom **et le type** exposé par
le Niagara sélectionné. Le `DA_GroundFire` dispose de la propriété
`bAutoBindKnownFireControls`, vraie par défaut. Aucun chemin de pack
commercial ou paramètre inexistant n'est inventé.

Si le système possède les paramètres attendus et les types exacts :

- `SpawnBox` en **Vector3** : dimension XY = taille de cellule ×
  `GroundFireCoverage` (en cm), corrigée du facteur de taille global.
  La dimension Z native du pack est conservée pour que le feu
  naisse près du sol. Le scaling XY du composant se limite alors
  à `GroundFireScale`, évitant de compter deux fois la couverture.
- `FlameRate` en **Float** : valeur native du Niagara ×
  `FlameDensity`. Une densité à 1 conserve le nombre original
  d'émissions ; à 0, le taux devient nul.
- `SmokeRate` en **Float** : valeur native ×
  `SmokeDensity`, ou 0 quand `bEnableSmoke=false`.
  Les éventuelles émissions `SmokeBurst` internes restent à vérifier.
- `Tint` en **Color**, **Vec3** ou **Vec4** : application de
  `FireTint` avec un setter compatible.

Les valeurs natives sont lues dans le **parameter store**, non depuis les
variables exposées (qui peuvent contenir une donnée périmée).
Si un nom est absent ou de type incompatible, aucune valeur n'est
envoyée à ce contrôle ; le diagnostic l'indique.

Les autres champs facultatifs de l'ancien `DA_GroundFire` restent
possibles pour des Niagaras différents, sans clobber une liaison FireFX
automatique valide.

Le système est par **instance de composant** : torches, brasiers et
flammes au sol peuvent réutiliser un seul Niagara avec différents
profils Grimrock. Aucun package FAB n'est modifié ou committé.

## Dans l'éditeur

1. Sur l'instance `BP_GridLevelRuntimeActor` de `L_Dungeon`,
   cliquer `Log Ground Fire Niagara Bindings` (hors PIE).
2. Le dialogue montre pour chaque variable son **type** et,
   pour les Float, sa **valeur native**, ainsi que la compatibilité
   de l'adaptateur automatique.
3. Dans `DA_GroundFire`, `bAutoBindKnownFireControls=true`,
   `GroundFireCoverage=1` pour viser 200 × 200 cm (case 200 cm).
4. Tester en PIE. La largeur finale visible des flammes peut dépasser
   les dimensions de la zone de naissance si le Niagara applique une
   vélocité horizontale ; `SpawnBox` ne borne pas les positions futures.

## Ce qui reste à confirmer visuellement

- Types réels des quatre entrées : le test sans pack vérifie le comportement
  des conversions, pas le graphe propriétaire.
- `FlameSizeMin/Max` n'est pas encore modifié : aucune supposition
  sur ses types ou sa relation hauteur/largeur n'est faite.
- `Tint` peut être un multiplicateur et donner une couleur différente
  de celle d'un éclairage physiquement réaliste.
- La simulation World Space peut ignorer les transformations locales.
