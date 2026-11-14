# ARM-CLEAN06.3V — Munitions sans niveaux : ItemLevel=0 canonique

**Base :** 06.3U, compilation et `Grimrock` verts obligatoires sur le SHA exact.

## Problème observé

Les flèches réelles `Item_Arrows_01` sont un objet non-armement à `ItemLevel=0` dans le sac, la munition consommée par `GridTurnManagerPlayerActions.cpp`, le projectile récupérable et la sauvegarde. Le résolveur `UGridItemSubsystem::ResolveItem` exigeait N1–N20 pour **toute** famille, ce qui empêchait les icônes, le poids, la masse physique et les meshes de pickups de lire l'autorité canonique des flèches. Remplacer toutes les flèches N0 par N1 en cours de partie changerait silencieusement l'état de sauvegarde.

## Correction du contrat

`ResolveItem` garde **N1–N20 strict pour UGridArmamentDefinitionAsset**. Les objets non-armement autorisent aussi `ItemLevel=0` sans ajout de niveau, re-roll, bonus ou multiplication de poids. Les anciens consommateurs N1–N20 des items non-ARM restent momentanément acceptés, car ils lisent les mêmes valeurs statiques et les tests historiques n'ont pas encore été convertis. Les niveaux négatifs ou supérieurs à 20 restent invalides pour toutes les familles.

Les flèches d'arc sont désormais reconnues par les lecteurs déjà migrés depuis 06.3I : nom, icône, poids unitaire et total, mesh de pickup, plaque de pression et Chaos si l'opt-in est actif. Aucun `FGridItemInstance`, niveau sauvegardé, objet `.uasset`, projectile, mode de vol, inventaire, WBP ni règle de combat n'est modifié. `AGridThrownItemActor` garde explicitement le vol historique des munitions jusqu'au cutover du visuel de projectile des items non-ARM ; la nouvelle logique R–U reste réservée aux armes cataloguées.

## Preuves

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalUnlevelledAmmunitionN0` lit le vrai `DA_ItemSystem` et `Item_Arrows_01`, compare le contenu N0/N1, le stack x12 avec poids historique contradictoire, l'icône de slot, le texte de l'item et le mesh de pickup. Il affirme aussi le refus de N-1/N21 et le maintien du refus N0 pour une arme. La suite `Grimrock` et UE5.5.4 sont exécutés par VALIDATION-AUTO01 après publication.

**Reste à vérifier visuellement :** flèche tirée et récupérée, son mesh au sol, ses icônes et son poids, sauvegarde/reprise. Aucun changement localisable de texte : `UI.csv → ST_UI → WBP → Game/fr/en` inchangé.
