# CC1 - Checklist de validation dans Unreal Engine 5

## 1. Objet

Ce document decrit uniquement les operations humaines a effectuer dans Unreal Engine 5.5.4 apres l'implementation C++ de la tranche CC1.

La branche concernee est :

```text
codex/character-creation-cc1-rpg-model
```

---

## 2. Repartition des responsabilites

| Domaine | ChatGPT / Codex | Intervention humaine |
|---|---|---|
| Architecture | Definit les structures, les DataAssets et les calculs | Valide les choix fonctionnels |
| C++ | Ecrit les fichiers, la migration et les tests | Compile avec UnrealBuildTool et Visual Studio |
| Git | Cree et controle la branche | Recupere la branche et integre les changements valides |
| DataAssets `.uasset` | Definit leurs classes et les valeurs attendues | Cree et renseigne les assets dans Unreal Editor |
| Tests Automation | Ecrit les tests CC0 et CC1 | Execute les tests dans Unreal Editor |
| PIE | Fournit le protocole de non-regression | Joue le protocole et observe le resultat |
| Diagnostic | Analyse les erreurs et prepare les corrections | Transmet les erreurs de compilation, logs ou captures |

Les fichiers `.uasset` sont des fichiers binaires controles par Unreal Editor. Ils ne doivent pas etre fabriques par une modification textuelle Git.

---

## 3. Ce qui est deja realise par ChatGPT / Codex

Vous ne devez pas recreer ces elements manuellement :

- `FRPGAttributes` ;
- `FRPGDerivedStats` ;
- `URPGRaceAsset` ;
- `URPGClassAsset` ;
- `URPGCharacterRulesLibrary` ;
- integration de `RaceId`, `Experience`, `Attributes`, `DerivedStats` et `Portrait` dans `FGridCharacterInventoryState` ;
- migration de l'ancienne propriete `Strength` ;
- calcul de charge depuis `Attributes.Strength` ;
- tests Automation CC1.

---

## 4. Etape A - Recuperer et compiler la branche

### A.1 Recuperer la branche

Depuis le dossier du projet :

```bash
git fetch origin
git switch codex/character-creation-cc1-rpg-model
git pull
```

Verifier ensuite que les dossiers suivants existent :

```text
Source/GrimrockPrototype/Public/RPG/
Source/GrimrockPrototype/Private/RPG/
Source/GrimrockPrototype/Private/Tests/
```

### A.2 Regenerer les fichiers Visual Studio si necessaire

Cette etape est recommandee parce que CC1 ajoute plusieurs classes refletees par UnrealHeaderTool.

1. Fermer Unreal Editor.
2. Cliquer avec le bouton droit sur `GrimrockPrototype.uproject`.
3. Choisir **Generate Visual Studio project files**.
4. Ouvrir `GrimrockPrototype.sln`.

Si la commande n'apparait pas, lancer la generation avec votre installation UE 5.5 ou ouvrir directement le `.uproject`, qui peut proposer de reconstruire les modules.

### A.3 Compiler

Dans Visual Studio :

1. Configuration : **Development Editor**.
2. Plateforme : **Win64**.
3. Projet de demarrage : `GrimrockPrototype`.
4. Lancer **Build > Build Solution**.

Resultat attendu :

```text
Build succeeded
```

En cas d'erreur, ne pas modifier les nouveaux fichiers au hasard. Transmettre :

- la premiere erreur C++ ou UnrealHeaderTool ;
- environ 20 lignes avant et apres cette erreur ;
- le nom du fichier et le numero de ligne ;
- les erreurs suivantes seulement si elles semblent independantes.

---

## 5. Etape B - Creer les dossiers de contenu RPG

Dans le **Content Drawer** :

1. Ouvrir `Content/Grimrock`.
2. Creer le dossier `RPG`.
3. Dans `RPG`, creer `Races`.
4. Dans `RPG`, creer `Classes`.

Arborescence attendue :

```text
Content/Grimrock/RPG/
|-- Races/
`-- Classes/
```

---

## 6. Etape C - Creer DA_Race_Human

Dans `Content/Grimrock/RPG/Races` :

1. Cliquer avec le bouton droit dans une zone vide.
2. Choisir **Miscellaneous > Data Asset**.
3. Selectionner la classe `RPGRaceAsset`.
4. Nommer l'asset `DA_Race_Human`.
5. Ouvrir l'asset.
6. Renseigner les proprietes ci-dessous.

| Propriete | Valeur |
|---|---|
| `RaceId` | `Human` |
| `DisplayName` | `Humain` |
| `Description` | `Race polyvalente et adaptable.` |
| `AttributeBonuses.Strength` | 1 |
| `AttributeBonuses.Dexterity` | 1 |
| `AttributeBonuses.Constitution` | 1 |
| `AttributeBonuses.Intelligence` | 1 |
| `AttributeBonuses.Wisdom` | 1 |
| `AttributeBonuses.Charisma` | 1 |

Enregistrer l'asset.

Controle visuel : les six bonus doivent valoir `1`. Ils ne doivent pas conserver la valeur par defaut d'une caracteristique normale.

---

## 7. Etape D - Creer DA_Class_Warrior

Dans `Content/Grimrock/RPG/Classes` :

1. Cliquer avec le bouton droit dans une zone vide.
2. Choisir **Miscellaneous > Data Asset**.
3. Selectionner la classe `RPGClassAsset`.
4. Nommer l'asset `DA_Class_Warrior`.
5. Ouvrir l'asset.
6. Renseigner les proprietes ci-dessous.

| Propriete | Valeur |
|---|---|
| `ClassId` | `Warrior` |
| `DisplayName` | `Guerrier` |
| `Description` | `Combattant robuste specialise dans les armes et armures.` |
| `BaseAttributes.Strength` | 15 |
| `BaseAttributes.Dexterity` | 11 |
| `BaseAttributes.Constitution` | 13 |
| `BaseAttributes.Intelligence` | 9 |
| `BaseAttributes.Wisdom` | 9 |
| `BaseAttributes.Charisma` | 9 |
| `HealthAtLevelOne` | 18 |
| `HealthPerLevel` | 8 |
| `ManaAtLevelOne` | 0 |
| `ManaPerLevel` | 0 |
| `BasePhysicalArmor` | 0 |
| `BaseMagicalArmor` | 0 |

Enregistrer l'asset, puis utiliser **Save All**.

Profil combine attendu apres application des bonus humains :

| Caracteristique | Base guerrier | Bonus humain | Resultat |
|---|---:|---:|---:|
| Force | 15 | 1 | 16 |
| Dexterite | 11 | 1 | 12 |
| Constitution | 13 | 1 | 14 |
| Intelligence | 9 | 1 | 10 |
| Sagesse | 9 | 1 | 10 |
| Charisme | 9 | 1 | 10 |

Avec Constitution 14, le modificateur vaut `+2`. Le personnage possedera donc `20 PV` au niveau 1 : `18 + 2`.

---

## 8. Etape E - Executer les tests Automation

1. Ouvrir **Tools > Session Frontend**.
2. Ouvrir l'onglet **Automation**.
3. Attendre la fin de la decouverte des tests.
4. Rechercher `Grimrock.CharacterCreation`.
5. Selectionner les groupes `CC0` et `CC1`.
6. Lancer les tests.

Tests CC1 attendus :

| Test | Verification |
|---|---|
| `AttributeModifiers` | Formule `floor((caracteristique - 10) / 2)` |
| `HumanWarriorProfile` | Profil final, PV, mana et charge |
| `LegacyStrengthMigration` | Migration de l'ancienne Force vers `Attributes.Strength` |

Resultat attendu :

- les quatre tests CC0 restent verts ;
- les trois tests CC1 sont verts ;
- aucune erreur d'ownership n'apparait.

Les tests utilisent des definitions transitoires creees en C++. Ils ne verifient pas encore directement les deux fichiers `.uasset` crees dans l'editeur. Leur raccordement au flux de creation appartient a CC2.

---

## 9. Etape F - Test de non-regression en PIE

CC1 ne change pas encore l'ecran de jeu. Effectuer seulement ce controle :

1. Lancer le PIE.
2. Ouvrir l'Inventaire.
3. Verifier que `Hero_01` existe toujours.
4. Ramasser un objet.
5. Verifier qu'il rejoint l'inventaire du personnage selectionne.
6. Le prendre au curseur.
7. L'equiper en main.
8. Verifier qu'il n'existe jamais simultanement dans deux emplacements.

Resultat attendu : aucune difference visible par rapport a CC0 et aucune regression de l'inventaire.

---

## 10. Ce qui ne doit pas encore fonctionner

Les comportements suivants sont prevus dans les tranches suivantes :

- aucun ecran de creation de personnage ;
- `Hero_01` n'est pas encore remplace par un personnage nomme par le joueur ;
- les DataAssets Humain et Guerrier ne sont pas encore appliques automatiquement au runtime ;
- les nouvelles caracteristiques ne sont pas encore affichees dans l'Inventaire ;
- aucun choix de race ou de classe ;
- aucune sauvegarde du personnage JdR.

Ne pas considerer ces absences comme des erreurs de CC1.

---

## 11. Validation humaine de CC1

CC1 est validee localement lorsque :

- la compilation **Development Editor Win64** reussit ;
- `DA_Race_Human` et `DA_Class_Warrior` existent aux emplacements prevus ;
- leurs valeurs correspondent exactement aux tableaux ;
- les sept tests CC0 et CC1 sont verts ;
- le test PIE de non-regression est concluant ;
- les deux nouveaux `.uasset` sont inclus dans le prochain commit ou la prochaine fusion.

