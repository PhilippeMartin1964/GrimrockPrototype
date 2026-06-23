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

### D.0 Principe général

`WBP_CharacterCreation` reste un widget Blueprint de présentation. Le C++ fait déjà le travail fonctionnel suivant :

- remplit `ComboBox_Race` depuis `AvailableRaceDefinitions` ;
- remplit `ComboBox_Class` depuis `AvailableClassDefinitions` ;
- remplit `ComboBox_Portrait` depuis `AvailablePortraits` ;
- écoute les changements de sélection ;
- met à jour `Image_Portrait` ;
- copie le portrait sélectionné dans la requête de création du personnage.

Dans le Blueprint, il faut donc uniquement ajouter les widgets nommés correctement et les placer dans la mise en page. Il ne faut pas recréer la logique en Graph Blueprint.

### D.1 Ouvrir le widget

Dans le Content Browser, ouvrir :

```text
WBP_CharacterCreation
```

Passer dans l'onglet **Designer**.

Avant modification, vérifier que les widgets existants de CC6.1 sont toujours présents et compilent :

| Widget | Type attendu | Rôle |
|---|---|---|
| `EditableText_Name` | Editable Text | nom du personnage |
| `ComboBox_Race` | ComboBox String | choix de la race |
| `ComboBox_Class` | ComboBox String | choix de la classe |
| `Button_CreateCharacter` | Button | validation de la création |

Si l'un de ces widgets a été renommé, le C++ ne pourra plus le retrouver. Corriger le nom avant d'ajouter le portrait.

### D.2 Choisir l'emplacement du portrait dans la hiérarchie

Placement recommandé dans le Designer :

```text
Root
└─ panneau principal de création
   ├─ colonne gauche ou panneau d'identité
   │  ├─ bloc portrait
   │  │  ├─ Image_Portrait
   │  │  ├─ ComboBox_Portrait
   │  │  └─ Text_PortraitDescription optionnel
   │  └─ bloc nom / race / classe
   └─ panneau statistiques / aperçu
```

Il n'est pas obligatoire d'avoir exactement ces noms de conteneurs pour les panels. En revanche, les trois widgets consommés par le C++ doivent avoir les noms exacts indiqués plus bas.

Disposition recommandée :

- `Image_Portrait` au-dessus de la ComboBox ;
- `ComboBox_Portrait` juste sous l'image ;
- `Text_PortraitDescription` sous la ComboBox, en texte plus discret ;
- garder la race et la classe visibles sans devoir scroller.

### D.3 Ajouter ou vérifier `Image_Portrait`

Ajouter une **Image** si elle n'existe pas déjà.

| Propriété | Valeur |
|---|---|
| Nom exact | `Image_Portrait` |
| Type | Image |
| Is Variable | activé |
| Brush Image | vide ou texture temporaire |
| Visibility | Visible |

Réglages de taille conseillés :

| Cas | Réglage conseillé |
|---|---|
| Portrait carré CC6.2 | `SizeBox` autour de l'image en `256x256` ou `320x320` |
| Si l'UI manque de place | `192x192` minimum |
| Si vous testez un plein pied vertical | ne pas l'utiliser ici comme portrait final ; créer plutôt un recadrage carré `512x512` tête + buste |

Important : le C++ remplace l'image affichée au moment du `RefreshPreview()`. Le `Brush Image` défini dans le Designer n'est qu'un fallback visuel dans l'éditeur.

### D.4 Ajouter `ComboBox_Portrait`

Ajouter une **ComboBox String** sous le portrait.

| Propriété | Valeur |
|---|---|
| Nom exact | `ComboBox_Portrait` |
| Type | ComboBox String |
| Is Variable | activé |
| Options | laisser vide |
| Selected Option | laisser vide |
| Visibility | Visible |

Ne pas ajouter les options manuellement dans le Designer. Le C++ appelle `ClearOptions()` puis ajoute les entrées valides de `AvailablePortraits`.

Ne pas créer d'événement Blueprint `On Selection Changed`. Le C++ bind déjà :

```text
ComboBox_Portrait -> HandlePortraitSelectionChanged
```

Si un événement Blueprint est quand même ajouté, il risque de dupliquer ou de contredire la logique native.

### D.5 Ajouter `Text_PortraitDescription` optionnel

Ajouter un **Text Block** sous la ComboBox si vous voulez afficher une phrase descriptive.

| Propriété | Valeur |
|---|---|
| Nom exact | `Text_PortraitDescription` |
| Type | Text Block |
| Is Variable | activé |
| Text | vide ou `Portrait` temporaire |
| Auto Wrap Text | activé |
| Visibility | Visible |

Ce widget est optionnel grâce à `BindWidgetOptional`. S'il n'existe pas, la compilation reste valide et le portrait fonctionne quand même.

Usage recommandé du texte :

```text
Humain masculin - base neutre pour équipement.
```

Éviter les descriptions longues : le futur modèle race + genre + icône de classe doit rester lisible dans un écran de création dense.

### D.6 Vérifier les propriétés de binding

Les widgets doivent être exposés au C++ par leur nom exact. Dans l'onglet **Details**, vérifier :

| Widget | Is Variable | Obligatoire CC6.2 |
|---|---:|---:|
| `Image_Portrait` | oui | recommandé |
| `ComboBox_Portrait` | oui | oui pour sélection manuelle |
| `Text_PortraitDescription` | oui | non |

`Image_Portrait`, `ComboBox_Portrait` et `Text_PortraitDescription` sont déclarés en `BindWidgetOptional`. Cela signifie :

- si le nom est correct, le C++ les pilote ;
- si le widget manque, le jeu compile encore ;
- si le widget existe mais avec un mauvais nom, le C++ l'ignore silencieusement.

### D.7 Ne pas modifier le Graph pour le portrait

Dans l'onglet **Graph**, ne pas ajouter de logique pour :

- remplir `ComboBox_Portrait` ;
- copier le portrait dans `DefaultPortrait` ;
- changer manuellement `Image_Portrait` ;
- appeler `RefreshPreview()` après sélection ;
- construire une requête de personnage.

La seule logique Blueprint acceptable ici est de la mise en forme purement visuelle, par exemple une animation d'apparition ou un changement d'opacité sans toucher aux données.

### D.8 Compiler et sauvegarder le widget

Cliquer dans cet ordre :

1. **Compile** ;
2. corriger toute erreur de widget manquant ;
3. **Save**.

Résultat attendu : aucune erreur de compilation Blueprint.

Si une erreur indique qu'un widget obligatoire manque, contrôler d'abord :

- `EditableText_Name` ;
- `Button_CreateCharacter` ;
- `ComboBox_Race` ;
- `ComboBox_Class`.

`ComboBox_Portrait` ne devrait pas bloquer la compilation, mais son absence empêchera de choisir manuellement un portrait.

### D.9 Contrôle rapide dans le Designer

À la fin de l'étape D, le Designer doit permettre de voir au minimum :

- une zone de portrait ;
- une ComboBox de portrait ;
- les ComboBox race et classe ;
- le bouton de création ;
- l'aperçu des statistiques.

L'écran doit rester lisible en résolution PIE standard. Si le portrait pousse les statistiques hors écran, réduire la taille de `Image_Portrait` ou placer le bloc portrait dans une colonne plus compacte.

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
