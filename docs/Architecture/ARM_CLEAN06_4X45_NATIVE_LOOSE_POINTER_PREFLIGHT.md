# ARM-CLEAN06.4X.45 — Préflight natif des 44 objets libres, avant conversion binaire

**Parent :** `9f72e6e8094b66760ad0b9d1166f0a82bf083197`. Lot strictement C++ de tests et documentation.
**Aucune exécution Apply et aucun SavePackage.** Le périmètre binaire futur est
le **seul** `DA_GridLevel_00.uasset` (44 `FGridLooseItemInstance`), pas
les 17 contenus de réceptacles ni les trois overrides inactifs du même
package, et pas les trois autres niveaux.

## Pourquoi un préflight dédié

X.44 a cessé de créer des références historiques pour les nouvelles
poses. Les 44 objets libres déjà persistés dans
`/Game/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00`
gardent leurs anciens `ItemDefinition` malgré la présence de leurs
`CanonicalItemId` et `ItemLevel`. La conversion native pourra enlever
**uniquement** le pointeur de ces 44 objets sans reconstruire la
géométrie, le niveau, les GUID ou les objets de puzzle.

Les tests antérieurs X.2, X.4 et W exigeaient encore un pointeur
historique non nul pour **toutes** les 95 occurrences. Ils acceptent
maintenant l'état strictement canonique **uniquement** pour les 44
objets libres, si leur ID existe, leur quantité et leur niveau
physique sont valides. Les 18 contenus initiaux de réceptacles et
33 entrées de palette continuent à exiger leur définition d'origine
jusqu'à leurs propres transactions natives. Le test de véritable
donjon ARMEMENT01.10.3 accepte les deux états pour les objets libres.

## Correctif de compilation du premier contrôle Windows

Le SHA initial X.45 a échoué à la compilation sur
`GridEditorARMCLEAN064X45NativeLoosePreflightTests.cpp:29` :
la projection de `ReadableContentAsset` vers `UObject` dans
`GetPathNameSafe` exige la définition complète de sa classe.
Le test inclut désormais `Runtime/GridReadableContentAsset.h`.
Ce correctif fait partie du **même lot atomique X.45** ; aucun
code de production ni contenu binaire n'a été modifié.

## Nouveau test sur le véritable niveau, sans écriture

`Grimrock.RPG.ARM.CLEAN06_4X45.NativeLoose44GuardedPreflight` :

- charge le vrai `DA_ItemSystem` et le vrai `DA_GridLevel_00` ;
- exige 44 objets libres, zéro item spawn, GUID uniques, cellules
  valides, quantité et ItemLevel physique N1–N20 pour ARM / N0 pour
  non-ARM, identités enregistrées et quantité compatible avec MaxStack ;
- compare ID et ArchetypeId des anciennes sources si encore présentes ;
- refuse un demi-migré (seulement 44 anciens pointeurs ou zéro) ;
- duplique le niveau **en mémoire transitoire**, annule les
  44 pointeurs sur cette copie seulement et contrôle que les champs
  stables (GUID, ID, niveau, quantités, cellule, position, lecture et
  notes) restent égaux ;
- simule un ARM N0 corrompu sur la copie et vérifie le refus du
  même garde-fou ;
- vérifie que le vrai package n'est pas marqué `dirty`.

Aucune sauvegarde ni modification de package source. Le test est une
preuve de préparation **avant** une transaction native, pas une preuve
d'avoir réellement converti le `.uasset`.

## Prochaine étape distincte : transaction UE protégée

Un futur commandlet Editor-only devra offrir `-Preflight`,
`-Verify` et `-Apply` ; celui-ci sera **refusé sans une sauvegarde
SHA-256 privée du package exact**, un HEAD exact, un ensemble
de fichiers modifiés fermé et un processus Unreal distinct de
réouverture/vérification. En cas d'absence de changement, `-Apply`
doit être un no-op. Les 17 contenus de réceptacles, les trois
overrides inactifs, les autres niveaux, la palette, les serrures,
les monstre et SaveGame v27 sont formellement hors périmètre.
Une vérification PIE postérieure au changement binaire demeurera
nécessaire (la validation PIE utilisateur actuelle porte sur l'état
antérieur à cette conversion).

**06.4 PARTIELLE ; 06.5 BLOQUÉE.**
