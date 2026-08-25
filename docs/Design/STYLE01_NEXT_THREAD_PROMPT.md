# Prompt — STYLE01 C++ Formatting Baseline

Nous poursuivons le développement de **GrimrockPrototype**, clone/inspiration de *Legend of Grimrock 2* sous **Unreal Engine 5.5.4**, en C++ avec Visual Studio 2022.

Environnement de développement de référence : **Visual Studio 2022 — clang-format 19.1.5**.

Repo GitHub :
`https://github.com/PhilippeMartin1964/GrimrockPrototype`

Branche unique :
`master`

Avant toute écriture :

1. re-fetch immédiatement `origin/master` ;
2. vérifier le HEAD réel, car d'autres travaux peuvent avoir été poussés depuis ce prompt ;
3. ne créer aucune branche ;
4. travailler directement sur `master` ;
5. pousser sur `origin/master` sans me redemander ;
6. limiter le nombre de commits ;
7. documentation dans `docs/Design/`, jamais dans `Documentation/` ;
8. ne toucher à aucun `.uasset/.umap` pour STYLE01.

La baseline au moment de la préparation de ce prompt était :

`3ac78b3b892207c30734dacfba3d0ed2613f6542` — `Consolidate technical debt documentation`

Cette valeur est uniquement informative : **le HEAD courant doit être re-vérifié**.

## Contexte

MON20 est clos. MON21.1 est un audit d'architecture uniquement et **MON21.2 reste suspendu**. Nous sommes dans une phase d'exploitation / playtest / stabilisation et nous avons commencé à inventorier la dette technique.

Le registre autoritaire est :

`docs/Architecture/TECHNICAL_DEBT_REGISTER.md`

Nous avons décidé de traiter auparavant une dette de formatage C++ afin que tous les futurs refactors et corrections aient des diffs propres et homogènes.

Le document de cadrage STYLE01 est :

`docs/Design/STYLE01_CPP_FORMATTING_BASELINE.md`

**Le lire intégralement avant toute modification.**

## Objectif du thread

Exécuter **STYLE01 — C++ Formatting Baseline**.

Il s'agit d'un chantier de formatage et d'outillage, **pas d'un refactor fonctionnel**.

Aucun comportement gameplay ne doit changer.
Aucun TODO ne doit être corrigé pendant le reformatage.
Aucune classe ne doit être extraite, renommée ou réorganisée uniquement parce qu'elle est grosse.
Aucun asset Unreal ne doit être modifié.

## Autorité absolue

Le **Epic / Unreal Engine C++ Coding Standard** est supérieur à toute préférence locale.

**Nous ne violons aucune règle Unreal.**

En cas de conflit entre une préférence Grimrock et Epic/Unreal, Epic/Unreal gagne immédiatement.

Cela implique notamment :

- accolades conservées même pour une seule instruction si Unreal l'exige ;
- style Allman ;
- aucune suppression d'accolades pour économiser des lignes ;
- pas d'espace entre un nom de fonction et `(` : `Function()`, `FMath::Max(...)`, `Reset()` ;
- espace après un mot de contrôle : `if (...)`, `for (...)`, `while (...)`, `switch (...)`.

La préférence initiale de mettre `Function (...)` a été abandonnée parce qu'elle entre en conflit avec le standard Epic.

## Format Grimrock cible

Sous réserve de compatibilité avec la version réelle de clang-format disponible :

- largeur cible **160 colonnes** ;
- compacter paramètres et arguments lorsqu'ils tiennent lisiblement ;
- ne jamais appliquer systématiquement « un argument par ligne » ;
- expressions booléennes compactées lorsqu'elles restent lisibles ;
- tabs pour indentation, largeur logique 4 ;
- style Unreal `Type* Pointer`, `Type& Reference` ;
- includes non triés automatiquement ;
- `.generated.h` et règles Unreal/IWYU préservés ;
- commentaires non reflow automatiquement ;
- **ne pas ajouter de lignes vides sauf nécessité absolue de lisibilité** ;
- jamais plusieurs lignes vides consécutives ;
- aucune ligne vide au début ou à la fin d'un bloc ;
- une seule ligne vide lorsqu'elle sépare réellement deux groupes logiques ou deux définitions top-level ;
- newline finale valide.

## Travail demandé

### STYLE01.1 — Formatter Audit & Contract

Commencer par un audit, sans reformater immédiatement tout le dépôt :

1. identifier la version exacte de clang-format/LLVM réellement disponible avec l'environnement de développement ;
2. rechercher toute configuration `.clang-format`, `.editorconfig`, réglage Visual Studio ou script existant ;
3. auditer le périmètre `Source/` et repérer tout code vendor/third-party à exclure ;
4. vérifier les options réellement supportées par cette version de clang-format ;
5. créer une première `.clang-format` et `.editorconfig` conformes au document STYLE01 ;
6. préparer si pertinent :
   - `Scripts/FormatCpp.ps1`
   - `Scripts/CheckCppFormat.ps1`
   - éventuellement `.git-blame-ignore-revs` ;
7. ne pas supposer une option clang-format sans vérifier qu'elle existe dans la version utilisée.

### STYLE01.2 — Representative Formatting Validation

Tester le formatter sur un échantillon représentatif, par exemple :

- `GridLevelRuntimeActor.cpp/.h`
- `GridPartyInventoryComponent.cpp/.h`
- `GrimrockPartyPawn.cpp/.h`
- `GridActivationComponent.cpp/.h`
- un fichier UI ;
- un fichier Editor Slate ;
- un fichier Automation Test ;
- un `.inl` si présent.

Contrôler spécialement :

- `UCLASS`, `USTRUCT`, `UENUM` ;
- `UPROPERTY`, `UFUNCTION` ;
- macros Unreal ;
- `UE_LOG`, `TEXT`, delegates ;
- lambdas ;
- templates ;
- generated headers ;
- longues signatures ;
- longues expressions booléennes ;
- blocs préprocesseur ;
- commentaires.

Présenter les différences importantes et ajuster la configuration si nécessaire.

### STYLE01.3 — Repository-Wide Mechanical Reformat

Après validation du format :

- reformater tout le C++ first-party du projet ;
- inclure `.h`, `.cpp`, `.inl` ;
- exclure Binaries/Intermediate/Saved/DerivedDataCache/ThirdParty/généré ;
- ne faire **aucune** modification fonctionnelle ;
- ne pas renommer ;
- ne pas déplacer de fichier ;
- ne pas corriger les TODO ;
- ne pas réordonner les includes automatiquement ;
- ne toucher à aucun asset.

Le reformatage global doit rester un commit mécanique identifiable et séparé de toute correction gameplay.

### STYLE01.4 — Validation

Je compile et exécute Unreal localement.

Préparer la checklist exacte de validation :

- compilation UE5.5.4 ;
- Automation Regression appropriée ;
- éventuellement PIE si réellement nécessaire ;
- contrôle que les diffs ne contiennent aucun changement sémantique.

Ne jamais annoncer que la compilation/PIE est validée tant que je n'ai pas fourni le résultat.

### STYLE01.5 — Closure

Après validation utilisateur :

- figer la version exacte de Grimrock C++ Style v1 ;
- documenter la version du formatter ;
- mettre à jour `docs/Architecture/TECHNICAL_DEBT_REGISTER.md` ;
- marquer STYLE01 comme résolu ;
- rebaseliner les métriques des gros fichiers ;
- conserver MON21.2 suspendu ;
- reprendre ensuite :

`TD01.1 — Receptacle Removal Permission Persistence`

## Politique Git

Très important : **attention au nombre de commits**.

Cible recommandée :

1. un commit logique pour contrat/outillage STYLE01.1–01.2 si possible ;
2. un commit mécanique séparé pour le reformatage global ;
3. éviter les micro-commits de documentation ou de correction si le tout peut rester un jalon cohérent.

Toujours re-fetch `master` juste avant une écriture car d'autres conversations peuvent modifier le dépôt en parallèle.

## Stop conditions

Arrêter et documenter le problème au lieu de forcer si :

- une option de formatter nécessaire n'existe pas dans la version disponible ;
- clang-format modifie de manière dangereuse des macros Unreal ;
- les includes/generated headers sont réordonnés ;
- le formatter impose un style contraire au standard Epic ;
- le diff mélange formatage et changement sémantique ;
- le périmètre comprend du code tiers qui ne doit pas être reformaté.

Dans ces cas, adapter la configuration ou exclure précisément le cas problématique ; **ne jamais sacrifier la conformité Unreal pour obtenir un format plus compact**.

## Première action attendue

**Go STYLE01.1 — Formatter Audit & Contract.**

Commencer par re-fetch `master`, lire `docs/Design/STYLE01_CPP_FORMATTING_BASELINE.md`, auditer les outils/configurations existants et proposer la configuration réelle avant toute propagation globale.
