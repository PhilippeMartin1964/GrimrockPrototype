# CC3 - Construction et validation de l’écran de création dans UE5

## 1. Objet

Ce document décrit les opérations humaines nécessaires pour construire `WBP_CharacterCreation` et valider la tranche CC3 dans Unreal Engine 5.5.4.

Branche :

```text
codex/character-creation-cc3-startup-widget
```

---

## 2. Répartition des responsabilités

| Responsable | Travail CC3 |
|---|---|
| ChatGPT / Codex | Widget C++ natif, aperçu des statistiques, appel de CC2, ouverture modale, gestion de l’input et documentation |
| Utilisateur dans UE5 | Compilation, création visuelle du Widget Blueprint, assignation des DataAssets et tests en PIE |
| CC4 | Affichage détaillé des nouvelles statistiques dans l’Inventaire |

Le Graph de `WBP_CharacterCreation` ne doit contenir ni calcul de statistique, ni modification de `PartyInventoryState`, ni événement `OnClicked` pour le bouton de validation.

---

## 3. Étape A - Récupérer et compiler

```bash
git fetch origin
git switch codex/character-creation-cc3-startup-widget
git pull
```

Fermer Unreal Editor, puis compiler dans Visual Studio avec :

- configuration **Development Editor** ;
- plateforme **Win64** ;
- cible `GrimrockPrototypeEditor`.

Résultat attendu : `Build succeeded`.

Après compilation, la classe `RPGCharacterCreationWidget` doit être disponible dans Unreal Editor.

---

## 4. Étape B - Créer WBP_CharacterCreation

### B.1 Créer le dossier

Dans le **Content Drawer**, créer :

```text
Content/GrimrockPrototype/Blueprints/UI/RPG/
```

### B.2 Créer le Widget Blueprint

1. Dans ce dossier, créer un **User Interface > Widget Blueprint**.
2. Choisir `User Widget` si UE5 demande une classe initiale.
3. Nommer le widget `WBP_CharacterCreation`.
4. Ouvrir **File > Reparent Blueprint**.
5. Choisir `RPGCharacterCreationWidget`.
6. Compiler et enregistrer.

Le parent affiché dans **Class Settings** doit être `RPGCharacterCreationWidget`.

---

## 5. Étape C - Construire la hiérarchie UMG

Hiérarchie recommandée :

```text
Canvas Panel
└── Border_Backdrop
    └── SafeZone
        └── ScaleBox
            └── SizeBox_Main (1200 x 760)
                └── Border_MainPanel
                    └── VerticalBox_Main
                        ├── Text_Title
                        ├── HorizontalBox_Identity
                        │   ├── Image_Portrait
                        │   └── VerticalBox_IdentityFields
                        │       ├── EditableText_Name
                        │       ├── Text_RaceValue
                        │       └── Text_ClassValue
                        ├── UniformGridPanel_Attributes
                        │   ├── Text_StrengthValue
                        │   ├── Text_DexterityValue
                        │   ├── Text_ConstitutionValue
                        │   ├── Text_IntelligenceValue
                        │   ├── Text_WisdomValue
                        │   └── Text_CharismaValue
                        ├── HorizontalBox_DerivedStats
                        │   ├── Text_HealthValue
                        │   ├── Text_ManaValue
                        │   └── Text_CarryWeightValue
                        ├── Text_ValidationMessage
                        └── Button_CreateCharacter
```

Les libellés statiques, séparateurs et éléments décoratifs peuvent être ajoutés librement. Les widgets listés ci-dessous doivent conserver exactement leur nom.

---

## 6. Étape D - Widgets obligatoires

| Nom exact | Type UMG | Rôle |
|---|---|---|
| `EditableText_Name` | Editable Text | Nom du personnage |
| `Button_CreateCharacter` | Button | Validation native |
| `Image_Portrait` | Image | Portrait optionnel |
| `Text_RaceValue` | Text Block | Nom de la race |
| `Text_ClassValue` | Text Block | Nom de la classe |
| `Text_StrengthValue` | Text Block | Force |
| `Text_DexterityValue` | Text Block | Dextérité |
| `Text_ConstitutionValue` | Text Block | Constitution |
| `Text_IntelligenceValue` | Text Block | Intelligence |
| `Text_WisdomValue` | Text Block | Sagesse |
| `Text_CharismaValue` | Text Block | Charisme |
| `Text_HealthValue` | Text Block | PV maximum |
| `Text_ManaValue` | Text Block | Mana maximum |
| `Text_CarryWeightValue` | Text Block | Charge maximale |
| `Text_ValidationMessage` | Text Block | Erreur de validation |

Pour chacun de ces widgets, cocher **Is Variable**.

`EditableText_Name` et `Button_CreateCharacter` utilisent désormais `BindWidget` obligatoire. Après compilation C++, le Widget Blueprint doit signaler une erreur de compilation si leur nom, leur type ou leur statut de variable ne correspond pas.

Réglages conseillés :

- widget racine : `Interaction > Is Focusable = false` ; le C++ focalise directement le champ du nom ;
- `EditableText_Name` : texte vide, lecture autorisée, limite visuelle adaptée à 24 caractères ;
- `Text_ValidationMessage` : visibilité initiale `Collapsed` ;
- `Image_Portrait` : taille stable, par exemple `256 x 320` ;
- `Button_CreateCharacter` : texte enfant `Créer le personnage`, `Interaction > Is Enabled = true`, sans binding sur cette propriété ;
- racine et panneaux : aucun événement Blueprint nécessaire.

Le bouton est automatiquement activé ou désactivé par le C++ selon la validité du nom et des DataAssets.

---

## 7. Étape E - Assigner les données du widget

Dans **Class Defaults** de `WBP_CharacterCreation` :

| Propriété | Valeur |
|---|---|
| `RaceDefinition` | `DA_Race_Human` |
| `ClassDefinition` | `DA_Class_Warrior` |
| `DefaultPortrait` | Portrait du guerrier humain, facultatif pour CC3 |

Compiler et enregistrer le widget.

L’aperçu attendu est calculé nativement :

| Valeur | Résultat |
|---|---:|
| Force | 16 |
| Dextérité | 12 |
| Constitution | 14 |
| Intelligence | 10 |
| Sagesse | 10 |
| Charisme | 10 |
| PV | 20 |
| Mana | 0 |
| Charge maximale | 80 |

---

## 8. Étape F - Assigner le widget au PartyPawn

1. Ouvrir `BP_GrimrockPartyPawn`.
2. Ouvrir **Class Defaults**.
3. Rechercher la catégorie **RPG > Character Creation**.
4. Assigner `WBP_CharacterCreation` à `CharacterCreationWidgetClass`.
5. Compiler et enregistrer `BP_GrimrockPartyPawn`.

Sans cette assignation, le jeu reste volontairement bloqué et écrit dans le log :

```text
CharacterCreation UI Show Failed ... Reason=NoWidgetClass
```

### F.1 Diagnostic si le bouton reste désactivé

Si l'écran affiche `Race non configurée` ou `Classe non configurée` :

1. ouvrir `WBP_CharacterCreation`, et non `BP_GrimrockPartyPawn` ;
2. ouvrir **Class Defaults** ;
3. rechercher la catégorie **RPG > Character Creation** ;
4. assigner `DA_Race_Human` à `RaceDefinition` ;
5. assigner `DA_Class_Warrior` à `ClassDefinition` ;
6. compiler puis enregistrer le widget ;
7. relancer entièrement le PIE.

Les valeurs de caractéristiques affichées par des tirets indiquent également que l'une de ces deux références manque.

Si les valeurs sont correctes mais que le bouton reste grisé :

1. vérifier dans la **Hierarchy** que le champ est bien de type **Editable Text**, et non **Editable Text** ou **Text Box** ;
2. vérifier son nom exact `EditableText_Name` et cocher **Is Variable** ;
3. vérifier le nom exact `Button_CreateCharacter` et cocher **Is Variable** ;
4. compiler `WBP_CharacterCreation` et corriger toute erreur `BindWidget` ;
5. sélectionner `Button_CreateCharacter` dans le Designer ;
6. vérifier `Interaction > Is Enabled = true` ;
7. supprimer tout binding Blueprint placé à droite de `Is Enabled` ;
8. vérifier que tous ses panneaux parents sont également activés ;
9. consulter les logs `CharacterCreation NativeConstruct` et `CharacterCreation SubmitState`.

Avec un nom valide, le log attendu contient :

```text
CanSubmit=true ButtonEnabled=true NameLength=5 Inventory=true Completed=false Race=true Class=true Attributes=true
```

La touche **Entrée** valide également la création lorsque la requête est valide.

L'option `Interaction > Is Focusable` du widget racine doit rester désactivée. Le mode UI ne tente plus de focaliser la racine ; il focalise directement `EditableText_Name`.

---

## 9. Étape G - Tests Automation

Dans **Tools > Session Frontend > Automation** :

1. rechercher `Grimrock.CharacterCreation` ;
2. exécuter les quatre tests CC0 ;
3. exécuter les trois tests CC1 ;
4. exécuter les trois tests CC2.

Les dix tests doivent rester verts. CC3 repose principalement sur une validation UMG et PIE ; aucun test Automation supplémentaire n’est ajouté à cette tranche.

---

## 10. Étape H - Validation en PIE

### H.1 Ouverture modale

1. Lancer le PIE.
2. Vérifier que `WBP_CharacterCreation` apparaît immédiatement.
3. Vérifier que le champ du nom reçoit le focus.
4. Vérifier que le bouton est désactivé tant que le nom est vide.

### H.2 Blocage du jeu

Pendant que l’écran est ouvert, vérifier que :

- `W`, `A`, `S`, `D`, `Q` et `E` ne déplacent pas le groupe ;
- les rotations ne fonctionnent pas ;
- l’interaction monde ne fonctionne pas ;
- l’Inventaire ne peut pas être ouvert ;
- un clic hors de l’écran n’interagit pas avec le donjon.

### H.3 Aperçu

Vérifier les valeurs suivantes :

```text
Humain / Guerrier
FOR 16, DEX 12, CON 14, INT 10, SAG 10, CHA 10
PV 20, Mana 0, Charge 80
```

### H.4 Création

1. Saisir un nom, par exemple `Élias`.
2. Vérifier que le bouton devient actif.
3. Cliquer sur **Créer le personnage**.
4. Vérifier que l’écran disparaît.
5. Vérifier que les déplacements et interactions fonctionnent à nouveau.
6. Ouvrir l’Inventaire et vérifier que le membre actif porte le nom saisi.
7. Ramasser puis équiper un objet pour contrôler l’ownership.

En l’absence de sauvegarde CC5, l’écran réapparaît normalement à chaque nouveau lancement du PIE.

---

## 11. Ce qui ne doit pas encore être attendu

- affichage complet des six caractéristiques dans l’Inventaire ;
- sauvegarde du personnage entre deux sessions PIE ;
- choix d’une autre race ou classe ;
- répartition manuelle des points ;
- équipement de départ automatique.

---

## 12. Validation humaine de CC3

CC3 est validée lorsque :

- la compilation **Development Editor Win64** réussit ;
- `WBP_CharacterCreation` possède le bon parent natif ;
- tous les widgets obligatoires sont présents et correctement nommés ;
- les deux DataAssets et la classe du widget sont assignés ;
- les dix tests CC0 à CC2 restent verts ;
- l’écran modal bloque effectivement le jeu ;
- la création ferme l’écran et restaure les contrôles ;
- le nom créé apparaît dans l’Inventaire sans duplication d’objet.

