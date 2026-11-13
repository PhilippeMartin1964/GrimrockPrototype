# ARM-CLEAN06.2P — Premier assemblage canonique des 22 armes de production

Ce jalon assemble **en mémoire**, par le vrai manifeste
`Config/Armement/WeaponProductionManifest.csv`, les 22
`UGridArmamentDefinitionAsset` et leurs liaisons de visuels
dans un `UGridItemSystemAsset` transitoire. Il réemploie,
sans en créer de nouveaux, les convertisseurs validés
en ARM-CLEAN06.2J (visuels), 06.2K (icônes de commandes),
06.2L/06.2O (ItemId, nom, description, masse et tags).

Les armes individuelles contiennent exclusivement les six
champs canoniques ; les visuels sont factorisés par `VisualId`.
Le catalogue visuel possède une unique map `ActionIcons`.
Le candidat est ancré sous `GetTransientPackage()` et tous
les descendants sont des objets transients Unreal, sans
`SavePackage` ni écriture de fichiers sous `Content/`.

**Limite de sécurité explicite :** ce n'est PAS encore
un `DA_ItemSystem` complet. Les catalogues numériques,
d'actions et de présentations ne sont pas affectés par
ce constructeur. `UGridItemSystemAsset::Validate` doit
le refuser tant que ces trois références manquent.
Le test Automation vérifie ce refus ainsi que la
présence de 22 identités et l'absence de duplication
des visuels et de la palette d'icônes.

Les `ItemTags` historiques, même invalides selon
le schéma canonique final, restent inchangés ; le nombre
des définitions ne passant pas `HasValidIdentity` est
enregistré. Un tel refus bloque toute matérialisation
binaire, mais ne fait pas perdre l'original.

Le mapping vers les 82 archétypes N1–N20, les
`ItemActionBinding`, les variantes audio Sword,
les munitions et les SaveGame sont des étapes
à effectuer avant le cutover. Les 60 ARM sans
définition concrète restent légitimes et n'ont pas
besoin d'une fiche factice.

Ce commit ne modifie ni gameplay, ni mesh,
ni texture, ni audio, ni niveau, ni SaveGame.
La preuve de compilation et d'Automation
`Grimrock` doit être obtenue sur le SHA exact.
