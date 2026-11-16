# ARM-CLEAN06.4X.16.9 — Contrat SaveGame du monde courant, sans mutation

## Base et décision

Parent Git : a792e14422a4e7fcc039bb4b97e5edd8a7e37273. La validation PIE des serrures cuivre et fer de ARM-CLEAN06.4X.16.8.5 a été confirmée par le propriétaire le 16 novembre 2026. Les deux réceptacles avaient été validés précédemment.

Le SaveGame actuel est **v27 exact-match** (voir UGrimrockPartySaveGame). Il ne faut PAS implémenter de compatibilité avec des versions antérieures. Le présent jalon n'active aucun lecteur legacy, ne change pas le schéma et n'incrémente pas SaveVersion.

## Preuve ajoutée

Le test d'automatisation **Grimrock.RPG.ARM.CLEAN06_4X169** comprend :

1. CurrentSaveSchemaGate : contrôle des propriétés portant SaveGame, y compris les deux pointeurs historiques encore présents, ProjectileDefinitionAsset et FGridPendingWorldItemState.ItemDefinitionAsset.
2. CurrentWorldMemoryRoundTrip : UGameplayStatics::SaveGameToMemory puis LoadGameFromMemory pour deux niveaux transitoires. Vérifie GUID, ItemDefinitionId, N0, N20, quantité, pose Chaos, état lumineux, réceptacle, porte, serrure, trappe, variable d'énigme et objet en transit inter-niveau.
3. Les noms d'objets de test sont **des données synthétiques de sérialisation**, non des tentatives de résolution via les DataAssets du jeu.

Aucun appel à SaveGameToSlot, aucun chemin .sav, aucune lecture des sauvegardes personnelles et aucun SavePackage. Aucun .uasset, .umap ou WBP touché. Cette preuve en mémoire ne prétend PAS démontrer la compatibilité avec un fichier utilisateur existant, ni le rendu PIE, ni Cook/Shipping.

## Garde-fous et suite

Ne jamais retirer les deux propriétés historiques sous prétexte que le test en mémoire passe. Les pistes qui restent à vérifier pour 06.5 :

- Lecture/écriture et restauration de **copies isolées** de sauvegardes courantes v27, seulement lorsque des fichiers de test appropriés sont disponibles et avec retour arrière protégé.
- Inventaire complet des lecteurs runtime/Editor encore liés aux pointeurs UGridItemDefinitionAsset dans le monde, les réceptacles, les transferts PIT et les UI/presets.
- Suppression ponctuelle des champs obsolètes UNIQUEMENT après preuve de non-utilisation et de conversion des données.
- 06.6 : PIE complet, Chaos, Save/Continue, Cook et Shipping.

Validation requise sur le SHA final de ce jalon : UE5.5.4/SilentTask et UE5.5.4/LocalAuto = SUCCESS. Un statut absent/pending n'est pas une réussite.

**Règle permanente :** un seul commit final au maximum sur master ; ne pas rejouer les anciennes migrations binaires ARM-CLEAN06.4W/X.5 ou X.16.7/X.16.8.
