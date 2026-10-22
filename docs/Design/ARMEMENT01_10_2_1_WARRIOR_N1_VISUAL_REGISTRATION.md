# ARMEMENT01.10.2.1 — Raccordement des quatre visuels Guerrier au catalogue UE5

Date : 22 octobre 2026.
État : **code de préflight/raccordement proposé ; compilation/test UE et fabrication des assets non encore validés.**

## Référence attestée

Journal utilisateur TD04-20261022-141522 : filtre Grimrock.RPG.ARMEMENT01.10,
**3/3 Success, 0 warning, 0 failed, exit code 0**.

Préflight du commandlet GridWarriorN1ItemAuthoring :
- ARM014 BLOCKED, ARM015 BLOCKED, ARM025 BLOCKED, ARM031 BLOCKED.
- 0 READY / 4 BLOCKED / aucun package créé.
- Cause : binding visuel complet (icône + mesh réels) absent, pas une erreur de profil N1.

Cette étape n'invente ni source Blender ni asset PBR. Elle construit le **raccordement contrôlé** qui devient utilisable seulement après import artistique réel et validation humaine.

## Contrat de nommage, identités et dépendances

La source compilée GridWarriorN1VisualSpecs.h est la table de référence des 4 identités et chemins UE de présentation. Ces noms de fichiers sont **réservés**, pas des preuves de leur existence.

| Arme | ArchetypeId | VisualId | Stem des ressources |
|---|---|---|---|
| Épée droite | ARM014 | VIS_StraightSword_01 | StraightSword |
| Épée longue | ARM015 | VIS_LongSword_01 | LongSword |
| Pic de guerre | ARM025 | VIS_WarPick_01 | WarPick |
| Grande hache | ARM031 | VIS_GreatAxe_01 | GreatAxe |

Pour chaque ligne de stem X, importer manuellement dans UE5.5.4 :

- Content/GrimrockPrototype/Meshes/Weapons/SM_Weapon_X_01.uasset : vrai StaticMesh provenant de Blender ou autre outil 3D.
- Content/GrimrockPrototype/UI/Icons/Weapons/T_Weapon_X_01.uasset : véritable icône carrée RGBA, recommandation **512 x 512**, lisible à 48 x 48.
- Content/GrimrockPrototype/Art/Materials/Instances/Weapons/MI_X.uasset : instance valide d'un matériau PBR **assignée à un Material Slot du StaticMesh**.
- Content/GrimrockPrototype/Art/Textures/Final/Weapons/X/T_X_01_BC.uasset : albédo (sRGB ON).
- Content/GrimrockPrototype/Art/Textures/Final/Weapons/X/T_X_01_N.uasset : normale (sRGB OFF).
- Content/GrimrockPrototype/Art/Textures/Final/Weapons/X/T_X_01_ORM.uasset : occlusion/roughness/metallic (sRGB OFF, convention des canaux O-R-M conservée).

**Ne pas introduire de nouveau catalogue RPG ou de nouvelles données de dégâts.**

### Critères artistiques de recette, non automatisables

- ARM014 : épée droite une main, lame et garde reconnaissables, dimensions et prise crédibles.
- ARM015 : épée longue, silhouette réellement distincte de ARM014 ; usage des mains **toujours déterminé par le profil XLSX**, non par une supposition graphique.
- ARM025 : pic de guerre distinct d'une hache, pic et panne modélisés, orientation et centre de masse cohérents.
- ARM031 : grande hache imposante, **deux mains** selon la donnée N1 approuvée.
- Tous : pivot de prise, axe Z vers le haut, taille en cm (1 Unreal Unit = 1 cm), surfaces et normales correctes, UV sans étirements grossiers, sol sombre, ombrage réaliste.
- Matériaux PBR : un ou plusieurs Material Slots peuvent exister, mais MI_X doit être dans au moins un slot ; pas d'override à travers l'acteur de niveau. Contrôler **manuellement** le branchement effectif de BC/N/ORM dans le parent/instance : le commandlet vérifie la présence et l'espace colorimétrique, pas la topologie complète du graphe.
- Visuels : ne pas réutiliser arbitrairement le mesh de la dague, de l'arc ou du bouclier en guise de ces nouvelles armes.
- Toute appréciation du réalisme, des proportions et de l'alpha de l'icône est une **revue visuelle humaine** (captures UE recommandées), pas un résultat de test statique.

## Ce qui est ajouté en C++

UGridWarriorN1VisualRegistrationCommandlet :
- Sans -Apply, vérifie les quatre sets importés ; les absences donnent BLOCKED et un exit code **0**, sans modifier le Content.
- Avec -Apply -Archetype=ARMxxx, vérifie le set exact, crée uniquement **un binding + un FGridWeaponVisualProfile** dans le **DataAsset existant** DA_WeaponVisualCatalogue.
- Si l'arme possède déjà le même binding et les mêmes ressources, retourne REGISTERED, sans écriture.
- Si un binding déjà présent diffère ou si le VisualId est occupé, **refuse** toute correction/écrasement automatique.
- Conserve les trois pilotes historiques ARM001/ARM045/ARM075 et tous les profils additionnels, ainsi que leurs transforms, matériaux, ornements, etc.
- EquippedMesh reste optionnel : le résolveur existant utilise WorldMesh par défaut. Aucun transform ni effet artificiel.
- Contrôle taille de l'icône, géométrie et échelle minimale du mesh, matériaux attribués, instance parentée, trois maps PBR et sRGB approprié.
- Ne touche **jamais** au XLSX/DA_WeaponCatalogue, à la palette, aux 12 kits existants, au SaveGame, aux définitions d'items ni à la physique.

Test additionnel : Grimrock.RPG.ARMEMENT01.10.Visual.WarriorAssetContracts.
Avant import il valide identités et conservation des trois pilotes sans déclarer les quatre visuels prêts ; après chaque ajout il vérifie les chemins, les chargements, les Material Slots et les textures importées.

## Campagne immédiate — code seulement, aucune création d'assets

Fermer Unreal Editor et vérifier le working tree avant le pull :

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"

.\Scripts\RegisterARMEMENT011021WarriorVisuals.ps1 -EngineRoot D:\UE_5.5
~~~

Attendu **non encore attesté** : **4/4 tests**, 0 warning/failed ; le préflight visuel affiche 0 ready, 4 blocked, 0 registered, sans modification. La sortie détaillée du commandlet est conservée sous Saved/ARMEMENT01/ARMEMENT01_10_2_1_VisualRegistration.log ; le script ne reproduit que les lignes ARMEMENT pour éviter les milliers de lignes de démarrage UE.

## Après import et approbation visuelle d'une première arme

Depuis master (sans modifier artificiellement les profils), fermer l'éditeur et exécuter d'abord le préflight ciblé :

~~~powershell
.\Scripts\RegisterARMEMENT011021WarriorVisuals.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM014
~~~

Si le log affiche READY ARM014 et les captures montrent une silhouette conforme :

~~~powershell
.\Scripts\RegisterARMEMENT011021WarriorVisuals.ps1 -EngineRoot D:\UE_5.5 -Archetype ARM014 -Apply
~~~

Seul DA_WeaponVisualCatalogue.uasset peut alors devenir modifié par cette commande. Le script lance les tests ciblés après sauvegarde et vérifie le statut Git. Il n'effectue **aucun add / commit / push**.

Puis GridWarriorN1ItemAuthoring peut être relancé en préflight ; si ARM014 passe READY, exécuter son -Apply -Archetype=ARM014 en **opération distincte** pour créer DA_Weapon_StraightSword_01. Le DataAsset issu de ce second commandlet reste à tester en runtime et en PIE.

Ne pas essayer de finaliser plusieurs armes simultanément, ni utiliser git add . ou git clean. Les six chemins Shuriken/ThrowingKnife et les anciens assets visuels sont strictement exclus de ce lot. Contrôler le Reference Viewer, les dépendances des matériaux et Git LFS avant un commit binaire dédié.

**Critères de clôture finale ARMEMENT01.10.2 :** 4 nouveaux profils réellement importés/validés et liés, 4 définitions d'items N1 matérialisées et testées, combat/slots/PIE et revue artistique OK ; les trois pilotes préservés. Aucun de ces critères n'est déclaré réalisé par le seul présent commit.
