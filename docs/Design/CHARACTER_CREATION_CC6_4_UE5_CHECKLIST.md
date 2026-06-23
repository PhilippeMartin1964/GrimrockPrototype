# CC6.4 - Icones et identite de classe

## 1. Objet

CC6.4 ajoute l'icone de classe au modele visuel composite de creation de personnage.

Modele vise :

```text
Portrait principal = race + genre + variante
Surimpression = icone de classe
```

Cette tranche ne cree pas de portrait final par combinaison race/classe. Elle reutilise les portraits CC6.3 et ajoute une icone de classe en overlay.

Convention de documentation : les explications fonctionnelles sont en francais. Les noms de proprietes, widgets, classes C++ et assets Unreal restent dans leur forme technique exacte.

---

## 2. Resume technique

Ajouts C++ :

- `URPGClassVisualAsset` ;
- `AvailableClassVisuals` dans `WBP_CharacterCreation` ;
- `Image_ClassIcon` dans `WBP_CharacterCreation` ;
- `Image_CharacterClassIcon` dans `WBP_GridInventory` ;
- `ClassIcon` dans `FRPGCharacterCreationRequest` ;
- `ClassIcon` dans les structures d'etat et de resume inventaire ;
- `SetCharacterVisualSelection()` et `GetCharacterVisualSelection()` dans `UGridPartyInventoryComponent` ;
- test `Grimrock.CharacterCreation.CC6.ClassVisualMatchesClass`.

Principe :

- `URPGClassAsset` reste responsable des regles de classe ;
- `URPGClassVisualAsset` porte uniquement les donnees visuelles ;
- le widget de creation resout l'icone par `ClassId` ;
- l'inventaire relit automatiquement la selection visuelle persistante via `GetCharacterVisualSelection()`.

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

| DataAsset | ClassId | ClassIcon | DisplayName |
|---|---|---|---|
| `DA_ClassVisual_Warrior` | `Warrior` | `T_ClassIcon_Warrior` | Guerrier |
| `DA_ClassVisual_Rogue` | `Rogue` | `T_ClassIcon_Rogue` | Voleur |
| `DA_ClassVisual_Ranger` | `Ranger` | `T_ClassIcon_Ranger` | Rodeur |
| `DA_ClassVisual_Mage` | `Mage` | `T_ClassIcon_Mage` | Mage |
| `DA_ClassVisual_Priest` | `Priest` | `T_ClassIcon_Priest` | Pretre |
| `DA_ClassVisual_Alchemist` | `Alchemist` | `T_ClassIcon_Alchemist` | Alchimiste |

Descriptions recommandees :

| ClassId | Description |
|---|---|
| `Warrior` | Combattant robuste specialise dans le corps a corps, capable d'encaisser les coups et de tenir la ligne. |
| `Rogue` | Aventurier agile et opportuniste, efficace avec les armes legeres, les esquives et les attaques precises. |
| `Ranger` | Explorateur polyvalent, a l'aise avec les armes a distance, la survie et les deplacements tactiques. |
| `Mage` | Lanceur de sorts offensif, fragile mais capable d'infliger de lourds degats elementaires. |
| `Priest` | Soutien spirituel capable de proteger, soigner et renforcer le groupe pendant l'exploration. |
| `Alchemist` | Specialiste des potions, bombes et preparations, utile pour transformer les ressources en avantages tactiques. |

Pour chaque DataAsset :

- renseigner `ClassId` exactement comme dans le DataAsset de classe ;
- renseigner `DisplayName` et `Description` ;
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

Important : si une icone ne s'affiche pas dans l'ecran de creation, verifier d'abord que `ClassId` correspond exactement a celui de la classe choisie.

---

## 8. Etape F - Modifier `WBP_GridInventory`

### F.1 Ajouter l'icone sans remplacer le portrait

Dans la fiche centrale personnage, conserver l'image existante :

```text
Image_CharacterPortrait
```

Ajouter une nouvelle Image nommee exactement :

```text
Image_CharacterClassIcon
```

Il ne faut pas remplacer `Image_CharacterPortrait`. `Image_CharacterClassIcon` est une petite icone de classe en surimpression du portrait.

Hierarchie recommandee :

```text
Overlay_CharacterPortrait
├─ Image_CharacterPortrait
└─ Image_CharacterClassIcon
```

Reglages recommandes pour `Image_CharacterClassIcon` :

| Propriete | Valeur |
|---|---|
| Is Variable | oui |
| Placement | coin superieur droit ou inferieur droit du portrait personnage |
| Taille | `32x32` a `64x64` |
| Brush Image | vide par defaut |
| Visibilite initiale | `Collapsed` |
| ZOrder | au-dessus de `Image_CharacterPortrait` |

### F.2 Logique C++ attendue

Apres le correctif CC6.4, `UGridInventoryWidget` declare et rafraichit automatiquement :

```text
Image_CharacterClassIcon
```

Le Blueprint n'a donc pas besoin d'appeler lui-meme `GetCharacterVisualSelection()` pour afficher l'icone. Le C++ lit la selection visuelle persistante du personnage selectionne et applique :

```text
OutSelection.ClassIcon -> Image_CharacterClassIcon
```

Le chemin Blueprint manuel reste possible pour diagnostic, mais il n'est plus necessaire pour l'affichage normal.

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
5. Verifier que l'icone Mage apparait en surimpression du portrait dans la creation de personnage.
6. Choisir `Guerrier`.
7. Verifier que l'icone change immediatement.
8. Creer le personnage.
9. Ouvrir l'inventaire.
10. Verifier que `Image_CharacterPortrait` affiche le portrait.
11. Verifier que `Image_CharacterClassIcon` affiche l'icone de classe en surimpression.

Si l'icone apparait dans la creation mais pas dans l'inventaire :

- verifier que le code contient le correctif `Image_CharacterClassIcon` dans `UGridInventoryWidget` ;
- verifier que le widget dans `WBP_GridInventory` est nomme exactement `Image_CharacterClassIcon` ;
- verifier que `Is Variable` est coche ;
- verifier que l'image est dans un `Overlay` ou un conteneur qui ne la masque pas ;
- verifier qu'elle est placee apres `Image_CharacterPortrait` dans l'Overlay ;
- verifier que `Visibility` n'est pas forcee a `Collapsed` dans le Graph Blueprint ;
- verifier que `ClassId` du `DA_ClassVisual_*` correspond exactement au `ClassId` de la classe choisie ;
- relancer la creation du personnage apres correction, car l'icone est persistee au moment de `Create Character`.

---

## 11. Critere de validation

CC6.4 est validee lorsque :

- la compilation C++ reussit ;
- les 19 tests `Grimrock.CharacterCreation` sont verts ;
- les 6 `DA_ClassVisual_*` existent ;
- `WBP_CharacterCreation` affiche l'icone correcte en changeant de classe ;
- `WBP_GridInventory` affiche `Image_CharacterClassIcon` en surimpression du portrait ;
- aucune image finale race + classe n'est necessaire.
