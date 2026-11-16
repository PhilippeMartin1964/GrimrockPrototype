# ARM-CLEAN06.4X.16.5 — Préflight distinct des clés de serrure (read-only)

Parent fonctionnel : ARM-CLEAN06.4X.16.4,
`50e9f628d844f96f187f72cca38c3a2d03a9f6bd`,
validé par les deux contrôles UE5.5.4.

## Pourquoi un jalon distinct ?

`AGridWallLockActor::InitializeRuntimeWorldObject` construit
les clés autorisées en réunissant **deux sources** :
`FGridLockBehaviorParams::AcceptedKeyItems`
(pointeurs hérités) et `AcceptedKeyIds` (identifiants
saisis directement), avec `AddUnique`.
Une surcharge d'instance peut remplacer les deux listes
via `bOverrideAcceptedKeys`.

Les identifiants explicitement authorés peuvent être
des clés narratives/scriptées **absentes** du catalogue
canonique des 34 objets. Il serait incorrect de les
rejeter ou renommer au seul motif qu'ils ne sont pas
dans `DA_ItemSystem`.

## Modèle de proposition

`FGridLockKeyMigrationPreview::TryPreview` :
- préserve intégralement les `AcceptedKeyIds` explicites,
  y compris leur ordre, leurs doublons, et leurs IDs
  narratifs ; n'en supprime aucun ;
- propose seulement l'ajout à la fin des IDs issus
  de pointeurs **réellement connus** du registre et non
  déjà présents ;
- refuse **toute la liste** si l'un des anciens pointeurs
  est nul, sans ItemId, ou inconnu du registre ;
- ne modifie jamais les pointeurs, le mode
  `bConsumeKeyOnUnlock`, `bStartsUnlocked`,
  les messages ou les liens d'énigme ;
- garantit par test que l'union des clés acceptées
  par l'ancien runtime est identique avant/après
  la proposition, en conservant les surcharges.

Une proposition sûre n'est **pas encore** un commit
de migration des assets. Ajouter un ID explicite
issu d'un pointeur peut introduire une duplication
technique temporaire mais pas une nouvelle clé
acceptée : l'acteur effectue `AddUnique`.

## Tests

- `Grimrock.RPG.ARM.CLEAN06_4X165.AtomicLockKeyUnionPreview` :
  deux pointeurs canoniques, doublons, ID scripté
  inconnu, refus de source vide/inconnue et
  conservation de l'ensemble accepté.
- `Grimrock.RPG.ARM.CLEAN06_4X165.RealLockKeyListsReadOnly` :
  lecture des vrais quatre niveaux et des définitions
  via AssetRegistry, comptage par type de règle,
  absence de mutation des packages.

Les nombres sont disponibles dans les journaux UE
Windows. Un statut GitHub SUCCESS ne les affiche pas.
L'éditeur Selected Object conserve pour l'instant
ses deux listes historiques ; une bascule de l'UI
requiert un nouveau jalon.

Aucun `.uasset`, `.umap`, `.sav`, WBP ni Blueprint
modifié. Convertir les vrais assets demande des
copies, une validation de réouverture et PIE, et une
preuve de non-régression du déverrouillage.
