# ARM-CLEAN06.4D — Dry-run Unreal Editor sur les données réelles

**Source :** 06.4C SHA `5da0a702310fa05505d0eb8571d788f9f22d219e`,
validé par les trois statuts Windows et l'issue n°136.
Conclusions **provisoires** : quatre packages anciens du domaine
armes/catalogues, huit UPROPERTY classées « gameplay », quatre
dépendances et quatre binaires modifiés par rapport à 06.0,
zéro changement de profil sur 1 640.

## Ce qui est réellement exécuté

Après la compilation et la suite Automation `Grimrock`, le
validateur Windows déclenche uniquement au sujet exact
`chore(ARM-CLEAN06.4D): verify live UE mapping candidates without binary writes`.

Le script `Scripts/PlanARM_CLEAN06_4D_EditorDryRun.ps1` :
1. vérifie le commit détaché `HEAD` et le worktree propre ;
2. retrouve **une seule** qualification et un seul verrou privé
   06.4C sur le SHA prévu ; rejette les incohérences de provenance,
   les compteurs différents ou toute autorisation d'écriture ;
3. retrouve **une seule** baseline 06.4A dont le manifeste a le
   SHA-256 de l'issue 134 ;
4. exécute `UnrealEditor-Cmd.exe` avec
   `-run=GridArmClean06Baseline -NullRHI`, sans `SavePackage` ;
5. compare **les 56 assets réels** : package, classe, objet,
   propriétés UE exportées, tableaux triés de dépendances et
   référents, tailles et SHA-256 des fichiers binaires ;
6. compare **tous les 1 640 profils** au contenu de 06.4A ;
7. vérifie, pour les quatre packages divergents, que les
   **huit valeurs courantes**, les **quatre graphes** et les
   **quatre empreintes** correspondent exactement au rapport
   privé 06.4C ;
8. propose une **destination candidate** de chaque champ selon
   le contrat normatif actuel, sans interpréter ou réécrire
   ses valeurs `ExportText`. Les destinations non déterminées
   restent `UNMAPPED_REVIEW`.
9. produit `dry-run-private.json` et un rapport GitHub
   strictement agrégé ; vérifie enfin un worktree toujours propre.

## Correspondances suggérées, non autorisées

| Ancien champ connu | Destination potentielle unique |
|---|---|
| `WeaponCatalogue`, `bUseWeaponCatalogue` | `DA_ItemSystem.WeaponCatalogue`, puis retrait de la copie |
| `WeaponVisualCatalogue`, `bUseWeaponVisualCatalogue` | `DA_ItemSystem.WeaponVisualCatalogue`, puis retrait de la copie |
| `CombatActions`, `QuickItemCombatAction` | `DA_ActionCatalogue` et vraies liaisons `ItemActionBinding` |
| `PlayerAttackPresentationProfile`, audio, Niagara | `DA_ActionPresentationCatalogue` |
| `ThrowSpeed`, `ThrowArc`, balistique | `DA_WeaponCatalogue.ProjectilePolicies` |
| PA, dés, portée, armure et parades | `DA_WeaponCatalogue` |
| Icône, mesh, visuel | `DA_WeaponVisualCatalogue` |
| Nom, poids, identité persistante | Définition canonique ou instance physique selon le champ |

Ce tableau décrit **le contrat de destination**. Il ne prétend
pas que les huit champs observés sont tous ceux du tableau.
Les champs exacts et leurs anciennes/nouvelles valeurs restent
privés. Chaque candidat possède le statut
`UNAPPROVED_DRY_RUN_PROPOSAL` et exige une preuve de
parité avec la vraie destination avant migration.

## Interdictions et conditions d'arrêt

Aucun `.uasset`, `.umap`, `.sav`, WBP ou `DA_ItemSystem`
n'est modifié. Les rapports demeurent dans le répertoire
privé `Runs` de l'agent Windows. Les IDs de packages,
champs et valeurs divergents ne sont pas publiés dans l'issue.

Un succès `UE5.5.4/ARM-CLEAN06.4D` signifie seulement :
**les sources UE réelles n'ont pas divergé de 06.4A et les
candidats ont été produits**. Il ne prouve ni parité
legacy/canonique ni autorisation de conversion. Les listes
`allowed_binary_writes`, `allowed_package_deletions` et
`allowed_savegame_writes` restent vides.

Un asset différent, une capture privée incohérente, un niveau
de profil altéré ou un rapport absent **bloque** le jalon.

## Suite 06.4E — condition impérative

Comparer **valeur par valeur** les candidates approuvables à
`DA_ItemSystem` et aux catalogues réels : action + présentation,
règle de mains, PA, armure, projectile physique/visuel, métadonnées,
identité/références d'inventaire, niveaux et SaveGame.
Exiger un simulateur **dry-run sans écriture** qui calcule ce que
ferait un convertisseur Editor-only. Ensuite, et ensuite seulement,
préparer un convertisseur explicitement opt-in, avec sauvegarde
des fichiers avant `SavePackage`, allowlist et rollback.

UI non concernée : `UI.csv → ST_UI → WBP → Game/fr/en`
inchangé. PIE/Chaos, collisions, sauvegardes, Cook et Shipping
restent des validations distinctes.
