# ARM-CLEAN06.5H.46 — Prévisualisation Great Axe / Round Shield canonique

## Conversion du test de production

Le test `Grimrock.UI.EquipmentPreview043A.ProductionGreatAxeAndRoundShield` ne charge plus les anciens `DA_Weapon_GreatAxe_01` et `DA_Weapon_RoundShield_01`. Une fixture `FGridCanonicalWeightTestWorld` fournit le registre de production et un inventaire rattaché à un vrai monde Unreal, sans aucune définition enregistrée historiquement.

Les profils ARM031 Great Axe N1 (`TwoHanded`) et ARM075 Round Shield N1 (5 points d'armure physique) viennent de `ResolveItem`. Les deux instances physiques N1, avec GUID indépendants, sont préparées par `ApplyItemDefinitionToInstance`. Le test exerce ensuite `PreviewEquipFromInventorySlot` sur le scénario de déplacement du bouclier par la hache à deux mains.

## Invariants conservés

Prévisualisation sans mutation, occupation virtuelle de la main secondaire, libération des deux mains, un seul objet bouclier déplacé, perte exacte de cinq points d'armure et hache toujours dans le sac sont vérifiés. Les autres tests synthétiques du fichier restent délibérément inchangés pour caractériser les politiques d'équipement et d'atomicité.

Aucun code gameplay, WBP, `.uasset`, `.umap` ni `.sav` modifié. Les 53 packages historiques H.23 restent protégés.

H.47 est bloqué jusqu'à `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS sur ce SHA, avec compilations Editor/Shipping et `Grimrock.UI.EquipmentPreview043A`. Sept sauvegardes natives à réauditer avant toute suppression.
