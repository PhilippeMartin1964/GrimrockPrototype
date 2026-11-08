# FIREFX10 — Runtime Niagara simplifié, sans legacy

## Périmètre

Nettoyage pur de la présentation **feu au sol**, sans changement des
règles de combat, des surfaces, des meshes de bouteille, de la
logique de cellule, ou du système audio (reporté).

Le code repose uniquement sur :

1. `DA_GroundFire` : `GroundFireSystem`, couverture, hauteur cible,
   échelle générale, densités, teinte, budgets, lumière, son existant,
   et `UserParameterOverrides` typés.
2. `FGridFireNiagaraBindings::Resolve` : détection et vérification
   **type + nom** de `SpawnBox`, `FlameRate`, `SmokeRate`, `Tint`,
   plus les surcharges nommées du Data Asset.
3. `FGridFireNiagaraBindings::Apply` : **un seul point d'application**,
   dans `RefreshFireSurfaceVisuals`, avant `Activate`.
4. `FGridFireNiagaraGeometry` : échelle XY et Z indépendante.
   `SpawnBox` contrôle la vraie emprise XY si compatible ; sans lui,
   `GroundFireCoverage` ne fournit qu'une échelle relative.

**Supprimés, sans redirection ni rétrocompatibilité :**

- `SpawnAreaParameter`, `FlameHeightParameter`,
  `FlameDensityParameter`, `SmokeDensityParameter`,
  `FireTintParameter` : chemins parallèles manuels obsolètes.
- `bAutoBindKnownFireControls` : les liaisons typées sont
  systématiquement tentées si elles existent. Sinon, elles sont ignorées.
- `NiagaraReferenceWidthCm` : inapproprié pour le pack connu disposant
  de `SpawnBox`. Son absence n'est pas remplacée par une valeur
  de largeur inventée.
- `bEnableSmoke` : pour désactiver le **taux** de fumée continu,
  régler `SmokeDensity=0`. Les bursts sont une commande indépendante.

Les deux types `UGridSurfaceVfxProfileAsset` (profil) et
`FGridNiagaraUserOverride` (surcharge typée) demeurent. Le système
Niagara du vendeur est **réutilisé**, non modifié. Il est autorisé à
l'exécution uniquement si installé localement. Les deux packs tiers
`Content/FireFX/` et `Content/F_FreeFlameFx_Pack/` restent
**exclus de Git**. Le profil appartient à Grimrock et reste versionné.

## Rafraîchissement

- Au lancement d'une surface de feu ou au changement de sa topologie,
  `RefreshFireSurfaceVisuals` construit les composants, leur applique
  les réglages une fois, puis les active.
- En **éditeur uniquement**, si des flammes existent en PIE,
  le runtime compare `FGridFireVfxSettings::StaticStruct()` avec
  le dernier profil appliqué **toutes les 0,25 seconde** :
  profil identique → retour immédiat, aucune écriture Niagara ;
  profil modifié → reconstruction via la seule voie canonique.
- En jeu packagé, **aucun polling de profil**.
- Le système audio existant n'est pas refactorisé dans ce jalon.

L'actualisation d'un Data Asset en plein PIE peut réinitialiser brièvement
les particules, puisqu'une reconstruction est déclenchée.
C'est une décision de simplification délibérée, et non un système
à chaud qui modifie silencieusement des centaines de particules.

## Réglage actuel conseillé pour huile en croix (case 200 cm)

| Champ | Réglage |
|---|---|
| `GroundFireSystem` | `NS_FFX_Ground_Fire` (pack local) |
| `GroundFireScale` | 1.0 |
| `GroundFireCoverage` | 0.9 → 180 × 180 cm de spawn |
| `FlameHeightCm` | 30 |
| `NiagaraReferenceHeightCm` | 150 → échelle Z 0.2 |
| `FlameDensity` | 0.5 → base FlameRate 30 × 0.5 = 15 |
| `SmokeDensity` | 0.15 → base SmokeRate 6 × 0.15 = 0.9 |
| `FireLightIntensity` | 250 |
| `FireLightRadiusCells` | 0.8 |

Les paramètres `UserParameterOverrides` ne sont **pas** effacés ;
les réglages déjà saisis pour `FlameSizeMin/Max`, `GlowSize`,
`SmokeSizeMin/Max`, etc., s'appliquent en dernier **s'ils sont
effectivement présents et de type compatible**.

## Tests automatisés

La suite `Grimrock.RPG.FIREFX10.GeometryAndProfileRefresh`
reste conservée pour la géométrie pure ; les assertions sont adaptées
au nouveau contrat XY. Elle inclut les contrôles de :
- absence des propriétés supprimées dans la réflexion UE ;
- identité des profils inchangés et détection des modifications
  (`CompareScriptStruct`), y compris les surcharges typées ;
- conservation des ratios hauteur/couverture et priorités typées.

Les tests Windows compilent et exécutent la suite sans pack commercial,
donc **une recette PIE avec votre FireFX local reste nécessaire**.
