# TD04.1 — CI / Shipping Validation Contract Audit

Date : 26 août 2026  
Projet : GrimrockPrototype — Unreal Engine 5.5.4  
Branche : `master`  
Baseline auditée : `fbab179a7366cce9322b39fb4f70eabb5d618dc8` — `Clean TD03.4 dungeon level Details action`  
Statut : **TERMINÉ — AUDIT / CONTRAT DOCUMENTÉ**

## 1. Objectif

TD04.1 ne crée volontairement ni workflow GitHub Actions ni pseudo-CI Unreal. Son objectif est d'établir le contrat réel de validation du projet avant toute automatisation supplémentaire :

- identifier ce que le dépôt sait réellement valider aujourd'hui ;
- distinguer formatage, compilation UE, Automation, PIE et Shipping ;
- définir quelle validation peut être qualifiée d'autoritaire ;
- préparer TD04.2 sans introduire de chemins machine codés en dur ;
- empêcher qu'un contrôle GitHub superficiel soit interprété comme une validation UE5.5.4.

Aucun code gameplay, aucune API, aucun SaveGame et aucun `.uasset/.umap` ne sont modifiés par TD04.1.

## 2. État constaté sur `master`

### 2.1 GitHub Actions

À la baseline `fbab179a...` :

- le répertoire `.github` n'existe pas dans le dépôt ;
- aucun workflow `.github/workflows/*.yml` n'est donc versionné ;
- le commit HEAD ne possède aucun statut de CI associé ;
- il n'existe donc actuellement **aucune CI GitHub autoritaire** pour GrimrockPrototype.

Conclusion : un commit poussé sur `master` n'est pas, par sa seule présence sur GitHub, prouvé compilable sous UE5.5.4.

### 2.2 Scripts versionnés

Le répertoire `Scripts/` contient actuellement :

```text
Scripts/CheckCppFormat.ps1
Scripts/FormatCpp.ps1
```

Ces scripts matérialisent le contrat STYLE01 et sont utiles, mais ils ne lancent pas UnrealBuildTool, UnrealEditor-Cmd, AutomationTool ni le packaging.

Une recherche du dépôt ne montre pas de harness versionné `RunUAT`, `BuildCookRun` ou équivalent. Les occurrences de commandes `UnrealEditor-Cmd` concernent essentiellement la documentation de validation de jalons antérieurs, pas un runner commun réutilisable.

### 2.3 Validation UE réelle aujourd'hui

La validation effective reste effectuée sur l'environnement de développement UE5.5.4 :

```text
1. compilation C++ / UHT / UBT dans l'environnement UE5.5.4 ;
2. exécution des Automation demandées pour le jalon ;
3. PIE lorsque le contrat touche assets, bindings, présentation ou workflow éditeur ;
4. vérification manuelle ciblée lorsqu'Automation ne peut pas prouver un aspect visuel/interactif.
```

Cette validation locale est actuellement plus autoritaire qu'un éventuel job GitHub qui ne disposerait pas réellement d'UE5.5.4.

## 3. Vocabulaire autoritaire TD04

Pour éviter toute ambiguïté, les termes suivants ont désormais un sens précis dans le projet.

### `Format validation`

Vérifie uniquement le contrat source STYLE01 : formatage, fichiers texte concernés et règles associées.

Cela **ne prouve pas** :

- que le projet compile ;
- que UHT accepte les headers ;
- que les Unity Builds compilent ;
- que les Automation passent ;
- que le jeu cook/package.

### `Editor build validation`

Est autoritaire seulement si elle exécute réellement le build UE5.5.4 de la cible Editor du projet sur Win64 avec l'outil du moteur correspondant.

Elle doit notamment pouvoir détecter :

- erreurs C++ ;
- erreurs UHT ;
- erreurs de linkage ;
- collisions révélées par Unity Build ;
- dépendances de modules invalides.

### `Automation validation`

Est autoritaire seulement si elle lance réellement Unreal Editor / UnrealEditor-Cmd 5.5.4 et exécute un filtre Automation explicitement défini, avec code retour/log exploitable.

Un test documenté comme ayant été exécuté dans le passé n'est pas équivalent à un test exécuté sur le commit courant.

### `PIE validation`

Reste nécessaire lorsque le contrat dépend de :

- `.uasset/.umap` ;
- Blueprint / UMG binding ;
- input réel ;
- présentation visuelle ;
- interaction éditeur ;
- séquences dont Automation ne reproduit pas fidèlement l'environnement.

PIE peut rester manuel ; TD04 n'impose pas d'automatiser artificiellement tout ce qui est visuel.

### `Shipping validation`

Ne sera utilisée comme qualification que lorsqu'un processus reproductible exécute au minimum un cook/package correspondant à la cible de distribution attendue et retourne un succès exploitable.

Un build Editor réussi n'est pas une validation Shipping.

## 4. Hiérarchie de confiance actuelle

À l'issue de TD04.1 :

```text
Niveau A — UE5.5.4 local réellement exécuté
           build + Automation + PIE ciblé selon le changement
           -> autorité actuelle

Niveau B — scripts de dépôt reproductibles
           aujourd'hui : formatage C++ seulement
           -> utile mais non suffisant pour valider UE

Niveau C — statut GitHub
           aujourd'hui : aucun workflow / aucun check UE
           -> aucune preuve de compilation ou de gameplay
```

Cette hiérarchie changera seulement lorsqu'un runner exécute réellement les mêmes outils UE et contrats que le niveau A.

## 5. Contrat de TD04.2 — Local UE Validation Harness

Le prochain jalon doit créer un harness local commun, versionné et portable. Il doit d'abord automatiser **ce qui est déjà fait manuellement**, sans inventer une architecture CI prématurée.

Cible recommandée :

```text
Scripts/ValidateUE.ps1
```

Responsabilités minimales prévues :

```text
- résoudre la racine du dépôt depuis le script ;
- résoudre l'installation UE5.5.4 via paramètre et/ou variable d'environnement ;
- refuser explicitement une installation UE invalide ;
- construire GrimrockPrototypeEditor Win64 Development ;
- lancer un filtre Automation fourni explicitement ;
- propager un code de sortie non nul en cas d'échec ;
- afficher les commandes et chemins réellement utilisés ;
- ne jamais dépendre de D:\Development\GrimrockPrototype ;
- ne jamais supposer D:\UE_5.5 comme unique emplacement moteur.
```

Le harness ne doit pas stocker un second catalogue de tests faisant concurrence aux Automation elles-mêmes. Un petit socle stable pourra être défini ultérieurement, mais le filtre doit rester paramétrable.

## 6. Contrat de TD04.3 — Cook / Package

Après validation de TD04.2, un jalon séparé ajoutera le contrat de cook/package.

Pourquoi séparé :

- un échec Editor et un échec Cook n'ont pas les mêmes causes ;
- le packaging peut dépendre d'assets/configuration non exercés par les Automation ;
- il faut conserver des diagnostics lisibles et une responsabilité unique par jalon.

TD04.3 devra définir la cible et le profil réellement utilisés par le projet avant d'écrire la commande définitive.

## 7. Contrat de TD04.4 — CI UE éventuelle

Une CI distante ne sera ajoutée que si un runner autorisé dispose réellement :

- de la version UE5.5.4 nécessaire ;
- des dépendances de compilation correspondantes ;
- de l'accès au contenu requis ;
- d'un espace disque et d'un temps d'exécution compatibles avec le projet.

Un workflow ne sera jamais qualifié de `UE validation` s'il ne fait que :

```text
checkout
git diff
clang-format
analyse de texte
```

Ces contrôles peuvent être utiles, mais ils doivent porter un nom correspondant à ce qu'ils prouvent réellement.

## 8. Politique de validation par type de changement

### C++ runtime pur

```text
Editor build UE5.5.4
+ Automation de caractérisation/régression du domaine
```

### C++ Editor

```text
Editor build UE5.5.4
+ Automation EditorContext
+ vérification manuelle du Grid Editor Mode si le workflow visuel est touché
```

### Blueprint / UMG / asset / map

```text
Editor build si C++ touché
+ Automation disponible
+ PIE ciblé obligatoire
```

### SaveGame

```text
Editor build
+ capture/restore
+ migration legacy si version impactée
+ Save -> Continue lorsque des assets réels sont impliqués
```

### Shipping / distribution

```text
Editor validation pertinente
+ cook/package reproductible
+ lancement/smoke test du produit packagé lorsque le jalon le demande
```

## 9. Stop condition TD04.1

TD04.1 est terminé lorsque le dépôt possède une description non ambiguë de la réalité actuelle et du prochain contrat d'automatisation.

Cette condition est atteinte :

- absence de CI GitHub UE constatée ;
- scripts existants inventoriés ;
- autorité de validation actuelle définie ;
- termes Build / Automation / PIE / Shipping séparés ;
- prochaine étape bornée à un harness local portable.

**Décision : passer à TD04.2, sans créer de workflow GitHub Actions avant d'avoir un harness UE local fiable.**
