# ARMEMENT-SFX01 — Inventaire des 72 exports ElevenLabs (11.11.2026)

**Révision du 11.11.2026 — correspondances confirmées par l'auteur des générations.** L'inventaire documente les WAV téléchargés, sans analyse de leur audio ni import Unreal. La liste fournie inclut 72 fichiers distincts, dont un essai antérieur aux 15 prompts Sword.

## 1. Bilan corrigé

- **72 fichiers WAV = 18 générations × 4 variantes**, avec identifiants de téléchargement distincts.
- **15 prompts Sword sur 15 représentés**. Deux prompts ont été lancés deux fois : **Sword Swing Heavy** (8 sons) et **Sword Parry Heavy** (8 sons). Chaque autre prompt Sword fournit 4 sons.
- **4 sons supplémentaires** proviennent d'un ancien essai commençant par « A realistic medieval longsword slicing rapidly through the air ».
- Le lot autrefois indéterminé `SW-EXTRA` est confirmé comme la seconde génération de **Sword Swing Heavy** : désormais `SW-HEAVY-R2`. Aucun son ne reste non classé par prompt.
- Les **8 nouveaux exports** `Two_medieval_steel_s_` appartiennent aux deux générations du prompt « Two medieval steel swords collide forcefully during a powerful defensive parry ».
- Les sons sont identifiés par **nom et ID source**, pas par l'écoute : les variantes individuelles restent à valider qualitativement.

**Manifeste exhaustif des 72 fichiers et noms UE5 :** [ARMEMENT_SFX01_ELEVENLABS_IMPORT_MANIFEST.csv](ARMEMENT_SFX01_ELEVENLABS_IMPORT_MANIFEST.csv).

## 2. Convention de nommage

Répertoire Unreal conseillé : `/Game/GrimrockPrototype/Audio/Weapons/Sword/` avec sous-dossiers `Swing`, `Impact`, `Parry`, `HeavyAttack`, `Critical`, et `Review` pour l'essai ancien.

- Nommage du WAV/SoundWave : `SFX_Sword_[Famille]_[StyleOuSurface]_[NN].wav` ; l'asset UE5 conserve ce nom sans extension.
- Un prompt lancé une fois reçoit les variantes `_01` à `_04`. Les **secondes générations du même prompt** reçoivent `_05` à `_08` ; les numéros `#1` à `#4` d'ElevenLabs sont préservés uniquement dans la colonne `OriginalWav`.
- Les identifiants d'export restent nécessaires pour distinguer les fichiers dont le préfixe est identique.

## 3. Index des 18 lots (4 fichiers par lot)

| Lot | Prompt / action | Premier fichier original (ID) | Famille SoundWave | Variantes | ARMXXX | Slot et état |
|---|---|---:|---|---|---|---|
| `SW-REF` | Ancien essai de Sword Swing | `1794403594476` | `SFX_Sword_Swing_Reference` | 01–04 | ARM014, ARM015 à vérifier | `AttackAudio.Sounds[] si retenu` — `REVIEW` |
| `SW-STD` | Sword Swing Standard | `1794409479075` | `SFX_Sword_Swing_Standard` | 01–04 | ARM014, ARM015 | `AttackAudio.Sounds[]` — `CANDIDATE` |
| `SW-FAST` | Sword Swing Fast | `1794409500623` | `SFX_Sword_Swing_Fast` | 01–04 | ARM014, ARM015 | `AttackAudio.Sounds[]` — `CANDIDATE` |
| `SW-HEAVY` | Sword Swing Heavy (génération 1) | `1794409520668` | `SFX_Sword_Swing_Heavy` | 01–04 | future épée deux mains, ARM015 conditionnel | `AttackStart/Heavy (proposé)` — `CONDITIONAL` |
| `SW-HEAVY-R2` | Sword Swing Heavy (génération 2) | `1794409542433` | `SFX_Sword_Swing_Heavy` | 05–08 | future épée deux mains, ARM015 conditionnel | `AttackStart/Heavy (proposé)` — `CONDITIONAL` |
| `IM-FLESH` | Sword Impact Flesh | `1794409561928` | `SFX_Sword_Impact_Flesh` | 01–04 | ARM014, ARM015 | `ImpactHit/Flesh (proposé)` — `FUTURE_FX01` |
| `IM-METAL` | Sword Impact Metal | `1794409581380` | `SFX_Sword_Impact_Metal` | 01–04 | ARM014, ARM015 | `ImpactHit/Metal (proposé)` — `FUTURE_FX01` |
| `IM-WOOD` | Sword Impact Wood | `1794409595743` | `SFX_Sword_Impact_Wood` | 01–04 | ARM014, ARM015 | `ImpactHit/Wood (proposé)` — `FUTURE_FX01` |
| `PAR-LIGHT` | Sword Parry Light | `1794409612520` | `SFX_Sword_Parry_Light` | 01–04 | ARM013 OffHand | `DefenseParry/Light (proposé)` — `FUTURE_FX01` |
| `PAR-HEAVY-R1` | Sword Parry Heavy (génération 1) | `1794409625160` | `SFX_Sword_Parry_Heavy` | 01–04 | ARM013 OffHand | `DefenseParry/Heavy (proposé)` — `FUTURE_FX01` |
| `PAR-HEAVY-R2` | Sword Parry Heavy (génération 2) | `1794409639359` | `SFX_Sword_Parry_Heavy` | 05–08 | ARM013 OffHand | `DefenseParry/Heavy (proposé)` — `FUTURE_FX01` |
| `PAR-DEFLECT` | Sword Parry Deflection | `1794409654134` | `SFX_Sword_Parry_Deflection` | 01–04 | ARM013 OffHand | `DefenseParry/Deflection (proposé)` — `FUTURE_FX01` |
| `HA-SWING` | Sword Heavy Attack Swing | `1794409667052` | `SFX_Sword_HeavyAttack_Swing` | 01–04 | future épée deux mains | `AttackStart/HeavySwing (proposé)` — `CONDITIONAL` |
| `HA-OVERHEAD` | Sword Heavy Attack Overhead | `1794409683814` | `SFX_Sword_HeavyAttack_Overhead` | 01–04 | future épée deux mains | `AttackStart/HeavyOverhead (proposé)` — `CONDITIONAL` |
| `HA-HORIZ` | Sword Heavy Attack Horizontal | `1794409696814` | `SFX_Sword_HeavyAttack_Horizontal` | 01–04 | future épée deux mains | `AttackStart/HeavyHorizontal (proposé)` — `CONDITIONAL` |
| `CR-FLESH` | Sword Critical Hit Flesh | `1794409712617` | `SFX_Sword_Critical_Flesh` | 01–04 | ARM014, ARM015 | `CriticalAccent/Flesh (proposé)` — `FUTURE_FX01` |
| `CR-METAL` | Sword Critical Hit Metal | `1794409741328` | `SFX_Sword_Critical_Metal` | 01–04 | ARM014, ARM015 | `CriticalAccent/Metal (proposé)` — `FUTURE_FX01` |
| `CR-HEAVY` | Sword Critical Hit Heavy | `1794409764744` | `SFX_Sword_Critical_Heavy` | 01–04 | future épée deux mains; ARM014/015 conditionnel | `CriticalAccent/Heavy (proposé)` — `CONDITIONAL` |

## 4. Justification des trois lots au préfixe identique

| Préfixe d'export | IDs des quatre variantes | Correspondance confirmée | Renommage Unreal |
|---|---|---|---|
| `A_single_heavy_medie_` | 1794409520668, 1794409528806, 1794409533829, 1794409538154 | **Swing Heavy — génération 1** | `SFX_Sword_Swing_Heavy_01…04` |
| `A_single_heavy_medie_` | 1794409542433, 1794409546806, 1794409551951, 1794409556239 | **Swing Heavy — génération 2** | `SFX_Sword_Swing_Heavy_05…08` |
| `A_single_heavy_medie_` | 1794409595743, 1794409601383, 1794409605373, 1794409608743 | **Impact Wood** | `SFX_Sword_Impact_Wood_01…04` |

## 5. Deux générations du prompt Sword Parry Heavy

- `PAR-HEAVY-R1` : identifiants 1794409625160, 1794409628828, 1794409632569, 1794409636037 ; renommage `SFX_Sword_Parry_Heavy_01…04`.
- `PAR-HEAVY-R2` : identifiants 1794409639359, 1794409644298, 1794409647863, 1794409650962 ; renommage `SFX_Sword_Parry_Heavy_05…08`.
- **Prompt commun :** « Two medieval steel swords collide forcefully during a powerful defensive parry… ».

## 6. Correspondances réelles des Data Assets

- `ARM014` — Épée droite : `DA_Weapon_StraightSword_01` ; `ARM015` — Épée longue : `DA_Weapon_LongSword_01`. Ces deux armes sont `MeleeOneHand` en MainHand.
- `ARM013` — Main-Gauche : `DA_Weapon_MainGauche_01` ; parade défensive **uniquement en OffHand et lorsque `bEquipmentParried` est réellement vrai**.
- Champs **existants** : `UGridItemDefinitionAsset::bProvidesAttackPresentation` et `PlayerAttackPresentationProfile.AttackAudio.Sounds[]`, `ImpactHitAudio.Sounds[]`, `ImpactMissAudio.Sounds[]` (volume/pitch associés). `Sounds[]` sélectionne des variantes, **pas** des familles de matériaux.
- Pilote immédiat possible après écoute : assigner les WAV retenus des lots `SW-STD` et `SW-FAST` à `AttackAudio.Sounds[]` des deux DA d'épées ; ne pas y mélanger Flesh, Metal ou Wood.
- Les événements `ImpactHit/Flesh|Metal|Wood`, `DefenseParry`, `CriticalAccent` et les attaques lourdes spécialisées relèvent de **l'extension FX01 non implémentée** : futur `UGridWeaponFXCatalogueAsset` / `DA_WeaponFXCatalogue`. Les termes des colonnes FX01 du CSV sont **des slots proposés**, non des propriétés éditables actuellement.
- Aucun son de parade ne confère une parade aux épées ARM014 ou ARM015. Le son critique doit répondre à un vrai critique et ne pas doubler l'impact. Les prompts d'épées deux mains ne confèrent pas de nouvelle action à une arme une main.
- `DA_EquippedWeaponActionIcons` reste une palette d'icônes, non un Data Asset sonore. Les raccourcis HUD 0–9 ne sont pas non plus des slots de SFX.

## 7. Contrôles avant import UE5

1. Écouter et évaluer les 72 sons individuellement (plusieurs coups, voix, bruit parasite, temps mort, saturation et réverbération). Les confirmations présentes portent **sur les prompts**, pas sur la qualité des fichiers.
2. Garder les fichiers ElevenLabs d'origine intacts hors du Content. Copier et renommer uniquement les SoundWave retenus à partir du CSV ; si certaines variantes sont rejetées, conserver leur ID de traçabilité.
3. Vérifier le format réel des WAV et convertir si nécessaire en PCM 48 kHz adapté au pipeline sonore UE5.
4. Associer uniquement les sons du système de présentation **actuellement disponibles** ; attendre le catalogue FX01 pour les sélections par matériau, parade, critique et attaque lourde.
5. Valider ensuite en PIE le déclenchement selon la véritable attaque, la vraie main et le résultat autoritaire, sans doublons.

**Périmètre de cette révision : documentation GitHub uniquement.** Aucun WAV n'a été téléversé ou importé dans Unreal ; aucun test UE, compilation ou validation d'écoute n'a été réalisé.

**Références :** [ARMEMENT-FX01](ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md), `Config/Armement/WeaponProductionManifest.csv`, `Source/GrimrockPrototype/Public/Runtime/Combat/GridPlayerAttackPresentationTypes.h`.
