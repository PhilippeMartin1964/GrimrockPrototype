# ARM-CLEAN06.4X.50 — Overrides de serrures : copie canonique par ID sans anciens pointers

**Parent :** `336ad30a946284bca1af58a5178eb57836ec681c` (X.49 validé par les deux contrôles Windows).
**Portée :** helper Editor C++, panneau Slate existant, tests transitoires
et sur deux vraies définitions en lecture seule ; aucun package UE écrit.

## Constats et justification

Les deux serrures réelles `DA_Lock_CopperWall` et
`DA_Lock_IronWall` ont déjà `AcceptedKeyIds=[Key_Copper]`
et `[Key_Iron]`, mais leurs anciens `AcceptedKeyItems`
restent sérialisés. Le runtime `FGridWallLockAcceptedKeyReadModel`
rend les `AcceptedKeyIds` non vides autoritaires. En revanche,
l'inspecteur `SGridEditorObjectInspectorPanel` recopiait
encore `AcceptedKeyItems` **et** `AcceptedKeyIds` lorsqu'un
utilisateur activait `Override Accepted Keys` pour un placement.
Cela pouvait ajouter de nouvelles références historiques dans
le niveau après une migration.

## Comportement X.50

- Le helper `FGridEditorCanonicalLockRuleAuthoring::TryCopyDefaults`
  s'appuie sur la projection déjà validée
  `FGridLockKeyMigrationPreview::TryPreview`. Elle préserve les
  IDs explicites tels que saisis (y compris une clé scriptée
  hors `DA_ItemSystem`), leur ordre et leurs répétitions,
  puis ajoute les IDs connus des anciens pointeurs non encore
  représentés. Les pointeurs nuls ou vers des IDs réellement
  non enregistrés **refusent toute la copie**.
- L'override construit en mémoire ne contient **que**
  `AcceptedKeyIds` : `AcceptedKeyItems` est vide. Aucune
  règle par défaut partagée n'est modifiée.
- Le toggle `Override Accepted Keys` convertit atomiquement
  les defaults avant d'écrire dans le niveau. Un refus conserve
  l'override préexistant sans changement.
- Tant qu'une liste d'IDs explicites est autoritative,
  l'éditeur masque les commandes qui ajouteraient ou
  remplaceraient des sources `AcceptedKeyItems` obsolètes ;
  l'édition des `AcceptedKeyIds` existants reste accessible.
  Le picker historique reste disponible pour un override
  réellement dépourvu d'IDs explicites.

## Tests

`Grimrock.RPG.ARM.CLEAN06_4X50.PointerlessLockOverrideAuthoring` :

- registre réel, liste mixte comportant des définitions
  Key_Copper / Key_Iron et une clé scriptée, identité finale
  conservée dans le modèle d'instance sans source `UObject`
- vérification de `GridObjectInstanceBehavior::Resolve` :
  les autres paramètres de serrure sont préservés et le
  runtime voit le même ensemble d'IDs
- refus du registre manquant, d'une source hors registre
  et d'un pointeur nul, sans écriture partielle
- projection **en lecture seule des vrais assets**
  `DA_Lock_CopperWall` et `DA_Lock_IronWall`,
  confirmation des clés Copper/Iron, absence de
  pointeurs dans les copies et absence de `dirty` natif

Les tests X.29 et X.165 préexistants continuent de
vérifier la priorité des clés explicites et les anciennes
fixtures synthétiques ; X.48 reste le verrou sur les 63
pointeurs réellement non migrés.

## Suite

X.50 interdit de **réécrire de nouveaux pointeurs** mais
n'efface pas les deux sources actuellement dans les
DataAssets partagés. Leur conversion native devra faire
l'objet d'une transaction UE isolée, d'un snapshot privé,
d'une comparaison stricte de la liste des clés acceptées,
d'une réouverture en second processus, de la suite Grimrock,
des deux validations GitHub puis d'une recette PIE ciblée.
Les 18 contenus de réceptacles, 33 palette, 3 overrides
inactifs, 5 loots et SaveGame v27 restent inchangés.

**06.4 PARTIELLE ; 06.5 BLOQUÉE.**
