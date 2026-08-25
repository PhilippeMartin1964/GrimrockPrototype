# STYLE01 — C++ Formatting Baseline

Statut : **STYLE01.5 — CLOS — GRIMROCK C++ STYLE v1 FIGÉ**
Date de préparation : **25 août 2026**  
Baseline de préparation : `3ac78b3b892207c30734dacfba3d0ed2613f6542`  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**  
Environnement de développement : **Visual Studio 2022 — clang-format 19.1.5**

## 1. Objectif

STYLE01 doit définir puis appliquer une convention C++ unique, mécanique et reproductible à l'ensemble du code first-party du projet avant la campagne principale de réduction de dette technique.

Le but n'est **pas** de modifier l'architecture, le comportement ou les assets. Le but est de réduire le bruit syntaxique, limiter les retours à la ligne artificiels, rendre les gros fichiers plus lisibles et faire en sorte que tous les futurs diffs C++ utilisent le même format.

STYLE01 ne doit pas démarrer MON21.2 et ne doit pas corriger TD01.x pendant le reformatage.

## 2. Autorité de style

### Règle absolue

Le **Epic / Unreal Engine C++ Coding Standard** est l'autorité supérieure.

Si une préférence Grimrock, une option `clang-format`, Visual Studio ou un exemple de ce document entre en conflit avec une règle Unreal/Epic, **la règle Unreal/Epic gagne sans exception**.

En particulier :

- accolades conservées selon le standard Unreal, y compris pour un bloc d'une seule instruction ;
- style Allman pour les blocs ;
- aucune transformation destinée uniquement à économiser des lignes si elle viole le standard Unreal ;
- les conventions Unreal de noms, types, pointeurs, références, macros et headers restent intactes.

### Parenthèses

Le projet historique contient fréquemment :

```cpp
Reset ();
FMath::Max (...);
Function (...);
```

La préférence humaine initiale était de conserver cet espace pour la lisibilité. Cependant, le standard Unreal/Epic ne met pas d'espace entre le nom d'une fonction et `(`. La baseline doit donc produire :

```cpp
Reset();
FMath::Max(...);
Function(...);
```

Les mots de contrôle conservent l'espace :

```cpp
if (...)
for (...)
while (...)
switch (...)
```

Cette distinction est obligatoire afin de respecter Unreal.

## 3. Règles Grimrock complémentaires

Ces règles ne s'appliquent que là où elles ne contredisent pas Unreal.

### 3.1 Largeur de ligne

Cible : **160 colonnes**.

Le formatter doit préférer une instruction sur une seule ligne lorsqu'elle tient lisiblement dans cette limite.

Exemple souhaité :

```cpp
bool AGridLevelRuntimeActor::CanPartyInteractWithEdgeObject(int32 ObjectCellX, int32 ObjectCellY, EGridEdge ObjectEdge, const AGrimrockPartyPawn* PartyPawn) const;
```

plutôt qu'un paramètre par ligne sans nécessité.

La limite n'est pas une obligation de compacter une expression illisible. Une coupure reste autorisée lorsque :

- la ligne dépasse la limite ;
- une macro Unreal l'exige ;
- une expression complexe gagne réellement en clarté ;
- une directive préprocesseur ou un commentaire impose une structure particulière.

### 3.2 Arguments et paramètres

- bin-pack des arguments et paramètres tant qu'ils restent lisibles et dans la limite ;
- ne jamais appliquer systématiquement « un argument = une ligne » ;
- lorsqu'une coupure est nécessaire, remplir les lignes avant de créer une colonne verticale artificielle ;
- ne pas modifier l'ordre des arguments.

### 3.3 Expressions booléennes

Les expressions simples ou moyennes doivent être compactées lorsqu'elles tiennent dans 160 colonnes.

Avant :

```cpp
const bool bRestoreDead =
    RestoreState &&
    (RestoreState->bIsDead ||
        RestoreState->CurrentHealth <= 0 ||
        RestoreState->MonsterState == EGridMonsterState::Dead);
```

Cible lorsque lisible :

```cpp
const bool bRestoreDead = RestoreState && (RestoreState->bIsDead || RestoreState->CurrentHealth <= 0 || RestoreState->MonsterState == EGridMonsterState::Dead);
```

Si une expression doit être coupée, la coupure doit suivre les unités logiques, pas une règle mécanique « un opérande par ligne ».

### 3.4 Accolades et blocs

- conserver les accolades imposées par Unreal ;
- style Allman ;
- ne jamais supprimer les accolades pour gagner deux lignes ;
- ne pas mettre un `if` et son corps sur la même ligne ;
- ne pas mettre une fonction complète sur une seule ligne si cela contrevient au standard Unreal.

### 3.5 Lignes vides

Principe : **ne pas ajouter de ligne vide sauf nécessité réelle de lisibilité**.

Règles :

- jamais plusieurs lignes vides consécutives ;
- aucune ligne vide au début ou à la fin d'un bloc `{ ... }` ;
- une ligne vide peut séparer deux groupes logiques distincts dans une fonction ;
- une ligne vide entre deux définitions top-level reste acceptable ;
- ne pas insérer automatiquement des lignes vides entre chaque déclaration et chaque appel ;
- ne pas « aérer » artificiellement du code déjà court ;
- conserver une fin de fichier valide avec newline finale.

### 3.6 Indentation

- indentation conforme Unreal ;
- tabs pour l'indentation ;
- largeur logique 4 ;
- espaces uniquement pour les alignements internes lorsque nécessaire.

### 3.7 Pointeurs et références

Conserver le style Unreal :

```cpp
AGridMonsterActor* Monster;
const FGridLevelObjectData& ObjectData;
```

### 3.8 Includes

Le formatter ne doit pas réordonner automatiquement les includes.

Raisons :

- préserver les règles Unreal/IWYU ;
- préserver la position du `.generated.h` ;
- éviter qu'un changement de formatage devienne aussi un changement d'include graph.

### 3.9 Commentaires

- ne pas reformuler les commentaires ;
- ne pas reflow automatiquement les commentaires existants lors de STYLE01 ;
- seules les indentations manifestement cassées peuvent être normalisées mécaniquement.

## 4. Périmètre

Périmètre first-party visé :

```text
Source/**/*.h
Source/**/*.cpp
Source/**/*.inl
```

Après audit de l'arborescence réelle, le périmètre automatisé est limité aux modules first-party suivants :

```text
Source/GrimrockPrototype/**/*.h|cpp|inl
Source/GrimrockPrototypeEditor/**/*.h|cpp|inl
Source/GrimrockLua/**/*.h|cpp|inl
```

`Source/GrimrockLua` est le module Unreal first-party d'intégration Lua et fait donc partie du périmètre. Le code Lua tiers reste fourni par le sous-module `ThirdParty/Lua54` et est exclu.

Sont exclus :

```text
Binaries/
Intermediate/
DerivedDataCache/
Saved/
ThirdParty/
fichiers générés par Unreal
assets .uasset/.umap
Build.cs et autres langages non C++
```

## 5. Audit formatter et outillage

### 5.1 Environnement de référence

Audit effectué le **25 août 2026** sur l'environnement de développement réel :

```text
IDE : Visual Studio 2022 Community
clang-format : 19.1.5
Chemin vérifié : C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\Llvm\x64\bin\clang-format.exe
```

`clang-format.exe` n'est pas exposé dans le `PATH` de la session PowerShell. Les scripts STYLE01 ne dépendent donc pas du `PATH` : ils recherchent d'abord l'installation Visual Studio 2022 via `vswhere`, puis le chemin Community par défaut, puis seulement un éventuel `clang-format` disponible dans le `PATH`.

La version **19.1.5** est figée pour **Grimrock C++ Style v1**. Les scripts refusent une autre version afin d'éviter des diffs dépendant de la version de formatter.

### 5.2 Fichiers versionnés

STYLE01.1 introduit :

```text
.clang-format
.editorconfig
Scripts/FormatCpp.ps1
Scripts/CheckCppFormat.ps1
```

Le commit mécanique global `3c4032beaad5a0fa9e7b3809701ee3de85c4e1de` est enregistré dans `.git-blame-ignore-revs` afin que les analyses `git blame` puissent ignorer le bruit du reformatage STYLE01.

### 5.3 Contrat des scripts

`Scripts/FormatCpp.ps1` :

- résout automatiquement clang-format 19.1.5 depuis Visual Studio 2022 ;
- refuse toute autre version ;
- ne parcourt que les trois modules first-party audités ;
- ne traite que `.h`, `.cpp`, `.inl` ;
- exclut explicitement code généré et répertoires tiers/build ;
- applique `.clang-format` en place.

`Scripts/CheckCppFormat.ps1` :

- utilise exactement le même périmètre et la même version ;
- fonctionne sans écrire dans les sources ;
- utilise `--dry-run --Werror` ;
- retourne un code non nul si un fichier doit être reformaté.

## 6. Contrat `.clang-format`

La configuration versionnée est dérivée des options réellement supportées par clang-format 19.1.x et atteint les comportements suivants :

```text
ColumnLimit = 160
Allman braces
accolades conservées
SpaceBeforeParens = ControlStatements
bin-pack arguments/parameters
pas de fonction/if/boucle condensé sur une seule ligne
MaxEmptyLinesToKeep = 1
pas de lignes vides au début des blocs
SortIncludes = Never
IncludeBlocks = Preserve
SortUsingDeclarations = Never
pas de reflow des commentaires
indentation tabs / largeur 4
pointeurs et références attachés au type
case labels indentés conformément aux exemples Epic
préprocesseur non réindenté artificiellement
newline finale
```

Les options qui pourraient modifier des tokens ou la sémantique ne sont pas activées. En particulier, STYLE01 ne demande pas à clang-format d'ajouter des accolades : il interdit leur suppression (`RemoveBracesLLVM: false`) et laisse toute éventuelle violation structurelle à une correction explicite hors reformatage mécanique.

## 7. Sous-jalons proposés

### STYLE01.1 — Formatter Audit & Contract

**État : établi.**

Résultat :

- Visual Studio 2022 confirmé comme IDE réel ;
- clang-format 19.1.5 confirmé ;
- absence préalable de `.clang-format`, `.editorconfig` et scripts STYLE01 confirmée ;
- périmètre first-party audité ;
- contrat `.clang-format` / `.editorconfig` défini ;
- scripts de formatage et de contrôle préparés ;
- aucun fichier `Source/` reformaté à ce stade.

Aucun changement gameplay.

### STYLE01.2 — Representative Formatting Validation

Échantillon recommandé :

```text
GridLevelRuntimeActor.cpp/.h
GridPartyInventoryComponent.cpp/.h
GrimrockPartyPawn.cpp/.h
GridActivationComponent.cpp/.h
un fichier UI
un fichier Editor Slate
un fichier Automation Test
un .inl si présent
```

Objectif :

- appliquer le formatter uniquement à cet échantillon ;
- contrôler manuellement les diffs ;
- vérifier macros Unreal, UPROPERTY/UFUNCTION, lambdas, templates, delegates, UE_LOG/TEXT, generated headers ;
- ajuster la configuration avant propagation globale.

### STYLE01.3 — Repository-Wide Mechanical Reformat

Une fois STYLE01.2 validé :

- reformater le périmètre first-party complet ;
- **aucun changement fonctionnel dans le même commit** ;
- aucun renommage ;
- aucun déplacement de fichier ;
- aucun refactor ;
- aucune correction TODO ;
- aucun changement `.uasset/.umap` ;
- aucun changement d'ordre des includes sauf nécessité Unreal démontrée.

Le commit de reformatage doit être identifiable comme purement mécanique.

### STYLE01.4 — Build / Automation Regression

Après le reformatage global :

- l'utilisateur compile sous Unreal Engine 5.5.4 / Visual Studio 2022 ;
- exécuter le socle Automation approprié ;
- lancer PIE si nécessaire pour vérifier qu'aucun asset/binding n'a été indirectement affecté ;
- traiter toute anomalie comme une régression STYLE01, pas comme une occasion de refactor.

### STYLE01.5 — Closure & Debt Re-baseline

Après validation :

- documenter la version exacte du formatter ;
- figer Grimrock C++ Style v1 ;
- ajouter STYLE01 au registre de dette comme résolu ;
- réévaluer les métriques des gros fichiers après compactage ;
- reprendre ensuite TD01.1.

## 8. Politique Git

- branche unique : `master` ;
- toujours re-fetch `master` avant une écriture ;
- ne créer aucune branche ;
- documentation dans `docs/Design/` ;
- limiter le nombre de commits.

Cible recommandée :

```text
Commit A — Define STYLE01 formatting contract/tooling
Commit B — Apply repository-wide mechanical formatting baseline
```

Le reformatage global doit rester séparé de toute correction fonctionnelle afin que Git permette de l'ignorer ou de l'isoler facilement.

## 9. Validation du caractère mécanique

Avant acceptation du commit global :

1. vérifier qu'il ne contient que les extensions prévues ;
2. inspecter les diffs sur plusieurs fichiers sensibles ;
3. confirmer qu'aucun identifiant, littéral, opérateur, include, macro ou branche logique n'a été changé intentionnellement ;
4. compiler ;
5. exécuter Automation ;
6. seulement ensuite marquer STYLE01 comme clos.

Lorsque cela est faisable, utiliser un contrôle token-aware ou AST/compilation plutôt qu'un simple « diff visuellement plausible ».

## 10. Critères de clôture

STYLE01 est clos uniquement lorsque :

- le standard Unreal/Epic est explicitement l'autorité ;
- la configuration versionnée produit le format attendu ;
- 160 colonnes est la cible de compactage ;
- les accolades Unreal sont conservées ;
- les fonctions n'ont pas d'espace avant `(` ;
- les contrôles ont leur espace avant `(` ;
- les lignes vides superflues sont supprimées et ne sont pas réintroduites automatiquement ;
- les includes ne sont pas réordonnés mécaniquement ;
- tout le C++ first-party est formaté de manière reproductible ;
- le build UE5.5.4 est validé par l'utilisateur ;
- la régression Automation est validée ;
- aucun comportement gameplay n'a changé ;
- le prochain chantier redevient `TD01.1 — Receptacle Removal Permission Persistence`.

## 11. Relation avec la dette technique

STYLE01 est une dette **P2 de maintenabilité**, mais constitue un préalable pratique à la campagne TD01/TD02 car elle :

- rend les futurs diffs plus petits et plus homogènes ;
- évite qu'un refactor structurel reformate implicitement des centaines de lignes ;
- rend les métriques de lignes physiques plus comparables ;
- réduit le bruit lors des revues de gros fichiers.

Elle ne change pas la priorité fonctionnelle de TD-PERSIST-001, TD-PARTY-001 ou TD-EVENT-001. Elle doit être courte, mécanique et fermée avant de reprendre ces corrections.

## 12. Clôture STYLE01 — 25 août 2026

**État final : CLOS.**

### 12.1 Commits de référence

```text
4c8c1c082d5ccbc5d6262ede308ca1ea6ca39df2
    Define STYLE01 formatting contract and tooling

cc007331dc18d0e8679ac9f3892e776496b9d6da
    Refine STYLE01 formatter after representative validation

3c4032beaad5a0fa9e7b3809701ee3de85c4e1de
    Apply repository-wide mechanical C++ formatting baseline

d02d1484ddcc3bf99dd5b7b9ffc2a98d2ddd637d
    Fix stale automation contracts after STYLE01 formatting
```

Le commit `3c4032be...` est le commit mécanique à ignorer dans `git blame`. Le commit `d02d1484...` ne modifie que quatre fichiers de tests historiques dont les assertions avaient dérivé par rapport au contrat SaveGame v8 ou dépendaient littéralement de l'ancien espacement `UPROPERTY (`.

### 12.2 Validation mécanique

Résultats validés pendant STYLE01.2 / STYLE01.3 :

- clang-format **19.1.5** de Visual Studio 2022 ;
- échantillon représentatif validé avant propagation ;
- Slate validé avec la configuration racine finale, sans override local ;
- **480 fichiers** modifiés par le reformatage global ;
- contrôle fichier par fichier : séquence hors espaces identique au `HEAD` pré-formatage ;
- aucun asset `.uasset/.umap` modifié ;
- aucun changement gameplay dans le commit mécanique ;
- `Scripts/CheckCppFormat.ps1` : **486 fichiers first-party conformes** ;
- audit UTF-8 strict : **486 / 486 fichiers valides**.

### 12.3 Build / Automation

Validation réelle après reformatage :

- compilation UE5.5.4 / Visual Studio 2022 : **validée par l'utilisateur** ;
- première campagne Automation globale : 6 tests historiques en échec ;
- diagnostic : 5 assertions figées sur `SaveVersion == 7` alors que MON20.9.2 a porté le contrat courant à v8 ; 1 test de frontière cherchait littéralement l'ancien espacement `UPROPERTY (` ;
- correctif limité aux tests, commit `d02d1484...` ;
- campagne Automation complète relancée par l'utilisateur après correctif : **aucun échec remonté** ;
- les tests de contrats corrigés observés dans le log fourni terminent en `Success` ;
- aucun `Ensure condition failed`, `Assertion failed` ou `Fatal error` signalé dans la validation finale transmise.

### 12.4 Baseline durable

Grimrock C++ Style v1 signifie désormais :

```text
Unreal / Epic Coding Standard = autorité supérieure
clang-format 19.1.5          = version figée
ColumnLimit                  = 160
UTF-8                        = obligatoire pour le C++ first-party
LF                           = .h / .cpp / .inl
includes                     = jamais triés mécaniquement
commentaires                 = jamais reflow mécaniquement
reformatage                  = Scripts/FormatCpp.ps1
contrôle                     = Scripts/CheckCppFormat.ps1
```

Les futurs changements C++ doivent partir de cette baseline ; un chantier fonctionnel ou structurel ne doit plus embarquer de reformatage global parasite.

### 12.5 Suite

STYLE01 ne change pas les priorités fonctionnelles du registre de dette.

Le prochain chantier redevient :

```text
TD01.1 — Receptacle Removal Permission Persistence
        -> TD-PERSIST-001
```
