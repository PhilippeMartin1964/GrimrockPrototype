# ARM-CLEAN06.4X.58 — Cutover sûr des 18 contenus initiaux des réceptacles

**Parent exact :** `8ae270b3e3f2f90ae46e7b88a80676427e7073d6`, commit binaire X.57B, 1 991 tests réussis (30 avec avertissements), SilentTask + LocalAuto SUCCESS, recette éditeur/PIE 8/8 confirmée. X.48 : 26 pointeurs résiduels.

## Problèmes et correction

Le runtime X.9.1/X.9.2/X.21/X.22 résout déjà les contenus par CanonicalItemId et niveau physique, mais Selected Object utilisait encore un picker d'ancienne définition et désactivait le niveau en l'absence de pointeur. X.58 remplace ce picker dans Initial Content par le champ ItemId enregistré, comme pour les placements libres ; l'édition de niveau se fonde sur le registre. Un nouvel ID conserve son niveau valide ou commence explicitement à N1 ARM/N0 non-ARM ; aucune correction implicite d'un niveau natif déjà persisté. Recommitter l'identité inchangée ne migre pas silencieusement la ligne. Les modifications réellement validées passent par la frontière de configuration existante ; une identité refusée ne marque pas la carte dirty. ItemDefinition reste sérialisé et en lecture seule dans Details, CanonicalItemId éditable. Les signatures et champs Blueprint restent compatibles. Les diagnostics de validation distinguent les entrées canoniques et le legacy résiduel.

Les audits 4W/X.2/X.4/X.48 et les tests X.91/X.92 acceptent les deux états complets. X.48 impose désormais palette pointerless, exactement 18 lignes d'origine et 18/0 pointeurs, avec le total 26/8. X.91 projette chaque vrai contenu sans pointeur et compare quantité/niveau/identité, X.92 vérifie aussi le véritable acteur non-ARM sans ancien asset ; X.21/X.22 couvrent ARM N1/N20 et la restauration. X.163 teste sélection, N0/N20, ID inexistant, contradiction, conservation de quantité, édition sans pointeur. Pas de réactivation des trois overrides dormants.

## Périmètre binaire, test et transaction protégée

L'audit X.19 établit :
- `DA_GridLevel_00.uasset` : 17 contenus initiaux ;
- `DA_GridLevel_01.uasset` : un contenu initial ;
- `DA_GridLevel_New_Level_03` et `DA_GridLevel_Pillars_of_Light` : zéro, lecture seule et vérification SHA256.

Les deux opérations d'écriture sont indivisibles pour cette transaction. `FGridEditorNativeInitialContentPlan` construit uniquement des copies valeur, valide noms de packages, GUID/cellules d'objets propriétaires, IDs, niveaux, quantités et correspondance d'archétype, refuse état partiel. `Grimrock.RPG.ARM.CLEAN06_4X58.NativeInitialContentPlan` contrôle la réalité native, tous les champs réfléchis des objets, idempotence, identité inconnue et état mixte. Le commandlet `GridArmClean064X58NativeInitialContent` (Editor-only) ne fait qu'effacer ItemDefinition dans ReceptacleInitialContent, après reçus byte-equal hors projet ; aucun autre champ ni package ne doit changer. Le manifeste ordonné des propriétés persistées des quatre niveaux est exporté après normalisation des seuls pointeurs visés, comparé avant/après en mémoire puis après réouverture indépendante. Compare le contenu authoré, pas le rendu GPU/Chaos.

Le runner `Scripts/ApplyARM_CLEAN06_4X58_NativeInitialContent.ps1` vérifie master, worktree, les deux statuts Windows sur le SHA exact avant Apply, LFS, en-têtes UE, hash des quatre niveaux, puis build, préflight et Verify. Sauvegarde privée des deux cibles, Apply, Verify réouvert, manifeste, suite Grimrock entière (0 failed, 0 not run), LFS staged conforme à SHA256, commit binaire contenant uniquement les deux cibles. Restauration des deux backups en cas d'échec avant commit lorsque HEAD et périmètre le permettent ; aucun reset d'un commit existant.

Exécution opérateur uniquement, UE fermé, après succès des validations du SHA code X.58 :

~~~powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ApplyARM_CLEAN06_4X58_NativeInitialContent.ps1 `
  -RepoRoot D:\Development\GrimrockPrototype `
  -EngineRoot D:\UE_5.5 `
  -StateRoot D:\GrimrockNativeSnapshots `
  -ExpectedHead <SHA40_DU_CODE_X58> `
  -Apply -Publish
~~~

Ne pas relancer les anciens commandlets de matérialisation. Avant intervention opérateur, il reste 26 pointeurs ; seulement après transaction réelle, 8 (3 overrides inactifs et 5 loots, dont quatre IDs hors registre). Recette après le SHA binaire et deux statuts : Selected Object affiche les IDs existants, édition sur un réceptacle de test avec ID valide/invalide, ARM niveaux physiques et non-ARM N0, contenu initial visible et ramassable dans les deux niveaux, quantités et save/continue. Aucun niveau de production sauvegardé uniquement pour la recette d'édition.

**UI** : workflow de localisation relu ; modification de l'inspecteur Slate d'édition seulement (tooltip existant repris). Aucun texte joueur/WBP ni modification UI.csv, ST_UI ou Game/fr/en. Aucun SaveGame v27 modifié, aucun changement de comportements/visuels authorés. Aucune UPROPERTY supprimée. **ARM-CLEAN06.4 partielle ; 06.5 bloquée ; 06.6 non achevée.**
