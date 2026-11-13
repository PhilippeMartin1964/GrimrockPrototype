# ARM-CLEAN06.2X — Visuels des projectiles ARM, sans duplication de gameplay

## Source autorisée

Les 22 lignes actives de
`Config/Armement/WeaponProductionManifest.csv` possèdent
22 `VisualId` distincts. Trois armes sont déclarées
`ThrowingOneHand` : **ARM054, ARM057 et ARM058**.
Les DataAssets du manifeste sont la seule source des
paramètres visuels à projeter.

## Destination unique

`FGridCanonicalProjectileVisual` vit désormais **dans**
`FGridCanonicalWeaponVisual::ProjectileAppearance`
du `DA_WeaponVisualCatalogue` canonique. Les propriétés
conservées sans inférence sont :

- `ThrowVisualRelativeRotation` → `RelativeRotation`
- `ThrowVisualRelativeScale` → `RelativeScale`
- `ProjectileMeshForwardAxis` → `MeshForwardAxis`
- `ThrowVisualMode` → `Stable/Tumble/Spin` explicite
- `ThrowVisualTumbleAxis` → `TumbleAxis`
- `ThrowVisualTumbleDegreesPerSecond` → `TumbleDegreesPerSecond`
- `ThrowVisualSpinDegreesPerSecond` → `SpinDegreesPerSecond`
- le mesh volant réel `WorldMesh` du DataAsset →
  `ProjectileMesh` (rôle distinct, sans nouveau mesh ni identité).

Le mode `Stable` reste aligné sur l'axe pointe du mesh importé.
Le convertisseur ne déduit jamais l'orientation depuis le Nord
de la grille ou le mesh du donjon. La politique de physique
(`InitialSpeed`, `Arc`, `FlightLifetime`,
`ImpactDropOffset`) reste **uniquement** dans
`DA_WeaponCatalogue` (ARM-CLEAN06.2W).

La projection exige l'identité `ItemId/ArchetypeId/VisualId`
et l'accord `BuildKind / bCombatThrowWeapon`.
Elle refuse les modes invalides, ressources manquantes,
données non finies et toute écriture sur un profil déjà projeté.
Une arme non jetable ne reçoit pas de projectile artificiel.
Les données réellement partagées devront avoir une preuve
d'équivalence avant de réutiliser un `VisualId` commun ;
les 22 identifiants présents n'ont aucun conflit.

## Validation

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionProjectileVisualParity`
charge uniquement les 22 armes du manifeste, crée un registre
visuel transitoire avec les projecteurs validés 06.2P,
vérifie les sept paramètres de vol visuel et les chemins
de mesh de chaque arme, puis valide le catalogue.
`ProjectileVisualProjectionSafety` rejette les incohérences
de type et les tentatives d'écrasement.

Le contenu `.uasset`, les WBP, le projectile historique,
la collision et les SaveGame restent inchangés. L'intégration
effective à la visualisation/animation du runtime ne sera
autorisée qu'après migration binaire et parité PIE dédiée.
