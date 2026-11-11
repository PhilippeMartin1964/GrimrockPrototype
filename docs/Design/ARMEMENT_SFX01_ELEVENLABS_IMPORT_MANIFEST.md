# ARMEMENT-SFX01 — Inventaire des 64 exports ElevenLabs (11.11.2026)

**Nature :** documentation et rapprochement par noms tronqués, horodatages et prompts de cette conversation. Les 64 WAV sont sur l'ordinateur de l'utilisateur : **non fournis ici**, donc aucun contrôle acoustique, renommage physique, import UE ou test PIE.

## Bilan

- **16 lots de quatre sorties = 64 WAV**, tous explicitement présents dans le CSV associé.
- **14 prompts sur les 15 de la série Sword rapprochés** (56 WAV). Quatre WAV supplémentaires proviennent du prompt de comparaison antérieur commençant par « A realistic medieval longsword... ».
- **Quatre WAV non classés** : deuxième lot commençant par `A_single_heavy_medie_`, IDs 1794409542433 à 1794409556239. Possible reprise d'un autre prompt, mais impossible à affirmer depuis le nom tronqué.
- **Sword Parry — Heavy absent** : aucun fichier commençant par `Two_medieval_steel...` pour le prompt « Two medieval steel swords collide forcefully... ». Ne pas inventer ce lot.
- Les répétitions `A_single_realistic_m_`, `A_single_heavy_medie_` et `A_single_devastating_` sont **désambiguïsées par la chronologie**, non par le contenu audio. Vérification ElevenLabs nécessaire avant authoring définitif.

**Manifeste exhaustif et utilisable pour renommer les fichiers :** [ARMEMENT_SFX01_ELEVENLABS_IMPORT_MANIFEST.csv](ARMEMENT_SFX01_ELEVENLABS_IMPORT_MANIFEST.csv).

## Nomenclature Unreal Engine

Répertoire proposé : `/Game/GrimrockPrototype/Audio/Weapons/Sword/` avec sous-répertoires Swing, Impact, Parry, HeavyAttack, Critical et Review.
Schéma : `SFX_Sword_[Action]_[MatériauOuGeste]_[01..04].wav`. Le SoundWave UE5 reçoit le nom sans extension ; la colonne UnrealWav du CSV porte le nom exact prévu.

## Correspondances (chaque ligne vaut 4 fichiers)

| Lot | Préfixe original | Premier ID | Début du prompt / catégorie | Nom Unreal (NN = 01–04) | ARMXXX | Routage | Statut |
|---|---|---|---|---|---|---|---|
| SW-REF | `A_realistic_medieval_#N-...` | `1794403594476` | Sword Swing (ancien essai) — « A realistic medieval longsword slicing rapidly through the air… » | `SFX_Sword_Swing_Reference_NN` | ARM014;ARM015 à vérifier | `AttackStart/Reference` | `REVIEW` |
| SW-STD | `A_single_realistic_m_#N-...` | `1794409479075` | Sword Swing Standard — « A single realistic medieval longsword being swung swiftly through the air… » | `SFX_Sword_Swing_Standard_NN` | ARM014;ARM015 | `AttackStart/Standard` | `CANDIDATE` |
| SW-FAST | `A_single_extremely_f_#N-...` | `1794409500623` | Sword Swing Fast — « A single extremely fast medieval sword slash cutting through the air… » | `SFX_Sword_Swing_Fast_NN` | ARM014;ARM015 | `AttackStart/Fast` | `CANDIDATE` |
| SW-HEAVY | `A_single_heavy_medie_#N-...` | `1794409520668` | Sword Swing Heavy — « A single heavy medieval two-handed sword being swung forcefully through the air… » | `SFX_Sword_Swing_Heavy_NN` | épée à 2 mains future; ARM015 conditionnel | `AttackStart/Heavy` | `CONDITIONAL` |
| SW-EXTRA | `A_single_heavy_medie_#N-...` | `1794409542433` | Lot Heavy supplémentaire non identifié — « A single heavy medieval... (prompt non déterminé)… » | `SFX_Sword_Review_HeavyMedieval_NN` | non attribué | `—` | `HOLD` |
| IM-FLESH | `A_single_realistic_m_#N-...` | `1794409561928` | Sword Impact Flesh — « A single realistic medieval steel sword striking and cutting into a solid fleshy creature… » | `SFX_Sword_Impact_Flesh_NN` | ARM014;ARM015 | `ImpactHit/Flesh` | `FUTURE_FX01` |
| IM-METAL | `A_single_powerful_me_#N-...` | `1794409581380` | Sword Impact Metal — « A single powerful medieval steel sword striking a solid steel plate armor… » | `SFX_Sword_Impact_Metal_NN` | ARM014;ARM015 | `ImpactHit/Metal` | `FUTURE_FX01` |
| IM-WOOD | `A_single_heavy_medie_#N-...` | `1794409595743` | Sword Impact Wood — « A single heavy medieval steel sword striking a thick solid wooden shield… » | `SFX_Sword_Impact_Wood_NN` | ARM014;ARM015 | `ImpactHit/Wood` | `FUTURE_FX01` |
| PAR-LIGHT | `A_single_quick_parry_#N-...` | `1794409612520` | Sword Parry Light — « A single quick parry between two medieval steel swords… » | `SFX_Sword_Parry_Light_NN` | ARM013 OffHand | `DefenseParry/Light` | `FUTURE_FX01` |
| PAR-DEFLECT | `A_medieval_sword_swi_#N-...` | `1794409654134` | Sword Parry Deflection — « A medieval sword swiftly deflects an incoming steel blade… » | `SFX_Sword_Parry_Deflection_NN` | ARM013 OffHand | `DefenseParry/Deflection` | `FUTURE_FX01` |
| HA-SWING | `A_single_powerful_sw_#N-...` | `1794409667052` | Sword Heavy Attack Swing — « A single powerful swing of a massive medieval two-handed sword… » | `SFX_Sword_HeavyAttack_Swing_NN` | épée à 2 mains future; ARM015 conditionnel | `AttackStart/HeavySwing` | `CONDITIONAL` |
| HA-OVERHEAD | `A_massive_medieval_g_#N-...` | `1794409683814` | Sword Heavy Attack Overhead — « A massive medieval greatsword swung violently downward in one brutal overhead attack… » | `SFX_Sword_HeavyAttack_Overhead_NN` | épée à 2 mains future | `AttackStart/HeavyOverhead` | `CONDITIONAL` |
| HA-HORIZ | `A_gigantic_medieval__#N-...` | `1794409696814` | Sword Heavy Attack Horizontal — « A gigantic medieval greatsword performing one devastating horizontal swing… » | `SFX_Sword_HeavyAttack_Horizontal_NN` | épée à 2 mains future | `AttackStart/HeavyHorizontal` | `CONDITIONAL` |
| CR-FLESH | `A_single_devastating_#N-...` | `1794409712617` | Sword Critical Hit Flesh — « A single devastating critical hit from a medieval steel sword striking a fleshy creature… » | `SFX_Sword_Critical_Flesh_NN` | ARM014;ARM015 | `CriticalAccent/Flesh` | `FUTURE_FX01` |
| CR-METAL | `A_single_devastating_#N-...` | `1794409741328` | Sword Critical Hit Metal — « A single devastating critical strike from a heavy medieval sword against thick steel plate armor… » | `SFX_Sword_Critical_Metal_NN` | ARM014;ARM015 | `CriticalAccent/Metal` | `FUTURE_FX01` |
| CR-HEAVY | `A_single_brutal_crit_#N-...` | `1794409764744` | Sword Critical Hit Heavy — « A single brutal critical blow from a massive medieval greatsword hitting a large armored monster… » | `SFX_Sword_Critical_Heavy_NN` | épée à 2 mains future; autres cas selon animation | `CriticalAccent/Heavy` | `CONDITIONAL` |

## Correspondance avec les Data Assets du dépôt

- **ARM014**, épée droite : `DA_Weapon_StraightSword_01` ; **ARM015**, épée longue : `DA_Weapon_LongSword_01` ; tous deux `MeleeOneHand` et équipés en MainHand.
- **ARM013**, Main-Gauche : `DA_Weapon_MainGauche_01` ; parade uniquement depuis OffHand **si** `bEquipmentParried` est vrai. Aucun effet de parade ne confère une parade aux deux épées.
- Champs **actuellement implémentés** dans `UGridItemDefinitionAsset::PlayerAttackPresentationProfile` : `AttackAudio.Sounds[]`, `ImpactHitAudio.Sounds[]` et `ImpactMissAudio.Sounds[]` (plus volume et pitch). `Sounds[]` sélectionne des variantes, **pas** des matériaux.
- Pour un **pilote sans nouveau C++**, après écoute : mettre les 4 `SW-STD` et 4 `SW-FAST` dans `AttackAudio.Sounds[]` des deux DA d'épées (ou un sous-ensemble sélectionné). Ne pas y ajouter les sons lourds deux mains arbitrairement.
- Les événements `ImpactHit/Flesh|Metal|Wood`, `DefenseParry`, `CriticalAccent` et les gestes spécialisés HeavyAttack exigent le **catalogue FX01 futur** `UGridWeaponFXCatalogueAsset` / `DA_WeaponFXCatalogue` : ils **ne sont pas encore des propriétés réelles du projet**.
- Ne jamais mélanger Flesh, Metal, Wood dans le même `ImpactHitAudio.Sounds[]` en prétendant qu'Unreal choisit le matériau. Ne pas jouer critique et impact normal complets en double.
- La palette `DA_EquippedWeaponActionIcons` concerne uniquement les **icônes** ; les slots de barre de raccourcis 0–9 ne définissent pas les SFX.

## Règles de contrôle

1. Retrouver les quatre sons du lot `SW-EXTRA` dans l'historique ElevenLabs et lire le **prompt complet**. Le lot est `HOLD` : ne pas importer en production avant clarification.
2. Vérifier si le prompt `Two medieval steel swords collide forcefully...` a été exécuté ; aucune sortie correspondante n'est présente dans cette liste.
3. Écouter chaque WAV et écarter les sorties avec événements multiples, voix, bruit de fond ou durée excessive. Ne pas déduire la qualité d'un nom de fichier.
4. Conserver les originaux hors du Content, puis copier/renommer uniquement les variantes retenues en utilisant les deux premières colonnes du manifeste CSV.
5. Pour les SoundWave UE5, viser WAV PCM 48 kHz ; contrôler import, mixage, concurrence et synchronisation PIE ; ne pas attribuer de capacité de combat par le seul SFX.

**Documents de référence :** [ARMEMENT-FX01](ARMEMENT_FX01_AUDIO_VFX_PROGRESSION_N1_N20.md), `Config/Armement/WeaponProductionManifest.csv`, `GridPlayerAttackPresentationTypes.h`.
