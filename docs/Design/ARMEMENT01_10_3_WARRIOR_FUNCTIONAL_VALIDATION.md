# ARMEMENT01.10.3 — Validation fonctionnelle des quatre armes du premier lot Guerrier

**Statut : tests ajoutés, exécution locale et campagne PIE EN ATTENTE.** Aucun résultat n'est déclaré PASS avant le rapport TD04.2 et les observations du testeur.

## Périmètre et autorités

Armes réelles : ARM014 Épée droite, ARM015 Épée longue, ARM025 Pic de guerre et ARM031 Grande hache. Les quatre ItemDefinitions sont des objets de niveau fixe 1, dont les statistiques et l'usage des mains proviennent du DA_WeaponCatalogue. ARM031 est obligatoirement à deux mains. Le pic de guerre suit le catalogue, sans valeur inventée. Sources de présentation : DA_WeaponVisualCatalogue et meshes/icônes réellement importés.

Carte de vérification : L_Dungeon, DataAsset de niveau DA_GridLevel_00. Palette : DA_ObjectPalette_Default, catégorie Weapon. Ne pas toucher aux autres niveaux ou à la création de personnage. Les quatre armes ont été publiées puis ajoutées au donjon sur master.

Aucune reconfiguration de statistiques, talents, kits, SaveGame, commandes du personnage, assets artistiques ou classe métier n'appartient à ce jalon.

## Tests automatisés nouvellement ajoutés

Fichier : Source/GrimrockPrototypeEditor/Private/Tests/GridEditorARMEMENT01103WarriorFunctionalTests.cpp.

| Test (suffixe sous Grimrock.RPG.ARMEMENT01.10.3) | Ce qu'il vérifie | Limitation |
|---|---|---|
| ProductionBindings | Quatre vrais DataAssets, correspondance N1/visuel, palette Weapon sans doublons, présence des quatre items dans DA_GridLevel_00 | Aucun joueur réel ne ramasse les armes |
| TransientEditorPlacement | Quatre vrais items placés sur des cellules dans un niveau **temporaire**, identités et meshes visibles dans l'aperçu | Ce test ne valide ni l'apparence artistique ni les clics PIE |
| RealInventoryEquipUnequip | Instanciation N1, ajout au sac, équipement MainHand, réservation de la main secondaire selon le catalogue, retrait et intégrité GUID | Pas de widget UMG ni déplacement souris |
| GreatAxeTwoHandedTransaction | Grande hache à deux mains remplaçant une vraie épée et un vrai bouclier, sans pertes/duplications | Ne valide pas l'animation tenue en main |
| RealCatalogueCombatProfiles | Attaque équipée dérivée de N1, dés/bonus/PA/portée/type/scaling, jet déterministe via le résolveur | Ne teste ni monstre réel, ni initiative, ni consommation effective de PA |

Tous les tests sont en mémoire, ne sauvegardent aucune carte ni aucun DataAsset et n'essaient pas d'inférer un succès PIE.

## Étape A — compilation et automatisation

Fermer Unreal Editor. Sur master, avec les dernières ressources versionnées et sans travaux locaux à écraser :

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.3"
~~~

**Résultat attendu si tout passe : cinq nouveaux tests réussis, zéro warning/failed, code 0.** Relever le répertoire TD04, le journal et le code retour. En cas d'échec de compilation ou de test, conserver les logs et ne pas déclarer le jalon validé.

Puis régression des contrats précédents :

~~~powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.RPG.ARMEMENT01.10"
~~~

Ce filtre inclut l'ancienne suite (six tests connus) **et** les cinq nouveaux tests. Il ne remplace pas la campagne PIE.

## Étape B — essai PIE en L_Dungeon

**Préparation.** Utiliser une sauvegarde de test séparée ; éviter de déplacer/supprimer les originaux du niveau éditorial. Démarrer le jeu normalement, ouvrir l'inventaire avec I et sélectionner un personnage ayant suffisamment de place dans son sac. Noter le nom de l'arme, le nombre d'items avant/après et l'état du slot MainHand/OffHand. Faire une capture des problèmes d'orientation/échelle du mesh.

Pour chacune des quatre armes :

1. **Monde et interaction :** localiser l'arme au sol, vérifier forme et matériau ; ramasser par le clic prévu ; s'assurer que l'arme disparaît du monde et apparaît une fois dans le sac.
2. **Inventaire :** vérifier l'icône carrée, le nom, le poids et le tooltip, sans valeur dupliquée ou illisible ; inspecter l'arme dans l'interface existante.
3. **Équipement :** équiper en MainHand ; vérifier le retrait du sac, la silhouette tenue si affichée et la gestion des mains ; déséquiper puis vérifier le retour au sac sans duplication.
4. **Dépôt et récupération :** déposer l'arme dans une zone dégagée puis la reprendre ; vérifier identité, quantité, position et collisions. Ne pas utiliser le même objet de la sauvegarde principale.

**Cas spécifique ARM031 :** équiper d'abord ARM014 en MainHand et un bouclier en OffHand, puis équiper ARM031. Les deux équipements précédents doivent retourner dans le sac (s'il y a de la place), le slot OffHand doit être réservé et aucune seconde arme/bouclier ne doit rester effectivement équipée. Vérifier ensuite le retrait depuis la main secondaire virtuelle.

**Cas de sac plein :** tenter le remplacement des deux mains par ARM031 avec une capacité insuffisante ; le refus doit conserver chaque objet à son emplacement initial. Tester d'abord dans une sauvegarde de test.

## Étape C — combat réel

Sur une sauvegarde de test, avec un monstre à portée et une initiative/phase autorisant le personnage à agir :

- Lancer une attaque par arme et relever l'identifiant d'action présenté, si visible, ainsi que PA avant/après, portée, résultat hit/miss et dégâts observés.
- Vérifier un refus pour PA insuffisants et pour cible hors portée ou absente, sans consommation abusive des ressources.
- Pour ARM031 : confirmer qu'une seule action d'arme est fournie malgré la réservation visuelle des deux mains.
- Répéter pour ARM014/015/025/031 ; vérifier les différences de profils sans attribuer des dégâts numériques inventés.

En cas de problème, conserver capture vidéo/capture écran, arme concernée, séquence reproduisant l'échec, log de session et état du personnage.

## Étape D — sauvegarde/restauration

Sur la sauvegarde de test seulement : sauvegarder avec une arme équipée, recharger, contrôler ItemDefinitionId, ItemLevel=1, quantité, slot et autres armes laissées dans le sac. Vérifier notamment la conservation de l'occupation virtuelle OffHand par ARM031. Ne pas conclure de la seule validité d'un FGridPartyInventoryState en mémoire que le SaveGame complet est vérifié.

## Matrice d'acceptation manuelle

| ID | Scénario | ARM014 | ARM015 | ARM025 | ARM031 |
|---|---|---|---|---|---|
| P1 | Visuel et placement dans le donjon | À tester | À tester | À tester | À tester |
| P2 | Ramassage, retrait du monde, item unique | À tester | À tester | À tester | À tester |
| P3 | Inventaire, icône, tooltip | À tester | À tester | À tester | À tester |
| P4 | Équipement/déséquipement et contraintes de mains | À tester | À tester | À tester | À tester |
| P5 | Dépôt et récupération | À tester | À tester | À tester | À tester |
| P6 | Attaque réelle, coût PA, portée, dégâts | À tester | À tester | À tester | À tester |
| P7 | Sauvegarde, rechargement, persistance | À tester | À tester | À tester | À tester |

ARM031 demande en outre le scénario bouclier+épée -> hache à deux mains et la gestion de l'inventaire plein.

**Clôture :** 5/5 nouveaux tests techniques + ancien filtre en régression sans échec, puis observations PIE P1–P7 pour les quatre armes. Un test non exécuté = NON TESTÉ, jamais PASS. Aucune publication d'asset et aucun changement de comportement gameplay dans le présent ticket.
