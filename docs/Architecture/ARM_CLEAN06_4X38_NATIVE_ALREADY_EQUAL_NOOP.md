# ARM-CLEAN06.4X.38 — Parité native réelle, migration sans écriture

**Date :** 17 novembre 2026. **Parent :** `5d83c683e7ea6c867187b6dacd9ace9abe96d236`.

## Preuve constatée sur le poste Windows utilisateur

Le deuxième essai du script `ApplyARM_CLEAN06_4X37_ArmWorldProfiles.ps1 -Apply`
a réussi à compiler `GrimrockPrototypeEditor` localement
(452 actions UBT, 0 erreur de compilation). Le processus UE de
`-Preflight` a chargé les 22 sources ARM et les deux catalogues
`.uasset` réels, puis affiché textuellement :

```text
[ARM-CLEAN06.4X.37] Read-only preflight: sources=22 mismatching_profiles=0; allowed binaries=2; no packages saved.
Success - 0 error(s), 0 warning(s)
```

L'ancien script, ignorant ce résultat de parité, a quand même généré
une copie privée de 24 fichiers et demandé `-Apply`. Le commandlet
a refusé le manifeste, code 5 (`verified private 24-file backup
receipt required`), **avant SavePackage**, et le PowerShell a annoncé
la restauration des deux catalogues. Ne PAS interpréter cette erreur
comme un besoin de forcer la réécriture : les 220 champs vérifiés
étaient déjà identiques sur disque.

## Comportement corrigé

Après le `-Preflight`, `-Apply` démarre maintenant un **second
processus indépendant UE `-Verify` en lecture seule** :

- code 0 : 22 profils déjà identiques, aucune sauvegarde,
  aucun `SavePackage`, aucun changement LFS ; terminaison réussie
  avec le message `VERIFIED NO-OP` ;
- code 4 : divergences de profil réellement détectées, continuer vers
  le chemin protégé avec 24 sauvegardes et la validation des fichiers
  (refus de toute écriture sans manifeste correct) ;
- tout autre code : échec sans écriture.

Un `-Apply` commandlet directement invoqué est lui aussi idempotent
si `Plan.ChangedProfiles==0`, avant la validation des backups,
puisqu'aucune opération de sauvegarde UE ne peut être atteinte.

Le test Editor réel
`Grimrock.RPG.ARM.CLEAN06_4X37.NativeArmWorldProfileStaging`
exige désormais **zéro profil modifié** et contrôle explicitement
`VerifyDiskProfiles` sur les `.uasset` réels. Ce test passe dans la
suite automatique Windows `Grimrock` si les deux catalogues
effectivement versionnés sont déjà en parité. C'est une preuve
de contenu UE, pas une comparaison d'octets ni un test de rendu.

## Périmètre non autorisé

Le statut « no-op » couvre seulement **22 profils × (8 visuels +
2 physiques)** des deux catalogues existants. Il ne convertit pas
les **107 pointeurs historiques des neuf packages**, les 95
occurrences de niveaux/palette, les **44 références des anciens
catalogues**, les lecteurs/éditeurs legacy C++, les quatre identités
loot non enregistrées, le WBP ancien ni les SaveGames v27.
Il n'autorise ni suppression de classes/UPROPERTY, ni
`ARM-CLEAN06.5`. Aucun `.uasset`, `.umap` ou SaveGame modifié
dans ce commit, et aucun commit LFS « vide » à créer.

**Résultat attendu du script sur le nouveau SHA :** affichage
`VERIFIED NO-OP` sans erreur, sans nouvel instantané privé.
**ARM-CLEAN06.4 : PARTIELLE ; ARM-CLEAN06.5 : BLOQUÉE.**
