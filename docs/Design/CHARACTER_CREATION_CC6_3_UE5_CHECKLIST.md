# CC6.3 - Portraits race + genre

## 1. Objet

CC6.3 remplace progressivement la liste plate `AvailablePortraits` de CC6.2 par des sets de portraits filtrés par race et par genre.

Le modèle cible est :

```text
Race choisie + genre choisi -> variantes de portrait disponibles
```

L'icône de classe en surimpression reste hors CC6.3. Elle arrive en CC6.4.

---

## 2. Résumé technique

Ajouts C++ :

- `ERPGCharacterPortraitGender` ;
- `FRPGCharacterPortraitVariant` ;
- `FRPGCharacterVisualSelection` ;
- `URPGCharacterPortraitSetAsset` ;
- `AvailablePortraitSets` dans `WBP_CharacterCreation` ;
- `ComboBox_Gender` ;
- `ComboBox_PortraitVariant` ;
- deux tests supplémentaires de filtrage/fallback.

Compatibilité :

- `AvailablePortraits` reste présent comme fallback CC6.2 ;
- si aucun `AvailablePortraitSets` valide ne correspond à la race choisie, le widget revient à l'ancien comportement.

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

## 4. Étape B - Vérifier les textures race + genre

Les 12 textures minimales doivent exister :

| Race | Masculin | Féminin |
|---|---|---|
| Humain | `T_Portrait_Human_Male_01` | `T_Portrait_Human_Female_01` |
| Nain | `T_Portrait_Dwarf_Male_01` | `T_Portrait_Dwarf_Female_01` |
| Elfe | `T_Portrait_Elf_Male_01` | `T_Portrait_Elf_Female_01` |
| Halfelin | `T_Portrait_Halfling_Male_01` | `T_Portrait_Halfling_Female_01` |
| Gnome | `T_Portrait_Gnome_Male_01` | `T_Portrait_Gnome_Female_01` |
| Demi-orc | `T_Portrait_HalfOrc_Male_01` | `T_Portrait_HalfOrc_Female_01` |

Ces textures sont les portraits UI carrés, idéalement en `512x512` tête + buste.

---

## 5. Étape C - Créer les DataAssets de portraits

Créer un dossier :

```text
Content/GrimrockPrototype/Core/DataAssets/RPG/Visuals/PortraitSets/
```

Créer 6 DataAssets de classe :

```text
URPGCharacterPortraitSetAsset
```

Noms recommandés :

| DataAsset | RaceId |
|---|---|
| `DA_PortraitSet_Human` | `Human` |
| `DA_PortraitSet_Dwarf` | `Dwarf` |
| `DA_PortraitSet_Elf` | `Elf` |
| `DA_PortraitSet_Halfling` | `Halfling` |
| `DA_PortraitSet_Gnome` | `Gnome` |
| `DA_PortraitSet_HalfOrc` | `HalfOrc` |

Pour chaque DataAsset :

- renseigner `RaceId` exactement comme dans le DataAsset de race ;
- renseigner `DisplayName` ;
- ajouter au moins une entrée dans `MalePortraits` ;
- ajouter au moins une entrée dans `FemalePortraits`.

Exemple pour `DA_PortraitSet_Elf` :

| Tableau | VariantId | DisplayName | Portrait |
|---|---|---|---|
| `MalePortraits` | `Elf_Male_01` | `Elfe masculin 01` | `T_Portrait_Elf_Male_01` |
| `FemalePortraits` | `Elf_Female_01` | `Elfe féminin 01` | `T_Portrait_Elf_Female_01` |

---

## 6. Étape D - Modifier `WBP_CharacterCreation`

### D.1 Ajouter les widgets

Dans le Designer, ajouter ou vérifier :

| Nom exact | Type | Is Variable | Rôle |
|---|---|---:|---|
| `ComboBox_Gender` | ComboBox String | oui | choisir Masculin / Féminin |
| `ComboBox_PortraitVariant` | ComboBox String | oui | choisir la variante du set filtré |
| `Image_Portrait` | Image | oui | aperçu du portrait sélectionné |
| `Text_PortraitDescription` | Text Block | oui | optionnel |

Ces widgets sont `BindWidgetOptional`, donc une erreur de nom ne bloque pas forcément la compilation, mais le C++ ne les pilotera pas.

### D.2 Placement recommandé

```text
Bloc portrait
├─ Image_Portrait
├─ ComboBox_Gender
├─ ComboBox_PortraitVariant
└─ Text_PortraitDescription
```

Garder `ComboBox_Race` et `ComboBox_Class` visibles dans le même écran.

### D.3 Graph Blueprint

Ne pas ajouter de logique Blueprint pour :

- remplir `ComboBox_Gender` ;
- remplir `ComboBox_PortraitVariant` ;
- choisir un fallback ;
- changer `Image_Portrait` ;
- copier le portrait dans la requête.

Le C++ gère ces opérations.

---

## 7. Étape E - Configurer `AvailablePortraitSets`

Dans `WBP_CharacterCreation` :

1. Ouvrir **Class Defaults**.
2. Dans **Details**, chercher :

```text
AvailablePortraitSets
```

3. Ajouter les 6 DataAssets :

```text
DA_PortraitSet_Human
DA_PortraitSet_Dwarf
DA_PortraitSet_Elf
DA_PortraitSet_Halfling
DA_PortraitSet_Gnome
DA_PortraitSet_HalfOrc
```

`AvailablePortraits` peut rester renseigné temporairement, mais il ne doit plus être le chemin principal de CC6.3.

---

## 8. Étape F - Tests Automation

Dans **Tools > Session Frontend > Automation**, rechercher :

```text
Grimrock.CharacterCreation
```

Exécuter les tests.

Nombre attendu après CC6.3 : **18 tests**.

Nouveaux tests :

- `Grimrock.CharacterCreation.CC6.PortraitSetFiltersByGender` ;
- `Grimrock.CharacterCreation.CC6.PortraitSetFallbackByGender`.

---

## 9. Étape G - Validation PIE

1. Régler `PartyStartupMode = NewGame`.
2. Lancer le PIE.
3. Choisir `Humain` puis `Masculin`.
4. Vérifier que la variante humaine masculine apparaît.
5. Passer à `Féminin`.
6. Vérifier que la variante humaine féminine apparaît.
7. Changer la race vers `Elfe`.
8. Vérifier que la liste des variantes est remplacée par les portraits elfes du genre courant.
9. Créer le personnage.
10. Ouvrir l'inventaire.
11. Vérifier que le portrait sélectionné est celui affiché dans la fiche centrale.
12. Relancer en `Continue`.
13. Vérifier que le portrait reste affiché.

---

## 10. Critère de validation

CC6.3 est validée lorsque :

- la compilation C++ réussit ;
- les 18 tests `Grimrock.CharacterCreation` sont verts ;
- les 6 DataAssets `DA_PortraitSet_*` existent ;
- changer la race recharge les portraits disponibles ;
- changer le genre recharge les portraits disponibles ;
- l'aperçu affiche le bon portrait ;
- l'inventaire affiche le portrait créé ;
- la sauvegarde restaure le portrait.

---

## 11. Suite

CC6.4 ajoutera :

- `URPGClassVisualAsset` ;
- les 6 DataAssets `DA_ClassVisual_*` ;
- `Image_ClassIcon` dans `WBP_CharacterCreation` ;
- `Image_CharacterClassIcon` dans `WBP_GridInventory` ;
- la surimpression de l'icône de classe sur le portrait.
