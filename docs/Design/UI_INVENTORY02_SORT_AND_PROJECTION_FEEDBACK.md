# UI-INVENTORY02 — Tri et feedback de projection du sac

Date : **22 septembre 2026**  
Statut : **UI-INVENTORY02.9 C++ PRÊT — Automation/PIE À VALIDER**

## Objectif

Compléter UI-FILTER01 sans modifier l'ordre physique ni l'autorité de l'inventaire.

Le tri est désormais choisi dans une liste déroulante à six options :

```text
Nom ordre croissant
Nom ordre décroissant
Type ordre croissant
Type ordre décroissant
Poids ordre croissant
Poids ordre décroissant
```

Le tri reste une projection de présentation : il ne déplace jamais physiquement les items.

## Invariant principal

Le tri ne déplace jamais un item dans `FGridCharacterInventoryState::InventorySlots`.

Exemple :

```text
item physique : slot 17
projection triée : première case visible
interaction : cible toujours le slot physique 17
```

Les `RuntimeObjectId`, drag/drop, clic, tooltip et menu contextuel continuent donc à utiliser l'identité physique réelle.

## Interaction filtres + tri

`EGridInventoryFilterCategory` et `EGridInventorySortMode` sont deux états de présentation transients.

- **la grille conserve toujours exactement la capacité du sac** (par exemple 40 cases) ;
- quel que soit le mode de tri, les items visibles sont projetés dans les premières cases selon l'ordre choisi, puis les cases restantes sont virtuellement vides ;
- un filtre nommé projette uniquement les items correspondants, toujours selon le tri actif, puis complète avec des cases vides ;
- filtre + tri s'appliquent ensemble ;
- changer de personnage conserve le filtre et le tri tant que le widget existe.

## État vide

Nouveau binding optionnel :

```text
Text_InventoryEmptyState
```

Le C++ ne définit pas son texte ; il gère seulement sa visibilité.

Contrat UMG recommandé :

```text
Type        = Text Block
Name        = Text_InventoryEmptyState
Is Variable = Yes
Text        = Aucun objet à afficher
Visibility  = Collapsed
```

Il devient `HitTestInvisible` lorsque la projection contient zéro item.

## Contrôle de tri

`WBP_InventoryBag` expose uniquement :

```text
ComboBox_SortInventory
```

Le C++ peuple les six options et route nativement `OnSelectionChanged` vers `SetInventorySortMode()`.

`Button_SortInventory`, `Text_SortInventory` et `CycleInventorySortMode()` sont supprimés. Aucun Event Graph n'est requis.

## Modification UMG demandée

Dans :

```text
Content/GrimrockPrototype/Blueprints/UI/Inventory/WBP_InventoryBag
```

dans la zone de tri, remplacer l'ancien bouton par :

```text
ComboBox_SortInventory        Combo Box (String), Is Variable = Yes
```

Ne pas saisir manuellement les options : elles sont ajoutées par le C++.

Puis ajouter juste avant `Border_InventoryGridFrame` :

```text
Text_InventoryEmptyState      Text Block, Is Variable = Yes
Text = Aucun objet à afficher
Visibility = Collapsed
```

Ne créer aucun événement Blueprint.

## Tests

```text
Grimrock.UI.Inventory02.SortingProjection
Grimrock.UI.Inventory02.ControlsContract
```

Validation locale :

```powershell
.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.UI.Inventory02"
```

## Hors scope

Ce ticket ne réordonne pas réellement les objets du sac et ne crée pas de commande gameplay « ranger automatiquement ». Il s'agit strictement d'une projection UI.


## Validation finale — 22 septembre 2026

Validation locale fournie par l'utilisateur :

```text
Filter                 : Grimrock.UI.Inventory02
Succeeded              : 5
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le build Development Editor est validé et la validation PIE est confirmée pour :

- cycle Ordre / Nom / Type / Poids ;
- filtres combinés au tri ;
- compteur de projection ;
- état vide ;
- maintien des interactions sur les slots physiques réels.

Le WBP final a été poussé sur `master` dans :

```text
1e19e8d5183453f1fc4dd67880925250e122b897
UI-INVENTORY02 complete inventory bag presentation
```

`UI-INVENTORY02` est **CLOS**.


## UI-INVENTORY02.1 — correction de la grille à capacité fixe

La première version de UI-INVENTORY02 compactait la **grille elle-même** lorsque le tri ou un filtre était actif. Ce comportement est abandonné.

Contrat corrigé :

```text
Capacité du sac = 40
=> toujours 40 cellules visibles

Tous + tri actif
=> items projetés dans l'ordre demandé
=> cellules restantes vides jusqu'à 40

Filtre Équipement / Consommables / ...
=> items correspondants projetés en tête
=> cellules restantes vides jusqu'à 40
```

Les cellules vides créées par la projection utilisent `INDEX_NONE` et ne prétendent pas représenter un slot physique occupé/caché.

Interactions :

- un item visible conserve toujours son vrai `InventorySlotIndex` source ;
- tooltip, clic droit, drag source et actions continuent donc de viser l'objet physique réel ;
- un clic avec item au curseur sur une cellule vide virtuelle utilise l'insertion inventaire existante ;
- un drop depuis équipement/curseur vers une cellule vide virtuelle utilise la première capacité physique disponible ;
- un drag inventaire -> cellule vide virtuelle n'effectue pas de réorganisation physique implicite, car cette cellule n'a volontairement pas d'identité physique.

Test de régression ajouté :

```text
Grimrock.UI.Inventory02.FixedCapacity
```

La clôture précédente de UI-INVENTORY02 reste historiquement valide pour la version initiale ; UI-INVENTORY02.1 doit être revalidé par Automation + PIE avant nouvelle clôture.


## UI-INVENTORY02.9 — Fast in-place inventory projection

Le tri et le filtrage ne reconstruisent plus la grille UMG lorsque sa topologie est inchangée.

Avant :

```text
tri/filtre
-> projection différente
-> suppression des slots générés
-> recréation de tous les WBP_InventorySlot
-> réenregistrement des delegates
-> rafraîchissements répétés
```

Après :

```text
tri/filtre
-> projection recalculée
-> mêmes widgets conservés
-> InventorySlotIndex réaffecté par position visuelle
-> contenu rafraîchi en place
```

Une reconstruction complète reste limitée aux vrais changements de topologie :

- nombre total de slots ;
- nombre de colonnes ;
- classe de widget de slot ;
- panneau de grille.

Le tri pré-calcule désormais une clé par item (nom, type, poids total) avant `Sort()` au lieu de refaire les recherches de définition et conversions de texte dans le comparateur.

`GetVisibleInventoryItemCount()` compte directement les items correspondant au filtre et ne reconstruit plus une projection triée.

L'enregistrement d'un slot ne rafraîchit plus toute la collection : seul le slot nouvellement enregistré est actualisé.

Lors d'une reprojection, une cellule dont le `InventorySlotIndex` ne change pas n'est pas rafraîchie. Cela évite notamment de retraiter toutes les cases vides de fin de grille.

Enfin, `SetItem()` utilise d'abord `TSoftObjectPtr::Get()` pour une icône déjà résidente avant de recourir à `LoadSynchronous()`.


Test de non-régression ajouté :

```text
Grimrock.UI.Inventory02.InPlaceProjection
```

Il vérifie qu'un changement de tri conserve strictement les mêmes instances `GeneratedInventorySlotWidgets` et ne modifie que leur projection vers les slots physiques.


## UI-INVENTORY02.10 — Inventory interaction cleanup

Nettoyage de stabilisation après les essais de scission au clavier :

- suppression de `bSplitStack` et `RequestedQuantity` du drag UI ;
- `HandleSlotDrop` ne transporte plus que source et destination ;
- `HandleInventorySlotClicked` n'expose plus de paramètre de scission ;
- la scission reste exclusivement l'action contextuelle `Scinder` ;
- `Scinder` écrit directement la moitié séparée dans la première case physique libre, sans `CursorItem` ;
- le drag vers un portrait transfère toujours la pile complète et loggue sa quantité réelle ;
- les notifications de `UGridPartyInventoryComponent` sont l'autorité de rafraîchissement après mutation ;
- le workspace inventaire revient au curseur système normal ; le curseur custom reste réservé aux interactions monde ;
- les tests verrouillent l'absence de plomberie de split dans le drag UI et le choix de la première case libre.

Ce ticket ne modifie aucun asset `.uasset`.
