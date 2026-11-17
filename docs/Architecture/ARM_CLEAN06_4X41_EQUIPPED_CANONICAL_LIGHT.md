# ARM-CLEAN06.4X.41 — Une seule autorité pour l'éclairage équipé

**Parent :** `396a269290f9bad65cebd4dc827faa2dd93e589b`. Lot cohérent : deux lecteurs d'éclairage équipés,
tests de régression UE et documentation. Aucun asset natif ni SaveGame réécrit.

## Cause

Après X.38, les 22 profils ARM de `DA_CanonicalWeaponVisualCatalogue`
correspondent déjà aux anciennes définitions pour les 8 champs
visuels et 2 champs physiques audités. Pourtant, deux lecteurs
continuaient à utiliser `DA_Weapon_*` comme autorité **pour la lumière
des ARM équipées** :

- `AGrimrockPartyPawn::DoesEquippedItemEmitLight` ;
- `UGridPartyIlluminationComponent::RefreshFromEquipment`.

Un ancien pointeur de même ID mais dont l'émetteur avait divergé
pouvait conduire à des décisions incohérentes avec le catalogue.
Le résolveur acceptant volontairement certains non-ARM N1 pour
consultation familiale, cette lecture devait aussi rejeter le N1
**physique** d'une torche.

## Correction

Tout ItemId enregistré utilise désormais **uniquement**
`FGridResolvedItem::LightEmitter`, à son niveau physique validé :
ARM N1–N20 ; non-ARM N0. ARM N0 et non-ARM N1 sont refusés,
sans rétrogradation sur un ancien `UGridItemDefinitionAsset`.

Seuls les ItemId **non enregistrés** conservent temporairement la
résolution par définition historique, le temps des conversions de
loot/alchimie. Le pipeline `EquipHeldItem(FName)`, qui ne reçoit
pas encore d'ItemLevel, reste séparé et n'est pas considéré comme
cutover complet des maillages de main ARM.

## Validation automatisée

`Grimrock.RPG.ARM.CLEAN06_4X41.CanonicalEquippedLightReaders`
teste un vrai `Weapon_StraightSword_01` N1 et N20 face à un faux
`DA_Weapon` de même ID avec `LightEmitter` inversé, puis ARM
N0 invalide, torche N1 invalide, et une définition hors registre
dont la lumière doit rester disponible. Le même cas passe dans
les deux lecteurs : prédicat du Pawn et véritable composant
`UGridPartyIlluminationComponent`.

La validation Windows automatique compile et exécute la suite
`Grimrock` sous UE 5.5.4 ; `NullRHI` ne remplace pas
les contrôles visuels PIE de l'éclairage.

**ARM-CLEAN06.4 : PARTIELLE ; ARM-CLEAN06.5 : BLOQUÉE.**
