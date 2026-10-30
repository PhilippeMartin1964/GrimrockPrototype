# ARMEMENT01.15 — Potion de soin N1 commune au Prêtre et à l'Apothicaire

## Objectif

Produire **un seul** `UGridItemDefinitionAsset` réel :
`/Game/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01`
(`ItemDefinitionId = Item_Potion_Healing_01`).

Cette définition est consommée par **deux kits approuvés**, sans imposer une
branche ou attribuer un talent de niveau 2 :

- `Kit_Priest_Restoration` : sceptre sacré N1 + potion de soin ×1 (sac).
- `Kit_Alchemist_Apothecary` : dague N1 + potion de soin ×1 (sac).

Les autres kits ne changent pas ; aucun nouvel inventaire ni moteur de potion n'est introduit.

## Contrat de gameplay N1

| Propriété | Valeur initiale |
|---|---|
| ItemDefinitionId | `Item_Potion_Healing_01` |
| ItemType | `Potion` |
| Quantité au départ | 1 dans chacun des deux kits |
| Stackable | Oui, maximum 10 par pile |
| Poids | 0,2 kg |
| Action QuickItem | `Use_Item_Potion_Healing_01` |
| Targeting / Resolution | `Self` / `Effect` |
| Restauration | **20 PV** |
| Coût en PA | **1** |
| Coût source | **1 objet** |
| Tags | `QuickItem.Potion`, `QuickItem.Potion.Positive` |
| Restriction | Inutile à pleine santé ; aucun objet ni PA consommé en cas de refus |
| Compétence | Aucun rang ni talent automatiquement accordé |

**Équilibrage provisoire :** 20 PV / 1 PA est la configuration type existante de
`docs/Design/MON12_8_4_QUICK_ITEM_COMBAT_ACTIONS.md`. Ce montant est
conservé pour le premier essai en jeu ; sa valeur finale exige une revue d'équilibrage.
Le talent `EnhancedPotion` N2, s'il est acquis ultérieurement, reconnaît le tag
`QuickItem.Potion.Positive` via le système C2/C7 existant.

**Localisation :** `Localization/Source/UI.csv` définit les deux clés
`Item.StartingPotion.Healing01.Name` et
`Item.StartingPotion.Healing01.Description`. L'asset conserve des références
`FText` à `ST_UI`. La présentation n'est pas codée en dur dans le runtime.

## Production

Le script **existant** `Scripts/AuthorRPGAlchemist.ps1` offre désormais
`-StartingHealthPotion -Apply`, un mode isolé qui :

1. Compile le Development Editor avec `ValidateUE.ps1` ;
2. Synchronise l'asset existant `ST_UI.uasset` depuis `UI.csv` via
   `UIStringTableAuthoring` ;
3. Utilise le commandlet **existant** `RPGAlchemistAuthoring`
   en mode `-StartingHealthPotion -Apply` ;
4. Vérifie que la définition est valide et **refuse de remplacer** un package
   existant incompatible ;
5. Exécute les tests `Grimrock.RPG.ARMEMENT01.15.HealthPotion`.

Une seconde exécution sur un package identique est idempotente
(`PRESENT_SAME`), et les autres assets Alchimiste ne sont pas modifiés.
Le mode normal `AuthorRPGAlchemist.ps1` est conservé inchangé.

### Commande locale unique

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\AuthorRPGAlchemist.ps1 -EngineRoot D:\UE_5.5 -StartingHealthPotion -Apply
```

Après le succès des tests, vérifier les deux packages locaux dans le Content Browser :
`Content/Localization/StringTables/ST_UI.uasset` et
`Content/GrimrockPrototype/Core/DataAssets/Items/DA_Item_Potion_Healing_01.uasset`.
**Ces binaires ne peuvent pas être créés dans GitHub sans lancer UE** ; ils
doivent être committés depuis la machine Windows après acceptation PIE.

## Critères de validation

- Tests `AuthoringAndQuickItemContract` : définition réelle du QuickItem,
  action affichée pour un blessé, bloquée à pleine santé et en absence d'objets ;
- Tests `PriestAndApothecaryNewGame` : véritables deux classes et kits,
  attribution d'une potion par héros, arme N1 conservée, GUID,
  identité/quantité dans snapshot de sauvegarde et aucun changement
  des six DataAssets de classe ;
- Tests de régression `Grimrock.RPG.ARMEMENT01.13.ApprovedKits` et
  `Grimrock.RPG.ARMEMENT01.9` regroupés après matérialisation ;
- PIE : `New Game`, essai avec PV réduits, consommation unique,
  refus à pleine santé, `Save → Quitter → Continue` sur disque ;
- Icône/mesh définitifs de potion : **passe visuelle ultérieure**, ne pas
  substituer un mesh non validé et déclarer l'art terminé.

### État

Code C++ / configuration auteur / tests enregistrés sur `master` ;
**compilation et génération du package UE locales en attente**.


## ARMEMENT01.15.1 — Exécution consommable personnel (30.10.2026)

**Incident PIE :** le menu proposait « Consommer » sans exécution réelle.
Diagnostic dans GridInventoryWidget.cpp : absence de branche Consume dans
ExecuteResolvedInventoryContextAction. Les logs FacingTarget=None relèvent
d'une recherche indépendante d'objet face au groupe, pas de l'échec.

**Correction C++ :**

- Le menu Consume appelle désormais un chemin réel d'exécution.
- Hors combat, TryConsumeSelfRestorativeQuickItem est une opération d'inventaire
  à cible Self. Elle valide le GUID physique de la pile choisie, propriétaire,
  définition QuickItem, effet utile Health/Mana, puis modifie les ressources et
  retire exactement 1 unité de cette pile ; invalidation de la hotbar sur
  disparition du dernier item de cette définition et notification UI.
- En combat, le menu appelle RequestCharacterCombatAction sur le TurnManager
  avec SourcePolicy=QuickItem et TargetingPolicy=Self : PA, initiative et
  validation/consommation restent contrôlés par le moteur de combat.
- Un objet sans effet personnel exécutable ne présente pas Consume actif.
  À pleine santé, le menu expose une action désactivée et sa raison.
- Aucun effet complexe (statut, piège, zone, armure, mouvement, cible monde)
  n'est partiellement simulé par l'exécution d'exploration.

**Tests Automation :**

- Grimrock.RPG.ARMEMENT01.15.SelfConsume.ExplorationPotionTransaction :
  véritable kit Restauration, refus à pleine santé, GUID invalide,
  gain de PV, unique décrément, impossibilité d'un double usage et snapshot.
- Grimrock.RPG.ARMEMENT01.15.SelfConsume.ContextMenuByIndex :
  création monde/Pawn/widget, action du menu à pleine santé puis blessé,
  sélection par index et vérification de la consommation réelle.

**Périmètre volontaire :** hotbar hors combat = ARMEMENT01.15.2,
iconographie/mesh = ARMEMENT01.15.3, audit industriel = lot ultérieur.
Aucun changement au soin initial de 20 PV.

Validation UE locale :

    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.15.SelfConsume"

Puis PIE avec un Prêtre/Restauration blessé. Les deux binaires locaux
ST_UI.uasset et DA_Item_Potion_Healing_01.uasset sont préservés ;
ils ne sont ni réinitialisés ni publiés par ce commit GitHub.


## ARMEMENT01.15.1 — Contrôle PIE accepté (30.10.2026)

- Le Prêtre/Restauration consomme correctement sa potion par clic droit, restaure les PV, retire l'objet du sac et bloque à pleine santé.
- Les références Icon et WorldMesh de la potion ont été installées et vérifiées en jeu par le développeur ; leurs packages binaires restent locaux, non encore publiés.
- Le filtre Grimrock.RPG.ARMEMENT01.15.SelfConsume : **2 tests réussis, 0 avertissement, 0 échec** ; build UE 5.5.4 OK.

## ARMEMENT01.15.2 — Self-QuickItem de la hotbar en exploration

**Diagnostic :** GridCombatHudWidget interrogeait GetAvailableCombatActions lorsque bCombatActive=false. Le catalogue de combat déclare alors les actions indisponibles (CombatInactive). RequestHotbarSlot ne disposait d'aucun chemin d'exécution pour une potion pendant l'exploration ; l'icône et la quantité pouvaient s'afficher, mais le bouton n'exécutait rien.

**Correction :** en exploration uniquement, GridCombatHudWidget projette les QuickItems Potion/Food dont la définition est Self/Effect avec restauration personnelle simple, sans statut, piège, zone ou mouvement. Il conserve la correspondance stricte ActionId/SourceDefinitionId avec la liaison hotbar et une quantité issue de l'inventaire. Une action avec effet utile devient disponible, à pleine santé elle est grisée avec NoApplicableEffect et un motif localisé. Une action de combat/attaque conserve son comportement historique. Les projectiles physiques conservent leur propre routage.

Au clic, la hotbar d'exploration récupère un objet physique appartenant au personnage sélectionné, puis appelle l'autorité d'inventaire TryConsumeSelfRestorativeQuickItem (ARMEMENT01.15.1) avec son GUID. Le traitement partage le gain de PV/PM, le décrément, la désactivation de raccourci et l'actualisation de l'interface. Aucun PA fictif n'est dépensé. Quand bCombatActive=true, les actions restent résolues et exécutées par le TurnManager, sans changement.

**Test :** Grimrock.RPG.ARMEMENT01.15.ExplorationHotbar.PriestAndApothecary : génération des personnages avec les deux kits de production, hotbar persistante hors combat, potion désactivée à pleine santé, activation blessé, soin, retrait d'une unité, suppression du raccourci épuisé et impossibilité de double emploi.

**Validation locale :**

    git pull --ff-only origin master
    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.15.ExplorationHotbar"

Terminer par PIE depuis une partie hors combat, après fermeture de l'inventaire ; contrôler clic hotbar et raccourci clavier. La politique existante IsCombatHotbarExecutionBlocked bloque intentionnellement la hotbar lorsque l'inventaire modal est ouvert : ce jalon ne change pas ce contrat UI.

**État :** code publié sur master, build/Automation/PIE locaux en attente. ARMEMENT01.15.3 visuels déjà matérialisés en local ; publication des assets en attente de validation finale.
