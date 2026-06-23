# CC6.2 - Portrait du personnage

## 1. Objet

CC6.2 ajoute un portrait sélectionnable à la création de personnage.

Le portrait choisi doit :

- être visible dans `WBP_CharacterCreation` pendant l'aperçu ;
- être transmis à `CreateInitialCharacter` ;
- être affiché dans la fiche centrale de l'inventaire ;
- être conservé par la sauvegarde CC5 et restauré en mode `Continue`.

Cette tranche ne traite pas encore la répartition de points, l'équipement initial, les compétences, les dons ou les sorts.

---

## 2. Répartition des responsabilités

| Responsable | Travail CC6.2 |
|---|---|
| ChatGPT / Codex | Type `FRPGCharacterPortraitOption`, ComboBox native optionnelle, transmission du portrait, test de persistance, documentation |
| Utilisateur dans UE5 | Importer ou créer les textures de portrait, configurer `WBP_CharacterCreation`, vérifier le rendu PIE |
| Hors CC6.2 | Génération automatique de portraits, portraits par sexe/origine, équipement initial, points libres, compétences, dons et sorts |

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

## 4. Étape B - Préparer les textures de portrait

Créer ou importer quelques textures dans un dossier dédié, par exemple :

```text
Content/GrimrockPrototype/UI/Portraits/
```

Noms recommandés pour commencer :

| Asset | Usage |
|---|---|
| `T_Portrait_HumanWarrior` | portrait par défaut Humain / Guerrier |
| `T_Portrait_ElfMage` | validation Elfe / Mage |
| `T_Portrait_DwarfWarrior` | variation robuste |
| `T_Portrait_Rogue` | variation voleur |

Paramètres conseillés :

- texture carrée, par exemple `256x256` ou `512x512` ;
- compression UI ou réglage équivalent pour éviter un flou excessif ;
- nom lisible dans le Content Browser ;
- éviter les textures trop sombres tant que le cadre d'inventaire est sombre.

---

## 5. Étape C - Modifier `WBP_CharacterCreation`

### C.1 Ajouter la ComboBox de portrait

Dans le Designer, ajouter une **ComboBox String** :

| Nom exact | Type | Is Variable |
|---|---|---|
| `ComboBox_Portrait` | ComboBox String | activé |

Ne renseigner aucune option manuellement dans le Designer. Le C++ remplit la liste à partir de `AvailablePortraits`.

Ne créer aucun événement `On Selection Changed` dans le Graph.

Cette ComboBox utilise `BindWidgetOptional` : l'absence du widget ne bloque pas la compilation, mais elle empêche la sélection manuelle du portrait.

### C.2 Vérifier l'image d'aperçu

Le widget existant doit conserver ou ajouter :

| Nom exact | Type | Is Variable |
|---|---|---|
| `Image_Portrait` | Image | activé |

Le C++ met à jour cette image avec le portrait choisi.

### C.3 Ajouter une description facultative

Ajouter éventuellement un **Text Block** :

| Nom exact | Type | Is Variable |
|---|---|---|
| `Text_PortraitDescription` | Text Block | activé |

Ce champ est optionnel. Il affiche la description de l'option sélectionnée si elle existe.

---

## 6. Étape D - Configurer `AvailablePortraits`

Dans `WBP_CharacterCreation`, ouvrir **Class Defaults > RPG > Character Creation > Choices**.

Renseigner `AvailablePortraits` avec quelques entrées :

| PortraitId | DisplayName | Portrait | Description |
|---|---|---|---|
| `HumanWarrior` | `Humain guerrier` | `T_Portrait_HumanWarrior` | Portrait par défaut du prototype. |
| `ElfMage` | `Elfe mage` | `T_Portrait_ElfMage` | Profil agile et orienté magie. |
| `DwarfWarrior` | `Nain guerrier` | `T_Portrait_DwarfWarrior` | Profil robuste et défensif. |
| `Rogue` | `Voleur` | `T_Portrait_Rogue` | Profil discret et mobile. |

Règles :

- `PortraitId` ne doit pas être vide ;
- `Portrait` doit pointer vers une texture valide ;
- `DisplayName` peut être vide, mais il est préférable de le renseigner ;
- si `DefaultPortrait` est vide, le premier portrait valide de `AvailablePortraits` devient le portrait par défaut.

---

## 7. Étape E - Vérifier l'inventaire

Dans `WBP_GridInventory`, vérifier que l'image de fiche centrale existe :

| Nom exact | Type | Is Variable |
|---|---|---|
| `Image_CharacterPortrait` | Image | activé |

Le code CC4 affiche déjà `Summary.Portrait` dans cette image. Aucun calcul Blueprint n'est nécessaire.

---

## 8. Étape F - Tests Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Exécuter les seize tests :

- les treize tests CC0 à CC5 ;
- `Grimrock.CharacterCreation.CC6.AllRaceClassCombinations` ;
- `Grimrock.CharacterCreation.CC6.ElfMageCreation` ;
- `Grimrock.CharacterCreation.CC6.PortraitSelectionPersists`.

Les seize tests doivent être verts.

---

## 9. Étape G - Validation PIE

### G.1 Création nouvelle partie

1. Régler `PartyStartupMode = NewGame`.
2. Lancer le PIE.
3. Vérifier que `ComboBox_Portrait` contient les portraits configurés.
4. Sélectionner `Elfe mage`.
5. Vérifier que `Image_Portrait` change immédiatement.
6. Saisir `Aelwen`.
7. Choisir `Elfe` et `Mage`.
8. Cliquer sur **Créer le personnage**.
9. Ouvrir l'inventaire.
10. Vérifier que `Image_CharacterPortrait` affiche le même portrait.

### G.2 Continuer la partie

1. Arrêter le PIE après création du personnage.
2. Régler `PartyStartupMode = Continue`.
3. Relancer le PIE.
4. Ouvrir l'inventaire.
5. Vérifier que le portrait est toujours affiché dans la fiche centrale.

---

## 10. Critère de validation

CC6.2 est validée lorsque :

- la compilation C++ réussit ;
- les seize tests `Grimrock.CharacterCreation` sont verts ;
- le portrait sélectionné s'affiche dans la création ;
- le même portrait s'affiche dans l'inventaire ;
- le portrait reste présent après sauvegarde et chargement ;
- aucun Graph Blueprint ne calcule ou ne copie manuellement le portrait.
