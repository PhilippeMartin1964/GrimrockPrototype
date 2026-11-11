# ARMEMENT-FX01 — Spécification sonore, Niagara et évolution esthétique N1–N20

**Date :** 11 novembre 2026 · **Version :** 1.0 · **Statut :** SPÉCIFICATION PROPOSÉE, NON IMPLÉMENTÉE · **UE :** 5.5.4  
**Périmètre autoritaire :** les 22 lignes `Active` de `Config/Armement/WeaponProductionManifest.csv`, couvrant 22 × 20 = **440 niveaux d'objet possibles** ; le catalogue complet contient 82 × 20 profils, mais les 60 autres archétypes ne sont pas des DataAssets de production déclarés ici.  
**Matrice éditable :** [ARMEMENT_FX01_ACTIVE22_CUE_MATRIX.csv](ARMEMENT_FX01_ACTIVE22_CUE_MATRIX.csv), **22 lignes sans assets sonores/Niagara prétendument existants**. Les descriptions et IDs proposés sont des intentions de direction artistique, pas des effets déjà créés.

## 1. Décision, preuves et état du projet

Le propriétaire du projet a confirmé le **11.11.2026** avoir exercé en PIE les 22 armes N1 sous différentes configurations et considérer la recette comme validée. Il ne s'agit pas d'une certification exhaustive par capture de chaque scénario : le cycle **quitter le processus → relancer → Continue**, les cultures `Game/en`, les médias réels de chaque arme et les 20 niveaux de rendu restent des audits distincts.

**L'extension doit être intégralement cosmétique.** Elle n'invente aucune compétence de classe, poison, soin, feu persistant, blocage actif, nouvelle attaque, dés, bonus, coût de PA, projectile physique ou résistance. Aucun effet ne doit annoncer un dommage, une parade, un statut ou un blocage qui n'a pas réellement eu lieu.

| Autorité actuelle vérifiée | Ce qu'elle possède déjà | Frontière FX01 |
|---|---|---|
| XLSX canonique ARMEMENT01.2 + `UGridWeaponCatalogueAsset` | Statistiques, type de dégâts, défense, NdM+K et PA **N1–N20** | Jamais de SFX, d'intensité visuelle ou de règle de FX ici |
| `WeaponProductionManifest.csv` | Identités, BuildKind, slots, chemins d'art de production | Ne pas y ajouter 440 colonnes/paliers d'effets |
| `UGridItemDefinitionAsset` | Actions, `WeaponArchetypeId`, visuels, `bProvidesAttackPresentation`, `PlayerAttackPresentationProfile` | Profil déjà présent = repli admissible pour attaques simples |
| `UGridWeaponVisualCatalogueAsset` | `ArchetypeId → VisualId → StaticMesh/Icon` et ornement | Conserver la silhouette et les Material Slots comme autorités |
| `UGridPlayerAttackPresentationComponent` | `Attack`, `ImpactHit`, `ImpactMiss` + sons/Niagara optionnels, séquence double frappe | Étendre le résolveur, **pas** réécrire la résolution de combat |
| `UGridItemVisualEffectsComponent` | Niagara persistants/conditionnels pour item physiquement visible | Garder ce canal pour l'ornement de l'objet, pas les impacts de combat |
| `FGridAttackResult` et `FGridEquippedGuardResolver` | `bEquipmentParried`, `bEquipmentBlocked`, `EquipmentBlockedDamage` et sources d'armure | Ajouter uniquement la restitution sonore/visuelle des vrais résultats |
| `UGridEquippedWeaponActionIconPalette` | Icônes **d'actions** par famille | Conserver distincte des icônes **d'objets** et de leurs FX |

**Contrats contraignants :** `docs/Architecture/MATERIAL_OWNERSHIP.md`, `docs/Architecture/ITEM_VFX01_DATA_DRIVEN_ITEM_NIAGARA.md`, `docs/Design/ARMEMENT_LOT02_19_2_2_SEQUENCED_COMBAT_PRESENTATION.md`, `docs/Design/ARMEMENT_LOT02_19_3_PARRY_SHIELD_GUARD.md`, `docs/UI_UMG_PRESENTATION_CONTRACT.md` et `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`. Les guides historiques contraires à l'état courant ne réactivent aucune propriété Legacy.

## 2. Sens des « slots » et taxonomie des événements

Deux notions à ne pas confondre :

- **Slot d'équipement :** `MainHand`, `OffHand`, et **OffHand occupé virtuellement** par une arme deux mains (aucune seconde source réelle). Le son d'attaque suit l'arme et **la main physique de la frappe**. Une attaque double appelle ses profils MainHand puis OffHand indépendamment, comme ARMEMENT-LOT02.19.2.2.
- **Slot 0–9 de raccourci :** ne possède **aucun FX propre**. C'est l'action résolue (`SourcePolicy/ActionId/WeaponArchetypeId/OffensiveEquipmentSlot`) qui sélectionne un FX, qu'on la déclenche avec la souris ou au clavier. Les sorts mémorisés, potions et consommables non armement gardent leur présentation spécifique.

Événements à paramétrer, avec statut d'implémentation :

| Événement de présentation | Déclencheur autoritaire | État du moteur |
|---|---|---|
| `AttackStart` | Action d'attaque acceptée | Déjà disponible : `AttackAudio`, `AttackVFX` |
| `ImpactHit` / `ImpactMiss` | Résultat **réel** de chaque frappe | Déjà disponibles : `ImpactHit/ImpactMissAudio/VFX` |
| `ProjectileLaunch`, `ProjectileFlight`, `ProjectileWorldImpact` | Acteur projectile réellement créé puis événement de collision/arrêt | Complément de présentation à ajouter ou raccorder à l'acteur existant |
| `DefenseParry` | `Result.bEquipmentParried` + ARM013 physique OffHand | **À raccorder** ; aucun son de parade sur un simple `bHit=false` |
| `DefenseShieldAbsorb` | `Result.bEquipmentBlocked && EquipmentBlockedDamage > 0`, bouclier OffHand réel | **À raccorder** ; quantité absorbée et source d'armure sont l'autorité |
| `DefenseFocusMagicAbsorb` | Débit réel de `AbsorbedEquipmentMagicalArmor` sur ARM070 OffHand | **À instrumenter** à partir des sources réelles, pas sur n'importe quel dégât magique |
| `DefenseSourceExhausted` | Charge positive avant coup, zéro après, absorption strictement positive | Extension optionnelle, **sans casse/durabilité** de l'objet |
| `Equip`, `Unequip`, `IdlePresent` | Transition de présentation d'un item physique | Facultatifs, distincts d'une action de combat |
| `CriticalAccent` | `bCriticalHit=true` **et résultat réellement réussi** | Accent sur impact existant, pas deuxième frappe |

**Défense = réaction**, non nouvelle action dans la hotbar. ARM013 ne pare qu'en OffHand et seulement selon les règles validées (physique, mêlée, non critique, jet réussi). ARM075 et ARM081 ne font **aucun blocage actif** ; ils matérialisent leur absorption physique existante. ARM070 n'attaque et ne pare pas ; une éventuelle brillance de protection magique ne s'affiche qu'au débit de sa charge.

## 3. Matrice des 22 armes de production

Les slots ci-dessous sont repris **tels quels** du manifeste. `MainHand` sur une arme deux mains représente le propriétaire physique ; l'autre main n'ajoute jamais un second événement. `—` n'est pas une fonctionnalité manquante lorsque l'arme est passive. Le CSV donne pour chaque ligne **quatre idées audio**, **cinq idées Niagara** et une famille de finition.

| ARMXXX | Arme | Slots déclarés | Offensive | Défense possible | Finition |
|---|---|---|---|---|---|
| ARM001 | Dague | MainHand / OffHand | attaque MeleeOneHand | aucune défense d'arme | acier-fin |
| ARM002 | Dague d'assassin | MainHand | attaque MeleeOneHand | aucune défense d'arme | acier-fin |
| ARM003 | Stylet | MainHand | attaque MeleeOneHand | aucune défense d'arme | acier-fin |
| ARM013 | Main-Gauche | MainHand / OffHand | attaque MeleeOneHand | parade OffHand seulement | acier-fin |
| ARM014 | Épée droite | MainHand | attaque MeleeOneHand | aucune défense d'arme | acier-classique |
| ARM015 | Épée longue | MainHand | attaque MeleeOneHand | aucune défense d'arme | acier-classique |
| ARM020 | Masse | MainHand | attaque MeleeOneHand | aucune défense d'arme | fer-lourd |
| ARM025 | Bec de guerre | MainHand | attaque MeleeOneHand | aucune défense d'arme | fer-lourd |
| ARM031 | Grande hache | MainHand | attaque MeleeTwoHand | aucune défense d'arme | fer-lourd |
| ARM038 | Lance courte | MainHand | attaque MeleeTwoHand | aucune défense d'arme | hampe |
| ARM045 | Arc court | MainHand | attaque Ranged | aucune défense d'arme | bois-corde |
| ARM048 | Arc recourbé | MainHand | attaque Ranged | aucune défense d'arme | bois-corde |
| ARM054 | Couteau de jet | MainHand | attaque ThrowingOneHand | aucune défense d'arme | projectile-acier |
| ARM057 | Javelot | MainHand | attaque ThrowingOneHand | aucune défense d'arme | hampe |
| ARM058 | Fléchette | MainHand | attaque ThrowingOneHand | aucune défense d'arme | projectile-acier |
| ARM061 | Baguette arcanique | MainHand | attaque MagicOneHand | aucune défense d'arme | arcane |
| ARM062 | Baguette de feu | MainHand | attaque MagicOneHand | aucune défense d'arme | braise |
| ARM067 | Bâton élémentaire | MainHand | attaque MagicTwoHand | aucune défense d'arme | elementaire |
| ARM069 | Sceptre sacré | MainHand | attaque MagicOneHand | aucune défense d'arme | sacre |
| ARM070 | Orbe arcanique | OffHand | — (passif) | absorption magique réelle | focus |
| ARM075 | Bouclier rond | OffHand | — (passif) | absorption physique réelle | bouclier |
| ARM081 | Égide sacrée | OffHand | — (passif) | absorption physique réelle | bouclier-sacre |

**Nuance d'authoring :** un slot physiquement autorisé n'autorise pas à lui seul une attaque si `CanProvideAttackFromSlot` / le catalogue actuel ne la proposent pas. ARM001 et ARM013 sont les seules lignes de cette matrice autorisant MainHand **et** OffHand ; aucune arme autre que ARM013 ne gagne une parade par analogie. Les armes de jet ARM054/057/058 sont des unités individuelles non empilables dans le pipeline de production : les bruitages et VFX suivent **l'objet physique réellement lancé**.

### 3.1 Direction audio, par rôle et matière

- **Armes légères (ARM001/002/003/013)** : mouvement court sec, fines harmoniques acier ; attaque rapide mais jamais « silencieuse » au point d'être illisible. Parade ARM013 : *tink* latéral métallisé + petit frottement, seulement sur réussite.
- **Lames, armes lourdes et hast (ARM014/015/020/025/031/038)** : distinction tranchant / contondant / perforant, amplitude de mouvement plus grave pour hache et masse ; l'impact dépend de la surface réellement connue, pas d'un bruit de métal joué sur chaque monstre.
- **Arcs (ARM045/048)** : tension de corde, libération, léger souffle de vol, pointe sur cible ou impact sur décor ; **une seule** flèche de gameplay `Item_Arrows_01` et aucun deuxième projectile « décoratif » ressemblant à une vraie munition.
- **Jet (ARM054/057/058)** : départ, vol discret suivant vitesse/orientation du vrai `AGridThrownItemActor`, bruit d'atterrissage du véritable point de collision, puis ramassage existant. Les trois timbres sont distincts (lame, hampe, fléchette).
- **Magie (ARM061/062/067/069)** : texture arcanique/cristalline, braises très brèves pour ARM062, résonance grave pour le bâton, carillon contenu pour le sceptre. Le type effectif de dégâts prévaut : pas de statut Feu, Lumière, poison, froid, soin ou surface simplement parce que l'arme est nommée ainsi.
- **Passifs (ARM070/075/081)** : aucun `AttackAudio`. Son **uniquement** lors d'une absorption effectivement réalisée, ou petit son d'équipement facultatif ; fatigue/épuisement peut avoir un cue subtil sans « bouclier brisé » permanent.

Exiger idéalement **3 variantes** pour un whoosh fréquent, **2–3** pour chaque impact de famille et **2** pour chaque réaction défensive ; la matrice peut partager une même banque entre armes tout en ajoutant un accent distinct. Les champs existants `Sounds[]`, `VolumeMultiplier`, `PitchMin/Max` sont directement réutilisables. Prévoir des `SoundWave` sources, éventuellement `SoundCue` ou `MetaSound Source` pour les assemblages, et une concurrence séparée **armes / impacts / défenses / ambiance**. Aucune bibliothèque SFX sous licence inconnue ni « faux nom d'asset » ne doit être committée.

### 3.2 Direction Niagara et matériaux

Bibliothèque réutilisable souhaitée : `BladeTrailLight`, `BladeTrailHeavy`, `ThrustLine`, `BluntDust`, `PierceSpark`, `ArrowFlightAccent`, `ThrowingFlightAccent`, `ArcaneCast`, `FireCast`, `ElementalCast`, `HolyCast`, `ParrySpark`, `ShieldAbsorb`, `FocusAbsorb`, `GenericAirMiss`. Ce sont des **identifiants de conception**, pas des ressources `NS_*` confirmées.

Le *système Niagara* fait la géométrie des particules ; une *Material Instance* définit leur aspect (couleur, alpha, emissive, bruit, érosion). Les effets durables sur le mesh utilisent **`UGridItemVisualEffectsComponent`** (`Always` / `WhenLightEnabled`), mais les sons et explosions d'attaque/impact sont gérés par le **composant de présentation de combat**. Un même éclat ne doit jamais être créé dans les deux canaux.

**À contrôler pour chaque Niagara :** espace local vs monde, socket/pivot réel, orientations des axes, collision du vrai projectile, occlusion par la géométrie, disparition à l'arrêt, limitation du nombre de composants, transparence et lisibilité en pièce sombre. N'utilisez pas Niagara pour appliquer des dégâts.

## 4. Intégration dans les DataAssets — architecture cible

### 4.1 Pourquoi un catalogue unique de présentation ?

Plutôt que 22 DataAssets × 20 niveaux × plusieurs événements, proposer **un unique** `UGridWeaponFXCatalogueAsset : UPrimaryDataAsset`, par exemple à **`/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_WeaponFXCatalogue`**, référencé une fois par le service de présentation. Il contient :

- `FamilyProfiles[]` : jeux audio/VFX communs par famille (`Light`, `Sword`, `Blunt`, `Pick`, `Axe`, `Polearm`, `Ranged`, `Throwing`, `Magic`, `Holy`, `Focus`, `Shield`), toutes les références en soft.
- `ArchetypeBindings[]` : **une entrée par ARMXXX actif**, résolue depuis `UGridItemDefinitionAsset::WeaponArchetypeId`, avec famille héritée et surcharges artistiques facultatives.
- `EventProfiles[]` : `EventId`, `EquipmentSlot` (physique ou `None` pour source sac), `ActionId` optionnel, `SoundVariants[]`, volume, pitch, concurrence, `NiagaraSystems[]`, socket, offsets/scale, durée, priorité et type d'impact. Pour les défenses, rôle `Parry` / `ShieldAbsorb` / `FocusMagicAbsorb` défini sans attaque factice.
- `LevelFinishes[]` : **cinq bandes** N1–4, N5–8, N9–12, N13–16, N17–20, réglages uniquement visuels (sans statistiques ni rareté).
- `CosmeticParameters` : intensités Niagara, couleurs par famille, motifs/runes, présence et LOD d'un ornement non collisionnant ; courbes ou paramètres Material Instance par instance physique.

Les champs actuels de `FGridPlayerAttackPresentationProfile` (`AttackAudio/ImpactHitAudio/ImpactMissAudio` et équivalents VFX, `MotionStyle`, `ImpactDelaySeconds`, `RecoveryDelaySeconds`) **restent valides**. Déploiement en deux temps :
1. **Pilote rapide sans changement de schéma :** renseigner ces six champs dans les vrais `DA_Weapon_*` offensifs, tester leurs trois événements et la double frappe.
2. **Généralisation :** mettre le catalogue FX central en autorité quand son entrée est complète ; garder le profil local d'item comme **repli explicite**, **jamais jouer les deux**. Pour une défense ou une finition de niveau, passer obligatoirement par le nouveau canal, pas par `AttackVFX`.

**Priorité déterministe pour un événement :** surcharge `(ArchetypeId, action, vraie main, événement)` → surcharge `(ArchetypeId, événement)` → famille → profil historique d'item **pour les trois événements offensifs existants seulement** → absence de média. En cas d'erreur de ressource, journaliser `FX_MISSING` avec l'identité, ne pas substituer silencieusement un son de poison ou de feu. Une entrée vide signifie « désactivée » ou « hériter », deux intentions à représenter explicitement dans le modèle, afin d'éviter les surprises de fallback.

### 4.2 Contexte d'événement à capturer

```text
FGridWeaponFXContext [transient, presentation-only]
  RequestId / ActionInstanceId / ResolutionIndex
  ItemDefinitionId / WeaponArchetypeId / ItemRuntimeObjectId
  ItemLevel (0 Legacy ou 1..20 réels)
  PhysicalEquipmentSlot (MainHand / OffHand / None)
  ActionId / SourcePolicy / PresentationEvent
  DamageType / PhysicalSubtype / ActualOutcome
  TargetActor / SourceActor / WorldImpactLocation (si réellement connus)
  bEquipmentParried / EquipmentBlockedDamage / ArmorSourceId
```

**Point à traiter avant les bandes N1–N20 :** `FGridPlayerAttackRequest` transmet aujourd'hui `OffensiveItemDefinitionId` et `OffensiveEquipmentSlot`, **mais pas le `ItemLevel` et GUID de la source**. Capturer un contexte transitoire **avant** l'extraction d'une arme jetée ou la suppression d'un QuickItem ; ne pas résoudre après coup depuis une main vidée, ne pas utiliser arbitrairement `DefaultWeaponItemLevel`. Pour une frappe double, chaque sous-frappe conserve l'identité et le niveau de **sa propre** arme, jamais ceux de la MainHand pour l'OffHand. La reconstruction du contexte est de la présentation : aucune modification du nombre de coups ou de la formule de dégâts.

Le niveau est **fixe sur `FGridItemInstance::ItemLevel`**, déjà sauvegardé. Ne pas ajouter une copie `FXTier` dans le SaveGame ; au chargement, la présentation est reconstruite depuis le niveau persistant et les DataAssets. Sans instance réelle (aperçu de définition, Legacy niveau 0), apparence neutre explicite et **aucun N1 inventé**.

### 4.3 Défenses et synchronisation

Créer un petit point d'émission de présentation **après** résultat autoritaire d'attaque monstre, consommant ses flags et la source d'armure. Pour `ARM013`, récupérer le vrai GUID OffHand lorsque `bEquipmentParried`. Pour un bouclier, n'émettre qu'avec `EquipmentBlockedDamage > 0` et la source physique correspondante ; pour l'orbe, avec la diminution effective de sa charge magique. Événements disjoints : pas de `DefenseShieldAbsorb` sur une parade, pas de son « miss » pour une parade.

Aucune réaction FX ne prend de PA, ne modifie le tirage de parade, ne crée un blocage actif ni un événement de combat supplémentaire dans le flux. Clé d'idempotence de présentation : `(ActionInstanceId ou RequestId, ResolutionIndex, EventId, SourceItemGuid)`, durée de vie **transiente**. À la fin de la rencontre, sur changement de niveau, recharge, EndPlay ou remplacement du monde, libérer les Niagara et voix bouclées et vider les files.

**Synchronisation audio/projectile :** une flèche a déjà un véritable `AGridThrownItemActor` avec sa propre trajectoire et sa collision. Préférer une seule responsabilité audible/visuelle par événement d'impact : impact physique réel pour le choc au décor et la cible si instrumenté, repli au résultat de présentation si la trajectoire visuelle n'a pas atteint la cible. Une frappe calculée n'est **pas** un second projectile de combat ; aucune duplication du son de contact entre `ImpactHitAudio` et `ProjectileWorldImpact`. Les séquences de double frappe restent espacées selon ARMEMENT-LOT02.19.2.2 (départ MainHand puis OffHand), et conservent un son de chaque arme, jamais un unique `SoundCue` partagé qui étouffe le deuxième.

## 5. Évolution visuelle sans remailler : N1 → N20

**Niveau ≠ rareté ≠ enchantement gameplay.** Les niveaux ne changent que la **finition cosmétique** ; une épée N20 n'ajoute aucun statut ou pouvoir via son VFX. Les badges de niveau, l'icône de l'objet et les futurs cadres de rareté restent des systèmes distincts.

| Bande esthétique | Niveaux | Visuel commun proposé | Armes physiques / boucliers | Armes magiques / orbe |
|---|---:|---|---|---|
| **I — Brut** | 1–4 | Silhouette et PBR actuels | Acier, bois ou cuir mat ; aucune émission persistante | Pierre/cristal discret, pulsation quasi invisible |
| **II — Soigné** | 5–8 | Finition plus propre, reflets mieux contenus | Rehauts légers sur bords et rivets | Petite concentration lumineuse sur gemme |
| **III — Gravé** | 9–12 | Gravures/masques d'ornement révélés | Motifs sur lames, manches, boucliers | Runes fines, dégradé de couleur doux |
| **IV — Magistral** | 13–16 | Filets lumineux localisés, détail supplémentaire | Reflets métalliques et décoration, sans feu artificiel | Anneau de particules limité et pulses espacées |
| **V — Mythique** | 17–20 | Traitement distinctif mais lisible en donjon sombre | Ornement, gravure plus contrastée, flash d'impact élégant | Arcana/brume locale faible, glint unique à l'attaque |

Le niveau **à l'intérieur** d'une bande peut augmenter continûment un paramètre secondaire (reflet, contraste du masque, pulsation) plutôt que créer un nouveau mesh ou effet. `Band = clamp((ItemLevel-1)/4,0,4)`, mais la table ci-dessus reste **l'autorité de seuils**, pas un algorithme imposant de nouveaux effets au hasard. Un niveau 4 et un niveau 5 gardent la même silhouette et l'impression générale ; ne jamais créer cinq skins intégralement différents.

**Combinaison technique, par instance :**

```text
SM_Weapon_* EXISTANT + Material Slots EXISTANTS
  + paramètres Material Instance Dynamic (MID) créés sur l'instance visible,
    dérivés du matériau réellement assigné (pas de nouveau master forcé)
  + masque de gravures / surface déjà préparé dans le matériau si disponible
  + OrnamentMesh enfant NON collisionnant, facultatif
  + Niagara existant/partagé avec paramètres `User.*` dépendants de la bande
  + icône existante + badge Nxx dans l'UI (facultatif)
```

Paramètres artistiques proposés : `FX_Grade01` (0..1), `FX_EmissiveIntensity`, `FX_GlintIntensity`, `FX_AccentColor`, `FX_PatternReveal`, `FX_PulseSpeed`, `FX_OrnamentVisibility`. Ces noms sont **une convention à créer**, pas des paramètres déjà présents dans les matériaux de production. Utiliser les MIDs **par acteur/mesh**, jamais un Material Parameter Collection global pour différencier deux exemplaires de la même arme aux niveaux N3 et N17. Les `Material Slots` de chaque StaticMesh restent la source de matériau principal (MATERIAL-OWNERSHIP01). Un MID temporaire d'effet, ou un overlay indépendant, est admissible ; **ne pas écrire de remplacement principal permanent dans un DataAsset, un GridWorldObjectDefinitionAsset ou les slots de mesh**. Si le matériau actuel ne possède aucun paramètre, privilégier un petit overlay/ornement jusqu'à une mise à jour artistique du parent effectivement maîtrisée.

La famille visuelle guide les finitions : `acier-fin` (éclats précis), `acier-classique` (gravures de garde), `fer-lourd` (reliefs/rivets), `hampe` (ligatures et pointe), `bois-corde` (fibres/courbe de branche), `projectile-acier` (reflet fugitif, aucune aura qui trahit la trajectoire), `arcane`, `braise`, `elementaire`, `sacre`, `focus`, `bouclier` et `bouclier-sacre`. **Ne pas allumer toutes les armes physiques** simplement parce que leur niveau est élevé. Une arme déposée doit être la **même instance de niveau** visuellement que lorsqu'elle était équipée ; zéro modification de physique ou de pickup.

**État actuel du rendu permanent :** depuis MON11.4.2, une arme non lumineuse n'est **pas tenue en permanence** devant la caméra ; donc ne pas promettre un halo de lame visible en exploration première personne lorsqu'aucun `HeldItemActor` n'existe. La progression est visible au sol, lors d'une présentation réellement rendue, en inspection et éventuellement en vignette/tooltip. Les FX d'attaque peuvent être attachés à la caméra lorsque la représentation de l'arme est absente ; ne jamais créer une silhouette supplémentaire permanente sans ticket dédié.

## 6. Organisation et authoring Unreal 5.5.4

```text
/Game/GrimrockPrototype/
  Core/DataAssets/Weapons/
    DA_WeaponCatalogue                    [EXISTANT, stats N1–N20]
    DA_WeaponVisualCatalogue              [EXISTANT, meshes/icônes]
    DA_WeaponFXCatalogue                  [PROPOSÉ : profils audio/VFX/niveaux]
  Effects/Weapons/
    Audio/{Light,Steel,Blunt,Throw,Ranged,Magic,Guard}/
    Niagara/{Melee,Projectiles,Magic,Guard,Idle}/
  Art/Materials/Instances/Weapons/        [EXISTANT, préserver Material Slots]
  Art/Materials/WeaponFX/                 [PROPOSÉ : overlays/masques]
```

Nommage recommandé (exemples **non existants**) : `SC_WP_Light_Swing_01`, `SC_WP_Steel_Hit_01`, `SC_WP_MainGauche_Parry_01`, `SC_WP_RoundShield_Block_01`, `NS_WP_BladeTrail_Light`, `NS_WP_ParrySpark`, `NS_WP_ShieldBlock`, `MI_WP_RuneOverlay`. Aucune importation ou création de fichiers audio/PBR/Niagara n'est supposée par cette spécification.

**Chaîne de construction par arme :** manifester l'ID ARMXXX réel → lire les événements et slots de la matrice → sélectionner les médias appartenant à la bibliothèque partagée → authorer un profil famille → uniquement les surcharges spécifiques nécessaires → régler la bande niveau → vérifier les références soft dans UE (cook/package) → jouer en PIE et valider. Les assets de sons importés doivent avoir des droits de redistribution contrôlés. Les tailles d'échantillons et le mix se règlent à l'écoute, pas depuis le nom du fichier.

**Priorité de production :**
1. **FX01.1 — Pilote audible et visible :** ARM001 MainHand, ARM013 OffHand (attaque double + parade), ARM075 OffHand (bouclier), puis ARM062 MainHand (magie). Remplir les six champs offensifs **réels** quand ils existent ; instrumenter défense et level indépendamment. Valider le temps de frappe double et les deux sons réellement audibles.
2. **FX01.2 — Centralisation DataAsset et réactions :** créer `DA_WeaponFXCatalogue` et résolveur, événements de parade/blocage sans faux gameplay, fallback propre sur profils existants, test des deux mains et des GUID.
3. **FX01.3 — Familles :** terminer 22/22 lignes, projectile physique/impact, collisions et variations sonores ; ne pas utiliser `-NullRHI` comme preuve de rendu.
4. **FX01.4 — Esthétique N1–N20 :** matérialiser les cinq bandes, palettes familiales et variation par instance ; faire cohabiter deux copies **du même** ARMXXX à des niveaux différents dans une scène.
5. **FX01.5 — Audit final :** progression UI/monde, distance/occlusion, audio 2D vs 3D, performances, sauvegarde/Continue et régressions `Grimrock`.

Le pilote limite le nombre de fichiers binaires à authorer avant la preuve d'intégration. Aucun script ni commandlet **par arme** : conserver le principe industriel du seul pipeline `WeaponPipeline.ps1` et des tests de famille.

## 7. Critères d'acceptation et tests à réaliser

**Automatisation C++ (`Grimrock.RPG.ARMEMENT.FX01`, à créer) :** 22 correspondances exactes au manifeste Active ; aucun doublon `ArchetypeId` ; contenu des slots physiques et mains virtuelles ; aucun FX d'attaque sur passif ; `DefenseParry` exclusivement ARM013 OffHand et drapeau vrai ; `DefenseShieldAbsorb` exclusivement `EquipmentBlockedDamage>0` ; absorption de l'orbe seulement après débit magique réel ; absence de tout média et de coût sur action refusée ; 2 événements indépendants pour MainHand/OffHand d'une attaque double ; sélection de la bonne bande pour 1,4,5,8,9,12,13,16,17,20 ; niveau 0 neutre ; deux copies d'un ARMXXX de niveau différent visuellement indépendantes ; asset soft manquant → diagnostic et fallback ; aucun gameplay modifié et nul SaveGame de FX.

**Recette PIE/audio/vidéo :** vérifier des sons réellement audibles et différenciables dans l'ordre, volume et concurrence adaptés aux enceintes ; angle de lame, contact/miss distincts ; parade et bouclier visibles au bon instant ; projectile réellement récupérable sans duplication ; petits VFX occultés par mur, pas d'effet à travers la porte secrète ; contraste N1/N5/N9/N13/N17/N20 en donjon sombre sans masquer cible ni portrait ; pas de ralentissement sensible, ni fuite Niagara après plusieurs dizaines d'attaques ; restaurer la bonne teinte au retour d'une sauvegarde, avec même GUID et ItemLevel. **Les tests de compilation `-NullRHI` seuls ne valident ni audio réel ni rendu Niagara.**

**Budgets initiaux cibles (à mesurer en PIE, non constantes gameplay) :** une émission Attack et une émission Impact par frappe ; zéro doublon projectile ; au plus deux Niagara persistants visibles par arme en vue rapprochée, zéro sur objets hors scène après cullling ; effets ponctuels courts ; sons bouclés réservés aux rares focaliseurs actifs visuellement et coupés à la sortie de scène ; LOD/culling selon distance ; limiter surbrillance et brouillard à courte portée. La performance prévaut sur le nombre d'émetteurs.

**Localisation et UMG :** les nouveaux sons/Niagara/paramètres matériels ne créent aucun texte utilisateur. Si une ligne de tooltip/nom de palier est décidée ultérieurement, appliquer **`UI.csv → UIStringTableAuthoring → ST_UI.uasset → WBP → target Game fr/en`** avant toute construction de WBP, et garder le style graphique dans le Designer. Aucun nouveau widget nécessaire pour FX01.1.

## 8. Ce qui reste réellement à faire, hors FX

- **Pilote puis fabrication/importation de médias** : contenus `SoundWave/SoundCue/MetaSound`, Niagara, matériaux paramétriques et placements dans les vrais DataAssets ; la présente spécification ne crée aucun `.uasset`.
- **Save → quitter complètement le jeu → Continue** : une recette dédiée reste souhaitable si elle n'a pas été explicitement réalisée avec des objets de niveaux différents, des flèches, un projectile au sol et les charges de bouclier/orbe épuisées. Cela ne doit pas bloquer le pilote sonore.
- **Icônes d'actions** : vérifier si les douze icônes illustrées récemment ont été importées et assignées à `DA_EquippedWeaponActionIcons` dans UE ; un fichier image généré dans une conversation ne constitue pas une Texture2D cuisinée.
- **N1–N20** : les 440 profils statistiques des 22 actifs existent dans le catalogue, mais leur **rendu progressif** est précisément le chantier FX01.4. La rareté, la forge, les enchantements jouables et les compétences particulières restent des décisions RPG **séparées** ; ne pas les introduire à l'occasion des VFX.
- **Extension ARMXXX** : 22 archétypes sont actuellement Active ; les autres lignes du catalogue (82 au total) devront suivre la même procédure de production, sans copie de l'architecture FX.

## 9. Décision requise avant implémentation

Cette spécification propose **des finitions de niveau visibles mais maîtrisées**, sans changer la silhouette. Les choix de nuance exactes, des sons finaux et des systèmes Niagara seront approuvés lors de la recette artistique. En attendant, les conventions de couleurs/matériaux sont des propositions, les profils sont des données à créer et **aucun composant C++ ni DataAsset de production n'est modifié par ce document**.

**Références du projet :** [Guide de production ARMEMENT](../Architecture/ARMEMENT_PIPELINE_GUIDE.md), [Acceptation fonctionnelle N1](ARMEMENT_LOT02_16_N1_FUNCTIONAL_ACCEPTANCE.md), [Présentation des attaques](MON11_4_PLAYER_ATTACK_PRESENTATION.md), [Séquençage de la double frappe](ARMEMENT_LOT02_19_2_2_SEQUENCED_COMBAT_PRESENTATION.md), [Parade et boucliers](ARMEMENT_LOT02_19_3_PARRY_SHIELD_GUARD.md), [Niagara sur les items](../Architecture/ITEM_VFX01_DATA_DRIVEN_ITEM_NIAGARA.md), [Propriété des matériaux](../Architecture/MATERIAL_OWNERSHIP.md).
