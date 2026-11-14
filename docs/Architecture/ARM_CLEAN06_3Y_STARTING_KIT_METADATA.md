# ARM-CLEAN06.3Y — Métadonnées des équipements de départ depuis ItemSystem

**Prérequis :** ARM-CLEAN06.3X, tests UE5.5.4 / `Grimrock` verts sur son SHA.

`FRPGStartingEquipmentService::TryBuildKit` reste transactionnel : il construit une copie de l'inventaire/équipement et publie uniquement si **tous** les objets du kit passent les validations existantes. Le service accepte un pointeur **optionnel** de registre canonique, fourni par le `GameInstance` de `UGridPartyInventoryComponent`. Les tests et appels éditeur sans `GameInstance` conservent la branche historique, sans changer leur signature ancienne.

Après le choix `FixedLevel` validé une seule fois (N1–N20 pour arme, N0 pour item non progressif), `FGridStartingKitStaticMetadataReadModel` lit le `DisplayName` et le `WeightKg` de `DA_ItemSystem` pour toute identité enregistrée. Ainsi les kits créés dans le frontend n'introduisent plus de copies numériques des anciens `DA_Weapon_*` dans les nouvelles instances. Les objets hors registre continuent à utiliser leur auteur historique ; un ItemId connu mais à niveau invalide provoque **un refus atomique**, sans création partielle ni réparation N1 implicite. L'ordre des slots, les règles de mains et le choix du kit ne changent pas.

Le test `Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitStaticMetadata` valide les ARM014/054/031/075 N1/N20, les vraies flèches N0, des copies contradictoires, et l'absence de fallback pour une arme N0. La suite `Grimrock` historique conserve les tests des kits de classe et de la création du personnage.

Aucune modification `.uasset`, `.umap`, SaveGame, WBP, menu ou chaîne de localisation. **UI.csv → ST_UI → WBP → Game/fr/en : inchangé**. Le contrôle final requiert `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` verts sur le SHA exact.
