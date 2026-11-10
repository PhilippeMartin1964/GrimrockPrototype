# ARMEMENT-LOT02.19.1 — Main secondaire jouable et barre d'action cohérente

> **HISTORIQUE :** ce jalon introduisait l'attaque OffHand autonome avant l'attaque double de 02.19.2. Le raccourci 1 est désormais **contextuel : double frappe si deux armes sont éligibles, puis MainHand, OffHand, mains nues**. Pour le fonctionnement actuel des icônes et la règle provisoire d'aptitudes d'armes, consulter [ARMEMENT-LOT02.20](ARMEMENT_LOT02_20_EQUIPPED_ACTION_ICON_FOUNDATION.md). Les étapes originelles ci-dessous restent une trace de test et non une recette WBP courante.

## Périmètre

Incident PIE signalé après validation ARMEMENT-LOT02.18 : Voleur / Ombre commence avec **ARM001 dague MainHand + ARM013 Main-gauche OffHand**. Le raccourci primaire n'affiche que la dague ; aucune action Main-gauche n'est proposée sans intervention manuelle, ce qui rend son rôle offensif invisible.

Ce jalon rend **l'attaque indépendante OffHand N1 accessible dès l'entrée dans le donjon**, sans inventer de double frappe, d'attaque gratuite, de parade ou de talent niveau 2.

## Diagnostic de l'existant

- `UGridTurnManagerComponent::BuildPlayerCombatActionContributions` parcourt physiquement MainHand et OffHand, avec `ItemDefinitionId`, `RuntimeObjectId`, `SourceEquipmentSlot` et le profil d'attaque de chaque item.
- `RequestCharacterCombatAction` accepte déjà une source OffHand identifiée et la chaîne de résolution sait débiter **les PA de cette action**, toucher la cible et produire les effets ordinaires.
- `FGridCombatHudViewModelBuilder::BuildHotbarActions` n'affiche que les `FGridCombatHotbarBinding` sauvegardés ; `ApplyPrimaryAttackProjection` force le raccourci 1 à MainHand, puis `Attack_Unarmed`. Les autres emplacements restent vides jusqu'au glisser-déposer.
- `CanEquipToSlot(OffHand)` est volontairement plus permissif que `CanProvideAttackFromSlot(OffHand)` : ce dernier exige un slot OffHand approuvé et une vraie attaque. Un équipement passif, une dague MainHand-only ou la représentation virtuelle d'une arme à deux mains **ne doit pas déclencher d'attaque OffHand**.
- `ARM013` est une vraie arme offensive à une main avec action N1, compatible MainHand et OffHand. La mention de conception « Parade en main secondaire » n'est **pas** une parade programmée : ce sujet est réservé à 02.19.3.

## Contrat d'interface appliqué — fondation de la future double attaque

| Contexte réel | Raccourci 1 (alias protégé PrimaryAttack) | Premier raccourci libre à partir de 2 |
| --- | --- | --- |
| MainHand seule | Attaque MainHand | Libre |
| MainHand + OffHand offensive | Attaque MainHand | **Suggestion contextuelle** d'attaque OffHand |
| OffHand offensive seule | Attaque OffHand | Libre : aucune seconde icône |
| Deux mains (une arme physique MainHand) | Attaque MainHand | Libre : pas de copie virtuelle |
| MainHand + bouclier/orbe passif | Attaque MainHand | Libre : pas d'attaque fictive |
| Aucune arme offensive | Attaque à mains nues | Libre |
| OffHand assignée par le joueur à un autre raccourci | MainHand si présente | Pas de suggestion doublonnée |
| Raccourcis 2+ déjà personnalisés | Inchangés | Suggestion dans le **premier véritable emplacement vide**, ou aucune si la barre est pleine |

Une suggestion est calculée à partir de **l'action OffHand réelle du TurnManager**. Elle affiche l'icône, le nom, la disponibilité, le coût en PA et les raisons d'indisponibilité de cette vraie action. Elle est cliquable et activable par son raccourci clavier, au même titre qu'un binding normal.

**Important :** la suggestion n'est PAS un `FGridCombatHotbarBinding` persistant. Elle n'altère jamais `PartyInventoryState`, ne déplace rien et ne crée aucune copie d'arme. Le clic droit ne tente pas de supprimer cette action contextuelle et son glisser-déposer n'est pas autorisé, puisqu'elle n'a pas de source persistante à réordonner. Pour choisir manuellement un emplacement durable, glisser la Main-gauche depuis l'équipement : l'ancien contrat `SetCharacterCombatHotbarBindingFromItem` est conservé. Les suggestions s'effacent/évoluent dès que l'arme change ou qu'un raccourci est personnalisé.

Le slot 1 reste une **attaque simple**, même si les deux mains sont armées. Plus tard, 02.19.2 pourra faire évoluer son alias vers une **attaque double explicite, identifiée et validée** (MainHand + OffHand, deux vrais profils, deux jets, coût unique), et garder les attaques simples dans les raccourcis personnalisables. Pas de paiement AP double ni d'effet automatique en 02.19.1.

## Changement C++ limité

- `GridCombatHudWidget.h/.cpp` : vue `bTransientOffHandSuggestion`, résolution du raccourci 1 MainHand > OffHand > mains nues, projection non persistante d'une unique attaque OffHand dans le premier slot libre, prévention des doublons, distinction UI entre raccourci réel et suggestion (drag/clic droit/tooltip).
- `GridTurnManagerPlayerActionCatalog.cpp` : garde avant contribution offensive pour refuser OffHand lorsque `CanProvideAttackFromSlot(OffHand)` est faux ; aucune statistique, donnée de catalogue ou attaque créée.
- Tests Automation `Grimrock.RPG.ARMEMENT.LOT02.19.1` dans `GridMonsterMON12CombatHudTests.cpp`, en deux cas :
  1. `HandAttackProjection` : MainHand, OffHand, main droite vide, mains nues, personnalisation, emplacement libre, binding explicite et barre pleine ;
  2. `ShadowN1OffHandCombat` : véritables ARM001 / ARM013, catalogue, HUD, exécution OffHand dans un monde de combat, coût en PA, GUID distinct, sauvegarde de binding manuel, bouclier ARM075 passif, ARM002 MainHand-only et ARM031 deux mains.

**Non modifié :** WeaponCatalogue N1–20, kits approuvés, effets de combat, cooldown, initiative, inventaire, sérialisation, touches clavier, assets de WBP, cartes, textures et meshs.

## Localisation UI obligatoire — contrôle à l'ouverture et à la clôture

Autorités appliquées : `docs/UI_UMG_PRESENTATION_CONTRACT.md` et `docs/UI_WBP_LOCALIZATION_WORKFLOW.md`.

- WBP existants : `/Game/GrimrockPrototype/Blueprints/UI/.../WBP_GridCombatHud` et `WBP_GridCombatHudAction` ; C++ parent `UGridCombatHudWidget` / `UGridCombatHudActionWidget`. Aucun changement de leur hiérarchie, styles ni bindings Designer.
- Étiquettes de raccourci : notation numérique mécanique existante (1–0, ', ^).
- Nom/description/icône : `UGridItemDefinitionAsset` et `FGridCombatActionDefinition` existants, lus dynamiquement par le HUD ; pas de texte français codé dans le widget.
- Coût, disponibilité et infobulle : **clés déjà matérialisées** `Combat.Hotbar.Tooltip.Cost`, `Combat.Hotbar.Tooltip.KeepsItem`, `Combat.Hotbar.Tooltip.Unavailable` et `Combat.Hotbar.Tooltip.RemoveShortcut` via `FUILocalizationText::Get`. La dernière est supprimée **uniquement** pour une suggestion non effaçable.
- **Aucune nouvelle clé** dans `UI.csv`, `ST_UI.uasset` ou les locres ; pas d'authoring WBP. La chaîne `UI.csv → ST_UI → WBP → Game/fr/en` reste inchangée. L'Automation vérifie le comportement, pas la typographie ou l'anglais rendu en PIE. Aucune matérialisation d'asset ST_UI nouvellement requise.

## Vérification PIE demandée

1. L_MainMenu → New Game → Voleur / Ombre → L_Dungeon. Ouvrir l'inventaire et vérifier ARM001 MainHand / ARM013 OffHand.
2. Face à un monstre : raccourci **1** = dague principale et **2** (si libre) = Main-gauche ; cliquer/activer 2 : dégâts d'ARM013, **2 PA**, une seule attaque, aucune action gratuite de l'autre main.
3. Mettre un sort ou une capacité en 2 : la suggestion OffHand doit apparaître en **3** sans effacer 2 ; glisser Main-gauche dans 5 : elle y reste même après Save → Quitter → Continue et l'icône automatique disparaît.
4. Déséquiper MainHand : raccourci 1 utilise désormais OffHand, sans dupliquer cette dernière ailleurs. Retirer OffHand : raccourci 1 revient à mains nues.
5. Vérifier Guerrier / Guardian avec bouclier, Mage / Arcanist avec orbe, et Guerrier / Breaker avec arme deux mains : **aucun coup OffHand fictif**.
6. Vérifier en combat l'indisponibilité (0 PA / mauvais tour / mort), sans consommation de PA en cas de refus. Les actions restent cohérentes sur le HUD.
7. Confirmer en fr/en les infobulles existantes et la lisibilité des noms/icônes ; les captures GPU PIE restent indispensables pour l'acceptation visuelle.

## Conditions de validation

Un seul commit sur `master` ; pour son SHA exact, états séparés `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` (build + suite Grimrock). Le résultat `success` validera la logique automatisée ; **l'acceptation en PIE graphique reste à constater**.

Suivant : ARMEMENT-LOT02.19.2 (spécification transactionnelle d'une attaque double), puis 02.19.3 (parade Main-gauche, distincte du talent Esquive N2).
