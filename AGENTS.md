# GrimrockPrototype — Instructions permanentes

Répondre en français et vouvoyer. La racine locale est `D:\Development\GrimrockPrototype`, le dépôt est `PhilippeMartin1964/GrimrockPrototype`. Rédiger la documentation en français sous `docs/`. Commencer par le [point d’entrée documentaire](docs/README.md).

## Git et autonomie

- Travailler sur `master` exclusivement. Avant toute écriture, contrôler `git status`, la branche, HEAD et `origin/master`. Confirmer l’absence de concurrence distante avant le push. Préserver les modifications utilisateur et les fichiers locaux. Écarter tout nettoyage ou reset destructif.
- Un commit atomique par lot cohérent, code, tests et documentation associés inclus. Séparer les écritures natives ou changements de schéma si leurs garanties ou leur retour arrière diffèrent. Contrôler le diff, `git diff --check` et le parent. Publier uniquement le lot final sur `origin/master`.
- Une intervention peut traiter plusieurs lots autorisés. Attendre les deux statuts Windows exacts de chaque lot technique avant de poursuivre. Avancer de façon autonome. Une intervention utilisateur se justifie par un conflit, un risque pour ses données, une opération irréversible, une décision fonctionnelle ou une vérification native inaccessible après les contrôles automatisables.
- En cas d’échec publié, diagnostiquer et corriger atomiquement. Une réécriture éventuelle est limitée au HEAD invalide créé pendant l’intervention, avec SHA, parent, statuts et non-divergence vérifiés et protection `--force-with-lease`. Tout commit utilisateur ou distant imprévu est protégé. Si le push est bloqué, conserver le patch ou commit et indiquer la cause.

## Unreal, objets et sauvegardes

- Utiliser UE **5.5.4**, Visual Studio 2026 et v143 **MSVC 14.44.35207**. Le libellé UBT « Visual Studio 2022 » est normal. Le toolset physique est sous `C:\Program Files\Microsoft Visual Studio\18\Community\VC\Tools\MSVC\14.44.35207`. Formater avec clang-format **19.1.5 standalone**. Garder `Binaries/`, `Intermediate/`, `Saved/` et `DerivedDataCache/` hors Git.
- Respecter le [modèle normatif des objets](docs/Architecture/ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md), le [modèle relationnel](docs/Architecture/DataModel.md) et le [contrat d’édition](docs/Design/ARM_CLEAN06_ITEM_AUTHORING_CONTRACT.md). `DA_ItemSystem` et `UGridItemSubsystem` sont l’autorité de lecture. Chaque fiche d’arme relie un seul `ArchetypeId` aux catalogues communs. Préserver la possession unique et les identités. Écarter les copies de données statiques, les registres parallèles et les fallbacks historiques.
- Modifier `.uasset` et `.umap` uniquement lorsque nécessaire avec les outils natifs Unreal. Protéger les identités, GUID, Core Redirects et alias Primary Asset IDs. En cas de conversion approuvée, inventorier avant écriture et démontrer la parité. Protéger les deux Readables et les sauvegardes v27 personnelles. Toute vérification ou évolution de persistance se fait sur des copies isolées et avec contrôle des empreintes des originaux.

## UI et localisation

- Appliquer le [contrat UMG](docs/UI_UMG_PRESENTATION_CONTRACT.md). Le C++ porte le comportement, les lectures, les actions et la liaison aux widgets existants. Le WBP Designer porte l’apparence, le layout et les lignes de liste. Le C++ instancie un WBP complet pour une ligne dynamique. Pour une UI authorée, exclure la construction native de TextBlocks et de cellules et le réglage natif des polices, marges et positions. L’Event Graph intervient uniquement lorsqu’il apporte une logique distincte. Préserver le repli si un WBP n’est pas disponible.
- Pour tout travail UI/WBP, relire au début et à la fin le [workflow de localisation](docs/UI_WBP_LOCALIZATION_WORKFLOW.md). Dès qu’un texte visible change, classer sa provenance, réutiliser les clés existantes et saisir les nouvelles dans `Localization/Source/UI.csv`. Fermer l’éditeur, exécuter `-run=UIStringTableAuthoring`, contrôler le code 0 et vérifier chaque nouvelle clé dans `ST_UI` avant authoring UMG. Lier les textes statiques depuis le Designer. Préserver les textes métier des DataAssets et le formatage dynamique natif.
- Vérifier `UI.csv → ST_UI → WBP → Game/fr/en`, les tests de localisation et le rendu PIE pertinent. Inspecter et versionner les projections natives nécessaires. Un CSV seul ne matérialise pas `ST_UI`. Signaler l’étape native manquante et garder le lot ouvert si elle reste indispensable.

## Validation et publication

- Après un changement C++, test, runtime ou asset, compiler et exécuter les tests adaptés. Pour un lot documentaire, vérifier le corpus et utiliser le dispositif automatique installé.
- La [chaîne Windows](docs/Architecture/VALIDATION_AUTO01_WINDOWS_UE_AGENT.md) exécute `GrimrockPrototype-UEValidation` via `wscript.exe //B //Nologo`, bootstrap et agent du SHA distant. Elle utilise le worktree `D:\Development\GrimrockPrototype_UEValidation` pour compiler Development Editor et Shipping, puis exécuter Grimrock. Préserver l’action silencieuse, le worktree isolé et les données locales.
- Après publication, vérifier sur le SHA exact `UE5.5.4/SilentTask` et `UE5.5.4/LocalAuto`. Un statut absent ou pending laisse la validation ouverte. En cas d’échec, partir du diagnostic filtré `[UEValidation]`. Consulter les journaux privés localement si nécessaire. La compilation et `-NullRHI` ne prouvent pas le rendu PIE ou GPU. Rendre compte du SHA, des deux statuts, des tests et des vérifications visuelles réellement effectuées.
