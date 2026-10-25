# ARMEMENT-INDUS04.3A — Simulation globale d'équipement (read-only)

**Statut : sources proposées sur master ; compilation UE5.5.4 et tests LOCAUX à réaliser.**
Périmètre : moteur de projection avant/après ; **aucun** changement d'UMG,
de \`UI.csv\`, de DataAssets d'armes, de combat ou de sauvegarde. La mise
en page 04.2C personnalisée par l'utilisateur reste intacte.

## Motivation

Le tooltip 04.2B compare des objets **un par un** via
\`FGridItemTooltipEquipmentComparison\`. Cela masque le coût réel d'équiper
une arme à deux mains lorsque la main secondaire porte un bouclier.
Les caractéristiques finales (bonus de Force, PV, Mana, résistances,
protection d'armure physique) doivent être évaluées sur l'**ensemble**
de l'équipement, pas uniquement dans le slot visé.

La fonction réelle \`UGridPartyInventoryComponent::EquipItemFromInventorySlot\`
autorise une arme \`TwoHanded\` en MainHand ou OffHand, mais l'installe
**physiquement une seule fois dans MainHand** en retirant les DEUX objets
physiquement présents. Si une arme à deux mains était déjà équipée,
l'activation de l'une des mains la retire. Avant toute mutation, les
objets retirés doivent tenir dans le sac ; le slot d'inventaire contenant
la candidate est libéré et peut accueillir le premier objet déplacé.

## API disponible

\`UGridPartyInventoryComponent::PreviewEquipFromInventorySlot(
   CharacterIndex, InventorySlotIndex, TargetSlot, OutPreview)\`

Retourne \`false\` pour entrées ou équipement invalides. Une projection
de changement **valide mais impossible faute de place** retourne \`true\`
avec \`bValidPreview=true\`, \`bCanEquipNow=false\`,
\`Status=InventoryFull\`. Ne jamais assimiler \`bValidPreview\` à
une autorisation d'équiper.

\`FGridEquipmentPreviewResult\` (dans
\`Public/Runtime/GridEquipmentPreviewTypes.h\`) contient :

- \`RequestedSlot\` et \`ActualTargetSlot\` ;
- \`bCandidateUsesTwoHands\`, \`bWouldReleaseBothHands\` ;
- \`DisplacedItems\` par objet **physique** (Slot d'origine,
  ItemDefinitionId, identité runtime, nom, armure restante) ;
- \`RequiredSlotsForDisplacedItems\`,
  \`FreeInventorySlotsAfterSourceRemoval\`, \`bCanEquipNow\` et statut ;
- bonus de caractéristiques \`BeforeBonuses\`/\`AfterBonuses\`, résistances
  \`BeforeResistances\`/\`AfterResistances\`, armure physique effective
  \`EffectivePhysicalArmorBefore\`/\`After\` ;
- \`NumericDeltas\`, avec clés \`StatId\`, valeurs \`Before\`, \`After\`,
  \`Delta=After-Before\`. **19 indicateurs**, même si delta nul :
  six attributs finaux, PV/Mana maximaux, capacité de charge maximale
  finale (base + bonus d'équipement), bonus nominal d'armure, huit
  résistances, armure effective courante.
  La capacité de base reste identique avant/après.

La valeur effective d'armure physique respecte la règle existante :
\`CurrentPhysicalArmor\` de la réserve du personnage
+ les bonus équipés restants après
\`AbsorbedEquipmentPhysicalArmor\`, avec la même saturation que
\`GetEffectiveCurrentPhysicalArmor\`. Une armure \`+5\` qui a déjà
absorbé \`3\` ne fournit plus que \`2\`, pas \`5\`.

La simulation repose exclusivement sur des copies locales de
\`FGridCharacterEquipmentState\` : **aucun objet réel déplacé**, aucune
mutation de l'identité, des niveaux N1–N20, des pools d'armure, de
l'inventaire ou des delegates. Il n'y a aucune estimation de dégâts finaux
après résistances ennemies ; ce n'est pas une nouvelle résolution de combat.

**Attention à la cohérence des slots :** la prévisualisation consulte
\`CanEquipItemToSlot\` / \`CanEquipToSlot\` comme l'action réelle.
Une arme \`OneHanded\` est autorisée en \`MainHand\` ET \`OffHand\`,
même si son ancien \`CompatibleEquipmentSlots\` ne contient que
\`MainHand\`. Le tooltip historique 04.2B ne suit pas toujours cette
règle ; sa correction d'affichage et le choix de la simulation à
présenter relèvent de 04.3B.

## Tests automatiques 04.3A

Nouveau filtre dédié :

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.UI.EquipmentPreview043A"

Attendu **4/4**, 0 avertissement ou erreur. Tests synthétiques
sur un composant temporaire ; aucune modification des DataAssets réels.

1. **TwoHandsShieldArmorAndNoMutation** : épée +2 Force, bouclier +5
   physique (+3 déjà absorbés) et +3 résistance feu, candidat hache
   deux mains +7 Force. Attendus : 2 déplacements physiques,
   \`Delta Force=+5\`, \`Delta armure effective=-2\`,
   \`Delta résistance feu=-3\`. Comparaison du résultat simulé à la
   véritable transaction après vérification que la simulation n'a
   touché ni équipement, ni sac, ni armure absorbée.
2. **FullBagReadOnlyBlock** : sac d'une place occupé par l'arme candidate.
   Cette place se libère, mais deux objets doivent être rangés.
   Projection affichable, **équipement impossible** ; aucun objet déplacé.
3. **OneHandAndVirtualOffHand** : remplacement du bouclier par une arme
   \`OneHanded\` dans OffHand, MainHand inchangée, puis remplacement
   d'une hache à deux mains déjà installée : une seule instance
   physique de hache doit être comptée, pas deux.

4. **ProductionGreatAxeAndRoundShield** : vrais DataAssets ARM031 et
   ARM075 N1 en lecture seule. Vérifie l'usage deux mains de la hache,
   le déplacement du bouclier et la perte de ses 5 points de
   protection physique tant qu'ils sont tous disponibles.

Le filtre préexistant \`Grimrock.UI.WeaponTooltip04\` doit rester
**8/8** sans régression ; \`Grimrock.UI.Item01\` doit rester **5/5**.

## 04.3B et 04.3C (NON inclus dans 04.3A)

- **04.3B** sélectionnera un ou deux scénarios d'équipement réalistes
  dans le tooltip, produira les lignes \`FText\` localisées par
  \`Localization/Source/UI.csv\` -> \`ST_UI\`, et distinguera les pertes
  d'armure, les changements d'attaque et la capacité du sac. Éviter
  d'additionner des dégâts de deux types incomparables.
- **04.3C** associera les icônes historiques et d'armement dans les
  \`StatIconTextures\` du \`WBP_ItemTooltipNativeLine\` et appliquera
  les réglages Designer, avec validation PIE. Aucun Graph complexe.
- La prévisualisation de remplacement d'un objet depuis le **curseur**
  ou depuis un slot déjà équipé n'est pas couverte par cette API :
  la signature est explicitement **InventorySlot-backed**, comme
  \`EquipItemFromInventorySlot\`, pour garantir un calcul de capacité
  exact. Étendre l'API avec une route explicite quand nécessaire.
- La simulation ne valide pas le poids maximal ni d'éventuels prérequis
  RPG futurs qui ne font pas partie de la transaction actuelle.
  Toute règle nouvelle devra être ajoutée à l'autorité d'équipement
  avant d'être simulée, jamais uniquement dans le tooltip.

**Règle de sortie :** ne pas annoncer 04.3A validé sans journal
local de compilation + 4/4 + vérification des suites historiques.
