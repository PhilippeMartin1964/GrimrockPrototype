# ARM-CLEAN06.2J — Visuels réels des 22 ARM, sans duplication

## Propriété unique

`DA_WeaponVisualCatalogue` historique fournit les profils
`FGridWeaponVisualProfile`, dont `FamilyTag`, `BaseIcon`,
`WorldMesh`, `EquippedMesh` optionnel, `WorldVisualTransform`,
`OrnamentMesh` et `OrnamentRelativeTransform`.
Le catalogue visuel **canonique** conserve ces informations
dans un profil unique par `VisualId`, partagé lorsque plusieurs
ARM possèdent le même visuel. `FamilyTag` est un **groupe
artistique**, jamais un substitut à `ArmamentKind`.

La projection Editor-only copie sans charger les ressources et
compare tous les chemins soft, les transforms et les champs
d'ornement. Chaque `ArchetypeId` du manifeste est lié à
son `VisualId` explicite ; aucun ID de visuel n'est fabriqué.

La valeur `EquippedMesh` **null** peut être significative :
l'ancien résolveur utilise `WorldMesh` comme apparence équipée
de repli. Le `UGridItemSubsystem::ResolveItem` calcule ce
résultat **en mémoire** ; il ne sauvegarde pas une seconde
référence dans l'asset canonique ni n'ajoute de fallback
vers un `DA_Weapon_*` historique.

## Validation automatique

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ProductionVisualProjection`
charge **exclusivement les 22 armes du manifeste** et le
catalogue visuel source, construit un candidat transient,
compare les 22 liens, contrôle l'unicité des `VisualId`
et la non-modification des packages. Les éventuels écarts
d'icône/mesh directement sérialisés dans le DA_Weapon sont
comptés comme divergence historique à traiter au cutover,
non recopiés dans une deuxième autorité.

Le test `Grimrock.RPG.ARM.CLEAN06_1.SingleResolver.SwordN20`
vérifie la résolution du mesh équipé à partir de `WorldMesh`
quand `EquippedMesh` est nul (soft path de test transitoire).

Un statut Windows `SilentTask` + `LocalAuto` success sur le
SHA exact valide ce jalon ; aucun fichier binaire ou gameplay
historique n'a été modifié. Le transfert définitif des
visuels doit encore passer par une migration Editor UE
contrôlée et les preuves d'intégrité des références.
