# ARM-CLEAN06.4X.46 — Conversion native protégée des 44 pointeurs d'objets libres

**Parent :** `5e79797e8234ccf392642350a84d7d30aff10818`, X.45 validé par SilentTask/LocalAuto.
**Contenu de ce lot : code et documentation seulement.** L'asset
`DA_GridLevel_00.uasset` n'est pas modifié par le push de X.46.

## Package natif ciblé

Le seul package que ce migrateur peut écrire est :

`Content/GrimrockPrototype/Core/DataAssets/GrimrockLevels/DA_GridLevel_00.uasset`

Il contient 44 `FGridLooseItemInstance` ayant déjà
`CanonicalItemId` et `ItemLevel` physiquement définis.
**Seul le champ `ItemDefinition` de ces 44 entrées sera mis à
`nullptr`.** Ni les 17 contenus de réceptacles du même niveau,
ni les autres placements/triggers/liens/overrides, ni les trois autres
niveaux, ni `DA_ObjectPalette_Default`, ni les définitions ARM,
ni la moindre sauvegarde SaveGame ne sont éligibles.

## Commandlet `GridArmClean064X46NativeLoosePointers`

- `-Preflight` : lecture seule de l'asset réel et du registre
  `DA_ItemSystem`, 44 GUID uniques, 0 spawn, cellules/quantités
  valides, famille/niveau physique et anciens ItemId/ArchetypeId
  identiques ; exige **44 pointeurs présents ou zéro**, jamais
  de conversion partielle
- `-Verify` : même lecture après réouverture d'Unreal ; retourne
  **0** quand les 44 pointeurs sont absents, **4** pour un état
  intégralement historique et les autres codes pour les erreurs
- `-Apply` : si zéro pointeur, sortie no-op sans backup/écriture ;
  si 44, exige un SHA40, une sauvegarde binaire **hors dépôt**
  identique octet par octet à l'original, un package initialement
  non dirty et les mêmes conditions strictes d'identité
- applique exclusivement les 44 assignations `ItemDefinition=nullptr`,
  compare un snapshot de tous les champs de chaque objet libre (GUID,
  cellule, offset, état de lecture, tags, notes, identité/niveau et
  quantité) puis exécute **un seul `UPackage::SavePackage`**
- produit un rapport JSON privé avec les comptes, l'état d'écriture
  et une cible littérale unique. Aucun écrit de SaveGame.

## Script Windows à lancement explicite

`Scripts/ApplyARM_CLEAN06_4X46_NativeLoosePointers.ps1` :

- vérifie les chemins, le master exact, l'absence de modifications
  utilisateur et d'éditeur UE ouvert ; si le checkout est propre et
  en retard, n'autorise qu'un `git merge --ff-only` vers
  l'`origin/master` portant **exactement** le `-ExpectedHead`
- reconstruit le module `GrimrockPrototypeEditor` **dans le vrai
  checkout local** avant d'utiliser le nouveau commandlet
- effectue `-Preflight` puis `-Verify` avec deux processus UE
  indépendants ; refus ou no-op avant tout snapshot si l'état ne
  correspond pas aux 44 pointeurs à enlever
- exige que le fichier soit un vrai binaire Git LFS Unreal, puis
  sauvegarde **cet unique fichier** dans le dossier privé
  `StateRoot`, le compare par SHA-256 avant et après copie
- lance `-Apply`, vérifie son rapport et réouvre **dans un
  troisième processus Unreal** avec `-Verify`
- exige que Git constate exactement **un seul `.uasset` modifié**,
  relance la **suite complète `Grimrock`** sur ces binaires,
  recontrôle le SHA exact distant et les autres chemins
- sans `-Publish` : ne crée aucun commit et laisse le package
  local vérifié en attente ; avec `-Apply -Publish` : crée
  **un seul commit binaire contenant exactement ce package** puis
  push en avance rapide vers `master`, sans force-push ni branches
- si l'application échoue **avant le commit**, restaure le
  package autorisé depuis la sauvegarde privée et compare son SHA-256 ;
  ne touche jamais à un fichier utilisateur ou inattendu.

Le script n'est **pas relié** à `ValidateUE.ps1`, aux tâches
Windows ou à un événement automatique : la transaction binaire
n'est jamais lancée sans action explicite du propriétaire.

## Commandes sur le poste UE5.5.4 (après validation du commit de code)

Fermer Unreal Editor, garantir les changements personnels enregistrés
et lancer un premier préflight :

```powershell
.\Scripts\ApplyARM_CLEAN06_4X46_NativeLoosePointers.ps1 `
  -RepoRoot D:\Development\GrimrockPrototype `
  -EngineRoot D:\UE_5.5 `
  -StateRoot D:\Development\GrimrockPrototype_Audits `
  -ExpectedHead <SHA40_DE_X46>
```

Si ce préflight réussit, le passage protégé est le **même appel**
avec `-Apply -Publish`. Il ne faut **pas** utiliser les anciens
scripts X.72/X.83 liés à des commits historiques.

Après publication du binaire, attendre **les deux statuts Windows
sur son propre SHA**, puis exécuter une recette PIE post-conversion :
44 objets libres avec visuels, ramassage et transfert PIT02,
Save/Continue ; l'ancien contrôle PIE utilisateur ne couvre
pas le binaire nouvellement réenregistré.

**ÉTAT : code de migration préparé, aucun asset migré par ce lot.**
**ARM-CLEAN06.4 PARTIELLE ; 06.5 BLOQUÉE.**
