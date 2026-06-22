# CC6.1 - Choix de la race et de la classe

## 1. Objet

CC6.1 ouvre la création de personnage aux six races et six classes du prototype défini dans `Docs/Rules/RPG_Core_Rules_v0_1.md`.

Cette tranche ajoute uniquement la sélection de la race et de la classe. Les portraits, la répartition de points, l'équipement initial, les compétences, les dons et les sorts seront traités dans les sous-tranches suivantes.

Aucun calcul ne doit être ajouté dans le Graph de `WBP_CharacterCreation`.

---

## 2. Répartition des responsabilités

| Responsable | Travail CC6.1 |
|---|---|
| ChatGPT / Codex | Tableaux de DataAssets, ComboBox natives, recalcul de l'aperçu, validation des 36 combinaisons, tests et documentation |
| Utilisateur dans UE5 | Créer dix nouveaux DataAssets, compléter le Widget Blueprint et valider le PIE |
| Hors CC6.1 | Portraits, points libres, équipement initial, compétences, dons et sorts |

---

## 3. Étape A - Récupérer et compiler

```bash
git fetch origin
git switch codex/character-creation-cc3-startup-widget
git pull
```

Fermer Unreal Editor, puis compiler :

- configuration **Development Editor** ;
- plateforme **Win64** ;
- cible `GrimrockPrototypeEditor`.

Résultat attendu : `Build succeeded`.

---

## 4. Étape B - Créer les races

Créer les cinq nouveaux `RPG Race Asset` dans le même dossier que `DA_Race_Human`.

| Asset | RaceId | DisplayName | FOR | DEX | CON | INT | SAG | CHA |
|---|---|---|---:|---:|---:|---:|---:|---:|
| `DA_Race_Dwarf` | `Dwarf` | `Nain` | 1 | 0 | 2 | 0 | 0 | 0 |
| `DA_Race_Elf` | `Elf` | `Elfe` | 0 | 2 | 0 | 1 | 0 | 0 |
| `DA_Race_Halfling` | `Halfling` | `Halfelin` | 0 | 2 | 0 | 0 | 0 | 1 |
| `DA_Race_Gnome` | `Gnome` | `Gnome` | 0 | 0 | 1 | 2 | 0 | 0 |
| `DA_Race_HalfOrc` | `HalfOrc` | `Demi-orc` | 2 | 0 | 1 | 0 | 0 | 0 |

Conserver `DA_Race_Human` :

| RaceId | DisplayName | Bonus |
|---|---|---|
| `Human` | `Humain` | +1 aux six caractéristiques |

Descriptions recommandées :

| Race | Description |
|---|---|
| Humain | Polyvalent et adaptable. |
| Nain | Robuste, défensif et résistant. |
| Elfe | Agile, perceptif et proche de la magie. |
| Halfelin | Discret, chanceux et mobile. |
| Gnome | Intellectuel, inventif et versé dans l'alchimie. |
| Demi-orc | Puissant, brutal et intimidant. |

---

## 5. Étape C - Créer les classes

Créer les cinq nouveaux `RPG Class Asset` dans le même dossier que `DA_Class_Warrior`.

### C.1 Caractéristiques et ressources

| Asset | ClassId | DisplayName | FOR | DEX | CON | INT | SAG | CHA | PV niv. 1 | PV/niv. | Mana niv. 1 | Mana/niv. |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| `DA_Class_Rogue` | `Rogue` | `Voleur` | 9 | 15 | 10 | 13 | 9 | 10 | 14 | 6 | 0 | 0 |
| `DA_Class_Ranger` | `Ranger` | `Rôdeur` | 11 | 15 | 12 | 9 | 11 | 8 | 16 | 7 | 0 | 0 |
| `DA_Class_Mage` | `Mage` | `Mage` | 8 | 12 | 10 | 15 | 12 | 9 | 8 | 4 | 18 | 8 |
| `DA_Class_Priest` | `Priest` | `Prêtre` | 10 | 9 | 13 | 9 | 15 | 10 | 12 | 6 | 16 | 7 |
| `DA_Class_Alchemist` | `Alchemist` | `Alchimiste` | 9 | 13 | 12 | 15 | 9 | 8 | 12 | 5 | 10 | 5 |

Conserver les valeurs déjà validées de `DA_Class_Warrior`.

Pour toutes les nouvelles classes :

- `BasePhysicalArmor = 0` ;
- `BaseMagicalArmor = 0`.

Descriptions recommandées :

| Classe | Description |
|---|---|
| Guerrier | Combattant robuste spécialisé dans les armes lourdes et la défense. |
| Voleur | Expert des attaques ciblées, pièges et serrures. |
| Rôdeur | Combattant à distance spécialisé dans la survie et l'exploration. |
| Mage | Lanceur de sorts offensifs et de contrôle élémentaire. |
| Prêtre | Soutien spécialisé dans les soins et la protection. |
| Alchimiste | Expert des potions, bombes et altérations. |

---

## 6. Étape D - Modifier WBP_CharacterCreation

### D.1 Ajouter les ComboBox

Dans le Designer, remplacer les valeurs statiques de race et de classe ou les placer à côté de deux **ComboBox String** :

| Nom exact | Type | Is Variable |
|---|---|---|
| `ComboBox_Race` | ComboBox String | activé |
| `ComboBox_Class` | ComboBox String | activé |

Ne renseigner aucune option manuellement dans le Designer. Le C++ remplit les listes.

Ne créer aucun événement `On Selection Changed` dans le Graph.

### D.2 Ajouter les descriptions facultatives

Ajouter deux **Text Block** :

| Nom exact | Type |
|---|---|
| `Text_RaceDescription` | Text Block |
| `Text_ClassDescription` | Text Block |

Cocher **Is Variable** et ne créer aucun binding Blueprint.

Ces widgets utilisent `BindWidgetOptional` : ils peuvent être omis sans bloquer CC6.1.

### D.3 Configurer Class Defaults

Dans **Class Defaults > RPG > Character Creation** :

| Propriété | Valeur |
|---|---|
| `RaceDefinition` | `DA_Race_Human` |
| `ClassDefinition` | `DA_Class_Warrior` |

Dans `AvailableRaceDefinitions`, ajouter dans cet ordre :

1. `DA_Race_Human` ;
2. `DA_Race_Dwarf` ;
3. `DA_Race_Elf` ;
4. `DA_Race_Halfling` ;
5. `DA_Race_Gnome` ;
6. `DA_Race_HalfOrc`.

Dans `AvailableClassDefinitions`, ajouter dans cet ordre :

1. `DA_Class_Warrior` ;
2. `DA_Class_Rogue` ;
3. `DA_Class_Ranger` ;
4. `DA_Class_Mage` ;
5. `DA_Class_Priest` ;
6. `DA_Class_Alchemist`.

Compiler et enregistrer `WBP_CharacterCreation`.

---

## 7. Étape E - Tests Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Exécuter les quinze tests :

- les treize tests CC0 à CC5 ;
- `Grimrock.CharacterCreation.CC6.AllRaceClassCombinations` ;
- `Grimrock.CharacterCreation.CC6.ElfMageCreation`.

Les quinze tests doivent être verts.

---

## 8. Étape F - Validation PIE

### F.1 Valeurs par défaut

1. Régler `PartyStartupMode = NewGame`.
2. Lancer le PIE.
3. Vérifier que les ComboBox sélectionnent `Humain` et `Guerrier`.
4. Vérifier les valeurs déjà validées : `16 / 12 / 14 / 10 / 10 / 10`, PV `20`, mana `0`, charge `80`.

### F.2 Changement de sélection

1. Sélectionner `Elfe`.
2. Sélectionner `Mage`.
3. Vérifier immédiatement :

```text
FOR 8
DEX 14
CON 10
INT 16
SAG 12
CHA 9
PV 8
Mana 18
Charge 40
```

4. Saisir `Aelwen`.
5. Créer le personnage.
6. Ouvrir l'Inventaire.
7. Vérifier `Aelwen / Elfe / Mage / niveau 1` et les mêmes valeurs.
8. Arrêter le PIE.
9. Régler `PartyStartupMode = Continue`.
10. Relancer le PIE et vérifier que le personnage Elfe / Mage est restauré.

---

## 9. Critère de validation

CC6.1 est validée lorsque :

- les quinze tests sont verts ;
- les deux ComboBox contiennent six options ;
- chaque changement recalcule l'aperçu ;
- Elfe / Mage produit les valeurs attendues ;
- la création et la sauvegarde utilisent les définitions sélectionnées ;
- aucun Graph Blueprint ne calcule les caractéristiques.
