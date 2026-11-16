# ARM-CLEAN06.4X.3 — règle de matérialisation des niveaux authorés

**Code introduit, conversion binaire NON effectuée dans ce jalon.**
Base : `06.4X.2`, test read-only des identités du niveau et des
définitions historisées. Version Unreal Engine 5.5.4.

## Problème

Les `CanonicalItemId` des 95 références sont réellement enregistrés
depuis 06.4W, mais les placements authorés conservent leur ancien
`ItemLevel=0`. Un ARM N0 est une **instruction d'héritage** du
`DefaultWeaponItemLevel` historique propre à son ancienne définition,
pas un niveau d'arme réellement persisté ni une autorisation à choisir
N1 par défaut. Les objets non-ARM restent naturellement N0.

## Règle C++ pure et Editor-only

`FGridArmClean064X3AuthoredLevelMaterializer::TryResolve` reçoit
la définition **canonique**, le niveau authoré et la valeur de niveau
d'origine lue par le futur convertisseur Editor. Elle retourne un
niveau d'instance physique explicite et un drapeau d'héritage :

- ARM N0 : exige une source authorée N1–N20 ; retourne exactement
  cette source, y compris lorsqu'elle n'est pas N1.
- ARM N1–N20 explicite : garde exactement la valeur choisie.
- Non-ARM : accepte uniquement N0.
- Rejette un `ItemId` absent, un ARM sans `ArchetypeId`, une classe
  inconnue ou tout niveau interdit ; réinitialise toujours la sortie
  lorsqu'elle échoue.

Le helper ne charge aucune ancienne définition, ne crée aucun fallback
runtime et **n'enregistre aucun UObject**. La prochaine opération Editor
devra fournir les vraies valeurs historiques, vérifier toutes les sources,
puis persister les niveaux effectifs avant de supprimer les anciens
pointeurs. La palette doit également conserver une valeur explicite
pour les nouveaux placements ARM ; elle ne peut plus dépendre du
`DefaultWeaponItemLevel` d'un ancien DataAsset.

## Validation

Test `Grimrock.RPG.ARM.CLEAN06_4X3.AuthoredLevelMaterializationRules` :
N0 hérité de N7, N1 et N20 explicites, non-ARM N0, N0 source invalide,
N21, niveau négatif, non-ARM N1/N20, ItemId absent, archétype absent.

Ce test ne prouve pas encore la conservation du rendu, Niagara,
Chaos, réceptacles ou Save/Continue. Aucune écriture des cinq niveaux,
des .sav ou des WBP n'est autorisée par X3.
