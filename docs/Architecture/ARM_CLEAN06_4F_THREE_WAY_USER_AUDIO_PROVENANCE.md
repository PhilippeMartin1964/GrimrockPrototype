# ARM-CLEAN06.4F — Provenance des huit écarts de présentation Sword

**Source immédiate :** ARM-CLEAN06.4E, SHA
`e011d62ff9d2f446c427fba9798ac4af80ebcfeb` ;
issue 138, trois statuts SUCCESS. Résultat réel :
**4 liaisons d'attaque historiques et canoniques identiques,
0 différence, 0 liaison inconnue** dans les quatre assets changés.

## Pourquoi une vérification supplémentaire ?

La parité 06.4E couvre les **valeurs actuellement authorées**,
mais la baseline d'origine ARM-CLEAN06.0 précède les nouveaux
bruitages Sword ajoutés par le propriétaire au commit
`a60031db0d05aa444a7453f061e7463bd4e0b4d5`.
Le rapport 06.4B a identifié 8 différences de propriétés depuis
06.0. Le rapport 06.2A (issue 95) identifiait déjà exactement
**quatre définitions d'armes modifiées** au moment de cet ajout.

Avant tout cutover, il faut distinguer des effets audio utilisateur
volontairement conservés d'une dérive de valeur provoquée après
l'audit 06.2A.

## Mécanisme vérifiable

Le script `Scripts/CompareARM_CLEAN06_4F_ThreeWayProvenance.ps1`
ne fonctionne que sur l'agent Windows dans son worktree de validation.
Il exige les preuves privées :

- 06.0 : `asset-baseline.json` et reçu SHA-256 publiés issue 84 ;
- 06.2A : `post-audio-baseline.json`, reçu
  `source-parity.json`, commit source exact `2c0a4768...`
  et présence du commit audio utilisateur dans la provenance ;
- 06.4A : `asset-baseline.json` et reçu hash issue 134 ;
- 06.4E : `presentation-parity-private.json` dont le hash
  est validé par le verrou privé 06.4E, quatre égalités strictes,
  zéro différence ni liaison manquante.

Le comparateur calcule pour chacun des **56 packages** les noms
des propriétés modifiées de 06.0 à 06.2A et de 06.0 à 06.4A,
puis **compare les valeurs exactes** de 06.2A à 06.4A.
Il compare aussi les graphes de dépendances et les
**1640 profils** après l'audit audio, sans en inventer un seul.

Les noms exacts, anciennes valeurs et identités demeurent
dans `three-way-private.json` sur le PC Windows.
L'issue publique est un tableau **agrégé**, sans divulguer
un nom de package, chemin d'asset divergent ou chaîne métier.

## Interprétation et garde-fous

La propriété `source_parity_verified=true` n'est écrite
que si les **quatre** mêmes packages et les **huit** champs
sont déjà modifiés lors du contrôle 06.2A, conservés dans
la recapture 06.4A, avec zéro autre dérive des propriétés,
du graphe ou des profils. Toute anomalie rend le verdict
**faux** sans être réparée.

La qualification de provenance prouve un **état déjà constaté**
dans 06.2A : elle ne remplace pas un diff du commit audio
lui-même ni une validation artistique PIE de l'effet sonore.

Quel que soit le résultat, la migration binaire reste
interdite : `migration_authorized=false`, listes
`allowed_binary_writes`, `allowed_package_deletions`
et `allowed_savegame_writes` **vides**.

## Validation et suite

Le sujet exact
`chore(ARM-CLEAN06.4F): attest source provenance from post-audio UE capture`
active le contrôle uniquement après compilation et tests
Automation `Grimrock`. Exiger les trois statuts Windows
sur **le SHA 06.4F** et inspecter les compteurs réels.

Après cette preuve, les autres anciens pointeurs et champs
(armes et objets hors audio, jets génériques, niveaux,
réceptacles, palettes, SaveGame) nécessitent toujours une
conversion contrôlée via Unreal Editor, backups et rollback.
Ne pas déclarer le système final sans legacy avant 06.5/06.6.
Aucun changement UI : `UI.csv → ST_UI → WBP → Game/fr/en`
inchangé.
