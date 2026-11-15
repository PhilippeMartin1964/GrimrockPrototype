# ARM-CLEAN06.3BB — Munitions canoniques (15.11.2026)

**Parent :** ARM-CLEAN06.3BA (`3c537bc`), point de reprise `bb20a12`.
**Portée :** exigences de flèches d'une attaque d'arme à distance, affichage de disponibilité, préflight et débit à l'exécution ; préparation d'un projectile récupérable.
**Interdit :** nouveau type de flèche, doublon de catalogue, modification .uasset/SaveGame/WBP, bascule prématurée 06.4.

## Règles

1. Une arme enregistrée est résolue au niveau réel `ItemId + ItemLevel` dans `UGridItemSubsystem`. Son `Weapon.RequiredAmmoKindId` est le seul lien autorisé. Même avec un ancien DA_Weapon contradictoire (ou absent), ne jamais consulter `RequiredAmmunitionItemDefinitionId`.
2. Le registre validé garantit la **unicité de AmmoKindId**. `ResolveAmmoKind` ramène l'unique `UGridAmmoDefinitionAsset` vers le vrai `ItemId` (actuellement `Item_Arrows_01`), avec sa masse canonique N0. Pas de nouvelle map runtime.
3. `FGridRangedAmmunitionRequirement` est une décision transitoire, non persistée, recalculée au HUD et à l'exécution. Rejet si identité ou niveau connu invalide, kind introuvable, ou stock insuffisant. Hors registre, comportement `FGridRangedAmmunitionPolicy` historique inchangé. Action non-distante : aucun prélèvement.
4. Une attaque valide prépare une seule flèche physique `AGridThrownItemActor` avec GUID neuf **avant le débit** ; refus de spawn : destruction des préparations et restitution des PA. Après préparation, un seul `RemoveItemDefinitionFromCharacterInventory(...,1)`. La récupération conserve ItemId/ItemLevel/GUID du projectile.
5. La porte de spawn des projectiles **ranged ammo** accepte désormais les vrais `AmmoKindId` et `MaxStack` du registre même sans vieille définition d'item. Une identité enregistrée invalide ne revient jamais au chemin historique.
6. La trajectoire d'une flèche ordinaire reste un **point d'intégration différé** : vitesse d'une `ProjectilePolicy` du lanceur si réellement authorée, sinon `ThrowSpeed` du vrai asset flèche tant que le contenu binaire 06.4 n'est pas converti. Si aucun des deux n'existe, le tir échoue sans flèche perdue. On ne crée pas de valeur de vitesse arbitraire. Les paramètres de rotation/lifetime non encore convertis demeurent temporairement dans l'asset flèche quand il existe.

## Couverture attendue

- Canonique `Weapon_ShortBow_01` + `Item_Arrows_01` ; ancienne arme contradictoire/absente ; niveau N0 invalide.
- Stock réel par héros : vide/refus ; quantité 2 → 1 → 0 ; GUID de pile conservé lors du premier débit ; refus sans underflow ; attaque melee sans débit.
- Projetile canonique N0 sans ancien `DA_Item_Arrows_01` : un GUID, visuel du registre, récupération possible, aucun débit pendant la préparation.
- Mêmes contrôles pour armes hors registre via le contrat historique.

Les tests automatisés ne remplacent **pas** le parcours GPU/PIE complet avec animation, impacts hit/miss et Save/Continue. 06.4 devra convertir la vitesse/pose/lifetime des flèches afin de supprimer le dernier pont visuel historique. Le format SaveGame 27 et le pipeline `UI.csv → ST_UI.uasset → WBP → Game/fr/en` restent inchangés.
