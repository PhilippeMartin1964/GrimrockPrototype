# Roadmap - Identité visuelle de création de personnage

## 1. Décision de conception

Le portrait final du personnage ne doit pas être une image unique créée pour chaque combinaison race, genre et classe.

Le modèle cible est une composition UI :

```text
Portrait principal = Race + genre + variante
Surimpression = icône de classe
Optionnel = cadre, couleur ou fond de classe
```

Exemple :

```text
Race : Elfe
Genre : Féminin
Variante : Portrait 02
Classe : Mage
Affichage : portrait Elfe féminin 02 + icône Mage en surimpression
```

Cette approche évite l'explosion du nombre d'images. Avec 6 races, 2 genres et 6 classes, une production d'images finales demanderait déjà 72 portraits sans aucune variante. Le modèle composite demande seulement 12 portraits de base et 6 icônes de classe pour le minimum jouable.

---

## 2. Sets d'images à produire

### 2.1 Set de portraits race + genre

Minimum CC6 : un portrait par race et par genre.

| Race | Masculin | Féminin |
|---|---|---|
| Humain | `T_Portrait_Human_Male_01` | `T_Portrait_Human_Female_01` |
| Nain | `T_Portrait_Dwarf_Male_01` | `T_Portrait_Dwarf_Female_01` |
| Elfe | `T_Portrait_Elf_Male_01` | `T_Portrait_Elf_Female_01` |
| Halfelin | `T_Portrait_Halfling_Male_01` | `T_Portrait_Halfling_Female_01` |
| Gnome | `T_Portrait_Gnome_Male_01` | `T_Portrait_Gnome_Female_01` |
| Demi-orc | `T_Portrait_HalfOrc_Male_01` | `T_Portrait_HalfOrc_Female_01` |

Extension ultérieure : plusieurs variantes par race et genre.

```text
T_Portrait_Elf_Female_01
T_Portrait_Elf_Female_02
T_Portrait_Elf_Female_03
```

### 2.2 Set d'icônes de classes

Minimum CC6 : une icône claire par classe.

| Classe | Icône recommandée |
|---|---|
| Guerrier | `T_ClassIcon_Warrior` |
| Voleur | `T_ClassIcon_Rogue` |
| Rôdeur | `T_ClassIcon_Ranger` |
| Mage | `T_ClassIcon_Mage` |
| Prêtre | `T_ClassIcon_Priest` |
| Alchimiste | `T_ClassIcon_Alchemist` |

Les icônes doivent rester lisibles en petite taille, car elles seront probablement affichées en coin du portrait dans la création, l'inventaire et la liste du groupe.

### 2.3 Assets optionnels

Ces assets ne sont pas nécessaires pour la première version, mais doivent rester compatibles avec le modèle :

- cadre de classe ;
- couleur d'accent de classe ;
- fond de portrait par classe ;
- badge de niveau ;
- état visuel mort, blessé, empoisonné ou surchargé.

---

## 3. Organisation UE5 recommandée

Dossiers proposés :

```text
Content/GrimrockPrototype/UI/Portraits/Races/
Content/GrimrockPrototype/UI/Portraits/ClassIcons/
Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/
```

Conventions de nommage :

```text
T_Portrait_<Race>_<Gender>_<Variant>
T_ClassIcon_<Class>
DA_PortraitSet_<Race>
DA_ClassVisual_<Class>
```

Exemples :

```text
T_Portrait_Human_Male_01
T_Portrait_Elf_Female_01
T_ClassIcon_Mage
DA_PortraitSet_Elf
DA_ClassVisual_Mage
```

---

## 4. Changements C++ cible

### 4.1 Ajouter un genre ou type de portrait

Créer une énumération simple, exposée Blueprint :

```cpp
UENUM(BlueprintType)
enum class ERPGCharacterPortraitGender : uint8
{
    Male,
    Female
};
```

Le nom technique peut évoluer vers `BodyType` si l'on veut éviter de lier trop fortement les portraits à une notion de genre. Pour la roadmap actuelle, `Male` et `Female` suffisent.

### 4.2 Remplacer l'option portrait plate

Le type actuel `FRPGCharacterPortraitOption` est utile pour CC6.2, mais il doit évoluer vers un modèle structuré :

```cpp
USTRUCT(BlueprintType)
struct FRPGCharacterPortraitVariant
{
    GENERATED_BODY()

    FName VariantId;
    FText DisplayName;
    TSoftObjectPtr<UTexture2D> Portrait;
};
```

Créer ensuite un DataAsset par race :

```cpp
UCLASS(BlueprintType)
class URPGCharacterPortraitSetAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    FName RaceId;
    TArray<FRPGCharacterPortraitVariant> MalePortraits;
    TArray<FRPGCharacterPortraitVariant> FemalePortraits;
};
```

### 4.3 Ajouter les visuels de classe

Créer un DataAsset par classe :

```cpp
UCLASS(BlueprintType)
class URPGClassVisualAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    FName ClassId;
    FText DisplayName;
    TSoftObjectPtr<UTexture2D> ClassIcon;
    FLinearColor AccentColor = FLinearColor::White;
};
```

Le DataAsset de classe `URPGClassAsset` peut soit référencer directement son `URPGClassVisualAsset`, soit rester séparé avec une table visuelle dans le widget. La préférence actuelle est de garder les règles et les visuels séparés tant que l'UI évolue.

### 4.4 Étendre la sélection runtime

Ajouter une structure de sélection visuelle persistable :

```cpp
USTRUCT(BlueprintType)
struct FRPGCharacterVisualSelection
{
    GENERATED_BODY()

    FName RaceId;
    ERPGCharacterPortraitGender Gender = ERPGCharacterPortraitGender::Male;
    FName PortraitVariantId;
    TSoftObjectPtr<UTexture2D> Portrait;
    FName ClassId;
    TSoftObjectPtr<UTexture2D> ClassIcon;
};
```

Cette structure doit être stockée dans `FGridCharacterInventoryState` ou représentée par des champs équivalents :

- portrait principal ;
- identifiant de variante ;
- genre ou type de portrait ;
- icône de classe.

Le portrait principal et l'icône de classe doivent être sauvegardés par CC5.

### 4.5 Étendre `FRPGCharacterCreationRequest`

La requête de création devra contenir :

- nom ;
- race ;
- classe ;
- genre ;
- variante de portrait ;
- texture de portrait résolue ;
- icône de classe résolue.

Le widget peut résoudre les textures à partir des DataAssets, mais `CreateInitialCharacter` doit continuer à valider que la combinaison est cohérente :

- la race du portrait doit correspondre à la race choisie ;
- l'icône de classe doit correspondre à la classe choisie ;
- les références de texture peuvent être nulles uniquement si un fallback explicite est accepté.

---

## 5. Changements UI cible

### 5.1 `WBP_CharacterCreation`

Widgets à prévoir :

| Widget | Type | Rôle |
|---|---|---|
| `ComboBox_Gender` | ComboBox String | choisir Masculin / Féminin |
| `ComboBox_PortraitVariant` | ComboBox String | choisir une variante disponible pour la race et le genre |
| `Image_Portrait` | Image | afficher le portrait principal |
| `Image_ClassIcon` | Image | afficher l'icône de classe en surimpression |
| `Text_PortraitDescription` | Text Block | optionnel, expliquer la variante |

Comportement attendu :

- changer la race recharge la liste des portraits disponibles ;
- changer le genre recharge la liste des portraits disponibles ;
- changer la classe met à jour l'icône de classe ;
- l'aperçu final affiche le portrait et l'icône superposée ;
- aucun Graph Blueprint ne calcule les règles JdR.

### 5.2 `WBP_GridInventory`

Widgets à prévoir dans la fiche centrale :

| Widget | Type | Rôle |
|---|---|---|
| `Image_CharacterPortrait` | Image | portrait race + genre |
| `Image_CharacterClassIcon` | Image | icône de classe en surimpression |

La liste du groupe peut utiliser le même modèle en version réduite.

---

## 6. Découpage recommandé des tranches

### CC6.2 - Socle de portrait simple

État actuel : implémenté comme première marche avec `AvailablePortraits` et un portrait sélectionnable.

But : valider que le portrait circule correctement entre création, inventaire et sauvegarde.

Cette tranche reste acceptable comme transition, mais elle ne doit pas devenir le modèle final.

### CC6.2.1 - Roadmap visuelle composite

But : documenter et valider le modèle cible : race + genre + icône de classe.

Travail :

- documenter les sets d'images ;
- définir les futures structures C++ ;
- préciser les responsabilités UE5 ;
- éviter de produire des portraits combinés race/classe.

### CC6.3 - Race + genre + variantes de portrait

But : remplacer la liste plate de portraits par des portraits filtrés par race et genre.

Travail C++ :

- ajouter `ERPGCharacterPortraitGender` ;
- ajouter `URPGCharacterPortraitSetAsset` ;
- ajouter les champs de sélection visuelle au runtime ;
- mettre à jour `URPGCharacterCreationWidget` ;
- ajouter les tests de filtrage et de sauvegarde.

Travail UE5 :

- créer les 12 textures minimales ;
- créer 6 DataAssets `DA_PortraitSet_*` ;
- ajouter `ComboBox_Gender` et `ComboBox_PortraitVariant` ;
- vérifier l'aperçu PIE.

### CC6.4 - Icônes et identité de classe

But : afficher l'icône de classe en surimpression.

Travail C++ :

- ajouter `URPGClassVisualAsset` ;
- exposer l'icône de classe dans le résumé d'inventaire ;
- sauvegarder/restaurer la référence visuelle si nécessaire ;
- ajouter un test de cohérence classe -> icône.

Travail UE5 :

- créer les 6 icônes de classe ;
- créer 6 DataAssets `DA_ClassVisual_*` ;
- ajouter `Image_ClassIcon` dans `WBP_CharacterCreation` ;
- ajouter `Image_CharacterClassIcon` dans `WBP_GridInventory`.

### CC6.5 - Polissage visuel

But : améliorer le rendu sans changer les règles.

Options :

- cadre de classe ;
- couleur d'accent ;
- fond de portrait ;
- variantes supplémentaires ;
- affichage réduit dans la liste du groupe ;
- états visuels de condition.

---

## 7. Tests à prévoir

Tests automatisés recommandés :

| Test | Résultat attendu |
|---|---|
| `PortraitSetFiltersByRaceAndGender` | seuls les portraits de la race et du genre sélectionnés sont proposés |
| `RaceChangeSelectsValidPortraitFallback` | changer de race remplace un portrait devenu invalide |
| `GenderChangeSelectsValidPortraitFallback` | changer de genre remplace un portrait devenu invalide |
| `ClassChangeUpdatesClassIcon` | changer de classe change l'icône affichée |
| `VisualSelectionPersists` | portrait, genre, variante et icône survivent à la sauvegarde |
| `RejectMismatchedPortraitRace` | une requête avec portrait d'une autre race est refusée |
| `RejectMismatchedClassIcon` | une requête avec icône d'une autre classe est refusée |

---

## 8. Critère de validation cible

Le modèle visuel est validé lorsque :

- les 12 portraits race + genre existent ;
- les 6 icônes de classe existent ;
- la création affiche un portrait principal et une icône de classe superposée ;
- l'inventaire affiche la même composition ;
- la sauvegarde restaure la même composition ;
- les tests de filtrage et de persistance sont verts ;
- aucune image finale race + classe n'est requise pour fonctionner.
