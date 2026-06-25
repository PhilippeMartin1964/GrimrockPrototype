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

## 9. Roadmap visuelle cible — références CRPG modernes

### 9.1 Positionnement artistique

L’identité visuelle de la création de personnage doit viser un compromis crédible entre :

```text
Divinity Original Sin 2 = lisibilité fantasy, plein pied expressif, panneaux décoratifs
Pathfinder WotR        = richesse des choix, densité RPG, icônes et statistiques claires
Baldur's Gate 3        = présentation premium, personnage incarné, hiérarchie visuelle moderne
Grimrock Prototype     = donjon sombre, UI fonctionnelle, architecture simple et data-driven
```

Le but n’est pas de copier un de ces jeux, mais de construire une interface de création et d’inventaire cohérente avec le projet :

* vue subjective dungeon crawler ;
* groupe de personnages ;
* règles RPG lisibles ;
* inventaire plein pied ;
* équipements visibles à terme ;
* assets 2D/Texture2D simples à produire ;
* compatibilité future avec la création de contenu par les joueurs.

La direction recommandée est :

```text
Réalisme médiéval fantastique sobre
Ambiance sombre, pierre, cuir, parchemin, métal vieilli
Personnage plein pied au centre
Panneaux latéraux lisibles
Icônes nettes et cohérentes
Effets magiques rares mais valorisants
```

### 9.2 Principe directeur : “BG3-lite en 2D, Pathfinder-lite en règles, DOS2 en ambiance”

Pour rester réaliste dans le cadre du prototype Unreal Engine 5.5.4, il ne faut pas viser immédiatement une création de personnage 3D temps réel équivalente à Baldur’s Gate 3.

La cible crédible est :

```text
Court terme  = personnages plein pied 2D statiques
Moyen terme  = composition 2D en couches
Long terme   = éventuelle prévisualisation 3D ou modèle paper-doll avancé
```

L’écran doit donner une impression premium, mais rester techniquement simple :

* `UTexture2D` pour les portraits ;
* `UTexture2D` pour les corps plein pied ;
* `Overlay` UMG pour les couches d’équipement ;
* DataAssets pour les visuels ;
* aucune logique de règles dans le Blueprint ;
* aucune génération d’image combinée race + genre + classe + équipement.

### 9.3 Inspirations concrètes à reprendre

#### 9.3.1 À reprendre de Divinity Original Sin 2

Divinity Original Sin 2 donne une très bonne leçon de lisibilité fantasy :

* personnage affiché debout, entier ou presque entier ;
* fond d’ambiance illustratif ;
* UI décorative mais compréhensible ;
* onglets simples ;
* forte silhouette du personnage ;
* sentiment d’aventure avant même de commencer la partie.

Application Grimrock Prototype :

```text
Création personnage :
- personnage plein pied au centre ;
- fond sombre de crypte, couloir, torche ou parchemin animé très discret ;
- panneaux latéraux avec cadres métal/cuir/parchemin ;
- boutons sobres, gravés, médiévaux ;
- icône de classe visible immédiatement.
```

À ne pas reprendre tel quel :

* interface trop large ou trop “plein écran cinématique” ;
* complexité de personnalisation faciale ;
* origine narrative complète si le prototype ne la supporte pas encore.

#### 9.3.2 À reprendre de Pathfinder: Wrath of the Righteous

Pathfinder est la meilleure référence pour l’organisation d’une création de personnage riche en règles :

* beaucoup de classes ;
* beaucoup de choix ;
* attributs, dons, compétences, sorts ;
* informations techniques affichées sans perdre le joueur expert ;
* structure par étapes ;
* résumé final clair.

Application Grimrock Prototype :

```text
Création personnage :
- étape Race ;
- étape Classe ;
- étape Attributs ;
- étape Portrait / Apparence ;
- étape Résumé ;
- panneau d’explication à droite ;
- avertissement si une combinaison est invalide ;
- résumé final avant validation.
```

À reprendre surtout :

* la discipline dans la hiérarchie des informations ;
* les icônes de classes/rôles ;
* les tooltips riches ;
* le résumé final.

À ne pas reprendre tel quel :

* densité excessive dès CC6 ;
* listes interminables ;
* sous-classes complexes non encore implémentées ;
* surcharge textuelle dans le premier prototype.

#### 9.3.3 À reprendre de Baldur’s Gate 3

Baldur’s Gate 3 est la référence de présentation premium :

* personnage central fortement incarné ;
* identité visuelle de race et classe immédiate ;
* icônes de classes très reconnaissables ;
* présentation claire, élégante et moderne ;
* sensation que le personnage existe déjà avant le jeu.

Application Grimrock Prototype :

```text
Inventaire :
- personnage plein pied au centre ;
- équipement disposé autour du corps ;
- nom, race, classe, niveau très visibles ;
- icône de classe en haut ou près du nom ;
- PV / mana sous forme de jauges lisibles ;
- attributs et résistances dans un panneau latéral.
```

À reprendre surtout :

* la lisibilité premium ;
* la centralité du personnage ;
* le soin des portraits et icônes ;
* les contrastes forts ;
* les petits effets de lumière.

À ne pas reprendre tel quel :

* rendu 3D complet ;
* caméra animée ;
* personnalisation morphologique avancée ;
* complexité de production trop lourde.

---

## 10. Nouvelle roadmap visuelle proposée

### CC6.5A — Charte visuelle RPG médiévale

But : figer la direction artistique avant de produire trop d’assets.

Livrables :

* palette UI principale ;
* style de cadres ;
* style de boutons ;
* style de panneaux ;
* règles de contraste ;
* typographie recommandée ;
* style des icônes ;
* style des portraits ;
* style des corps plein pied.

Palette recommandée :

```text
Fond principal       : charbon / brun noir / pierre sombre
Panneaux             : cuir sombre, parchemin vieilli, pierre lissée
Cadres               : métal usé, bronze, fer noirci
Texte principal      : ivoire clair
Texte secondaire     : beige / gris chaud
Accent guerrier      : rouge sombre / acier
Accent voleur        : vert sombre / cuir
Accent rôdeur        : vert forêt / cuivre
Accent mage          : bleu violet / arcane
Accent prêtre        : or pâle / ivoire
Accent alchimiste    : vert bouteille / laiton
```

Critère de sortie :

```text
Un document ou tableau de référence permet de produire tous les futurs portraits,
icônes, fonds, cadres et slots sans changer de style à chaque itération.
```

### CC6.5B — Maquette écran création personnage “vertical slice”

But : produire une maquette visuelle crédible de l’écran complet avant d’implémenter tous les systèmes.

Disposition recommandée :

```text
+-------------------------------------------------------------+
| Nom du personnage / titre                                   |
+----------------------+----------------------+---------------+
| Étapes               | Personnage plein pied | Détails choix |
|                      |                      |               |
| 1. Race              |      FullBody         | Description   |
| 2. Genre             |      Portrait         | Bonus         |
| 3. Classe            |      + Classe Icon    | Attributs     |
| 4. Attributs         |                      | Compétences   |
| 5. Portrait          |                      |               |
| 6. Résumé            |                      |               |
+----------------------+----------------------+---------------+
| Retour                         Créer le personnage          |
+-------------------------------------------------------------+
```

Inspiration dominante :

```text
BG3 pour la centralité du personnage
Pathfinder pour les étapes et les informations
DOS2 pour les panneaux fantasy et l’ambiance
```

Livrables UE5 :

* `WBP_CharacterCreation_VisualTarget` ou maquette temporaire ;
* `Overlay_CharacterPreview`;
* `Image_CharacterFullBodyPreview`;
* `Image_PortraitCompactPreview`;
* `Image_ClassIconPreview`;
* panneau de résumé ;
* panneau d’aide contextuelle.

Critère de sortie :

```text
Même avec des données factices, l’écran doit donner une vision claire du produit final.
```

### CC6.5C — Portrait compact premium

But : améliorer le portrait compact existant sans changer l’architecture.

Travail :

* cadre de portrait commun ;
* badge de classe en coin ;
* couleur d’accent de classe ;
* fond discret selon race ou classe ;
* état sélectionné / survolé / indisponible ;
* fallback clair en cas de portrait manquant.

Règle visuelle :

```text
Le portrait compact doit être lisible en 64x64, 128x128, 256x256 et 512x512.
```

Critère de sortie :

```text
La liste du groupe, la création de personnage et l’inventaire affichent tous
le même personnage de manière cohérente.
```

### CC6.6A — Inventaire plein pied version “paper-doll statique”

But : installer la structure visuelle centrale de l’inventaire.

Disposition recommandée :

```text
+-------------------------------------------------------------+
| Nom | Race | Classe | Niveau | Expérience                   |
+-------------------+-------------------------+---------------+
| Groupe / Portraits| Personnage plein pied   | Stats         |
|                   |                         | FOR DEX CON   |
| Portrait 1        |  [Slots autour du corps]| INT SAG CHA   |
| Portrait 2        |                         |               |
| Portrait 3        |                         | PV / Mana     |
| Portrait 4        |                         | Résistances   |
+-------------------+-------------------------+---------------+
| Inventaire grille / actions / description objet             |
+-------------------------------------------------------------+
```

Slots autour du corps :

```text
Tête
Visage
Amulette
Épaulières
Torse
Cape
Gants
Ceinture
Pantalons
Bottes
Anneau gauche
Anneau droit
Main droite
Main gauche
```

Critère de sortie :

```text
Le joueur reconnaît immédiatement le personnage sélectionné,
son rôle, son état général et son équipement principal.
```

### CC6.6B — Jauges et statistiques lisibles

But : rendre la fiche centrale réellement RPG.

Éléments visuels requis :

* PV actuels / maximum ;
* mana actuelle / maximum ;
* niveau ;
* expérience ;
* Force ;
* Dextérité ;
* Constitution ;
* Intelligence ;
* Sagesse ;
* Charisme ;
* attaque ;
* défense ;
* dégâts ;
* critique ;
* résistances principales.

Règle :

```text
Le Blueprint affiche les valeurs reçues.
Il ne recalcule jamais les règles RPG.
```

Traitement visuel recommandé :

```text
PV      = jauge rouge sombre
Mana    = jauge bleue / violette
XP      = jauge or pâle
Stats   = icône + valeur + tooltip
Résist. = petites icônes élémentaires
```

### CC6.7A — Slots d’équipement visuels définitifs

But : aligner les slots affichés avec les vrais slots runtime.

Travail :

* confirmer la liste officielle des slots ;
* créer une texture de slot vide par type ;
* créer état normal ;
* état survolé ;
* état sélectionné ;
* état incompatible ;
* état verrouillé si le slot n’est pas encore géré ;
* relier chaque slot à l’inventaire runtime.

Critère de sortie :

```text
Chaque emplacement visible correspond à une donnée réelle ou à un futur slot documenté.
Aucun slot décoratif non relié ne doit rester ambigu.
```

### CC6.8A — Corps plein pied race + genre

But : remplacer le plein pied unique par les 12 corps de base.

Assets minimum :

```text
T_FullBody_Human_Male_01
T_FullBody_Human_Female_01
T_FullBody_Dwarf_Male_01
T_FullBody_Dwarf_Female_01
T_FullBody_Elf_Male_01
T_FullBody_Elf_Female_01
T_FullBody_Halfling_Male_01
T_FullBody_Halfling_Female_01
T_FullBody_Gnome_Male_01
T_FullBody_Gnome_Female_01
T_FullBody_HalfOrc_Male_01
T_FullBody_HalfOrc_Female_01
```

Style des corps :

* posture debout neutre ;
* cadrage de pied en cape ;
* sous-vêtements ou tenue de base très simple ;
* orientation frontale légèrement trois-quarts ;
* mains visibles ;
* proportions cohérentes par race ;
* fond transparent ;
* lumière venant du haut gauche ;
* aucune arme permanente ;
* aucun équipement lourd permanent.

Critère de sortie :

```text
La race et le genre sélectionnés dans la création sont visibles dans l’inventaire.
```

### CC6.9A — Première couche d’équipement visible

But : valider la composition d’équipement sans produire encore tout le catalogue.

Premier set recommandé :

```text
Guerrier Tier 1
- casque simple ou coiffe
- plastron cuir/métal léger
- gants
- ceinture
- pantalon
- bottes
- épée main droite
- bouclier main gauche
```

Ordre de rendu recommandé :

```text
BodyBase
Legs
Boots
Chest
Belt
Gloves
Shoulders
Cloak
Head
Face
MainHand
OffHand
VFX
```

Critère de sortie :

```text
Équiper ou retirer un objet modifie visuellement le personnage plein pied.
```

### CC6.10A — Composition dynamique complète

But : rendre l’inventaire visuellement réactif.

Travail :

* chaque item équipable peut référencer un `URPGEquipmentVisualAsset`;
* chaque asset visuel contient une ou plusieurs textures de couche ;
* le système choisit la texture selon race, genre, slot et tier ;
* fallback si texture manquante ;
* rafraîchissement automatique après équipement/retrait ;
* test de non-régression inventaire -> visuel.

Critère de sortie :

```text
Le personnage plein pied devient une vraie fiche visuelle équipée,
sans nécessiter une image finale unique pour chaque combinaison.
```

---

## 11. Priorité de production des assets

### Priorité 1 — Minimum crédible

* 12 portraits race + genre ;
* 6 icônes de classe ;
* 1 cadre de portrait ;
* 1 badge de classe ;
* 1 fond UI sombre ;
* 1 set de panneaux ;
* 1 corps plein pied de test ;
* slots vides de l’inventaire.

### Priorité 2 — Inventaire premium

* 12 corps plein pied race + genre ;
* icônes des attributs ;
* icônes PV, mana, XP ;
* icônes de résistances ;
* états de slots ;
* cadre plein pied ;
* fond d’inventaire.

### Priorité 3 — Équipement visible

* set guerrier tier 1 ;
* set voleur tier 1 ;
* set mage tier 1 ;
* armes main droite ;
* boucliers / main gauche ;
* bottes, gants, pantalons, torse, cape, ceinture.

### Priorité 4 — Polissage avancé

* variantes de portraits ;
* variantes de poses ;
* fonds selon classe ;
* effets magiques ;
* états blessé / empoisonné / mort ;
* tiers d’équipement 2 à 5 ;
* support futur de créations de joueurs.

---

## 12. Règles de qualité visuelle

### 12.1 Lisibilité

Une icône doit rester compréhensible à petite taille.

```text
512x512 = source
256x256 = affichage détaillé
128x128 = inventaire / portrait
64x64   = liste groupe / petits badges
```

### 12.2 Cohérence

Tous les assets doivent partager :

* même direction de lumière ;
* même contraste ;
* même niveau de détail ;
* même style de contour ;
* même saturation ;
* même ambiance médiéval fantastique réaliste.

### 12.3 Transparence

Tous les éléments destinés à être composés doivent être en RGBA avec canal alpha propre :

* portraits détourés si nécessaire ;
* corps plein pied détourés ;
* couches d’équipement détourées ;
* icônes détourées ;
* slots avec transparence contrôlée.

### 12.4 Sobriété

Le prototype doit éviter l’effet “cartoon” ou mobile game.

À privilégier :

* métal patiné ;
* cuir usé ;
* pierre sombre ;
* parchemin vieilli ;
* symboles gravés ;
* lumière de torche ;
* couleurs de classe discrètes.

À éviter :

* couleurs trop saturées ;
* contours trop épais ;
* icônes simplistes ;
* fonds verts ou unis ;
* effets magiques excessifs ;
* style cartoon.

---

## 13. Critère de validation visuelle cible

La roadmap visuelle est validée lorsque :

* la création de personnage donne une impression RPG moderne et crédible ;
* le personnage sélectionné est immédiatement reconnaissable ;
* race, genre, classe et portrait sont cohérents ;
* l’inventaire affiche un plein pied central lisible ;
* les slots d’équipement sont compréhensibles ;
* les statistiques principales sont visibles sans surcharge ;
* les assets peuvent être étendus sans refaire toute l’UI ;
* les règles restent côté C++ ;
* l’UI reste data-driven ;
* la direction artistique reste médiéval fantastique réaliste, sombre et sobre.
