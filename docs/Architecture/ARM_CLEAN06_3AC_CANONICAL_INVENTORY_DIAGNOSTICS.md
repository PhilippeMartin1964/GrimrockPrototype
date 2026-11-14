# ARM-CLEAN06.3AC — Diagnostics d'inventaire et poids canoniques

**Base :** ARM-CLEAN06.3AB (`97745f4`), validation Windows UE5.5.4 / `Grimrock` verte.

## Objectif

Les diagnostics C++ ne doivent plus afficher `FGridItemInstance.Weight` ou `DA_Weapon_*.Weight` pour les identités migrées, alors que le HUD, l'encombrement, les plaques et la physique lisent déjà `UGridItemSubsystem::ResolveItem`. Le nouveau `FGridInventoryDiagnosticWeightReadModel` est **purement en lecture et uniquement destiné aux journaux** ; il réutilise le `FGridItemWeightReadModel` existant pour chaque instance.

- `GetPartyInventoryDiagnostics` conserve son format `Item=... Qty=... Weight=... Owner=...` ; `Weight` continue à représenter une **masse unitaire**, et non la quantité multipliée. La valeur provient maintenant du `WeightKg` canonique au `ItemLevel` réel de l'instance. Pour une identité connue mais invalide (arme N0), `Weight=INVALID` signale le problème plutôt que d'afficher une ancienne copie.
- `GetItemDefinitionDiagnostics` ne possède **aucun ItemLevel d'instance**. Une arme enregistrée affiche explicitement `Weight=N1:x,N20:y` (bornes de lecture, non promesse de progression linéaire) ; une munition sans niveau affiche `Weight=N0:x`. On ne fabrique pas un niveau implicite ; on ne lit jamais `Definition->Weight` pour une identité connue.
- Les objets non enregistrés dans `DA_ItemSystem` et les outils/test isolés sans registre `GameInstance` conservent temporairement leur sortie historique `Weight=%.1f`. Les autres champs `Type`, `Slots` sont laissés tels quels : la migration des catégories n'entre pas dans le périmètre du présent jalon.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalInventoryDiagnosticWeight` interroge le vrai `DA_ItemSystem`, ARM013/014/075/081 aux niveaux 1 et 20, munitions N0, copies volontairement contradictoires, niveau invalide, quantité x3 / x12, identité non enregistrée et absence de registre. Le test historique `Grimrock.TechnicalDebt.TD02_3.PartyInventoryDiagnosticsContract` protège le format et les informations du diagnostic. Le calcul du poids porté n'est pas modifié.

Aucun fichier `.uasset`, `.umap`, SaveGame, WBP ni texte joueur n'est touché. **Localisation : UI.csv → ST_UI → WBP → Game/fr/en, inchangé.**

## Validation obligatoire

Un seul commit atomique sur `master`. Le statut `UE5.5.4/SilentTask` et le statut `UE5.5.4/LocalAuto` doivent être tous deux `success` sur son SHA exact (build UE5.5.4 + suite complète `Grimrock`). Le rendu du diagnostic en console PIE ne nécessite pas de validation visuelle GPU ; le comportement des autres interfaces reste couvert par les jalons précédents.
