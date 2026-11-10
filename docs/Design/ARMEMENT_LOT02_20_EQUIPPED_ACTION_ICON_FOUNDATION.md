# ARMEMENT-LOT02.20 — Contrat consolidé : actions d'armes équipées et icônes

> **CONTRAT DE BARRE ACTUEL (ARMEMENT-LOT02.21, 10.11.2026) :** la palette d'icônes et les catégories ARMXXX restent inchangées, mais la barre d'action ne projette **plus que la seule attaque normale en raccourci 1**. L'ancienne suggestion de MainHand/OffHand en emplacements 2+ est supprimée ; les sorts, capacités, consommables et actions spéciales restent personnalisables. Voir [la règle de projection](ARMEMENT_LOT02_21_SINGLE_EQUIPPED_ATTACK_HOTBAR.md).

**Date : 10.11.2026 — règle de prototype provisoire.** Ce jalon ne remplace pas le futur audit des compétences et ne crée aucun talent.

## Sources auditées

- Classeur canonique : **ARMEMENT01_Catalogue_Armes_Boucliers_Niveaux_1_20_ARMEMENT01_2.xlsx**, onglets Catalogue / Niveaux 1–20 / Classes / Implémentation UE5.
- Manifeste actif : **Config/Armement/WeaponProductionManifest.csv** ; catalogue cuisiné DA_WeaponCatalogue, définitions UGridItemDefinitionAsset et profils N1–N20.
- Autorité du combat : UGridTurnManagerComponent::GetAvailableCombatActions, FGridDualWieldPolicy::TryResolve, CanProvideAttackFromSlot et FGridCombatHotbarBinding.
- UI : WBP_GridCombatHudAction (une seule Image_ActionIcon), UI_UMG_PRESENTATION_CONTRACT et UI_WBP_LOCALIZATION_WORKFLOW.
- Les 82 archétypes du classeur ne correspondent **pas** à 82 DataAssets de production existants. Particularités éditoriales et intentions PLANNED ne sont pas des pouvoirs exécutables.

## Aptitudes spéciales liées à MainHand / OffHand : décision temporaire

**Aucune compétence d'armement particulière n'est exigée pour une action intrinsèque à l'équipement durant le prototype**, tant que le catalogue autoritaire propose réellement cette action et que toutes ses conditions de mains, armes, coûts et cibles sont respectées.

- Le Voleur/Ombre commence **avec l'aptitude d'attaque double au niveau 1**. Le moteur le permet déjà sans talent grâce à FGridDualWieldPolicy ; ne pas ajouter de Requirements artificiels.
- Tous les personnages équipant deux armes de mêlée compatibles disposent **provisoirement** de cette même attaque combinée (3 PA, MainHand 100 %, OffHand 50 %), sans création de talent débloqué ni modification d'arbre RPG.
- Les boucliers ARM072–082 fournissent leur protection passive. **Blocage actif non automatique** : pour l'obtenir, une vraie action Defense doit être authorée et prise en charge par le runtime. La palette n'invente rien.
- ARM013 mentionne « parade » dans le classeur ; cela ne signifie **pas** qu'une parade active existe. Le comportement relève d'un futur jalon distinct.
- Les capacités spéciales et talents RPG **déjà** authorés conservent tous leurs vrais Requirements, cooldowns, niveaux et effets. La convention provisoire ne les contourne pas.
- Les autres mentions narratives (saignement, brise-armure, contrôle, etc.) ne deviennent pas des actions par la seule présence de l'arme.

La révision des maîtrises/aptitudes et de l'arbre de compétences est **différée explicitement** ; le moment venu, les prérequis seront ajoutés au catalogue, pas à la résolution des icônes.

## Classification visuelle ARM001–ARM082

Une catégorie unique représente le **mode d'action équipé**. La source ARMXXX reste l'autorité des propriétés, dés, PA et portée. Les frontières de familles sont dérivées de l'onglet Catalogue ; aucun switch par arme individuelle.

| Archetypes | Famille | EGridEquippedWeaponActionIconKind |
| --- | --- | --- |
| ARM001–013 | Légères à une main | MeleeOneHand |
| ARM014–026 | Armes à une main | MeleeOneHand |
| Paire MainHand + OffHand offensive valide | Double frappe | MeleeDualWield |
| ARM027–037 | Lourdes deux mains | MeleeTwoHanded |
| ARM038–044 | Hast, souvent portée 2 cases | MeleePolearm |
| ARM045–048 | Arcs | RangedBow |
| ARM049–051 | Arbalètes | RangedCrossbow |
| ARM052–053 | Fronde, sarbacane | RangedOther |
| ARM054–060 | Armes de jet | ThrownWeapon |
| ARM061–065, ARM069 | Baguettes et sceptre | MagicOneHand |
| ARM066–068 | Bâtons magiques deux mains | MagicTwoHanded |
| ARM070–071 | Orbe / grimoire passif | Aucune action offensive |
| ARM072–082 | Boucliers passifs | Pas de blocage actif inventé ; DefenseBlock seulement pour une action réellement authorée |
| ARM013 | Main-gauche | DefenseParry seulement pour une action de parade réellement authorée |

**Mode d'action prioritaire :** si une hachette possède séparément un coup de mêlée et un lancer authorés, elle reçoit MeleeOneHand pour le premier, ThrownWeapon pour le second. Une arme legacy sans archetype ARM valide garde son icône originale. Les profils à tous les niveaux N1–N20 utilisent les mêmes catégories.

Les raccourcis de consommables (QuickItem), sorts mémorisés (Spell) et capacités (Ability) n'utilisent **jamais** cette classification, même si leurs icônes évoquent des armes.

## Architecture C++ réellement créée

- **FGridEquippedWeaponActionIconResolver::Classify** : détermine la famille de l'action existante, avec source Equipment et ItemDefinition valide. N'effectue aucune requête, aucun paiement de PA et aucune modification d'équipement.
- **UGridEquippedWeaponActionIconPalette** : UPrimaryDataAsset compact, map catégorie → Texture2D en référence soft. Le classeur et le manifeste ne contiennent aucun chemin graphique supplémentaire.
- **UGridCombatHudActionWidget::ResolveDisplayedIcon** : prend d'abord l'entrée configurée dans la palette, puis, seulement pour la double attaque et si la catégorie manque, l'ancien DualAttackIcon (migration non destructive), puis l'icône existante de l'action ARMXXX.
- **Aucune nouvelle Image dans le WBP.** La seule sortie est Image_ActionIcon. La vérification physique de l'objet (GUID, slot, ItemDefinitionId) empêche qu'une référence obsolète impose l'icône d'un autre équipement.
- Aucun ajout aux caractéristiques d'armes, aux 18 kits, au SaveGame, à l'initiative, aux réactions ou à la séquence des impacts.

## Paramétrage graphique dans UE5.5.4

1. Créer un DataAsset de classe **UGridEquippedWeaponActionIconPalette** appelé **DA_EquippedWeaponActionIcons** sous /Game/GrimrockPrototype/UI/Icons/Combat (chemin proposé, non créé dans ce commit).
2. Affecter les icônes disponibles dans la Map **Icons**, en laissant vides les catégories dont la Texture2D n'existe pas encore. Ne pas remplir la catégorie **None**.
3. Dans les Class Defaults du **WBP_GridCombatHudAction** existant (parent UGridCombatHudActionWidget), affecter **EquippedWeaponActionIconPalette** à ce DataAsset unique.
4. Lorsque l'icône **MeleeDualWield** est effectivement renseignée et validée en PIE, retirer éventuellement l'ancienne valeur **DualAttackIcon**. La propriété de repli est conservée côté C++ pour ne pas casser le WBP actuel.

Structure observée de votre WBP, **inchangée** :

~~~text
SizeBox_ActionRoot
└─ Button_Action
   └─ Overlay_Action
      ├─ SizeBox_ActionIcon
      │  └─ Image_ActionIcon
      └─ Text_ShortcutNumber
~~~

L'authoring du DataAsset et la sauvegarde du vrai WBP dans Unreal ne sont **pas** accomplis par un commit C++/Markdown, ni simulés par un faux .uasset. Les textures et le rendu final en PIE restent à contrôler.

## Tests, localisation et périmètre

- Test : **Grimrock.RPG.ARMEMENT.LOT02.20.EquippedActionIconCategories** : vérifie la partition ARM001–082, attaque double, défense réellement authorée, mode lancer/mêlée, aucune catégorie pour QuickItem/Spell et repli de palette.
- Régressions : **Grimrock.RPG.ARMEMENT.LOT02.19.2.1.ContextualDualAttackIcons** (image unique), **LOT02.19.2.N1DualWieldTwoProfilesOneCost** (3 PA une fois, deux jets), suite complète Grimrock.
- UI.csv → ST_UI → WBP → Game/fr/en : **aucun texte nouveau** ; CSV et String Table inchangés ; la création/sauvegarde des assets UMG reste une étape UE. La traduction anglaise des NSLOCTEXT historiques n'est pas attestée par cette compilation.
- **Hors périmètre** : changement de compétence, ajout de blocage/parade non authoré, sortilèges, consommables, nouveaux FX, synchronisation audio/impacts, création de textures.

À suivre : **ARMEMENT-LOT02.19.2.2** séquencement temporel des frappes, puis **ARMEMENT-LOT02.19.3** parade réelle (et éventuellement blocage actif) ; sans usine à gaz UMG.
