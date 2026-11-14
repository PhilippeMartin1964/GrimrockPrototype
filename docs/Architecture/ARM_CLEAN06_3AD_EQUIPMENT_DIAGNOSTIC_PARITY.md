# ARM-CLEAN06.3AD — Parité des diagnostics des bonus d'équipement et résistances

**Base :** ARM-CLEAN06.3AC (`a1d93cf`) ; les contrôles `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` sont verts sur ce SHA.

## Constat architectural

Le schéma canonique `FGridCanonicalResolvedWeapon` expose actuellement `PhysicalArmor` et `MagicalArmor`, mais **pas** les bonus de Force/Dextérité/etc., ni les résistances physiques ou élémentaires. Le gameplay, depuis 06.3H, utilise déjà `FGridEquipmentArmorReadModel` pour l'armure au niveau N1–N20 tout en conservant `DA_Weapon_*.EquipmentStatBonus` pour les autres attributs et `DA_Weapon_*.EquipmentResistanceBonus` pour les résistances. Inventer ces données dans `DA_ItemSystem` serait une fausse migration.

## Changement ciblé

Le modèle `FGridEquipmentContributionReadModel`, sans état ni DataAsset, centralise les deux projections qu'utilise désormais **le gameplay et la journalisation** :

- `ResolveStatBonuses` : les valeurs d'armure physique/magique viennent de `GridEquipmentArmorReadModel` et donc du registre canonique N1–N20 si l'identité est enregistrée ; les autres valeurs proviennent, sans changement, de `EquipmentStatBonus`. Une identité canonique invalide ne peut pas hériter d'une ancienne armure ; un item canonique valide peut résoudre son armure même si l'ancienne définition n'est plus disponible.
- `ResolveResistances` : conserve explicitement la seule autorité actuellement présente, `EquipmentResistanceBonus` de la définition historique, pour les instances valides. Elle est utilisée à la fois dans le calcul agrégé du personnage et dans la ligne diagnostique de l'équipement.

`UGridPartyInventoryComponent::GetEquipmentBonusesForInstance` délègue au même résolveur que `LogSelectedCharacterEquipmentStatBonusDiagnostics`. Les résistances agrégées et individuelles partagent également une unique fonction. Les lignes sans bonus continuent à ne pas être imprimées, et les avertissements de définition manquante sont conservés lorsqu'aucune contribution n'est disponible. Ni les jets, ni les réserves, ni les modificateurs ou attributs n'ont de nouvelle politique.

La **migration de schéma** et des bonus/résistances non-armure demeure explicitement en attente : elle ne peut pas être clôturée comme un lecteur devenu canonique tant que le modèle de données ne les expose pas (06.4/06.5 et décisions d'authoring).

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalEquipmentDiagnosticContributions` vérifie le vrai `DA_ItemSystem` sur ARM013/014/075/081 N1 et N20, des armures historiques volontairement fausses, les attributs et résistances historiques, un ARM N0 invalide, l'armure canonique sans ancienne définition et un objet non enregistré. Tous les tests `Grimrock` préexistants restent actifs.

Aucun `.uasset`, `.umap`, SaveGame, WBP, texte utilisateur ni chaîne de localisation n'est changé. **UI.csv → ST_UI → WBP → Game/fr/en : inchangé.** Le rendu PIE et le comportement des panneaux ne sont pas déduits des tests `-NullRHI`.
