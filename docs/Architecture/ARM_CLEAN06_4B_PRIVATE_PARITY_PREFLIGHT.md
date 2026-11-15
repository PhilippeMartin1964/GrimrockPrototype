# ARM-CLEAN06.4B — Comparaison privée des deux captures Unreal

## Sources contrôlées

- 06.0 : SHA c66bf68aa1a42eb8d9c47eb9b9336214d0bccf95 ; issue GitHub 84.
- 06.4A : SHA 27563be7bee162e86b5f5f9ac29704c1b50aa35c ; issue GitHub 134.
- Les manifestes et reçus sont réellement conservés dans le dossier
  privé de l'agent Windows. Leur contenu n'est pas dans le dépôt.

## Algorithme

Scripts/CompareARM_CLEAN06_4B_Baselines.ps1 exige les deux captures
et leurs SHA sources exacts. Il vérifie le SHA-256 de chaque manifeste,
les cardinalités (56 assets, 1640 profils et 260 profils passifs),
les 56 empreintes de vrais fichiers binaires et l'empreinte du vrai
DA_ItemSystem présente dans le reçu 06.4A.

Le rapport comparaison-private.json contient exclusivement sur la
machine Windows les écarts de noms de packages, classes, types,
UPROPERTY sérialisées, graphes de dépendances et référents
AssetRegistry, SHA-256 des assets et champs de chaque profil ARM
de N1 à N20. Les profils sont identifiés par leur ordre
ARM001..ARM082 et 20 niveaux, celui de la baseline d'origine.
Les données brutes et leurs valeurs avant/après ne sont jamais
publies en issue GitHub ; le ticket [ARM-CLEAN06.4B] contient
uniquement des compteurs de différences.

## Déclenchement et protection

Le seul sujet déclencheur est :

    chore(ARM-CLEAN06.4B): compare UE baselines in guarded dry-run

Après réussite complète de UE5.5.4/LocalAuto, l'agent invoque
le script dans son worktree détaché, puis publie un troisième
statut UE5.5.4/ARM-CLEAN06.4B. Une capture absente,
ambiguë, altérée ou au mauvais SHA entraîne un échec explicite.
Le script vérifie le git status avant et après. Il écrit uniquement
un rapport et un corps d'issue dans le répertoire privé Runs.
Il n'exécute aucun SavePackage, ne réécrit aucun uasset,
umap ou SaveGame, et ne crée aucun commit.

Un résultat positif du statut signifie : les deux captures ont été
comparées. Il n'autorise PAS la migration de binaires, même si
tous les compteurs sont nuls. Les divergences devront être
classifiées avant le premier changement matériel 06.4C.

## Suite

1. Exploiter les chiffres réels des écarts et vérifier les valeurs
   privées sur le poste Windows sans les envoyer dans GitHub.
2. Mettre au point le convertisseur UE Editor-only en dry-run
   avec allowlist, backup, checks de parité et d'identité.
3. Ne migrer les fichiers que lorsque ces contrôles sont prouvés.
4. Garder 06.5 pour le retrait legacy et 06.6 pour PIE,
   Save/Continue et Cook/Shipping.

Localisation UI.csv → ST_UI.uasset → WBP → Game/fr/en inchangée.

En cas d'échec, le statut utilise une phase allowlistée pour distinguer
la préparation des index, la comparaison des packages et celle des
profils. Aucune valeur ou identité d'asset privé n'est publiée.

Le diagnostic est partitionné jusqu'aux sous-phases de chaque asset :
classification, UPROPERTY, graphes de références et empreintes.
Ce diagnostic ne publie jamais le nom de l'asset en cours.

La routine de comparaison des UPROPERTY matérialise explicitement
chaque collection de propriétés PSCustomObject en tables nom/valeur.
Une propriété absente est distinguée d'une propriété dont la valeur
égale textuellement le marqueur de diagnostic.
