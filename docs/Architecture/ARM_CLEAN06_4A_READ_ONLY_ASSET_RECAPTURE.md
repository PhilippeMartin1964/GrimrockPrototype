# ARM-CLEAN06.4A — Nouvelle preuve en lecture seule des contenus réels UE 5.5.4

**Parent :** 06.3BG `31c0e675016c622a7f65576cfa66365c836a6be0`, les deux checks Windows ayant réussi.

## But

06.4A est **une porte d'audit, pas une conversion**. Avant de resauvegarder un
seul `.uasset`, refaire une capture par **l'Unreal Editor réel** des assets
et référents de la baseline 06.0, sans nouveau modèle d'export,
sans migration ou écriture de fichiers du projet.

L'agent de validation exécute normalement la compilation et la suite
Automation `Grimrock`. Un commit dont le sujet exact est
`chore(ARM-CLEAN06.4A): recapture read-only UE assets and references` déclenche ensuite une seule capture additionnelle.

## Exécution prouvable

Le script historique `Scripts/CaptureARM_CLEAN06_Baseline.ps1` accepte
un `-AuditLabel ARM-CLEAN06.4A` (le défaut 06.0 reste inchangé).
Il vérifie avant et après que le worktree Git détaché est intact.
`UGridArmClean06BaselineCommandlet` charge les vraies propriétés Unreal
et demande à `AssetRegistry` dépendances et référents des **56 assets
de la baseline**. Le script contrôle :

- les 24 `DA_Weapon`, 24 autres `DA_Item`, 2 catalogues ARM,
  4 niveaux, 1 donjon et 1 palette ;
- les 82 archétypes × 20 niveaux, soit **1640 profils**, dont **260
  passifs** ; toutes les cardinalités restent des assertions, pas
  un résumé supposé ;
- l'existence matérielle des **56 `.uasset` réels** (ni pointeurs LFS
  ni fichiers absents), leurs SHA-256 et tailles ;
- l'existence du **vrai `DA_ItemSystem.uasset` canonique** et son
  SHA-256, inventorié séparément des 56 anciennes ressources ;
- les anciennes sauvegardes `.sav` disponibles dans
  `Saved/SaveGames` du checkout local **uniquement** par nom, taille
  et empreinte dans un reçu privé, sans ouvrir ni publier leur contenu ;
- absence de fichier suivi/non suivi créé dans le worktree après l'audit.

Le statut additionnel `UE5.5.4/ARM-CLEAN06.4A` est publié sur
**le SHA exact** avec `pending`, puis `success` seulement si le
script renvoie 0 et un ticket `[ARM-CLEAN06.4A]` est publié.
Un échec conserve un code de phase allowlisté, sans divulguer les
journaux ou chemins locaux.

## Emplacement et sécurité

Le manifeste `asset-baseline.json` (UPROPERTY, profils et graphe
AssetRegistry), le reçu `asset-hashes.json` et les hashes éventuels
des `.sav` restent **privés** sur le validateur Windows, dans
`Runs/<SHA>-<date>/ARM-CLEAN06.4A`. L'issue publique ne révèle
que les nombres contrôlés et le SHA-256 du manifeste, plus la preuve
de présence du canonique. Aucun contenu métier, texte, asset ou
sauvegarde n'est retransmis.

**Aucune action** `SavePackage`, `EditorAssetLibrary::SaveAsset`,
écriture de `.uasset`/`.umap`, ni migration SaveGame n'est effectuée.
Le commandlet reste Editor-only et read-only.

## Critères de sortie

1. `UE5.5.4/SilentTask=success` sur le SHA publié ;
2. `UE5.5.4/LocalAuto=success` sur le même SHA ;
3. `UE5.5.4/ARM-CLEAN06.4A=success` sur le même SHA ;
4. issue publique `[ARM-CLEAN06.4A]` et reçu privé disponibles ;
5. aucun changement suivi/non suivi dans le worktree d'audit.

Si le troisième contexte est absent/pending/failure, 06.4A **n'est
pas validé**, même si le build est vert. Des variations de comptage
ou un asset canonique manquant **bloqueront la conversion**.

## Ce qui reste en 06.4B et après

- Le rapport 06.4A mesure le **présent** ; il ne compare pas encore
  automatiquement chaque propriété au manifesté privé de 06.0.
  Un diff contrôlé et la parité exacte sont obligatoires avant toute
  transformation binaire.
- Le graphe AssetRegistry de la baseline couvre ses 56 packages et
  leurs référents, **pas un audit complet des références soft dans
  tous les WBP, presets et fichiers SaveGame**.
- La migration Editor-only avec dry-run, backup et contrôle
  des valeurs (projectiles génériques, flèches, objets initiaux,
  armes, kits) reste à implémenter ; ne pas la déduire d'un audit vert.
- `06.5` suppression des lecteurs/UPROPERTY après migration
  prouvée ; `06.6` PIE, Save/Continue et Cook/Shipping.

**Localisation** `UI.csv → ST_UI.uasset → WBP → Game/fr/en`
inchangée : aucune interface joueur modifiée.
