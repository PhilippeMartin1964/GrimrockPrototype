# ARM-CLEAN06.4X.5 — matérialisation des niveaux physiques

**Périmètre :** quatre vrais `DA_GridLevel_*` et `DA_ObjectPalette_Default`,
déjà convertis vers `CanonicalItemId` en 06.4W, à partir de
`0a79e8a11368d8c68d0cfadc17fce4e23ac872e7` (X.4 validé).
**Objet :** enregistrer le niveau d'arme réel N1–N20 sans jamais inventer N1.
Les items non-ARM conservent N0. Les anciens pointeurs sont préservés ;
ce jalon n'est **pas** la bascule des lecteurs runtime/éditeur.

## Source de vérité et projection

Le commandlet `GridArmClean064X5PhysicalLevels`, uniquement Editor,
charge les 34 définitions du `DA_ItemSystem` puis les 95
`CanonicalItemId` déjà écrits : 44 objets libres,
0 spawns, 18 contenus initiaux des réceptacles et
33 entrées de palette.

Chaque `ItemId` doit correspondre à la définition historique
d'origine et à la famille du registre canonique.
La règle `FGridArmClean064X3AuthoredLevelMaterializer::TryResolve`
utilise le `DefaultWeaponItemLevel` **de cette source** pour transformer
un ARM initialement N0 en son vrai niveau N1–N20.
Les IDs, quantités, GUID, cellules, poses, textes, réceptacles, lumières,
règles d'énigme et autres propriétés restent intacts.

Les niveaux de ces placements sont maintenant persistés en tant que
`ItemLevel` effectif. Un champ nouveau,
`FGridObjectPaletteEntry::CanonicalDefaultItemLevel`, porte le niveau
physique d'une **nouvelle** pose depuis chacune des 33 entrées directes.
Il est N0 pour les non-ARM, N1–N20 pour les ARM ; aucun
`DefaultWeaponItemLevel` legacy n'est consulté par ce futur contrat.

L'ancien `DefaultItemDefinition` reste en place pour les lecteurs
non encore migrés. Son retrait n'est **pas** autorisé par X.5.

## Transaction et vérification

`Scripts/ApplyARM_CLEAN06_4X5_PhysicalLevelMigration.ps1` est
lancé uniquement pour le sujet exact du commit X.5 et le parent X.4,
dans le worktree isolé de l'agent UE5.5.4 :

1. Vérifie la liste fermée de cinq fichiers, Git LFS, les SHA-256 et les
   tailles réels publiés au commit final 06.4W.
2. Sauvegarde les cinq originaux dans un répertoire privé avec contrôle
   SHA-256 ; ne travaille jamais dans le checkout utilisateur.
3. Exécute le commandlet UE en mode `-Apply` puis interdit toute écriture
   hors des cinq paquets autorisés.
4. Relance un **autre processus** `UnrealEditor-Cmd` en `-Verify`.
   Exige les 95 identités, les niveaux explicites, le registre unique,
   les 44 + 0 + 18 + 33 références et la parité avant/après.
5. Relance la suite complète `Grimrock` sur ces vrais fichiers.
6. Publie **un seul commit X.5** avec les sources et les `.uasset`
   réellement changés, sous contrôle de concurrence Git
   `--force-with-lease`. Le nouvel SHA sera revalidé intégralement,
   avec statut spécifique `UE5.5.4/ARM-CLEAN06.4X.5`.
7. En cas d'échec avant la publication, tente une restauration
   uniquement sur la base de copies privées conformes et si le HEAD
   n'a pas changé. Un échec fermé ne doit jamais écraser une
   modification utilisateur ou un commit concurrent.

Les tests X.2 et X.4 restent compatibles avec les états
**avant et après** conversion ; ils ne sont pas, à eux seuls,
la preuve d'un fichier sérialisé. C'est le mode UE `-Verify`
après rechargement, puis le contrôle final GitHub du SHA, qui
autorisent la clôture de ce jalon.

## Interdictions et jalon suivant

Aucun SaveGame, `.umap`, WBP, source historique ou définition canonique
n'est modifié par cette opération. Les règles d'acceptation des
réceptacles, les clés de serrures et les conditions logiques
restent à auditer lors de la bascule.

**X.5 réussi ne prouve pas** la parité PIE, Chaos en mouvement,
lumière/Niagara visible, Save/Continue, Cook ou Shipping.
Prochaine étape : basculer runtime et éditeur vers les
`CanonicalItemId` et `CanonicalDefaultItemLevel` sans double
autorité, restaurer la projection canonique des effets visuels
et physiques et migrer séparément les sauvegardes sur copies,
puis seulement retirer les pointeurs historiques.
