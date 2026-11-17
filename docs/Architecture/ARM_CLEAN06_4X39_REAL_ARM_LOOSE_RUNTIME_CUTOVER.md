# ARM-CLEAN06.4X.39 — Bascule des objets libres ARM existants vers la présentation canonique

**Parent :** `519ae9433e8007f66f33793338b78516f590e740`. **Périmètre :** C++ runtime uniquement, aucune conversion de packages.

## Justification native

X.20 a volontairement conservé l'initialisation `DA_Weapon_*` des
ARM possédant leur pointeur historique dans les quatre niveaux, faute
de parité de l'émetteur lumineux, Niagara, scintillement et propriétés
Chaos. X.38 fournit désormais une **preuve native sur les deux
catalogues existants : 22 profils, aucune différence** pour les
8 champs de visuel et 2 champs de physique persistante par arme.
Le changement de lecteur est donc possible sans réécrire les catalogues
ni les 44 objets libres existants.

## Changement minimal et autorité

Dans `AGridLevelRuntimeActor::AddPlacedItemActor`, un placement
portant `CanonicalItemId` valide passe maintenant
**toujours** par `AGridItemActor::InitializeFromCanonicalItem`,
même si `FGridLooseItemInstance::ItemDefinition` contient encore
un `UGridItemDefinitionAsset` historique.

Cela applique les ressources enregistrées au monde : `WorldMesh`,
transform, `LightEmitter`, `VisualEffects`/Niagara persistants,
scintillement (via la présentation canonique), et physique de
`FGridResolvedItem`. Le GUID, l'ItemId et le niveau N1–N20 (ARM)
ou N0 (non-ARM) restent identiques. L'acteur créé n'attache
**aucune ancienne définition**. L'état initial de lumière provient
du même `FGridResolvedItem`, indépendamment de la présence d'un
ancien `DA_Weapon_*`.

La validation préalable de l'identité existante reste stricte :
GUID absent, quantité nulle, N0 ARM, niveau physique invalide,
ItemId contradictoire avec la définition historique : **refus
avant le spawn**, pas de fallback. Les placements **sans
CanonicalItemId** restent provisoirement sur leur chemin legacy.

Le champ historique `FGridSpawnedItemRuntimeEntry::ItemDefinitionAsset`
n'est **pas supprimé** dans ce jalon : ses éventuels lecteurs
(PIT02/restauration/états) doivent être audités séparément avant
retrait. Les pointeurs persistants des niveaux ne sont pas effacés
et les 107 références X.19 demeurent à migrer.

## Régression automatisée

Extension de
`Grimrock.RPG.ARM.CLEAN06_4X8.RealLooseItemCanonicalNonArmCutover`
sur les 44 objets libres réellement chargés : un ARM muni de son
vrai ancien pointeur est instancié et prouvé canonical-only
(mesh/transform, niveau, lumière, Niagara, identité) ; un faux
pointeur portant le **même ItemId** mais un émetteur contradictoire
ne peut plus influencer la présentation ; les variantes sans
pointeur N1/N20, le non-ARM N0, le N0 ARM invalide et la source
contradictoire d'un autre ItemId restent couverts.

### Ajustement de la fixture PIT02

La première validation Windows de X.39 a compilé puis signalé un
échec de `Grimrock.Pit.Issue181.FlyingProjectilePassage` (issue
UEValidation #190). Le test PIT02 lisait une définition de dague via
`F.Find(ItemId)->ItemDefinitionAsset`, qui est justement devenu
`nullptr` après le cutover X.39. Pour continuer à éprouver le
**lancer manuel historique**, la fixture charge explicitement le
`DA_Weapon_Dagger_01` déjà présent en lecture seule et vérifie
l'ItemId, tout en exigeant que l'**acteur physique canonique** ne
porte plus ce pointeur. Aucun repli legacy n'est ajouté au code de
production, ni aucune modification des assets natifs. Cette correction
appartient au **même jalon X.39**, pas à un commit séparé.

Aucune modification `.uasset`, `.umap`, WBP, SaveGame ni LFS ;
aucun retour d'ancienne propriété sérialisée sans migration native
ciblée. **PIE/Chaos/PIT02 et les autres lecteurs ne sont pas
validés par la seule suite `-NullRHI`.**

**06.4 PARTIELLE ; 06.5 BLOQUÉE.**
