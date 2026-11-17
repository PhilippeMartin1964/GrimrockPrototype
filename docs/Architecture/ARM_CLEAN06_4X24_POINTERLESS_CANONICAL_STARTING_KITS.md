# ARM-CLEAN06.4X.24 — Kits de départ par identité canonique, sans ancienne définition

**17 novembre 2026 ; base `3d353420e9fb1d61403e6a6b4f7981b50e880eeb`.** C++ + tests seulement. Pas de réécriture de `.uasset`, de presets réels, de `.umap`, de WBP ni de SaveGame.

## Portée

`FRPGStartingEquipmentEntry` reçoit l'option `CanonicalItemId`. Les kits générés historiquement par `RPGStartingEquipmentPresetCatalog` possèdent toujours seulement `ItemDefinition` et restent donc **sur leur chemin d'origine** (niveau par défaut authoré, règles de quantité, enregistrement historique des anciennes définitions). Les six `DA_Class_*` n'ont pas de kits natifs à migrer selon l'audit X.19.

Pour une nouvelle entrée **explicitement canonique** :
- ItemId enregistré exigé. Ancienne définition facultative, mais si présente son `ItemDefinitionId` doit correspondre exactement à `CanonicalItemId`; aucune conversion implicite vers un autre objet.
- Sans ancien pointeur, `UGridItemSubsystem::ResolveItem` est l'autorité des niveaux et `MaxStack`, du poids, du nom, du type de slot et du défaut de lumière. Les ARM ont un **niveau physique N1–N20 explicite**, les autres items restent à N0 ; pas de création automatique d'un N1 arbitraire.
- Pour les mains, `FGridStartingKitEquipmentPolicyReadModel::ResolveCanonical` applique le `HandPolicy` du `WeaponCatalogue`, et les emplacements `NonArmEquipmentSlots` des non-ARM. Un item non équipable est refusé, un objet deux mains réserve MainHand/OffHand sans duplication.
- `TryBuildKit` met en scène la totalité du kit dans des états locaux et n'accorde rien en cas d'erreur : niveaux invalides, quantité incompatible, conflit de mains, source contradictoire ou place insuffisante. Les entrées sans ancien pointeur n'ajoutent aucun objet aux `OutDefinitions` historiques.
- `URPGClassAsset::IsValidDefinition` accepte un nouvel élément explicitement canonique sans ancienne définition ; refuse deux identités contradictoires. Il ne valide pas le registre lui-même ; c'est `TryBuildKit` qui fail-close au moment de l'octroi.

## Tests

Nouveau `Grimrock.RPG.ARM.CLEAN06_4X24.PointerlessCanonicalStartingKit`, sur vrais `DA_ItemSystem` et catalogues mais avec classe et états totalement transitoires : un `Weapon_Dagger_01` N20 en main, `Item_Torch` N0 et trois flèches N0 dans le sac, sans aucun ancien `UGridItemDefinitionAsset`. Vérifie noms/identités, quantité/poids/lumière canoniques, GUID, absence de définitions historiques publiées et absence de duplication. Rejette ARM N0, munitions N1, ancien pointeur contradictoire, et conflit de mains arc à deux mains + bouclier ; aucun octroi partiel. Les anciennes suites `Grimrock.RPG.ARM.CLEAN06_3.CanonicalStartingKitMaterializedLevels` et `...CanonicalStartingKitEquipmentHands` restent obligatoires.

Compilation UE 5.5.4 et suite automatisée `Grimrock` par l'agent Windows VALIDATION-AUTO01 sur le SHA du commit. Le test n'est ni une recette PIE du wizard ni une réécriture des assets.

## Barrières

**06.4 reste partielle ; 06.5 BLOQUÉE.** Après les conversions UE natives des catalogues ARM et des objets alchimie hors registre, les presets de départ C++ devront être écrits par `CanonicalItemId` et niveau physique, avec vérification du parcours `New Game`/wizard et de ses icônes/slots. Pas de modification de presets dans ce jalon : certaines recettes historiques contiennent encore `EmptyVial` ou `Bomb_Fire` hors registre. Ne jamais supprimer leurs définitions avant une décision/migration de contenu. SaveGame v27 demeure réservé aux copies protégées.
