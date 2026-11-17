# ARM-CLEAN06.4X.37.2 — Compilation locale obligatoire du commandlet X.37

**Date :** 17 novembre 2026. **Parent :** `eefec795d2252dca8371892535063374582135a0`.

## Incident réel

Lors de l'essai sur `D:\Development\GrimrockPrototype`, le
commandlet `-run=GridArmClean064X37NativeArmWorldProfiles` a été
refusé : `Failed to find commandlet class
GridArmClean064X37NativeArmWorldProfilesCommandlet`.

La classe C++ est déclarée et la compilation/suite Windows était
verte, mais `VALIDATION-AUTO01` travaille sur le **worktree séparé**
`D:\Development\GrimrockPrototype_UEValidation`, et ne compile pas
les DLL du checkout utilisateur. La DLL Editor de ce dernier pouvait
donc être plus ancienne que les nouveaux fichiers X.37.

Le preflight a échoué **avant le snapshot, les sauvegardes et
SavePackage** : aucune écriture native n'a été exécutée.

## Correction fail-closed

Le script natif `ApplyARM_CLEAN06_4X37_ArmWorldProfiles.ps1`
vérifie désormais le `Build.bat` de l'installation Unreal puis
exécute **obligatoirement** :

`Build.bat GrimrockPrototypeEditor Win64 Development
-Project=<GrimrockPrototype.uproject> -WaitMutex -NoHotReloadFromIDE`

L'opération intervient après les contrôles de branche
`master`, HEAD exact, suivi de `origin/master`, absence de
modifications locales et **avant** `-Preflight`. Après le build,
la propreté Git est revérifiée. Si UBT échoue, l'opération s'arrête
sans backup ni migration. Aucun recours à Hot Reload et aucune
modification automatique des fichiers UE `.uasset`.

Le même ordre s'applique à `-Apply` et à la lecture seule :
l'utilisateur n'a pas à lancer Visual Studio ni un build séparé.
Le **SHA publié de X.37.2** doit remplacer le SHA X.37.1 dans
`-ExpectedHead` après synchronisation du checkout local.

## Validation

L'agent Windows valide les scripts versionnés avec le jalon,
le C++ inchangé et la suite `Grimrock` sur le SHA final ; il
ne prouve pas que le poste utilisateur possède une DLL Editor
à jour tant que ce script n'est pas exécuté localement.

Après le nouveau `-Apply`, confirmer la vérification native et
une comparaison `git status` limitée aux deux catalogues LFS
autorisés. Ne pas publier X.38 ni toucher aux niveaux/SaveGames
avant preuve de parité et inventaire des fichiers modifiés.

**ARM-CLEAN06.4 PARTIELLE, ARM-CLEAN06.5 BLOQUÉE.**
