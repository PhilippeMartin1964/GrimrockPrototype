# ARM-CLEAN06.4G — Prévol de transaction binaire, sans cutover

**Point de départ validé :** 06.4F `ec43ca762860533b0de43269f346a48dc25983ae`,
trois statuts SUCCESS, issue GitHub 139.

## État des preuves avant 06.4G

- 06.0, 06.2A et 06.4A : quatre assets Sword modifiés dès
  l'ajout utilisateur des sons, huit propriétés préservées,
  zéro dérive ultérieure de propriétés, référents et profils.
- 06.4E : quatre présentations d'attaque dans le
  `DA_ActionPresentationCatalogue` **réel** strictement
  identiques aux données historiques actuelles (pas uniquement
  à des prototypes transitoires).
- Le reste de la migration (24 armes, 24 items, niveaux,
  réceptacles, kits, projectiles et SaveGame) **n'est pas
  déclaré prêt**, malgré ces preuves audio.

## Objet du jalon

Installer un mécanisme de **protection matérielle** testable :
sauvegarde immuable de la source et répétition d'un retour
arrière **dans un bac à sable privé**, avant de développer
un convertisseur qui puisse sauvegarder des packages UE.

Le script Windows `Scripts/PrepareARM_CLEAN06_4G_BinarySnapshot.ps1`
est activé seulement par le sujet de commit exact :

`chore(ARM-CLEAN06.4G): stage immutable UE binary backups and rollback drill`

après compilation et suite Automation `Grimrock` réussies.

## Liste fermée et preuves matérielles

| Famille | Nombre | Origine |
|---|---:|---|
| 24 anciens `DA_Weapon_*` + 24 `DA_Item_*` | 48 | manifeste UE 06.4A |
| 2 anciens catalogues | 2 | manifeste UE 06.4A |
| 4 assets de niveaux | 4 | manifeste UE 06.4A |
| 1 donjon + 1 palette | 2 | manifeste UE 06.4A |
| 1 `DA_ItemSystem` + 4 catalogues canoniques | 5 | `CanonicalStage` |
| 23 définitions canoniques | 23 | `CanonicalStage/Definitions` |
| **Total protégé** | **84** | liste exacte figée par l'agent |

La liste de 56 est dérivée du reçu signé 06.4A, pas d'une
recherche approximative. Les 28 canoniques sont exclusivement
les cinq fichiers de base connus et 23 définitions `DA_Canonical_*`
actuellement suivies par Git. Chaque chemin doit appartenir
au périmètre autorisé et exister en **vrai binaire**, jamais
simple pointeur Git LFS ou lien symbolique.

Avant la copie :
1. exiger un HEAD détaché et propre égal au SHA du jalon ;
2. exiger le rapport privé 06.4F `source_parity_verified=true`
   et `migration_authorized=false` ;
3. vérifier le manifeste et reçu privé 06.4A (SHA-256 connu),
   les hash et tailles des 56 anciennes ressources et de
   `DA_ItemSystem` ;
4. **relancer Unreal Editor 5.5.4** et vérifier les valeurs
   UPROPERTY, toutes les dépendances/référents AssetRegistry,
   les 56 packages et les 1640 profils contre 06.4A ;
5. contrôler les 28 canoniques supplémentaires et enregistrer
   leur état actuel, sans prétendre que leurs hash existaient en 06.4A.

## Sauvegarde et simulation de restauration

La copie de chacun des 84 fichiers est écrite sous
`<StateRoot>/Runs/<validation>/ARM-CLEAN06.4G/SourceSnapshot/Content/...`,
**en dehors de tout worktree Git**.

Un reçu privé contient les chemins exacts, taille, hash SHA-256,
SHA de validation et preuve de copie. Après la copie, chaque
binaire source est *rehashé*, ainsi que son double privé.

Le scénario de **retour arrière simulé** recopie ensuite les
84 fichiers depuis la photographie vers
`ARM-CLEAN06.4G/RestoreDrill`, vérifie une nouvelle fois leur
taille et leur hash, puis supprime **seulement** ce répertoire
temporaire privé créé pour le test.

Cette simulation **ne restaure jamais** les vrais fichiers
du projet. Une future restauration de production doit être
une opération séparée, fermée par une allowlist exacte,
sur le même repo/commit préconverti, avec vérification
des hash de destination et arrêt immédiat en cas de dérive.
Un ancien backup ne doit jamais écraser silencieusement les
nouveaux changements utilisateur.

Le script écrit aussi `transaction-gate-private.json` :
`migration_authorized=false`, les trois listes d'écriture
vides, et les preuves encore exigées (niveaux, identité,
SaveGame, jeu/PIE et Cook). Aucune option `-Apply` ou
`-RestoreToProject` n'existe dans ce jalon.

## Ce qui n'est pas protégé ni migré ici

- Les `Saved/SaveGames/*.sav` personnels : leur présence
  était seulement recensée par hash en 06.4A, **sans
  ouverture ni copie**. Une stratégie de sauvegarde séparée
  est obligatoire avant toute conversion de sauvegardes.
- Les WBP/presets et assets périphériques hors des 84
  binaires : à inventorier et ajouter explicitement avant
  toute opération pouvant y écrire.
- Les données de niveau sont **sauvegardées** mais non converties ;
  la logique de lancers/réceptacles et les identifiants persistants
  ne sont pas encore démontrés entièrement migrables.
- Les anciens lecteurs, champs et tools restent en place
  pendant cette préparation.

## Conditions de validation

Un succès de `UE5.5.4/ARM-CLEAN06.4G` signifie **uniquement**
que les 84 binaires ont été photographiés sans erreur et que
la restauration du backup vers un dossier privé s'est déroulée
sans perte. Exiger `SilentTask`, `LocalAuto` et `06.4G`
à SUCCESS **sur le SHA exact** et le ticket public
`[ARM-CLEAN06.4G]`. Le statut ne vaut jamais
autorisation de migrer.

**Étape suivante :** simulation des changements fonctionnels
des catégories restantes en Editor-only, qualification des
références avec niveaux/réceptacles, puis authentique
transaction sauvegarde-modification-contrôle-rollback sous
verrou explicite. 06.5/06.6 restent ouvertes.

Aucun WBP ni texte joueur changé :
`UI.csv → ST_UI → WBP → Game/fr/en` inchangé.

Le prévol distingue maintenant les phases de construction de la liste
(Git suivi, 56 historiques, 28 canoniques, vérification de périmètre),
sans publier un identifiant ni chemin de fichier. L'agent refuse
les listes incomplètes, même lorsque le C++ et Grimrock passent.
