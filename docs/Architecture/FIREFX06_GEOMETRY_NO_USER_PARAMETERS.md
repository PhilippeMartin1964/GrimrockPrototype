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

Désormais `FlameHeightCm` change effectivement le facteur d'échelle Z des
flammes, et `GroundFireCoverage` change le facteur X/Y, même si le système
Niagara n'expose **aucun** paramètre utilisateur.
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
