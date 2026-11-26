# ARMEMENT-DESIGN01 — Fiche maîtresse d'un armement

Version 1.1 — approuvée le 26.11.2026, mise en conformité avec [ARMEMENT-INSPECTION01A](ARMEMENT_INSPECTION01A_ADDENDUM.md)
Projet GrimrockPrototype — Unreal Engine 5.5.4
Référence du code consulté : `da9f7cbc2fff4bec367a11f68ec7b6c18b866a50`

## Mode d'emploi

Dupliquer ce modèle dans `docs/Design/Armement/ARMXXX_<Nom>.md` (organisation proposée). Une fiche couvre un archétype et toutes ses variantes N1–N20 ; aucune nouvelle autorité runtime. Remplir une rubrique à la fois. Pour chaque paramètre, indiquer la **valeur actuelle**, la **cible proposée**, l'**autorité d'édition**, le **statut** (à relever / proposé / approuvé / réalisé / validé / N/A) et la preuve de validation. Ne pas assimiler « non configuré » à zéro. N/A signifie réellement inapplicable à cet armement. Une valeur proposée n'est pas une instruction de modification tant que le propriétaire ne l'a pas approuvée. Toute modification d'un profil partagé doit indiquer les autres armes affectées.

Les champs marqués **[E]** existent dans l'architecture consultée ; **[A]** relève de l'authoring d'assets ou d'une mesure ; **[P]** est une proposition à spécifier et à vérifier, pas une capacité runtime garantie. Les données numériques de jeu restent inchangées sauf décision explicite. N1–N20 désigne le niveau fixe d'une instance, pas la progression automatique d'une arme utilisée par le joueur.

## 01 — Identité et intention artistique

| Paramètre | Actuel | Cible / décision |
|---|---|---|
| ArchetypeId (ARMXXX) [E] | | |
| ItemId [E] | | |
| DataAsset / chemin UE [E] | | |
| DisplayName FR / EN [E] | | |
| Description FR / EN [E] | | |
| WeightKg (fixe N1–N20) [E] | | |
| Tags [E] | | |
| Kind / HandPolicy [E] | | |
| Famille visuelle / FamilyTag [E] | | |
| Affinité principale / secondaire (conseil) [E] | | |
| Origine dans l'univers, époque, artisanat [A] | | |
| Rôle, personnalité et sensation attendue [A] | | |
| Références artistiques et éléments à éviter [A] | | |

Références et images : …
Décision : …

## 02 — Géométrie, fabrication et import

| Paramètre | Actuel | Cible / décision |
|---|---|---|
| Source Blender / FBX et droits [A] | | |
| Longueur, largeur, épaisseur (cm) [A] | | |
| Parties et proportions / silhouette [A] | | |
| Densité de détails, triangles, LOD [A] | | |
| Origine/pivot, axes, échelle, transformations appliquées [A] | | |
| Normales, tangentes, smoothing [A] | | |
| UV, densité texel, coutures [A] | | |
| Material Slots : noms et parties couvertes [A] | | |
| Collision simple / complexe et limites [A] | | |
| Sockets éventuels : poignée, pointe, émission [A] | | |
| WorldMesh / EquippedMesh / ProjectileMesh [E] | | |
| OrnamentMesh / OrnamentTransform [E] | | |
| WorldTransform (translation cm / rotation ° / échelle) [E] | | |

Critères : bonne taille à côté d'une cellule de 200 cm ; silhouette reconnaissable ; pivot et orientation corrects ; contact au sol crédible ; aucune collision parasite liée aux ornements.

## 03 — Matériaux et textures PBR

Répéter la sous-fiche pour chaque partie réelle : lame, garde, poignée, pommeau, bois, ferrure, corde, gemme, etc.

| Paramètre de la partie : … | Actuel | Cible / décision |
|---|---|---|
| Material Slot et matériau parent / MI [A] | | |
| Substance physique et état de surface [A] | | |
| Base Color : texture / teinte / sRGB [A] | | |
| Metallic (0–1) [A] | | |
| Roughness (0–1) [A] | | |
| Normal map et intensité [A] | | |
| AO / packing ORM / import linéaire [A] | | |
| Résolution, compression, mipmaps [A] | | |
| Patine, rayures, usure, salissures [A] | | |
| Masque gravure/ornement [A/P] | | |
| Emissive : masque, teinte, intensité [A/P] | | |
| Paramètres réellement exposés du matériau [A] | | |

Ne pas écraser les Material Slots du mesh par une autorité parallèle. Préférer des instances dynamiques indépendantes lorsque le rendu varie selon l'instance. Les réglages sont contrôlés sous éclairage neutre et éclairage réel du donjon.

## 04 — Progression esthétique N1–N20

Base actuelle : cinq références `LevelFinishIds`, associées aux `Finishes` du catalogue visuel. Les champs actuels sont `FinishId`, `RoughnessScale`, `NormalScale`, `Tint`. Les noms de bandes sont conservés comme base de discussion, pas comme obligation artistique universelle. Chaque niveau consécutif doit apporter une différence visuelle perceptible, même à l'intérieur d'une bande. Les paliers peuvent renforcer cette progression sans remplacer les différences intermédiaires. Un changement de niveau ne crée pas implicitement un enchantement, une rareté ou un effet de gameplay.

| Bande | Niveau | FinishId actuel / cible | RoughnessScale | NormalScale | Tint RGBA | Masque / ornement / émission / NS | Décision |
|---|---|---|---|---|---|---|---|
| Brut | 1–4 | | | | | | |
| Soigné | 5–8 | | | | | | |
| Gravé | 9–12 | | | | | | |
| Magistral | 13–16 | | | | | | |
| Mythique | 17–20 | | | | | | |

Éléments invariants : …
Éléments évolutifs : …
Variation par niveau à l'intérieur d'une bande, obligatoire ; mécanisme de production à valider [P] : …
Partage de profils et impact sur d'autres armes : …

### Matrice de validation des vingt niveaux

Les valeurs mécaniques sont **lues** depuis le résolveur canonique ; les cellules ne constituent pas des données de jeu supplémentaires. La colonne rendu identifie le détail visible qui distingue le niveau du précédent ; « identique à N précédent » est un échec de recette. N1 constitue la référence.

| N | Dés/bonus ou protections résolus | FinishId résolu | Rendu / particularité | VFX persistant | Validation |
|---:|---|---|---|---|---|
| 1 | | | | | À vérifier |
| 2 | | | | | À vérifier |
| 3 | | | | | À vérifier |
| 4 | | | | | À vérifier |
| 5 | | | | | À vérifier |
| 6 | | | | | À vérifier |
| 7 | | | | | À vérifier |
| 8 | | | | | À vérifier |
| 9 | | | | | À vérifier |
| 10 | | | | | À vérifier |
| 11 | | | | | À vérifier |
| 12 | | | | | À vérifier |
| 13 | | | | | À vérifier |
| 14 | | | | | À vérifier |
| 15 | | | | | À vérifier |
| 16 | | | | | À vérifier |
| 17 | | | | | À vérifier |
| 18 | | | | | À vérifier |
| 19 | | | | | À vérifier |
| 20 | | | | | À vérifier |

Contrôles : les 19 transitions adjacentes, notamment N4→5, N8→9, N12→13, N16→17 ; N1/N20 côte à côte ; deux exemplaires indépendants ; rechargement ; absence d'aura systématique pour une arme physique. La progression visuelle par niveau est obligatoire ; le choix de l'interpolation, des masques par partie ou des seuils supplémentaires reste une décision technique et artistique du pilote.

## 05 — Représentation, placement et manipulation

| Situation | Paramètres à relever et valider | Actuel / cible / preuve |
|---|---|---|
| Éditeur de donjon | ItemId, niveau, quantité, pose, orientation, icône de sélection | |
| Objet au sol | WorldMesh, WorldTransform, pivot, collision, masse, inclinaison, lisibilité | |
| Sur table / étagère / réceptacle | Pose, contact, clipping, taille, interaction | |
| Inventaire | ItemIcon, cadrage, fond, lisibilité, tooltip | |
| Équipé | Main autorisée, EquippedMesh, orientation, apparition réelle | |
| Curseur / dépose / ramassage | Conservation GUID et niveau, pose, retour d'action | |
| Projectile, si applicable | ProjectileMesh, rotation, échelle, axe avant, Stable/Tumble/Spin, vitesses visuelles | |
| Examen 3D | Pivot de cadrage, distance, rotation, zoom, vues et éclairages | |

Physique (catalogue mécanique) : `bUseItemWeightAsWorldPhysicsMass`, `WorldPhysicsInitialTiltDegrees`, politique Projectile. Visuel (catalogue visuel) : `ProjectileAppearance.RelativeRotation`, `RelativeScale`, `MeshForwardAxis`, `Mode`, `TumbleAxis`, `TumbleDegreesPerSecond`, `SpinDegreesPerSecond`, `FlightVFX`. L'examen ne doit pas déplacer ni modifier l'objet réel.

## 06 — Mécaniques et usage : relevé avant toute décision

| Paramètre [E] | Actuel | Cible / décision |
|---|---|---|
| Kind, HandPolicy et slots physiques | | |
| DamageType / PhysicalSubtype | | |
| ScalingAttribute / AccuracyBonus | | |
| ActionPointCost / RangeCells | | |
| DamageDiceN1 (NdM + K) | | |
| DiceCountGrowth / DiceSidesGrowth / FlatBonusGrowth et exceptions | | |
| PhysicalArmorN1 / PhysicalArmorGrowth | | |
| MagicalArmorN1 / MagicalArmorGrowth | | |
| RequiredAmmoKindId / politique de projectile | | |
| Reactions et leurs conditions réelles | | |
| ActionId / PresentationId par action et main | | |
| Affinités de classe, exigences réelles de compétence | | |
| Force/faiblesse par rapport aux autres armes | | |

Lister les actions existantes, celles envisagées et celles inapplicables séparément. Attaque lourde et actions spéciales restent des propositions tant que leur mécanique n'est pas approuvée. Une protection passive ne gagne pas une attaque factice. Un son ou un effet ne prouve pas un résultat de combat.

## 07 — SFX : matrice des événements

| Événement | Champ réel / extension | Fichiers / intention / décision |
|---|---|---|
| Début d'attaque | AttackAudio [E] | |
| Impact réussi générique | ImpactHitAudio [E] | |
| Échec | ImpactMissAudio [E] | |
| Chair | ImpactFleshAudio [E] | |
| Métal | ImpactMetalAudio [E] | |
| Bois | ImpactWoodAudio [E] | |
| Critique réel | CriticalAudio, CriticalAudioSurface [E] | |
| Parade réelle | ParryAudio [E] | |
| Absorption physique réelle | BlockAudio [E] | |
| Absorption magique réelle | MagicalBlockAudio [E] | |
| Attaque lourde | HeavyAudio [E : champ, mécanique non garantie] | |
| Libération / collision projectile | Projectile.ReleaseAudio / CollisionAudio [E] | |
| Équiper, déséquiper, ramasser, poser, vol sonore, pierre | [P : vérifier le routage existant avant extension] | |

### Sous-fiche par événement sonore

- Source et licence : fichier WAV d'origine, auteur/fournisseur, date, conditions de génération et de redistribution, crédit éventuel ; conserver le prompt et les variantes écoutées.
- Intention et références : …
- Variantes `Sounds` [E] : … ; `VolumeMultiplier` [E] : … ; `PitchMin` / `PitchMax` [E] : …
- Durée, transitoire, dynamique, timbre, silences, fin propre [A] : …
- Format source recommandé : WAV 48 kHz, 16/24 bits, mono si spatialisation 3D [A] ; réglages UE SoundWave/SoundCue/MetaSound : …
- Spatialisation, atténuation, concurrence, SoundClass, mix [A/P : contrôler le chemin de lecture] : …
- Synchronisation au résultat réel ; comportement en double frappe : …
- Écoute isolée, écoute dans le donjon, niveau de référence, validation propriétaire : …

## 08 — VFX : matrice et sous-fiche Niagara

| Événement | Champ réel / extension | NS / intention / décision |
|---|---|---|
| Début d'attaque | AttackVFX [E] | |
| Impact réussi | ImpactHitVFX [E] | |
| Échec | ImpactMissVFX [E] | |
| Parade | ParryVFX [E] | |
| Absorption physique | BlockVFX [E] | |
| Absorption magique | MagicalBlockVFX [E] | |
| Libération / collision / arrêt projectile | Projectile.ReleaseVFX / CollisionVFX / StopVFX [E] | |
| Vol projectile | ProjectileAppearance.FlightVFX [E] | |
| Effet persistant de l'objet | VisualEffects, VisualEffectsMinimumItemLevel [E] | |
| Impacts VFX par matière / accent critique spécifique | [P : pas de champs spécialisés équivalents aux canaux audio dans FGridActionPresentation] | |

### Sous-fiche pour chaque système Niagara réellement retenu

| Paramètre | Actuel | Cible / décision |
|---|---|---|
| NS, usage, ressources partagées [A] | | |
| Emitters et Simulation Target CPU/GPU [A] | | |
| Local Space / World Space, boucle et durée [A] | | |
| Spawn Burst / Rate, nombre de particules [A] | | |
| Lifetime min/max, taille initiale et courbes [A] | | |
| Vitesse, direction, spread, gravité, drag [A] | | |
| Couleur, alpha, emissive et courbes [A] | | |
| Renderer Sprite/Mesh/Ribbon, matériau, texture/flipbook [A] | | |
| Rotation, alignement, SubUV éventuel [A] | | |
| Collision, réaction et disparition si pertinentes [A] | | |
| Bounds, culling, scalability, coût GPU/CPU [A] | | |
| `Systems`, `bAttachToHeldItem`, `SocketName` [E] | | |
| `LocationOffset`, `RotationOffset`, `Scale` [E] | | |
| Auto-destruction et nettoyage en fin de combat [E/A] | | |

Persistance : `LightEmitter`, `VisualEffects`, `VisualEffectsMinimumItemLevel`, `bEnableWorldSparkle`, `WorldSparkleMaterial`, `WorldSparkleColor`, `WorldSparkleIntensity`, `WorldSparkleSpeed`, `WorldSparkleVariation`. Relever séparément lumière réelle, émission de matériau et particules. Ne pas activer de lumière de gameplay par simple décision cosmétique.

## 09 — Mouvement, timing et synchronisation

| Paramètre [E] | Actuel | Cible / décision |
|---|---|---|
| MotionStyle | | |
| bAnimateHeldItem | | |
| MotionDurationSeconds | | |
| ImpactDelaySeconds | | |
| RecoveryDelaySeconds | | |
| PeakLocationOffset / PeakRotationOffset | | |
| FeedbackDurationSeconds | | |
| Intervalle minimal des frappes multiples (réglage du composant) | | |

Relever les événements temporels effectivement émis : départ, impact, retour, deuxième main, projectile, fin de combat. Distinguer calendrier de présentation et coûts/initiative du combat. Contrôler le rendu dans la vraie vue subjective ; ne pas supposer qu'une arme non lumineuse est toujours visible en main.

## 10 — Examen 3D et atelier de validation

Mode joueur : fond noir neutre, éventuelle ambiance lointaine 2.5D non intrusive ; l'action Examiner affiche l'instance réelle au niveau réel, description, statistiques autorisées, mesh et présentation résolus ; rotation, zoom, recentrage. Lecture seule, sans mutation du donjon.

Mode auteur [P] : sélectionner ARMXXX et N1–N20 indépendamment de l'instance réelle ; comparer deux niveaux ; éclairages studio/torche/donjon ; inspection des Material Slots et paramètres ; prévisualisation au sol/en main/en vol ; écouter SFX par événement et variante ; rejouer Niagara et gestes avec une cible de test et des résultats simulés explicitement étiquetés. Ne pas infliger de dégâts, consommer de PA ou de munitions, créer du butin ou écrire dans une sauvegarde. Un réglage temporaire ne devient une donnée de production que sur action explicite et validée dans l'outil d'édition approprié. L'écran joueur ne propose pas les contrôles auteur.

Pour cette arme : cadrage sur fond noir … ; scène de référence auteur … ; 19 transitions comparées … ; événements simulables … ; poses … ; paramètres à exposer … ; limites …

## 11 — Autorités d'édition et inventaire des ressources

| Autorité | Responsabilité | Références effectives de l'arme |
|---|---|---|
| DA_Weapon_* | ItemId, DisplayName, Description, WeightKg, Tags, ArchetypeId | |
| DA_WeaponCatalogue | Mécanique, progression, mains, munitions, réactions, balistique | |
| DA_WeaponVisualCatalogue | Liaisons, meshes, icônes, finitions, présentation persistante et en vol | |
| DA_ActionCatalogue | Actions véritables et leurs liaisons | |
| DA_ActionPresentationCatalogue | Audio, VFX d'action et timing | |
| Assets natifs | SM, M/MI, T, NS/emitters, SoundWave/Cue/MetaSound | |
| DA_GridLevel_* | Placement ItemId / niveau / quantité / pose | |

`DA_ItemSystem` référence exactement les quatre catalogues. `UGridItemSubsystem::ResolveItem` est l'autorité de résolution. L'outil d'examen doit utiliser ces données et les chemins métier de présentation existants ; aucune cinquième autorité ni copie persistante de profil.

## 12 — Recette et approbation finale

- [ ] Identité et rôle approuvés.
- [ ] Mesh, UV, textures, matériaux, collision et échelle approuvés.
- [ ] Placement dans le décor et manipulation approuvés.
- [ ] N1 à N20 prévisualisés ; 19 différences adjacentes perceptibles et approuvées ; frontières des bandes et indépendance des instances vérifiées.
- [ ] Icône et examen joueur vérifiés.
- [ ] Chaque événement audio applicable écouté et approuvé.
- [ ] Chaque Niagara applicable visualisé et approuvé, y compris fin de combat/fin PIE.
- [ ] Timing, usage et lisibilité réelle en donjon approuvés.
- [ ] Conservation de l'identité, niveau et rendu après transfert et rechargement vérifiée.
- [ ] Tests ciblés et campagne complète ; Development Editor et Shipping ; rendu réel en PIE/Shipping selon le périmètre.
- [ ] Licences et sources documentées ; pas de média provisoire présenté comme final.
- [ ] Validation explicite du propriétaire et SHA de référence.

| Rubrique | Statut | Décision / preuve | Date |
|---|---|---|---|
| Identité | À relever | | |
| Géométrie et matériaux | À relever | | |
| Niveaux | À relever | | |
| Placement et manipulation | À relever | | |
| Mécaniques | À relever | | |
| SFX | À relever | | |
| VFX | À relever | | |
| Examen 3D | À spécifier | | |
| Recette finale | Ouverte | | |

Historique : date / paramètre / ancienne valeur / décision / nouvelle valeur / impact / SHA / validation.

## Références

- `docs/Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md`
- `docs/Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md`
- `docs/Design/ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md`
- `docs/Architecture/MATERIAL_OWNERSHIP.md`
- `docs/Architecture/ITEM_VFX01_DATA_DRIVEN_ITEM_NIAGARA.md`
- `Source/GrimrockPrototype/Public/Runtime/Items/GridItemCatalogues.h`

Modèle approuvé ; voir [ARMEMENT-INSPECTION01A](ARMEMENT_INSPECTION01A_ADDENDUM.md). Cette révision documentaire ne modifie ni les assets ni le runtime.
