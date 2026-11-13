# ARM-CLEAN06.2M — Affinités et préférences de classes ARM

Le catalogue source compressé contient 82 entrées
`FGridWeaponArchetypeClassAffinity` et six préférences
`FGridWeaponClassPreference`. Ces données sont purement
**consultatives** pour la composition des équipements :
elles ne doivent jamais interdire un objet ni modifier
les règles de mains.

Le nouveau `UGridCanonicalWeaponCatalogueAsset` possède
déjà `PrimaryClassId`, `SecondaryClassIds`,
`SecondarySourceLabel` sur chaque `ARMxxx` et les
six `FGridCanonicalClassPreference`.

`GridArmClean06ClassAffinityProjection` (Editor-only)
remplit ces champs à partir des véritables structures UE
sans les déduire du libellé textuel et sans réécriture des
qualifications (l'ordre des secondaires et des lignes
de préférences est conservé). Il refuse un catalogue
source incomplet, des ARM en double et une cible déjà
renseignée. Les 82 entrées sont obligatoires, **mais cela
ne signifie pas créer 82 DataAssets d'armes concrètes**.

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.ClassAffinityProjection`
charge le catalogue réel, construit 82 archétypes transients,
compare chaque champ des 82 affinités et des six préférences,
vérifie la propreté du package et le refus d'un écrasement.

Aucun gameplay, build de kits, `.uasset`, WBP, carte
ou SaveGame n'est modifié. Le pipeline de migration
ne pourra utiliser ces valeurs qu'après validation UE
5.5.4 / Automation `Grimrock` du SHA exact.
