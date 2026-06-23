# CC6.4 - Icones et identite de classe

## 1. Objet

CC6.4 ajoute l'icone de classe au modele visuel composite de creation de personnage.

Modele vise :

```text
Portrait principal = race + genre + variante
Surimpression = icone de classe
```

Cette tranche ne cree pas de portrait final par combinaison race/classe. Elle reutilise les portraits CC6.3 et ajoute une icone de classe en overlay.

---

## 2. Resume technique

Ajouts C++ :

- `URPGClassVisualAsset` ;
- `AvailableClassVisuals` dans `WBP_CharacterCreation` ;
- `Image_ClassIcon` dans `WBP_CharacterCreation` ;
- `ClassIcon` dans `FRPGCharacterCreationRequest` ;
- `ClassIcon` dans les structures d'etat et de resume inventaire ;
- test `Grimrock.CharacterCreation.CC6.ClassVisualMatchesClass`.

Principe :

- `URPGClassAsset` reste responsable des regles de classe ;
- `URPGClassVisualAsset` porte uniquement les donnees visuelles ;
- le widget resout l'icone par `ClassId`.

---

## 3. Etape A - Recuperer et compiler

Fermer Unreal Editor, puis :

```bash
git fetch origin
git switch codex/character-creation-cc3-startup-widget
git pull
```

Compiler :

- configuration **Development Editor** ;
- plateforme **Win64** ;
- cible `GrimrockPrototypeEditor`.

Resultat attendu : `Build succeeded`.

---

## 4. Etape B - Verifier les textures d'icones

Les 6 textures minimales doivent exister :

| Classe | Texture |
|---|---|
| Guerrier | `T_ClassIcon_Warrior` |
| Voleur | `T_ClassIcon_Rogue` |
| Rodeur | `T_ClassIcon_Ranger` |
| Mage | `T_ClassIcon_Mage` |
| Pretre | `T_ClassIcon_Priest` |
| Alchimiste | `T_ClassIcon_Alchemist` |

Reglages recommandes :

| Propriete | Valeur |
|---|---|
| Format source | `512x512 RGBA` |
| Compression Settings | `UserInterface2D (RGBA)` |
| Texture Group | `UI` |
| Mip Gen Settings | `NoMipmaps` |
| sRGB | active |
| Alpha | conserve |

---

## 5. Etape C - Creer les DataAssets de visuel de classe

Creer le dossier :

```text
Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/
```

Creer 6 DataAssets de type :

```text
URPGClassVisualAsset
```

Noms recommandes :

| DataAsset | ClassId | ClassIcon |
|---|---|---|
| `DA_ClassVisual_Warrior` | `Warrior` | `T_ClassIcon_Warrior` |
| `DA_ClassVisual_Rogue` | `Rogue` | `T_ClassIcon_Rogue` |
| `DA_ClassVisual_Ranger` | `Ranger` | `T_ClassIcon_Ranger` |
| `DA_ClassVisual_Mage` | `Mage` | `T_ClassIcon_Mage` |
| `DA_ClassVisual_Priest` | `Priest` | `T_ClassIcon_Priest` |
| `DA_ClassVisual_Alchemist` | `Alchemist` | `T_ClassIcon_Alchemist` |

Pour chaque DataAsset :

- renseigner `ClassId` exactement comme dans le DataAsset de classe ;
- renseigner `DisplayName` ;
- assigner `ClassIcon` ;
- garder `AccentColor` sur blanc pour CC6.4, sauf besoin visuel immediat.

---

## 6. Etape D - Modifier `WBP_CharacterCreation`

### D.1 Ajouter l'image d'icone

Dans le bloc portrait, ajouter une Image nommee exactement :

```text
Image_ClassIcon
```

Reglages recommandes :

| Propriete | Valeur |
|---|---|
| Is Variable | oui |
| Visibility initiale | `Collapsed` ou `Hit Test Invisible` |
| Brush Image | vide par defaut |
| Anchors / Alignment | coin inferieur droit du portrait |
| Size | `48x48` a `72x72` selon la taille du portrait |
| ZOrder | au-dessus de `Image_Portrait` |

Le C++ changera automatiquement la texture et la visibilite.

### D.2 Hierarchie recommandee

```text
PortraitOverlay
├─ Image_Portrait
└─ Image_ClassIcon
```

Si le portrait est dans un `Overlay`, placer `Image_ClassIcon` apres `Image_Portrait` pour qu'elle soit dessinee au-dessus.

### D.3 Graph Blueprint

Ne pas ajouter de logique Blueprint pour :

- choisir l'icone selon la classe ;
- changer la texture de `Image_ClassIcon` ;
- cacher/afficher l'icone ;
- copier l'icone dans la requete.

Le C++ gere ces operations.

---

## 7. Etape E - Configurer `AvailableClassVisuals`

Dans `WBP_CharacterCreation` :

1. Ouvrir **Class Defaults**.
2. Dans **Details**, chercher :

```text
AvailableClassVisuals
```

3. Ajouter les 6 DataAssets :

```text
DA_ClassVisual_Warrior
DA_ClassVisual_Rogue
DA_ClassVisual_Ranger
DA_ClassVisual_Mage
DA_ClassVisual_Priest
DA_ClassVisual_Alchemist
```

Important : si une icone ne s'affiche pas, verifier d'abord que `ClassId` correspond exactement a celui de la classe choisie.

---

## 8. Etape F - Modifier `WBP_GridInventory`

Dans la fiche centrale personnage, ajouter une Image nommee :

```text
Image_CharacterClassIcon
```

Reglages recommandes :

| Propriete | Valeur |
|---|---|
| Is Variable | oui |
| Placement | coin du portrait personnage |
| Taille | `32x32` a `64x64` |
| Visibilite si vide | `Collapsed` |

Le resume C++ expose maintenant `ClassIcon` dans `FGridInventoryCharacterSummary`. Le binding Blueprint/UMG pourra donc utiliser la meme logique que le portrait, mais en lisant `Summary.ClassIcon`.

---

## 9. Etape G - Tests Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Nouveau test CC6.4 :

```text
Grimrock.CharacterCreation.CC6.ClassVisualMatchesClass
```

Nombre attendu apres CC6.4 : **19 tests**.

---

## 10. Etape H - Validation PIE

1. Regler `PartyStartupMode = NewGame`.
2. Lancer le PIE.
3. Choisir une race et un genre.
4. Choisir `Mage`.
5. Verifier que l'icone Mage apparait en surimpression du portrait.
6. Choisir `Guerrier`.
7. Verifier que l'icone change immediatement.
8. Creer le personnage.
9. Ouvrir l'inventaire.
10. Verifier que le portrait et l'icone de classe peuvent etre affiches ensemble.

---

## 11. Critere de validation

CC6.4 est validee lorsque :

- la compilation C++ reussit ;
- les 19 tests `Grimrock.CharacterCreation` sont verts ;
- les 6 `DA_ClassVisual_*` existent ;
- `WBP_CharacterCreation` affiche l'icone correcte en changeant de classe ;
- `WBP_GridInventory` peut afficher `Summary.ClassIcon` en surimpression du portrait ;
- aucune image finale race + classe n'est necessaire.
