# FIREFX06 — Hauteur et couverture garanties sans User Parameters du pack FAB

## Défaut et correction

Auparavant `FlameHeightCm` et `GroundFireCoverage` étaient exposés dans
`DA_GroundFire`, mais ne modifiaient pas l'apparence lorsque les champs
`FlameHeightParameter` et `SpawnAreaParameter` étaient `None`.
C'était un défaut du code, pas une erreur de votre configuration.

FIREFX06 corrige la **transformation du composant Niagara en C++**, quel que
soit le pack, sans toucher aux assets tiers :

- Échelle horizontale **X/Y** :
  `GroundFireScale × (CellSize × GroundFireCoverage / NiagaraReferenceWidthCm)`
- Échelle verticale **Z** :
  `GroundFireScale × (FlameHeightCm / NiagaraReferenceHeightCm)`

`FlameHeightCm` change effectivement le facteur d'échelle **Z du composant**,
et `GroundFireCoverage` change son facteur **X/Y**, même si le système Niagara
n'expose aucun paramètre utilisateur. Attention : pour un émetteur **World Space**
ou des tailles de particules définies en unités monde, le changement de
transformation du composant peut ne pas se refléter pleinement dans la hauteur
**visible des particules**. La transmission d'un `User.*` correctement relié
aux modules Niagara demeure alors nécessaire.
Cela ne présuppose pas que le Niagara connaît sa hauteur physique.
Les deux références de calibration indiquent combien mesure le système
d'origine à `GroundFireScale = 1`.

## Dans DA_GroundFire

- `GroundFireScale=1`
- `GroundFireCoverage=0.9`
- `FlameHeightCm=75`
- `NiagaraReferenceWidthCm=180` (référence par défaut pour une case 200 cm)
- `NiagaraReferenceHeightCm=75` (référence approximative ; ajustez une fois si la flamme native est plus grande ou plus petite)
- `SpawnAreaParameter=None`, `FlameHeightParameter=None` possibles et recommandés pour le contrôle de taille simple.

Pour essayer : dans PIE, relancer après modification des paramètres et comparer
`FlameHeightCm=35` contre `150`, `GroundFireCoverage=.4` contre `1`.
Modifier `FlameHeightCm` n'élargit plus les flammes.

Les deux contrôles physiques sont **proportionnels** à la taille
du système original et non des mesures exactes de chaque particule.
Les paramètres `FlameDensity`, `SmokeDensity`, `bEnableSmoke` et `FireTint`
restent dépendants des fonctionnalités Niagara du pack tant qu'un adaptateur
dédié ne leur a pas été implémenté ; ils ne doivent pas être décrits comme
des propriétés garanties.

## Tests

`Grimrock.RPG.FIREFX06.GeometryControlsWithoutVendorParameters`
vérifie l'indépendance XY/Z, le facteur global et la protection contre
une calibration égale à zéro. Les tests fonctionnent sans le pack commercial
et sans modifier `DA_GroundFire.uasset` versionné.

## FIREFX06.1 — Modification en direct dans le PIE

Le runtime contrôle désormais les valeurs de `DA_GroundFire` toutes les
0,25 secondes **uniquement tant que des composants de feu sont actifs**.
Quand `FlameHeightCm`, `GroundFireCoverage` ou `GroundFireScale` change,
la transformation XY/Z est actualisée sans recréer les flammes.
Les lumières et le volume de crépitement sont également actualisés.
Le timer s'arrête lorsque le feu est détruit, au nettoyage du niveau,
et à la fin du PIE.
Les changements de Niagara System ou des limites d'affichage reconstruisent
le feu pour rester cohérents avec le profil.

**Limite :** si le système Niagara possède ses propres limites/bounds ou
un volume de spawn codé en dur, le changement de transformation peut être
écrêté. Corriger alors les bounds et la taille native de cet asset Niagara
dans l'éditeur ; le C++ ne peut pas réécrire ses graphes de particules.

## FIREFX06.2 — Diagnostic immédiat du Niagara commercial

Dans `L_Dungeon`, sélectionner l'instance de `BP_GridLevelRuntimeActor`
(dans World Outliner), puis dans les **Details**, rechercher le bouton
`Log Ground Fire Niagara Bindings` (fonction `Call In Editor`).
Cliquer, puis ouvrir **Window → Developer Tools → Output Log**,
filtrer `[FIREFX06]`.

Le diagnostic lit réellement le système sélectionné par `DA_GroundFire`,
liste tous ses paramètres utilisateurs exposés, et indique pour chacun des
cinq champs du profil s'il est absent, renseigné et effectivement présent
dans le système Niagara. Il signale explicitement la limite *World Space*.
Aucun fichier du pack FAB n'est modifié ni versionné par ce diagnostic.

Le fait qu'un paramètre `User.*` existe **ne démontre pas encore** qu'il est
utilisé dans les modules Niagara. Pour agir sur la hauteur de particules
World Space ou sur la fumée réelle, le paramètre doit être relié dans le graphe
Niagara correspondant.
