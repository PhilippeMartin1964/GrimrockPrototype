# CC2 - Checklist de validation dans Unreal Engine 5

## 1. Objet

Ce document decrit les operations humaines necessaires pour valider la tranche CC2 dans Unreal Engine 5.5.4.

Branche :

```text
codex/character-creation-cc2-creation-api
```

CC2 ajoute l'API C++ de creation initiale. Cette API est utilisable depuis Blueprint, mais aucun widget ne l'appelle encore.

---

## 2. Repartition des responsabilites

| Responsable | Travail CC2 |
|---|---|
| ChatGPT / Codex | Requete de creation, validation, mutation atomique, restauration en cas d'erreur, tests et documentation |
| Utilisateur | Recuperation de la branche, compilation UE5, execution des tests Automation et controle PIE |
| CC3 | Creation du widget et appel Blueprint de l'API |

Vous ne devez modifier aucun Blueprint pour valider CC2.

---

## 3. Ce qui est implemente

- `FRPGCharacterCreationRequest` contient le nom, la race, la classe et le portrait optionnel ;
- `bInitialCharacterCreationCompleted` distingue le placeholder du personnage finalise ;
- `HasCompletedInitialCharacterCreation()` expose cet etat ;
- `CreateInitialCharacter()` valide la requete avant toute mutation ;
- une creation valide remplace le placeholder par exactement un personnage ;
- le personnage cree possede un equipement vide, 40 slots et un curseur vide ;
- les caracteristiques proviennent de la classe et des bonus raciaux ;
- les PV, la mana et la charge sont calcules par les regles CC1 ;
- une seconde creation est refusee ;
- si le diagnostic d'ownership echoue, l'ancien etat du groupe est restaure.

---

## 4. Etape A - Recuperer et compiler

```bash
git fetch origin
git switch codex/character-creation-cc2-creation-api
git pull
```

Fermer Unreal Editor avant la compilation si des modules C++ sont deja charges.

Dans Visual Studio :

1. choisir **Development Editor** ;
2. choisir **Win64** ;
3. compiler `GrimrockPrototype` ou la solution complete ;
4. rouvrir le projet apres un `Build succeeded`.

Regenerer les fichiers Visual Studio depuis le `.uproject` uniquement si les nouveaux fichiers ne sont pas detectes.

En cas d'erreur, transmettre d'abord la premiere erreur UnrealHeaderTool ou C++, avec son fichier, sa ligne et le contexte du journal.

---

## 5. Etape B - Verifier les DataAssets CC1

Verifier uniquement que les assets suivants existent toujours :

```text
Content/Grimrock/RPG/Races/DA_Race_Human
Content/Grimrock/RPG/Classes/DA_Class_Warrior
```

Aucune nouvelle valeur ne doit etre ajoutee pour CC2.

Si ces assets sont encore des changements Git locaux non valides, ne pas les supprimer lors du changement de branche. Ils devront etre inclus lors de l'integration finale de CC1/CC2.

---

## 6. Etape C - Executer les tests Automation

1. Ouvrir **Tools > Session Frontend**.
2. Ouvrir **Automation**.
3. Rechercher `Grimrock.CharacterCreation`.
4. Selectionner CC0, CC1 et CC2.
5. Lancer les dix tests.

Tests CC2 :

| Test | Verification |
|---|---|
| `CreateInitialCharacter` | Creation complete et coherente d'Elias |
| `RejectInvalidRequestAtomically` | Requetes invalides refusees sans modifier `Hero_01` |
| `RejectSecondCreation` | Deuxieme creation refusee sans remplacer le premier personnage |

Resultat attendu :

- 4 tests CC0 verts ;
- 3 tests CC1 verts ;
- 3 tests CC2 verts ;
- aucun test rouge ;
- aucune erreur d'ownership.

Les tests CC2 creent des DataAssets transitoires en memoire. L'utilisation directe des deux `.uasset` du Content Browser sera raccordee par le widget CC3.

---

## 7. Etape D - Controle PIE

1. Lancer le PIE.
2. Verifier que le jeu demarre normalement.
3. Ouvrir l'Inventaire.
4. Verifier que `Hero_01` est toujours present.
5. Ramasser puis equiper un objet.
6. Verifier l'absence de duplication.

Le maintien de `Hero_01` est attendu : CC2 fournit l'API, mais ne l'appelle pas automatiquement.

---

## 8. Ce qui ne doit pas encore fonctionner

- aucun ecran de creation ;
- aucun blocage du mouvement au demarrage ;
- aucun appel automatique a `CreateInitialCharacter()` ;
- aucun remplacement de `Hero_01` en PIE ;
- aucune nouvelle statistique affichee dans l'Inventaire ;
- aucune sauvegarde du personnage.

Ces fonctions appartiennent aux tranches suivantes.

---

## 9. Validation humaine de CC2

CC2 est validee lorsque :

- le projet compile en **Development Editor Win64** ;
- les deux DataAssets CC1 sont toujours presents ;
- les dix tests CC0, CC1 et CC2 sont verts ;
- le test PIE ne revele aucune regression ;
- aucune modification Blueprint temporaire n'a ete necessaire.

