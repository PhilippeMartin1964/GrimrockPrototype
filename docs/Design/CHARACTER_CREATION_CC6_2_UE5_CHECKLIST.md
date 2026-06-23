# CC6.2 - Portrait du personnage

## 1. Objet

CC6.2 ajoute un portrait sélectionnable à la création de personnage.

Cette tranche est volontairement une première marche technique : elle vérifie que le portrait circule correctement entre la création, l'inventaire et la sauvegarde.

La décision de direction visuelle est maintenant documentée dans :

```text
docs/Design/CHARACTER_CREATION_VISUAL_IDENTITY_ROADMAP.md
```

À terme, le portrait final ne sera pas une image unique par combinaison race + classe. Le modèle cible est :

```text
portrait principal race + genre
+
icône de classe en surimpression
```

---

## 2. Répartition des responsabilités

| Responsable | Travail CC6.2 |
|---|---|
| ChatGPT / Codex | Type `FRPGCharacterPortraitOption`, ComboBox native optionnelle, transmission du portrait, test de persistance, documentation |
| Utilisateur dans UE5 | Importer ou créer quelques textures de portrait temporaires, configurer `WBP_CharacterCreation`, vérifier le rendu PIE |
| Hors CC6.2 | Filtrage race + genre, icônes de classe, composition finale, variantes multiples et polissage visuel |

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

## 4. Étape B - Préparer les textures de portrait CC6.2

Pour CC6.2, quelques textures suffisent. Elles peuvent déjà suivre la convention cible race + genre, même si le filtrage automatique arrivera plus tard.

Créer ou importer quelques textures dans un dossier dédié, par exemple :

```text
Content/GrimrockPrototype/UI/Portraits/Races/
```

Noms recommandés :

| Asset | Usage |
|---|---|
| `T_Portrait_Human_Male_01` | portrait par défaut |
| `T_Portrait_Human_Female_01` | validation genre féminin plus tard |
| `T_Portrait_Elf_Male_01` | variation Elfe |
| `T_Portrait_Elf_Female_01` | validation Elfe / Mage |
| `T_Portrait_Dwarf_Male_01` | variation robuste |
| `T_Portrait_Dwarf_Female_01` | variation robuste |

Paramètres conseillés :

- texture carrée, par exemple `256x256` ou `512x512` ;
- compression UI ou réglage équivalent pour éviter un flou excessif ;
- nom lisible dans le Content Browser ;
- éviter les textures trop sombres tant que le cadre d'inventaire est sombre.

---

## 5. Étape C - Préparer le futur set d'icônes de classe

Le code CC6.2 ne consomme pas encore les icônes de classe, mais la roadmap cible les prévoit.

Dossier recommandé :

```text
Content/GrimrockPrototype/UI/Portraits/ClassIcons/
```

Icônes à produire dans une tranche suivante :

| Classe | Asset cible |
|---|---|
| Guerrier | `T_ClassIcon_Warrior` |
| Voleur | `T_ClassIcon_Rogue` |
| Rôdeur | `T_ClassIcon_Ranger` |
| Mage | `T_ClassIcon_Mage` |
| Prêtre | `T_ClassIcon_Priest` |
| Alchimiste | `T_ClassIcon_Alchemist` |

Ces icônes doivent être lisibles en petit format pour fonctionner en surimpression sur le portrait.

---

## 6. Étape D - Modifier `WBP_CharacterCreation`

### D.1 Ajouter la ComboBox de portrait

Dans le Designer, ajouter une **ComboBox String** :

| Nom exact | Type | Is Variable |
|---|---|---|
| `ComboBox_Portrait` | ComboBox String | activé |

Ne renseigner aucune option manuellement dans le Designer. Le C++ remplit la liste à partir de `AvailablePortraits`.

Ne créer aucun événement `On Selection Changed` dans le Graph.

Cette ComboBox utilise `BindWidgetOptional` : l'absence du widget ne bloque pas la compilation, mais elle empêche la sélection manuelle du portrait.

### D.2 Vérifier l'image d'aperçu

Le widget existant doit conserver ou ajouter :

| Nom exact | Type | Is Variable |
|---|---|---|
| `Image_Portrait` | Image | activé |

Le C++ met à jour cette image avec le portrait choisi.

### D.3 Ajouter une description facultative

Ajouter éventuellement un **Text Block** :

| Nom exact | Type | Is Variable |
|---|---|---|
| `Text_PortraitDescription` | Text Block | activé |

Ce champ est optionnel. Il affiche la description de l'option sélectionnée si elle existe.

---

## 7. Étape E - Configurer `AvailablePortraits`

Dans `WBP_CharacterCreation`, ouvrir **Class Defaults > RPG > Character Creation > Choices**.

Renseigner `AvailablePortraits` avec des entrées temporaires mais nommées selon le modèle cible :

| PortraitId | DisplayName | Portrait | Description |
|---|---|---|---|
| `Human_Male_01` | `Humain masculin 01` | `T_Portrait_Human_Male_01` | Portrait humain masculin de base. |
| `Human_Female_01` | `Humain féminin 01` | `T_Portrait_Human_Female_01` | Portrait humain féminin de base. |
| `Elf_Male_01` | `Elfe masculin 01` | `T_Portrait_Elf_Male_01` | Portrait elfe masculin de base. |
| `Elf_Female_01` | `Elfe féminin 01` | `T_Portrait_Elf_Female_01` | Portrait elfe féminin de base. |
| `Dwarf_Male_01` | `Nain masculin 01` | `T_Portrait_Dwarf_Male_01` | Portrait nain masculin de base. |
| `Dwarf_Female_01` | `Nain féminin 01` | `T_Portrait_Dwarf_Female_01` | Portrait nain féminin de base. |

Règles :

- `PortraitId` ne doit pas être vide ;
- `Portrait` doit pointer vers une texture valide ;
- `DisplayName` peut être vide, mais il est préférable de le renseigner ;
- si `DefaultPortrait` est vide, le premier portrait valide de `AvailablePortraits` devient le portrait par défaut.

Cette structure reste transitoire. La tranche suivante remplacera cette liste plate par des DataAssets de portraits filtrés par race et genre.

---

## 8. Étape F - Vérifier l'inventaire

Dans `WBP_GridInventory`, vérifier que l'image de fiche centrale existe :

| Nom exact | Type | Is Variable |
|---|---|---|
| `Image_CharacterPortrait` | Image | activé |

Le code CC4 affiche déjà `Summary.Portrait` dans cette image. Aucun calcul Blueprint n'est nécessaire.

L'icône de classe en surimpression n'est pas encore dans CC6.2. Elle est prévue par la roadmap visuelle.

---

## 9. Étape G - Changements C++ prévus après CC6.2

Les changements C++ suivants ne sont pas demandés pour valider CC6.2, mais ils structurent la suite :

- ajouter un enum de genre ou type de portrait ;
- remplacer la liste plate par `URPGCharacterPortraitSetAsset` ;
- ajouter `URPGClassVisualAsset` pour les icônes de classe ;
- exposer l'icône de classe dans le résumé d'inventaire ;
- ajouter `Image_ClassIcon` dans la création ;
- ajouter `Image_CharacterClassIcon` dans l'inventaire ;
- sauvegarder et restaurer la sélection visuelle complète.

La proposition détaillée est dans `CHARACTER_CREATION_VISUAL_IDENTITY_ROADMAP.md`.

---

## 10. Étape H - Tests Automation

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

## 11. Étape I - Validation PIE

### I.1 Création nouvelle partie

1. Régler `PartyStartupMode = NewGame`.
2. Lancer le PIE.
3. Vérifier que `ComboBox_Portrait` contient les portraits configurés.
4. Sélectionner `Elfe féminin 01`.
5. Vérifier que `Image_Portrait` change immédiatement.
6. Saisir `Aelwen`.
7. Choisir `Elfe` et `Mage`.
8. Cliquer sur **Créer le personnage**.
9. Ouvrir l'inventaire.
10. Vérifier que `Image_CharacterPortrait` affiche le même portrait.

### I.2 Continuer la partie

1. Arrêter le PIE après création du personnage.
2. Régler `PartyStartupMode = Continue`.
3. Relancer le PIE.
4. Ouvrir l'inventaire.
5. Vérifier que le portrait est toujours affiché dans la fiche centrale.

---

## 12. Critère de validation

CC6.2 est validée lorsque :

- la compilation C++ réussit ;
- les seize tests `Grimrock.CharacterCreation` sont verts ;
- le portrait sélectionné s'affiche dans la création ;
- le même portrait s'affiche dans l'inventaire ;
- le portrait reste présent après sauvegarde et chargement ;
- aucun Graph Blueprint ne calcule ou ne copie manuellement le portrait ;
- la suite visuelle est clairement orientée vers race + genre + icône de classe, et non vers des portraits combinés race/classe.
