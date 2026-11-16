# ARM-CLEAN06.4W — migration réelle des références d'items de niveaux

## Situation et périmètre vérifié

Socle fonctionnel impératif : ARM-CLEAN06.4V, commit b9750269732f1322ecca11ec2bcd30ba3a4796c2, trois validations UE5.5.4 SUCCESS, issue GitHub #155. Parent Git réel du jalon W : commit documentaire 5b7131fe55d7cd770e9ae2bcc10901db7d7dfd51, conservé sans réécriture. Le registre possède 34 définitions réelles.

L'inventaire des assets UE du jalon 06.4H (issue #141) avait trouvé 95 références, dont 40 alors privées de définition canonique : 44 objets au sol (13 absents), 18 contenus initiaux de réceptacles (17 absents), 33 entrées de palette directe (10 absentes). Il ne contenait aucun item spawn ni aucun contenu par défaut de définition de monde. Les 40 identités auparavant manquantes ont maintenant été migrées en 06.4T/U/V.

Le jalon W convertit **les 95 références**, pas uniquement les 40 précédemment sans définition.

## Conversion effective dans les assets

Les structures FGridLooseItemInstance, FGridItemSpawnInstance, FGridReceptacleInitialItemConfig et FGridObjectPaletteEntry reçoivent une seule propriété persistante CanonicalItemId (FName). Elle est issue exclusivement de l'ItemDefinitionId de la définition historiquement liée au placement, puis vérifiée contre le vrai DA_ItemSystem et, pour une arme ARM, contre son ArchetypeId.

Le commandlet GridArmClean064WWorldItemReferences exige exactement quatre DA_GridLevel dans GrimrockLevels et la palette DA_ObjectPalette_Default. Il contrôle 44 loose, 0 spawns, 18 réceptacles, 33 entrées de palette, 0 contenu d'items par défaut dans les définitions de monde, donc 95 références au total. Il refuse toute identité nulle, quantité invalide, niveau hors 0–20, classe ou archétype divergent et tout compte inattendu. Aucune donnée n'est déduite d'un mesh, d'une icône ou d'un nom de fichier.

Le passage -Apply sauvegarde réellement 5 binaires Unreal : DA_GridLevel_00, DA_GridLevel_01, DA_GridLevel_New_Level_03, DA_GridLevel_Pillars_of_Light et DA_ObjectPalette_Default. Le passage -Verify rouvre les 5 assets dans un autre processus Unreal et exige une parité de toutes les 95 identités. Le test NativeWorldReferences accepte seulement zéro référence encore migrée avant le passage, ou exactement 95 après le passage.

## Propriétés historiques et autorité runtime

W ne crée **aucun nouveau fallback runtime**. Pour permettre la lecture initiale des anciens assets binaires, les champs pointeurs préexistants restent temporairement dans les structures de données pendant la conversion. Ils n'acquièrent aucune nouvelle autorité. La suite doit basculer tous les lecteurs runtime/editor vers CanonicalItemId, puis supprimer ces anciens champs sans retour possible aux définitions legacy.

Les niveaux des items, quantités, GUID de placement, messages, interactions et valeurs physiques restent inchangés. Les anciens ItemLevel=0 des armes correspondent encore à une valeur de placement héritée : la bascule runtime devra calculer leur véritable niveau d'origine explicitement, sans N1 arbitraire. Les fichiers SaveGame personnels ne sont jamais ouverts dans W.

## Validation et publication

Sujet exact : feat(ARM-CLEAN06.4W): migrate real level and palette item references to canonical ItemIds

L'agent Windows appelle Scripts/ApplyARM_CLEAN06_4W_WorldReferenceMigration.ps1 après Build et suite Grimrock complète. Le script sauvegarde en privé les 5 originaux et leurs SHA256, contrôle une liste blanche Git LFS stricte, appelle Unreal -Apply puis -Verify dans deux processus, et exécute à nouveau Grimrock après écriture. Si le worktree de validation contient encore des pointeurs Git LFS malgré fetch/checkout, l'agent peut hydrater **uniquement ces cinq sources** depuis l'instantané privé signé ARM-CLEAN06.4G, à condition de prouver pour chacune la correspondance exacte avec les cinq OID SHA-256 et tailles publiés par Git au commit de base 5b7131fe, ainsi que l'absence de modification des cinq fichiers entre ce commit et le candidat 06.4W, puis d'exiger un worktree propre avant et après la copie. Le contenu textuel d'un pointeur local non matérialisé ne constitue pas une autorité : seuls les OID du commit de référence, les empreintes de l'instantané et l'état Git propre permettent la restauration isolée. Une preuve manquante ou divergente bloque strictement la conversion ; aucun fichier du checkout utilisateur n'est restauré. Il amende uniquement son propre commit de code, sans changer le parent validé 06.4V, avec une publication Git force-with-lease. Une seconde validation Windows sur le SHA final doit contrôler exactement les 5 assets publiés.

W n'est terminé qu'avec SilentTask, LocalAuto et ARM-CLEAN06.4W tous SUCCESS sur le commit **final contenant les cinq fichiers UE**.

Suite : cutover runtime/editor des 95 identités, suppression des anciens pointeurs de niveau/palette, migration des SaveGames sur copies, puis PIE/Chaos/Save-Continue/Cook. Aucune modification des fichiers .sav, .umap, WBP ou des armes canoniques n'est autorisée dans W.
