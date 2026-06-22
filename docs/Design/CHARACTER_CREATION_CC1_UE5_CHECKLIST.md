# CC1 - Checklist de validation dans Unreal Engine 5

## 1. Objet

Ce document décrit uniquement les opérations humaines a effectuer dans Unreal Engine 5.5.4 après l'implémentation C++ de la tranche CC1.

La branche concernée est :

```text
codex/character-création-cc1-rpg-model
```

---

## 2. Répartition des responsabilités

| Domaine | ChatGPT / Codex | Intervention humaine |
|---|---|---|
| Architecture | Définit les structures, les DataAssets et les calculs | Valide les choix fonctionnels |
| C++ | Écrit les fichiers, la migration et les tests | Compile avec UnrealBuildTool et Visual Studio |
| Git | Cree et contrôle la branche | Récupère la branche et intègre les changements validés |
| DataAssets `.uasset` | Définit leurs classes et les valeurs attendues | Cree et renseigne les assets dans Unreal Editor |
| Tests Automation | Écrit les tests CC0 et CC1 | Exécute les tests dans Unreal Editor |
| PIE | Fournit le protocole de non-régression | Joue le protocole et observe le résultat |
| Diagnostic | Analyse les erreurs et prépare les corrections | Transmet les erreurs de compilation, logs ou captures |

Les fichiers `.uasset` sont des fichiers binaires contrôles par Unreal Editor. Ils ne doivent pas être fabriques par une modification textuelle Git.

---

## 3. Ce qui est déjà réalisé par ChatGPT / Codex

Vous ne devez pas recréer ces éléments manuellement :

- `FRPGAttributes` ;
- `FRPGDerivedStats` ;
- `URPGRaceAsset` ;
- `URPGClassAsset` ;
- `URPGCharacterRulesLibrary` ;
- intégration de `RaceId`, `Experience`, `Attributes`, `DerivedStats` et `Portrait` dans `FGridCharacterInventoryState` ;
- migration de l'ancienne propriété `Strength` ;
- calcul de charge depuis `Attributes.Strength` ;
- tests Automation CC1.

---

## 4. Étape A - Récupérer et compiler la branche

### A.1 Récupérer la branche

Depuis le dossier du projet :

```bash
git fetch origin
git switch codex/character-création-cc1-rpg-model
git pull
```

Vérifier ensuite que les dossiers suivants existent :

```text
Source/GrimrockPrototype/Public/RPG/
Source/GrimrockPrototype/Private/RPG/
Source/GrimrockPrototype/Private/Tests/
```

### A.2 Régénérer les fichiers Visual Studio si nécessaire

Cette étape est recommandée parce que CC1 ajoute plusieurs classes reflétées par UnrealHeaderTool.

1. Fermer Unreal Editor.
2. Cliquer avec le bouton droit sur `GrimrockPrototype.uproject`.
3. Choisir **Generate Visual Studio project files**.
4. Ouvrir `GrimrockPrototype.sln`.

Si la commande n'apparaît pas, lancer la génération avec votre installation UE 5.5 ou ouvrir directement le `.uproject`, qui peut proposer de reconstruire les modules.

### A.3 Compiler

Dans Visual Studio :

1. Configuration : **Development Editor**.
2. Plateforme : **Win64**.
3. Projet de démarrage : `GrimrockPrototype`.
4. Lancer **Build > Build Solution**.

Résultat attendu :

```text
Build succeeded
```

En cas d'erreur, ne pas modifier les nouveaux fichiers au hasard. Transmettre :

- la première erreur C++ ou UnrealHeaderTool ;
- environ 20 lignes avant et après cette erreur ;
- le nom du fichier et le numéro de ligne ;
- les erreurs suivantes seulement si elles semblent indépendantes.

---

## 5. Étape B - Créer les dossiers de contenu RPG

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

## 6. Étape C - Créer DA_Race_Human

Dans `Content/Grimrock/RPG/Races` :

1. Cliquer avec le bouton droit dans une zone vide.
2. Choisir **Miscellaneous > Data Asset**.
3. Sélectionner la classe `RPGRaceAsset`.
4. Nommer l'asset `DA_Race_Human`.
5. Ouvrir l'asset.
6. Renseigner les propriétés ci-dessous.

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

Contrôle visuel : les six bonus doivent valoir `1`. Ils ne doivent pas conserver la valeur par défaut d'une caractéristique normale.

---

## 7. Étape D - Créer DA_Class_Warrior

Dans `Content/Grimrock/RPG/Classes` :

1. Cliquer avec le bouton droit dans une zone vide.
2. Choisir **Miscellaneous > Data Asset**.
3. Sélectionner la classe `RPGClassAsset`.
4. Nommer l'asset `DA_Class_Warrior`.
5. Ouvrir l'asset.
6. Renseigner les propriétés ci-dessous.

| Propriete | Valeur |
|---|---|
| `ClassId` | `Warrior` |
| `DisplayName` | `Guerrier` |
| `Description` | `Combattant robuste spécialisé dans les armes et armures.` |
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

Profil combiné attendu après application des bonus humains :

| Caractéristique | Base guerrier | Bonus humain | Résultat |
|---|---:|---:|---:|
| Force | 15 | 1 | 16 |
| Dexterite | 11 | 1 | 12 |
| Constitution | 13 | 1 | 14 |
| Intelligence | 9 | 1 | 10 |
| Sagesse | 9 | 1 | 10 |
| Charisme | 9 | 1 | 10 |

Avec Constitution 14, le modificateur vaut `+2`. Le personnage possédera donc `20 PV` au niveau 1 : `18 + 2`.

---

## 8. Étape E - Exécuter les tests Automation

1. Ouvrir **Tools > Session Frontend**.
2. Ouvrir l'onglet **Automation**.
3. Attendre la fin de la découverte des tests.
4. Rechercher `Grimrock.CharacterCreation`.
5. Sélectionner les groupes `CC0` et `CC1`.
6. Lancer les tests.

Tests CC1 attendus :

| Test | Verification |
|---|---|
| `AttributeModifiers` | Formule `floor((caracteristique - 10) / 2)` |
| `HumanWarriorProfile` | Profil final, PV, mana et charge |
| `LegacyStrengthMigration` | Migration de l'ancienne Force vers `Attributes.Strength` |

Résultat attendu :

- les quatre tests CC0 restent verts ;
- les trois tests CC1 sont verts ;
- aucune erreur d'ownership n'apparaît.

Les tests utilisent des définitions transitoires creees en C++. Ils ne vérifient pas encore directement les deux fichiers `.uasset` crees dans l'éditeur. Leur raccordement au flux de création appartient a CC2.

---

## 9. Étape F - Test de non-régression en PIE

CC1 ne change pas encore l'écran de jeu. Effectuer seulement ce contrôle :

1. Lancer le PIE.
2. Ouvrir l'Inventaire.
3. Vérifier que `Hero_01` existe toujours.
4. Ramasser un objet.
5. Vérifier qu'il rejoint l'inventaire du personnage sélectionné.
6. Le prendre au curseur.
7. L'équiper en main.
8. Vérifier qu'il n'existe jamais simultanement dans deux emplacements.

Résultat attendu : aucune différence visible par rapport a CC0 et aucune régression de l'inventaire.

---

## 10. Ce qui ne doit pas encore fonctionner

Les comportements suivants sont prévus dans les tranches suivantes :

- aucun écran de création de personnage ;
- `Hero_01` n'est pas encore remplacé par un personnage nommé par le joueur ;
- les DataAssets Humain et Guerrier ne sont pas encore appliqués automatiquement au runtime ;
- les nouvelles caractéristiques ne sont pas encore affichées dans l'Inventaire ;
- aucun choix de race ou de classe ;
- aucune sauvegarde du personnage JdR.

Ne pas considérer ces absences comme des erreurs de CC1.

---

## 11. Validation humaine de CC1

CC1 est validée localement lorsque :

- la compilation **Development Editor Win64** réussit ;
- `DA_Race_Human` et `DA_Class_Warrior` existent aux emplacements prévus ;
- leurs valeurs correspondent exactement aux tableaux ;
- les sept tests CC0 et CC1 sont verts ;
- le test PIE de non-régression est concluant ;
- les deux nouveaux `.uasset` sont inclus dans le prochain commit ou la prochaine fusion.

