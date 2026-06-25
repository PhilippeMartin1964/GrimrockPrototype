# Roadmap - Identité visuelle de création de personnage

## 1. Décision de conception

Le portrait final du personnage ne doit pas être une image unique créée pour chaque combinaison race, genre et classe.

Le modèle cible reste une composition UI :

```text
Portrait compact = Race + genre + variante
Surimpression = icône de classe
Optionnel = cadre, couleur ou fond de classe
```

Exemple :

```text
Race : Elfe
Genre : Féminin
Variante : Portrait 02
Classe : Mage
Affichage compact : portrait Elfe féminin 02 + icône Mage en surimpression
```

Cette approche évite l'explosion du nombre d'images. Avec 6 races, 2 genres et 6 classes, une production d'images finales demanderait déjà 72 portraits sans aucune variante. Le modèle composite demande seulement 12 portraits de base et 6 icônes de classe pour le minimum jouable.

### 1.1 Décision complémentaire inventaire

À partir de CC6.6, l'inventaire ne doit plus considérer le portrait carré comme la représentation principale du personnage sélectionné.

Le modèle cible devient :

```text
Création / liste du groupe / sauvegarde visuelle rapide : portrait compact 512x512
Inventaire central : personnage plein pied, de pied en cape, avec équipement visible
```

Conséquence importante : `Image_CharacterPortrait` ne doit pas être supprimée ni remplacée brutalement. Elle reste utile pour les vues compactes. L'inventaire doit ajouter une représentation plein pied séparée, par exemple `Image_CharacterFullBody` ou `Image_BodyBase`, selon la structure finale du widget.

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

Les icônes doivent rester lisibles en petite taille, car elles seront affichées en coin du portrait dans la création, l'inventaire, la fiche centrale et la liste du groupe.

### 2.3 Set plein pied pour l'inventaire

Le personnage plein pied est un asset différent du portrait compact.

Formats recommandés :

| Usage | Format recommandé | Rôle |
|---|---:|---|
| Source maître | `1024x1536` | génération, retouche, future découpe d'équipement |
| UI inventaire | `512x768` ou `768x1152` | affichage plein pied dans `WBP_GridInventory` |
| Portrait compact | `512x512` | tête + buste, création et liste du groupe |

Nommage recommandé :

```text
T_FullBody_Human_Male_01
T_FullBody_Human_Female_01
T_FullBody_Elf_Male_01
T_FullBody_Elf_Female_01
```

Le plein pied peut d'abord être une image statique complète. À terme, il doit évoluer vers une composition en couches : corps de base + équipement équipé.

### 2.4 Assets optionnels

Ces assets ne sont pas nécessaires pour la première version, mais doivent rester compatibles avec le modèle :

- cadre de classe ;
- couleur d'accent de classe ;
- fond de portrait par classe ;
- badge de niveau ;
- état visuel mort, blessé, empoisonné ou surchargé ;
- couches d'équipement plein pied ;
- icônes d'attributs, points de vie, mana, expérience et résistances.

---

## 3. Organisation UE5 recommandée

Dossiers proposés :

```text
Content/GrimrockPrototype/UI/Portraits/Races/
Content/GrimrockPrototype/UI/Portraits/ClassIcons/
Content/GrimrockPrototype/UI/Portraits/FullBody/
Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/
```

Conventions de nommage :

```text
T_Portrait_<Race>_<Gender>_<Variant>
T_FullBody_<Race>_<Gender>_<Variant>
T_ClassIcon_<Class>
DA_PortraitSet_<Race>
DA_ClassVisual_<Class>
DA_FullBodySet_<Race>
```

Exemples :

```text
T_Portrait_Human_Male_01
T_FullBody_Human_Male_01
T_Portrait_Elf_Female_01
T_FullBody_Elf_Female_01
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
    TSoftObjectPtr<UTexture2D> FullBody;
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

Le champ `FullBody` peut rester vide pendant CC6.3 et CC6.4. Il devient utile à partir de CC6.6.

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
    TSoftObjectPtr<UTexture2D> FullBody;
    FName ClassId;
    TSoftObjectPtr<UTexture2D> ClassIcon;
};
```

Cette structure doit être stockée dans `FGridCharacterInventoryState` ou représentée par des champs équivalents :

- portrait compact ;
- texture plein pied ;
- identifiant de variante ;
- genre ou type de portrait ;
- icône de classe.

Le portrait compact, l'image plein pied et l'icône de classe doivent être sauvegardés par CC5 ou par les extensions de persistance qui suivent.

### 4.5 Étendre `FRPGCharacterCreationRequest`

La requête de création devra contenir :

- nom ;
- race ;
- classe ;
- genre ;
- variante de portrait ;
- texture de portrait résolue ;
- texture plein pied résolue ;
- icône de classe résolue.

Le widget peut résoudre les textures à partir des DataAssets, mais `CreateInitialCharacter` doit continuer à valider que la combinaison est cohérente :

- la race du portrait doit correspondre à la race choisie ;
- la texture plein pied doit correspondre à la même race et au même genre ;
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
| `Image_Portrait` | Image | afficher le portrait compact |
| `Image_ClassIcon` | Image | afficher l'icône de classe en surimpression |
| `Text_PortraitDescription` | Text Block | optionnel, expliquer la variante |

Comportement attendu :

- changer la race recharge la liste des portraits disponibles ;
- changer le genre recharge la liste des portraits disponibles ;
- changer la classe met à jour l'icône de classe ;
- l'aperçu final affiche le portrait compact et l'icône superposée ;
- aucun Graph Blueprint ne calcule les règles JdR.

### 5.2 `WBP_GridInventory`

`WBP_GridInventory` doit évoluer en deux zones visuelles distinctes.

#### Zone compacte

| Widget | Type | Rôle |
|---|---|---|
| `Image_CharacterPortrait` | Image | portrait compact race + genre |
| `Image_CharacterClassIcon` | Image | icône de classe en surimpression |

Cette zone peut rester dans la liste du groupe, les résumés ou les vues réduites. Elle ne doit pas être confondue avec le personnage plein pied.

#### Zone centrale plein pied

Widgets recommandés :

| Widget | Type | Rôle |
|---|---|---|
| `Overlay_CharacterPaperDoll` | Overlay | conteneur central du personnage équipé |
| `Image_BodyBase` ou `Image_CharacterFullBody` | Image | personnage plein pied de base |
| `Image_Equipment_Head` | Image | casque ou coiffe future |
| `Image_Equipment_Chest` | Image | torse future |
| `Image_Equipment_Legs` | Image | pantalon ou jambes future |
| `Image_Equipment_Boots` | Image | bottes future |
| `Image_Equipment_Gloves` | Image | gants future |
| `Image_Equipment_Belt` | Image | ceinture future |
| `Image_Equipment_Cloak` | Image | cape future |
| `Image_Equipment_MainHand` | Image | main droite ou arme principale future |
| `Image_Equipment_OffHand` | Image | main gauche, bouclier ou objet secondaire future |

Pour CC6.6, les couches d'équipement peuvent rester vides. L'objectif est d'installer la structure d'affichage, pas encore de composer dynamiquement chaque pièce équipée.

### 5.3 Informations autour du plein pied

La fiche centrale de l'inventaire doit prévoir :

- nom du personnage ;
- race ;
- classe ;
- niveau ;
- expérience ;
- icône de race si disponible ;
- icône de classe ;
- points de vie actuels et maximum ;
- mana actuelle et maximum ;
- Force ;
- Dextérité ;
- Constitution ;
- Intelligence ;
- Sagesse ;
- Charisme ;
- attaque ;
- défense ;
- dégâts ;
- coup critique ;
- résistances principales.

Ces valeurs doivent venir du C++ ou du résumé d'inventaire. Le Blueprint ne doit pas recalculer les statistiques.

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

### CC6.6 - Inventaire personnage plein pied

But : remplacer la logique visuelle centrale de l'inventaire par une fiche personnage plein pied, sans supprimer le portrait compact.

Travail C++ :

- exposer dans le résumé d'inventaire une texture plein pied ou un fallback explicite ;
- conserver `Portrait` pour les vues compactes ;
- ajouter, si nécessaire, `FullBody` dans la sélection visuelle persistée ;
- ne pas encore composer dynamiquement les pièces d'équipement.

Travail UE5 :

- préparer au moins une texture plein pied de validation ;
- ajouter `Overlay_CharacterPaperDoll` ;
- ajouter `Image_BodyBase` ou `Image_CharacterFullBody` ;
- conserver `Image_CharacterPortrait` ;
- ajouter les emplacements visuels autour du personnage : tête, torse, jambes, bottes, gants, ceinture, cape, amulette, anneaux, main droite et main gauche ;
- ajouter ou repositionner les panneaux nom, race, classe, niveau, expérience, PV, mana, attributs et résistances.

Critère de sortie : l'inventaire affiche un personnage plein pied lisible au centre, avec les informations principales autour, tout en conservant le portrait compact pour les usages réduits.

### CC6.7 - Slots d'équipement visuels complets

But : aligner la fiche plein pied avec les slots réellement équipables.

Travail :

- confirmer la liste officielle des slots ;
- relier chaque slot visuel à un slot runtime ;
- afficher les icônes d'objets équipés dans les emplacements ;
- gérer les états vide, survolé, sélectionné et incompatible ;
- conserver le glisser-déposer validé par l'inventaire.

### CC6.8 - Corps plein pied race + genre

But : remplacer le plein pied unique de validation par les 12 bases race + genre.

Travail :

- produire les 12 images plein pied de base ;
- étendre les DataAssets de portrait ou créer des `DA_FullBodySet_*` ;
- filtrer le plein pied selon race, genre et variante ;
- sauvegarder/restaurer la même sélection.

### CC6.9 - Couches visuelles d'équipement

But : préparer la composition de l'équipement porté.

Travail :

- définir un asset visuel d'équipement ;
- associer chaque item équipable à une texture de couche ;
- définir l'ordre de rendu : corps, jambes, torse, bottes, gants, cape, armes, effets ;
- produire un premier set limité, par exemple guerrier tier 1.

### CC6.10 - Composition dynamique de l'équipement équipé

But : afficher réellement l'équipement porté sur le personnage plein pied.

Travail :

- composer les couches dans `Overlay_CharacterPaperDoll` ;
- rafraîchir la composition quand un objet est équipé ou retiré ;
- gérer les fallbacks si une pièce n'a pas encore de visuel plein pied ;
- ajouter des tests sur la cohérence inventaire -> fiche visuelle.

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
| `FullBodySelectionPersists` | la texture plein pied survit à la sauvegarde lorsqu'elle existe |
| `InventorySummaryProvidesFullBodyFallback` | l'inventaire reçoit un plein pied valide ou un fallback explicite |
| `RejectMismatchedPortraitRace` | une requête avec portrait d'une autre race est refusée |
| `RejectMismatchedClassIcon` | une requête avec icône d'une autre classe est refusée |

---

## 8. Critère de validation cible

Le modèle visuel est validé lorsque :

- les 12 portraits race + genre existent ;
- les 6 icônes de classe existent ;
- la création affiche un portrait compact et une icône de classe superposée ;
- l'inventaire conserve le portrait compact dans les usages réduits ;
- l'inventaire affiche un personnage plein pied dans la fiche centrale ;
- la sauvegarde restaure la même composition ;
- les tests de filtrage et de persistance sont verts ;
- aucune image finale race + classe n'est requise pour fonctionner ;
- la future composition d'équipement reste possible sans refaire toute l'UI.
