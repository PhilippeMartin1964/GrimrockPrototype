# ARM-CLEAN06.3O — Projection de tri de l'inventaire depuis ItemSystem

**Prérequis :** ARM-CLEAN06.3N, suite Windows `Grimrock` verte sur le SHA exact.

Le calcul des clés de tri alphabétique et de masse de `UGridInventoryWidget::BuildInventoryProjectionSourceSlotIndices` passe par `FGridInventorySortReadModel`, lui-même consommateur du résolveur de métadonnées d'instance `FGridInventoryItemMetadataReadModel` déjà utilisé par les slots et les tooltips (06.3K). Les objets enregistrés utilisent le `DisplayName` et le `WeightKg` du seul `DA_ItemSystem` au `ItemLevel` de l'instance, multiplié par sa quantité.

Les modes de tri nom ascendant/descendant, poids ascendant/descendant et type conservent leurs comparateurs existants, la stabilité de l'ordre et le même nombre fixe de slots. Le filtre par `ItemType` n'est **pas migré** dans ce jalon, car il s'appuie encore sur une définition non canonique : sa conversion requiert une politique de catégories de la nouvelle architecture. Les objets hors registre gardent leur ancien modèle ; pour un objet enregistré à niveau invalide, aucune valeur historique n'est prise comme secours.

La nouvelle vue est entièrement transitoire. Aucun changement de localisation, `WBP`, raccourci, SaveGame, asset binaire ou règle de poids n'est introduit.

## Localisation

`docs/UI_WBP_LOCALIZATION_WORKFLOW.md` : **UI.csv inchangé → ST_UI inchangée → WBP inchangé → Game/fr/en inchangé**. Ce jalon ne crée ou modifie aucune étiquette ; les noms proviennent des DataAssets canoniques. Les traductions de contenu seront traitées lors de la conversion des assets, pas par de nouveaux `LOCTEXT`.

## Validation

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventorySortProjection` compare les valeurs de tri tirées du vrai `DA_ItemSystem` (ARM014, ARM031, ARM075, ARM081, niveaux 1 et 20) face à des champs legacy contradictoires, avec quantité x2, niveau 0 et identité inconnue. La suite `Grimrock` valide aussi les tests de filtres/tri historiques.

La preuve finale requiert `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sur le SHA unique. Le rendu du nombre de slots, des icônes et de l'ordre visuel reste un contrôle PIE distinct.
