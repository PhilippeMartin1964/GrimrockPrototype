# STYLE01 — C++ Formatting Baseline

Statut : **PRÉPARÉ — NON DÉMARRÉ**  
Date de préparation : **25 août 2026**  
Baseline de préparation : `3ac78b3b892207c30734dacfba3d0ed2613f6542`  
Projet : **GrimrockPrototype — Unreal Engine 5.5.4**

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

Cela inclut les modules runtime, editor et les tests first-party.

Sont exclus sauf audit explicite :

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

Avant d'appliquer le formatage global, le prochain thread doit vérifier l'arborescence réelle et exclure tout code vendor/third-party éventuel présent sous `Source/`.

## 5. Fichiers d'outillage cible

STYLE01 doit évaluer puis, si compatible avec la version de clang-format réellement disponible, introduire :

```text
.clang-format
.editorconfig
Scripts/FormatCpp.ps1
Scripts/CheckCppFormat.ps1
```

Éventuellement :

```text
.git-blame-ignore-revs
```

afin d'ignorer le commit de reformatage mécanique lors d'un `git blame` local.

Ne pas créer d'option `.clang-format` supposée disponible sans vérifier la version installée sur la machine Visual Studio / LLVM utilisée par le projet.

## 6. Direction `.clang-format`

La configuration finale doit être dérivée d'un profil compatible Unreal et atteindre au minimum les comportements suivants :

```text
ColumnLimit = 160
Allman braces
accolades conservées
SpaceBeforeParens = contrôles seulement
bin-pack arguments/parameters
pas de fonction/if/boucle condensé sur une seule ligne
MaxEmptyLinesToKeep = 1
pas de lignes vides au début des blocs
SortIncludes = Never / équivalent supporté
IncludeBlocks = Preserve / équivalent supporté
pas de reflow des commentaires
indentation tabs / largeur 4
```

Cette liste décrit le **résultat attendu**, pas encore le fichier YAML définitif. STYLE01.1 doit vérifier les noms et valeurs exacts supportés par la version réellement utilisée.

## 7. Sous-jalons proposés

### STYLE01.1 — Formatter Audit & Contract

Objectif :

- identifier la version exacte de clang-format disponible ;
- vérifier s'il existe déjà une configuration locale Visual Studio/LLVM non versionnée ;
- auditer quelques fichiers représentatifs ;
- finaliser `.clang-format` et `.editorconfig` sans reformater tout le dépôt ;
- documenter toute divergence nécessaire avec les options disponibles.

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

- l'utilisateur compile sous Unreal Engine 5.5.4 / Visual Studio 2026 ;
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

Si STYLE01.1 et STYLE01.2 peuvent être regroupés proprement sans perdre la lisibilité du diff, préférer un seul commit logique.

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
