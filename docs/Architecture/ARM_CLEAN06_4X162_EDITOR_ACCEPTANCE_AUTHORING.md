# ARM-CLEAN06.4X.16.2 — Authoring canonique des AcceptedItems dans Selected Object

Parent fonctionnel vérifié : X.16.1
`d5a2dbdd4dd1b4c921aaf53473fb78e8602778a4`,
avec `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto` SUCCESS.

## Portée

Le Grid Editor possède déjà `ResolveEditorItemRegistry` avec
registre canonique validé. La section **Selected Object /
Receptacle / Override Acceptance** l'utilise désormais
pour écrire le `CanonicalItemId` à chaque sélection
d'une ancienne définition connue.

Changer la source remet à jour identité et pointeur temporaire.
Effacer la source les efface tous les deux. Un catalogue
inaccessible refuse le changement ; une source réellement
inconnue conserve explicitement le chemin historique.
L'activation initiale d'une surcharge recopie les règles
par défaut en projetant uniquement les ItemId reconnus ;
la liste est créée atomiquement et garde ordre, doublons,
bAcceptAnyItem et MaxContainedItems. Le DataAsset de
définition partagé ne change jamais lors de l'édition
d'une instance.

L'affichage read-only des règles héritées montre le
CanonicalItemId lorsqu'il est déjà présent. Les serrures,
leurs clés et les contenus initiaux restent hors de portée.

## Tests

`Grimrock.RPG.ARM.CLEAN06_4X162.CanonicalAcceptanceAuthoring`
vérifie la sélection, son changement, l'effacement, le
refus d'un registre absent, les sources inconnues,
la copie complète sans mutation de la définition,
les surcharges et les contradictions rejetées.

Aucun `.uasset`, `.umap`, `.sav` ni Blueprint
n'est modifié par ce jalon ; les véritables règles
déjà stockées dans les assets attendent une
migration transactionnelle distincte et une recette PIE.
