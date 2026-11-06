# ARMEMENT01.17.4B — Ground Fire Niagara piloté par le niveau unique

Date : 6 novembre 2026.
Parent : 4b27dfce (UE5.5.4/LocalAuto = SUCCESS).
Aucun contenu binaire, Niagara du pack Fab ou .umap modifié par ce jalon.

## Choix FAB

**Pack n° 1 : FireFX — Realistic Fire VFX (DmitryN)**.
https://www.fab.com/listings/04b309c0-f0ee-4a20-84a5-46a5794d7d3d

La fiche du vendeur confirme **Ground Fire** parmi 18 systèmes, avec flammes,
fumée, braises et distorsion. L'auteur indique des essais UE 5.0.3 :
**compatibilité et rendu en UE 5.5.4 à vérifier localement**.
Les noms exacts des User Parameters, leur type et leur échelle ne sont pas
contractuels sur cette fiche ; nous n'en codons donc aucun en dur.

## Autorité des données : UGridLevelAsset

L'asset de niveau contient une seule section **SurfaceVfxSettings**
(FGridLevelSurfaceVfxSettings), éditable dans la fenêtre Details.
Ce n'est ni un acteur supplémentaire par case, ni un nouveau DataAsset.

**bOverrideRuntimeVisuals** est faux par défaut : les niveaux existants
gardent FireSurfaceNiagaraSystem, FireSurfaceNiagaraScale,
FireSurfaceLightIntensity et les durées du catalyseur du runtime actor.
Activer cette case transfère l'autorité de ces aspects visuels au niveau.

Réglages proposés :
- bShowGroundFire (présentation seulement, le feu RPG03.6 continue à blesser).
- GroundFireSystem (soft reference à un Niagara sélectionné par l'auteur).
- GroundFireScale (1.0 pour un effet au sol dédié, 0.38 pour le fallback torche).
- GroundFireCoverage (0.9 = 180x180 cm pour une cellule 200x200).
- FlameHeightCm (75 cm), FlameDensity (1.0), bEnableSmoke, SmokeDensity (.5),
  FireTint, FireLightColor, FireLightIntensity (650), FireLightRadiusCells (1.2).
- MaxFireCells (48), MaxFireLights (8), bornes de sécurité 128/32.
- CatalystFlightDurationSeconds (.42), CatalystPulseDurationSeconds (.95),
  CatalystWaveDelayPerCellSeconds (.085) : conservent l'onde appréciée,
  sans modifier le gameplay.
- Paramètres Niagara facultatifs (FName) : SpawnAreaParameter (Vector3),
  FlameHeightParameter (Float), FlameDensityParameter (Float),
  SmokeDensityParameter (Float), FireTintParameter (LinearColor).

Les paramètres Niagara **ne sont transmis que pour un effet dédié réellement
chargé**, quand le niveau est en mode override et lorsqu'un nom est renseigné.
Une valeur FName vide signifie « ne pas surcharger » : aucun nom de pack
commercial n'est présumé.

La surface transmise à SpawnAreaParameter correspond au diamètre complet
**en coordonnées locales Niagara** : la largeur en cm est divisée par
GroundFireScale pour conserver l'empreinte souhaitée en monde.
La hauteur suit la même convention. Selon le contrat particulier du pack
(par exemple demi-étendue/rayon), adapter le paramètre ou le Niagara système.
Ces paramètres doivent être réellement *exposés* dans Niagara, sinon la
donnée ne produira pas l'effet attendu.

## Installation et utilisation dans UE 5.5.4

1. Ajouter légalement FireFX via la bibliothèque Fab à un projet compatible
   puis migrer/installer les assets vers GrimrockPrototype si nécessaire.
   Aucun achat, téléchargement ou import n'est fait par ce commit.
2. Ouvrir le système **Ground Fire** réellement fourni ; vérifier en PIE
   ses dimensions, son comportement sur un sol 200x200 et ses performances.
   Si nécessaire créer **NS_Grimrock_GroundFire** en dupliquant l'effet reçu.
3. Ouvrir l'asset du niveau actif (UGridLevelAsset / DA correspondant).
   Dans Level > Surface VFX, activer **Override Runtime Visuals**.
4. Affecter le Niagara réellement trouvé à **Ground Fire System**.
   Régler Coverage=.90, Scale=1.0, Flame Height=75, Fire Light=650,
   Max Fire Cells=48 et Max Fire Lights=8 pour commencer.
5. Relever les **noms exacts et types** des paramètres "User" visibles dans
   le panneau Parameters du Niagara ; renseigner seulement ceux qui
   correspondent aux 5 champs de binding ; laisser les autres vides.
6. Tester huile puis catalyseur, 1 / 3 / 9 cases, chargement de sauvegarde,
   extinction et transition entre niveaux. Vérifier que ni la torche murale,
   ni les dégâts périodiques/PA, ni l'onde de chaleur existante ne changent.

La flamme de torche existante sert de **fallback**, pas de nouveau Ground Fire.
L'effet pleine case attend un Niagara dédié installé et sélectionné.
Le runtime détruit ses composants transitoires avant rafraîchissement
comme auparavant. Aucune référence directe au pack commercial n'est livrée.

## Tests

- Grimrock.RPG.ARMEMENT01.17.4B.LevelSurfaceVfxProfile
- Grimrock.RPG.ARMEMENT01.17.4B.RuntimeLevelSwitch
- Toute la suite Grimrock, compilation Development Editor UE 5.5.4.
- Recette GPU/RHI/PIE séparée (effet Niagara, fumée, bruit de texture,
  performance, transitions et packaging).
