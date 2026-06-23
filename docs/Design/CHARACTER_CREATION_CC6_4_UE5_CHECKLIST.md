# CC6.4 - Icônes et identité de classe

## 1. Objet

CC6.4 ajoute l'icône de classe au modèle visuel composite de création de personnage.

Modèle visé :

```text
Portrait principal = race + genre + variante
Surimpression = icône de classe
```

Cette tranche ne crée pas de portrait final par combinaison race/classe. Elle réutilise les portraits CC6.3 et ajoute une icône de classe en surimpression.

Convention de documentation : les explications fonctionnelles sont en français. Les noms de propriétés, widgets, classes C++ et assets Unreal restent dans leur forme technique exacte.

---

## 2. Résumé technique

Ajouts C++ :

- `URPGClassVisualAsset` ;
- `AvailableClassVisuals` dans `WBP_CharacterCreation` ;
- `Image_ClassIcon` dans `WBP_CharacterCreation` ;
- `Image_CharacterClassIcon` dans `WBP_GridInventory` ;
- `ClassIcon` dans `FRPGCharacterCreationRequest` ;
- `ClassIcon` dans les structures d'état et de résumé inventaire ;
- `SetCharacterVisualSelection()` et `GetCharacterVisualSelection()` dans `UGridPartyInventoryComponent` ;
- test `Grimrock.CharacterCreation.CC6.ClassVisualMatchesClass`.

Principe :

- `URPGClassAsset` reste responsable des règles de classe ;
- `URPGClassVisualAsset` porte uniquement les données visuelles ;
- le widget de création résout l'icône par `ClassId` ;
- l'inventaire relit automatiquement la sélection visuelle persistante via `GetCharacterVisualSelection()`.

---

## 3. Étape A - Récupérer et compiler

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

Résultat attendu : `Build succeeded`.

---

## 4. Étape B - Vérifier les textures d'icônes

Les 6 textures minimales doivent exister :

| Classe | Texture |
|---|---|
| Guerrier | `T_ClassIcon_Warrior` |
| Voleur | `T_ClassIcon_Rogue` |
| Rôdeur | `T_ClassIcon_Ranger` |
| Mage | `T_ClassIcon_Mage` |
| Prêtre | `T_ClassIcon_Priest` |
| Alchimiste | `T_ClassIcon_Alchemist` |

Réglages recommandés :

| Propriété | Valeur |
|---|---|
| Format source | `512x512 RGBA` |
| Compression Settings | `UserInterface2D (RGBA)` |
| Texture Group | `UI` |
| Mip Gen Settings | `NoMipmaps` |
| sRGB | activé |
| Alpha | conservé |

---

## 5. Étape C - Créer les DataAssets de visuel de classe

Créer le dossier :

```text
Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/ClassVisuals/
```

Créer 6 DataAssets de type :

```text
URPGClassVisualAsset
```

Noms recommandés :

| DataAsset | ClassId | ClassIcon | DisplayName |
|---|---|---|---|
| `DA_ClassVisual_Warrior` | `Warrior` | `T_ClassIcon_Warrior` | Guerrier |
| `DA_ClassVisual_Rogue` | `Rogue` | `T_ClassIcon_Rogue` | Voleur |
| `DA_ClassVisual_Ranger` | `Ranger` | `T_ClassIcon_Ranger` | Rôdeur |
| `DA_ClassVisual_Mage` | `Mage` | `T_ClassIcon_Mage` | Mage |
| `DA_ClassVisual_Priest` | `Priest` | `T_ClassIcon_Priest` | Prêtre |
| `DA_ClassVisual_Alchemist` | `Alchemist` | `T_ClassIcon_Alchemist` | Alchimiste |

Descriptions recommandées :

| ClassId | Description |
|---|---|
| `Warrior` | Combattant robuste spécialisé dans le corps à corps, capable d'encaisser les coups et de tenir la ligne. |
| `Rogue` | Aventurier agile et opportuniste, efficace avec les armes légères, les esquives et les attaques précises. |
| `Ranger` | Explorateur polyvalent, à l'aise avec les armes à distance, la survie et les déplacements tactiques. |
| `Mage` | Lanceur de sorts offensif, fragile mais capable d'infliger de lourds dégâts élémentaires. |
| `Priest` | Soutien spirituel capable de protéger, soigner et renforcer le groupe pendant l'exploration. |
| `Alchemist` | Spécialiste des potions, bombes et préparations, utile pour transformer les ressources en avantages tactiques. |

Pour chaque DataAsset :

- renseigner `ClassId` exactement comme dans le DataAsset de classe ;
- renseigner `DisplayName` et `Description` ;
- assigner `ClassIcon` ;
- garder `AccentColor` sur blanc pour CC6.4, sauf besoin visuel immédiat.

---

## 6. Étape D - Modifier `WBP_CharacterCreation`

### D.1 Ajouter l'image d'icône

Dans le bloc portrait, ajouter une Image nommée exactement :

```text
Image_ClassIcon
```

Réglages recommandés :

| Propriété | Valeur |
|---|---|
| Is Variable | oui |
| Visibility initiale | `Collapsed` ou `Hit Test Invisible` |
| Brush Image | vide par défaut |
| Anchors / Alignment | coin inférieur droit du portrait |
| Size | `48x48` à `72x72` selon la taille du portrait |
| ZOrder | au-dessus de `Image_Portrait` |

Le C++ changera automatiquement la texture et la visibilité.

### D.2 Hiérarchie recommandée

```text
PortraitOverlay
├─ Image_Portrait
└─ Image_ClassIcon
```

Si le portrait est dans un `Overlay`, placer `Image_ClassIcon` après `Image_Portrait` pour qu'elle soit dessinée au-dessus.

### D.3 Graph Blueprint

Ne pas ajouter de logique Blueprint pour :

- choisir l'icône selon la classe ;
- changer la texture de `Image_ClassIcon` ;
- cacher/afficher l'icône ;
- copier l'icône dans la requête.

Le C++ gère ces opérations.

---

## 7. Étape E - Configurer `AvailableClassVisuals`

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

Important : si une icône ne s'affiche pas dans l'écran de création, vérifier d'abord que `ClassId` correspond exactement à celui de la classe choisie.

---

## 8. Étape F - Modifier `WBP_GridInventory`

### F.1 Ajouter l'icône sans remplacer le portrait

Dans la fiche centrale personnage, conserver l'image existante :

```text
Image_CharacterPortrait
```

Ajouter une nouvelle Image nommée exactement :

```text
Image_CharacterClassIcon
```

Il ne faut pas remplacer `Image_CharacterPortrait`. `Image_CharacterClassIcon` est une petite icône de classe en surimpression du portrait.

Hiérarchie recommandée :

```text
Overlay_CharacterPortrait
├─ Image_CharacterPortrait
└─ Image_CharacterClassIcon
```

Réglages recommandés pour `Image_CharacterClassIcon` :

| Propriété | Valeur |
|---|---|
| Is Variable | oui |
| Placement | coin supérieur droit ou inférieur droit du portrait personnage |
| Taille | `32x32` à `64x64` |
| Brush Image | vide par défaut |
| Visibilité initiale | `Collapsed` |
| ZOrder | au-dessus de `Image_CharacterPortrait` |

### F.2 Logique C++ attendue

Après le correctif CC6.4, `UGridInventoryWidget` déclare et rafraîchit automatiquement :

```text
Image_CharacterClassIcon
```

Le Blueprint n'a donc pas besoin d'appeler lui-même `GetCharacterVisualSelection()` pour afficher l'icône. Le C++ lit la sélection visuelle persistante du personnage sélectionné et applique :

```text
OutSelection.ClassIcon -> Image_CharacterClassIcon
```

Le chemin Blueprint manuel reste possible pour diagnostic, mais il n'est plus nécessaire pour l'affichage normal.

---

## 9. Étape G - Tests Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Nouveau test CC6.4 :

```text
Grimrock.CharacterCreation.CC6.ClassVisualMatchesClass
```

Nombre attendu après CC6.4 : **19 tests**.

---

## 10. Étape H - Validation PIE

1. Régler `PartyStartupMode = NewGame`.
2. Lancer le PIE.
3. Choisir une race et un genre.
4. Choisir `Mage`.
5. Vérifier que l'icône Mage apparaît en surimpression du portrait dans la création de personnage.
6. Choisir `Guerrier`.
7. Vérifier que l'icône change immédiatement.
8. Créer le personnage.
9. Ouvrir l'inventaire.
10. Vérifier que `Image_CharacterPortrait` affiche le portrait.
11. Vérifier que `Image_CharacterClassIcon` affiche l'icône de classe en surimpression.

Si l'icône apparaît dans la création mais pas dans l'inventaire :

- vérifier que le code contient le correctif `Image_CharacterClassIcon` dans `UGridInventoryWidget` ;
- vérifier que le widget dans `WBP_GridInventory` est nommé exactement `Image_CharacterClassIcon` ;
- vérifier que `Is Variable` est coché ;
- vérifier que l'image est dans un `Overlay` ou un conteneur qui ne la masque pas ;
- vérifier qu'elle est placée après `Image_CharacterPortrait` dans l'Overlay ;
- vérifier que `Visibility` n'est pas forcée à `Collapsed` dans le Graph Blueprint ;
- vérifier que `ClassId` du `DA_ClassVisual_*` correspond exactement au `ClassId` de la classe choisie ;
- relancer la création du personnage après correction, car l'icône est persistée au moment de `Create Character`.

---

## 11. Critère de validation

CC6.4 est validée lorsque :

- la compilation C++ réussit ;
- les 19 tests `Grimrock.CharacterCreation` sont verts ;
- les 6 `DA_ClassVisual_*` existent ;
- `WBP_CharacterCreation` affiche l'icône correcte en changeant de classe ;
- `WBP_GridInventory` affiche `Image_CharacterClassIcon` en surimpression du portrait ;
- aucune image finale race + classe n'est nécessaire.
