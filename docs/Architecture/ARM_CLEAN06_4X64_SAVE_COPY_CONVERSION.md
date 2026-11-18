# ARM-CLEAN06.4X.64 — conversion binaire v27 sur copies

Parent : `89f3beb4891633cd042ad153fcd96997c79cd38b` ; les deux statuts Windows de X.63 sont SUCCESS. Aucun nouveau Go ni contrôle PIE n'est nécessaire pour ce lot de code.

## Objet et limites

X.63 a prouvé la projection conjointe 38 -> 48 et le remapping des quatre ActionId d'élixir. X.64 utilise **les vraies fonctions UE** LoadGameFromMemory / SaveGameToMemory avant tout remplacement des fichiers. Pas de migration runtime ni d'alias historique. SaveVersion reste 27. Aucun changement de schéma. Aucun autre ItemId, GUID ou mécanisme n'est modifié. Les anciennes références d'objet dans le monde (ProjectileDefinitionAsset/PendingInboundItems.ItemDefinitionAsset) restent à traiter dans le futur lot SaveGame global, elles ne sont pas effacées par X.64.

`GridEditorAlchemySaveCopy.h` : charge exactement UGrimrockPartySaveGame v27 accepté par son validateur, contrôle les références SourceActionId des surfaces et des pièges (refus si ancien ActionId commun ambigu), convertit actifs et réserve, reconstruit un état attendu à partir d'un second décodage indépendant. Réencode en mémoire, recharge et compare chaque propriété SaveGame réfléchie du snapshot, puis confirme qu'il ne reste plus de liaison à substituer. Les données transitivement comparées incluent les caches reconstruits par le vrai loader ; une divergence bloque la production de la copie, même si elle provient d'un cache. Ne pas affaiblir arbitrairement ce contrôle. Si aucun remapping n'est nécessaire, restitue les mêmes octets, pas une nouvelle sérialisation. Refuse l'alias des buffers. En cas d'erreur, aucune sortie candidate et l'entrée est inchangée.

La politique SaveGame existante interdit l'enregistrement en combat actif ; la sauvegarde stable de précombat est aussi un .sav à inventorier. Les quatre cooldowns d'élixir sont 0, aucun champ de cooldown d'action persistant dans le snapshot v27 ; ne pas inventer une migration des timers. Les surfaces et pièges persistants disposent d'un SourceActionId mais pas d'ItemId : un ancien ID partagé y est incohérent et doit être signalé, pas deviné. Les sources valides de bombes, d'acide, d'huile etc. conservent leurs IDs et leurs données.

## Commandlet offline

`GridArmClean064X64SaveCopy` accepte exclusivement `-ConvertCopy -SnapshotRoot=<dossier absolu privé> -File=<nom-simple.sav>`. Lit <root>/Original/<fichier> et crée <root>/Converted/<fichier> (dossiers existants, sortie absente). Refuse Apply/Publish, chemins/nom de périphérique en guise de fichier, racine dans le projet ou son Saved, écrasement de fichier. Création via FILEWRITE_NoReplaceExisting ; comparaison des octets originaux avant/après, relecture de la sortie et vérification idempotente. En cas de sortie partielle/erreur, le fichier de travail n'est pas un résultat autorisé ; ne pas l'installer. L'opérateur doit créer de vrais dossiers privés, sans jonction/symlink, et copier les originaux (pas de hardlink). Aucun appel de SaveGameToSlot, pas de recherche ni de remplacement implicites de saves live.

`Scripts/PrepareARM_CLEAN06_4X64_SaveCopies.ps1` : vérifie le SHA et un checkout propre, les deux statuts Windows ; photographie les .sav d'un dossier explicitement désigné (y compris AutoCombat), refus de répertoires imbriqués et de reparse points, copie dans un nouveau snapshot privé, contrôle SHA256 des entrées et des copies, invoque le commandlet sur chaque copie, vérifie les entrées inchangées et les sorties, publie un reçu JSON privé uniquement après succès complet. Les saves live ne sont jamais écrites ni déplacées. La preuve d'un dossier vide est valide pour ce seul dossier ; les saves d'autres installations/packagings restent hors périmètre. Un format inconnu, un nom non pris en charge ou une sauvegarde invalide bloque la transaction, sans destruction.

Aucune exécution de ce script demandée maintenant : il sera intégré au protocole de bascule native ultérieur. La restauration/installation des copies vérifiées est une opération distincte, à synchroniser avec la migration native : ne pas jouer avec des raccourcis renommés sur le registre encore à 38. Ne pas versionner Saved ou les archives des joueurs. Ne pas exécuter les anciens migrateurs.

## Test

Grimrock.RPG.ARM.CLEAN06_4X64.SaveBinaryCopyRoundTrip : création réelle de personnage, quatre élixirs dans actifs et réserve, variables de niveau, objet monde N0, piège d'une autre action, portraits, position, temps de jeu. SaveGameToMemory puis conversion, LoadGameFromMemory, comparaison réfléchie avec l'état attendu indépendamment décodé, mêmes octets au no-op ; refus alias, entrée vide, ID ambigu en réserve, source ambiguë de piège. Test de mémoire exclusivement, pas de fichier joueur ouvert. Un test vert ne prouve pas qu'un fichier joueur particulier est convertible.

## Suite

Préparer la transaction de 12 packages du registre d'alchimie, les gates exacts 38/48, le kit Grenadier et la neutralisation de l'authoring historique. Contrôler l'exécution réelle des actions. X.64 ne modifie aucun package natif, fichier de sauvegarde live, WBP, UI.csv, ST_UI ou localisation Game/fr/en. **Le registre persistant reste à 38.** La prochaine écriture doit être protégée par le snapshot et la preuve de conversion des saves concernées.
