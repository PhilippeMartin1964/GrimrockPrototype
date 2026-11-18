# ARM-CLEAN06.4X.66C — résolution RPG à cache froid avant migration des sauvegardes

Parent validé : `5d13c82a3e3a6f93d1f8fcaf79b9f70d9f9cad99`. Première exécution native X.66B (18.11.2026) : build, Preflight et Verify préalable OK (code 4 attendu). Lors de la conversion préalable de la copie de GrimrockParty.sav, le chargeur v27 refuse ActiveCharacter[0] pour budget de caractéristiques incohérent (code 3). Le journal indique immédiatement avant deux conflits AssetManager : RPGRace / RPGRaceAsset pour DA_Race_Dwarf et RPGPortraitSet / RPGCharacterPortraitSetAsset pour DA_PortraitSet_Dwarf. La transaction n'a pas atteint Apply ni l'installation des saves, aucun commit natif créé.

## Analyse et correction

FRPGAttributePointService::TryGetBalance exige aussi une résolution valide de la race et de la classe. Son seul booléen ne distingue pas une définition non résolue d'un réel dépassement du budget. FRPGAuthoringIdentityResolver faisait un ScanPathsForPrimaryAssets général avec un type sémantique qui peut entrer en conflit avec le type natif renseigné ou deviné par l'AssetRegistry. Les fixtures X.64 créaient des définitions transitoires et alimentaient leurs caches ; elles ne démontraient pas la résolution de vrais assets.

Le résolveur conserve les caches d'identité et les résultats AssetManager dont GetPrimaryAssetId correspond exactement. Si aucune solution n'est disponible, il utilise l'AssetRegistry par classe concrète native et vérifie l'identité sémantique de chaque asset chargé, sans déduction depuis un nom de fichier ni nouvel ID. Refus d'ambiguïté sur cette recherche. Le scan synchrone Editor est limité au dossier d'authoring RPG ; la recherche de classe reste sous /Game. Pas de scan global AssetManager conflictuel. AssetRegistry ajouté aux dépendances privées. Aucun changement des règles de points, de l'XP, des identités, des assets ou du schéma v27.

Le commandlet conserve le motif exact GetLoadError et rapporte les IDs de classe/race, résolution, niveau, XP, validité de plage, somme des attributs, total initial et points accordés sur les personnages refusés. Ces diagnostics sont locaux ; aucune redistribution ni correction de données joueur n'est autorisée. Si la sauvegarde reste incompatible après résolution correcte, il faut analyser le budget réel avant tout changement.

## Validation

Grimrock.RPG.ARM.CLEAN06_4X66.ColdNativeRPGSaveCopy : six races et six jeux de portraits réels, six classes et six visuels réels, résolution après reset des caches, refus race inconnue ; création d'un nain guerrier réel, archive v27, conversion de raccourci après reset, nouveau chargement, parité GUID/attributs/portrait, rejet d'attribut réellement invalide. Un test de cache froid ne remplace pas la réouverture de la sauvegarde réellement concernée dans un nouveau processus local. Aucun fichier personnel ouvert par le test.

Après deux statuts Windows réussis sur ce SHA exact, relancer le script X.66 en remplaçant ExpectedHead par le nouveau SHA. Le script garde toutes les protections et crée de nouvelles copies privées ; conserver les snapshots précédents. Le registre natif reste à 38 tant que X.66B n'est pas effectif. Aucun nouveau texte joueur/WBP/localisation.

