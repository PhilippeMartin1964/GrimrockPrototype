# ARM-CLEAN06.4X.57 — Transaction native protégée de la palette unique

**Parent :** `bd02f2b0dbb5978e216c5edbe485fc31c4ad0104`. Nécessite SilentTask et LocalAuto succès sur le parent, puis sur le propre SHA X.57 avant toute opération native.

## Périmètre

Un seul fichier est autorisé : `Content/GrimrockPrototype/Core/DataAssets/DA_ObjectPalette_Default.uasset`. Les 33 `DefaultItemDefinition` disparaissent ; aucun autre champ, ordre, catégorie, EntryId, CanonicalItemId, niveau, icône de mécanisme ou définition de monde/monstre/compagnon ne change. Conserver 33 entrées directes. Zéro écriture de niveau, SaveGame ou autre asset. X.55 vérifie la projection transitoire et la parité de présentation; X.56 garantit les audits avant/après.

`GridArmClean064X57NativePalette` est strictement Editor-only : `-Preflight`, `-Verify` (0 si déjà pointerless, 4 si 33 pointeurs historiques), `-Apply` avec SHA40 opérateur et sauvegarde privée byte-equal. Le commandlet compare tous les champs réfléchis au plan X.55 avant sauvegarde du seul package, puis vérification par un nouveau processus UE. Rapport JSON : manifeste ordonné de toutes les entrées et des champs persistés hors ancien pointeur. Refus de toute conversion partielle. Ne jamais appeler Apply directement hors runner.

## Runner

`Scripts/ApplyARM_CLEAN06_4X57_NativePalette.ps1` : contrôle du worktree propre/master/SHA, fast-forward uniquement, LFS et signature native, compilation UE, préflight et Verify, backup SHA256 hors dépôt, Apply, Verify rechargé et comparaison du manifeste, suite Grimrock complète, hashes avant/après, vérification du pointeur LFS staged et commit strictement mono-asset. Sur erreur avant commit, retire uniquement le fichier autorisé de l'index, rétablit les octets du backup et exige le worktree propre; n'écrase jamais un commit ou un autre fichier utilisateur.

Par défaut lecture seule. Une fois le SHA du **code** validé et UE fermé, le runner peut exécuter préflight + Apply + Publish dans la même transaction (les deux modes de lecture restent obligatoires automatiquement). Si l'utilisateur préfère, exécuter d'abord le runner sans switches.

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ApplyARM_CLEAN06_4X57_NativePalette.ps1 `
  -RepoRoot D:\Development\GrimrockPrototype `
  -EngineRoot D:\UE_5.5 `
  -StateRoot D:\GrimrockNativeSnapshots `
  -ExpectedHead <SHA40_DU_CODE_X57> `
  -Apply -Publish
```

La suite complète doit rester verte, le commit binaire doit contenir uniquement la palette et obtenir les deux statuts Windows sur son SHA. PIE/éditeur : sélectionner et placer une arme N1 et un non-ARM N0, vérifier nom/icône/recherche, modifier le niveau d'une instance, ouvrir Details de la palette (ancien pointeur en lecture seule, Icon uniquement sur objets monde), tester save/continue. Le total X.48 attendu après conversion réelle est 26 (18 contenus initiaux, 3 overrides dormants, 5 loots). Le total demeure 59 avant exécution locale. Aucun SaveGame v27 ni WBP modifié.

**06.4 partielle ; 06.5 bloquée.** Le présent lot est uniquement du code, tests déjà présents X.55/X.56 et documentation ; ne pas prétendre avoir exécuté la transaction sans le log opérateur.
