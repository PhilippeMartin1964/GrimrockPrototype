# ARM-CLEAN06.4X.47 — Tests avant/après conversion du vrai niveau et diagnostic de restauration

**Parent :** `939ef5dc1846aec896cb2f04261a6e20b3444a30` (X.46, contrôles Windows verts).
**Périmètre :** tests runtime existants, message de restauration du script X.46
et documentation. Aucun `.uasset`, `.umap` ni SaveGame écrit par ce lot.

## Preuve du premier essai natif sur Windows

Le 17 novembre 2026, l'utilisateur a lancé
`ApplyARM_CLEAN06_4X46_NativeLoosePointers.ps1 -Apply -Publish` :

- compilation locale UE5.5.4 : succès
- `-Preflight` puis `-Verify` : 44 anciennes références détectées
  (le code 4 de `-Verify` signifie état encore legacy et non échec)
- `-Apply` : le vrai `DA_GridLevel_00.uasset` a été réenregistré ;
  44 pointeurs `FGridLooseItemInstance::ItemDefinition` retirés
- nouveau `-Verify` dans un autre processus : code 0
- suite `Grimrock` : **1961 réussis, 25 avec avertissements,
  1 échec, 0 non exécuté**, code 255
- unique échec :
  `Grimrock.RPG.ARM.CLEAN06_4X8.RealLooseItemCanonicalNonArmCutover`,
  dans `GridARMCLEAN064X8RealLooseItemCutoverTests.cpp`, ligne 212
  du code X.46 : l'assertion exigeait que le premier ARM ait toujours
  un pointeur `DA_Weapon_*` sérialisé dans le niveau
- le script a déclenché son `catch` de restauration avant tout
  commit binaire et tout push. Le journal ne montre pas le contrôle
  SHA final, même si la version X.46 du script vérifie le SHA
  avant de réémettre l'erreur.

## Correction de fond (sans fallback legacy)

Le test sur le vrai niveau accepte maintenant les deux états
**autorisés** de la même ligne : avant migration avec ancien pointeur
`ItemDefinition` strictement cohérent (ItemId + ArchetypeId),
ou après migration pointerless. Dans les deux cas il exige que le
runtime crée le même ARM au `CanonicalItemId`, au `ItemLevel`
physique et avec le mesh, la lumière et Niagara provenant du seul
catalogue canonique.

Deuxième dépendance cachée anticipée : l'ancien test négatif
`WrongSource` copiait un non-ARM réel avec son pointeur pour
fabriquer une contradiction. Une fois les 44 lignes pointerless,
cette copie devenait, par accident, un ARM valide sans source ;
le test ne devait pas dépendre de l'état du fichier natif.
Il crée maintenant explicitement un
`UGridItemDefinitionAsset` transitoire au mauvais ItemId,
puis exige son rejet. Le test de source legacy cohérente au même
ItemId avec effets contradictoires est conservé.

La correction concerne **uniquement la fixture** :
aucun changement de `AddPlacedItemActor`, du registre,
des champs physiques, de l'éditeur de niveaux ni de SaveGame.

## Journal amélioré en cas d'échec futur

Le script X.46 écrit désormais explicitement `ROLLBACK VERIFIED`
**après** copie du seul package autorisé, comparaison SHA-256
avec l'original et contrôle `git status` vide. S'il y a divergence,
il refuse de déclarer la restauration réussie. Le mécanisme
de sauvegarde privé et l'interdiction de toucher d'autres fichiers
restent inchangés.

## Suite

Après **deux statuts Windows verts sur le SHA de X.47**,
l'utilisateur devra relancer le même script de migration native
`-Apply -Publish` en utilisant son **nouvel ExpectedHead**.
Le script met à jour un checkout propre exclusivement par fast-forward,
recrée une sauvegarde privée, exécute les tests `Grimrock` complets
sur le binaire converti, vérifie sa réouverture et pousse un seul
package seulement si tous les tests passent.

Ne pas réutiliser l'ancien SHA X.46 dans `-ExpectedHead`.
Aucune conversion binaire n'est effectuée par ce commit de code.
Le PIT02, les armes au sol, l'éclairage et le Save/Continue doivent
être revalidés après la migration binaire effective. Les 18 contenus
de réceptacles, 33 entrées de palette, autres niveaux et SaveGames v27
sont hors du périmètre.

**Statut : 06.4 PARTIELLE ; 06.5 BLOQUÉE.**
