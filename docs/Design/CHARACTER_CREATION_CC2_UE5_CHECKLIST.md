# CC2 - Checklist de validation dans Unreal Engine 5

## 1. Objet

Ce document décrit les opérations humaines nécessaires pour valider la tranche CC2 dans Unreal Engine 5.5.4.

Branche :

```text
codex/character-création-cc2-création-api
```

CC2 ajoute l'API C++ de création initiale. Cette API est utilisable depuis Blueprint, mais aucun widget ne l'appelle encore.

---

## 2. Répartition des responsabilités

| Responsable | Travail CC2 |
|---|---|
| ChatGPT / Codex | Requête de création, validation, mutation atomique, restauration en cas d'erreur, tests et documentation |
| Utilisateur | Recuperation de la branche, compilation UE5, exécution des tests Automation et contrôle PIE |
| CC3 | Création du widget et appel Blueprint de l'API |

Vous ne devez modifier aucun Blueprint pour valider CC2.

---

## 3. Ce qui est implemente

- `FRPGCharacterCreationRequest` contient le nom, la race, la classe et le portrait optionnel ;
- `bInitialCharacterCreationCompleted` distingue le placeholder du personnage finalisé ;
- `HasCompletedInitialCharacterCreation()` expose cet etat ;
- `CreateInitialCharacter()` valide la requête avant toute mutation ;
- une création valide remplace le placeholder par exactement un personnage ;
- le personnage créé possède un équipement vide, 40 slots et un curseur vide ;
- les caractéristiques proviennent de la classe et des bonus raciaux ;
- les PV, la mana et la charge sont calculés par les règles CC1 ;
- une seconde création est refusée ;
- si le diagnostic d'ownership échoue, l'ancien etat du groupe est restauré.

---

## 4. Étape A - Récupérer et compiler

```bash
git fetch origin
git switch codex/character-création-cc2-création-api
git pull
```

Fermer Unreal Editor avant la compilation si des modules C++ sont déjà chargés.

Dans Visual Studio :

1. choisir **Development Editor** ;
2. choisir **Win64** ;
3. compiler `GrimrockPrototype` ou la solution complète ;
4. rouvrir le projet après un `Build succeeded`.

Régénérer les fichiers Visual Studio depuis le `.uproject` uniquement si les nouveaux fichiers ne sont pas détectés.

En cas d'erreur, transmettre d'abord la première erreur UnrealHeaderTool ou C++, avec son fichier, sa ligne et le contexte du journal.

---

## 5. Étape B - Vérifier les DataAssets CC1

Vérifier uniquement que les assets suivants existent toujours :

```text
Content/Grimrock/RPG/Races/DA_Race_Human
Content/Grimrock/RPG/Classes/DA_Class_Warrior
```

Aucune nouvelle valeur ne doit être ajoutée pour CC2.

Si ces assets sont encore des changements Git locaux non validés, ne pas les supprimer lors du changement de branche. Ils devront être inclus lors de l'intégration finale de CC1/CC2.

---

## 6. Étape C - Exécuter les tests Automation

1. Ouvrir **Tools > Session Frontend**.
2. Ouvrir **Automation**.
3. Rechercher `Grimrock.CharacterCreation`.
4. Sélectionner CC0, CC1 et CC2.
5. Lancer les dix tests.

Tests CC2 :

| Test | Verification |
|---|---|
| `CreateInitialCharacter` | Création complète et cohérente d'Elias |
| `RejectInvalidRequestAtomically` | Requêtes invalides refusées sans modifier `Hero_01` |
| `RejectSecondCreation` | Deuxième création refusée sans remplacer le premier personnage |

Résultat attendu :

- 4 tests CC0 verts ;
- 3 tests CC1 verts ;
- 3 tests CC2 verts ;
- aucun test rouge ;
- aucune erreur d'ownership.

Les tests CC2 créent des DataAssets transitoires en mémoire. L'utilisation directe des deux `.uasset` du Content Browser sera raccordée par le widget CC3.

---

## 7. Étape D - Contrôle PIE

1. Lancer le PIE.
2. Vérifier que le jeu démarre normalement.
3. Ouvrir l'Inventaire.
4. Vérifier que `Hero_01` est toujours présent.
5. Ramasser puis équiper un objet.
6. Vérifier l'absence de duplication.

Le maintien de `Hero_01` est attendu : CC2 fournit l'API, mais ne l'appelle pas automatiquement.

---

## 8. Ce qui ne doit pas encore fonctionner

- aucun écran de création ;
- aucun blocage du mouvement au démarrage ;
- aucun appel automatique à `CreateInitialCharacter()` ;
- aucun remplacement de `Hero_01` en PIE ;
- aucune nouvelle statistique affichée dans l'Inventaire ;
- aucune sauvegarde du personnage.

Ces fonctions appartiennent aux tranches suivantes.

---

## 9. Validation humaine de CC2

CC2 est validée lorsque :

- le projet compile en **Development Editor Win64** ;
- les deux DataAssets CC1 sont toujours présents ;
- les dix tests CC0, CC1 et CC2 sont verts ;
- le test PIE ne révèle aucune régression ;
- aucune modification Blueprint temporaire n'a été nécessaire.

