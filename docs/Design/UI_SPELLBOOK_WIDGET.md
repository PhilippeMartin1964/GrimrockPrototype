# UI Spellbook — contrat UMG et validation

Statut : **UI01.4.3c — EN VALIDATION**  
Date : **21 août 2026**

## Objectif

Ce document est la référence canonique pour l'interface du livre de sorts intégrée à `WBP_GrimrockMenu`.

Le Spellbook ne possède aucune copie gameplay des sorts connus. Il projette l'état runtime existant :

```text
UGridPartySpellbookComponent
        +
UGridPartyInventoryComponent
        ↓
UGridSpellbookWidget
        ↓
FGridSpellbookEntryView[]
        ↓
WBP_GridSpellbookEntry
```

Les identités restent des `SpellId` stables (`FName`).

---

## 1. Assets UMG

Les deux assets concernés sont :

```text
Content/GrimrockPrototype/Blueprints/UI/WBP_GridSpellbook
Content/GrimrockPrototype/Blueprints/UI/WBP_GridSpellbookEntry
```

### Parent classes obligatoires

```text
WBP_GridSpellbook
    Parent Class = GridSpellbookWidget

WBP_GridSpellbookEntry
    Parent Class = GridSpellbookEntryWidget
```

Aucune logique de construction de lignes ne doit être ajoutée dans l'Event Graph : la création, le rafraîchissement et la destruction des lignes sont natifs C++.

---

## 2. `WBP_GridSpellbook` — hiérarchie officielle

Hiérarchie actuelle :

```text
WBP_GridSpellbook
└── CanvasPanel_Root
    └── VerticalBox_Content
        ├── Text_Title
        ├── Text_EmptySpellbook
        └── ScrollBox_SpellEntries
            └── Panel_SpellEntries
```

### Contrat des widgets

| Nom exact | Type UMG | `Is Variable` | Imposé par C++ | Valeur / rôle |
|---|---|---:|---:|---|
| `CanvasPanel_Root` | Canvas Panel | non requis | non | racine du widget |
| `VerticalBox_Content` | Vertical Box | non requis | non | contenu vertical principal |
| `Text_Title` | Text Block | non requis | non | texte `Livre de sorts` |
| `Text_EmptySpellbook` | Text Block | **oui** | **oui** | texte `Aucun sort connu` ; visible uniquement si la liste est vide |
| `ScrollBox_SpellEntries` | Scroll Box | non requis | non | conteneur défilable vertical |
| `Panel_SpellEntries` | **Vertical Box** | **oui** | **oui** | reçoit les lignes créées nativement |

Les deux noms suivants sont des `BindWidgetOptional` du C++ et doivent donc être conservés exactement :

```text
Text_EmptySpellbook
Panel_SpellEntries
```

### Propriété de classe obligatoire

Dans le panneau `My Blueprint`, catégorie :

```text
Magic
└── Spellbook
    └── UI
        └── Presentation
            └── Spell Entry Widget Class
```

Valeur :

```text
Spell Entry Widget Class = WBP_GridSpellbookEntry
```

Nom C++ correspondant :

```cpp
SpellEntryWidgetClass
```

Si cette propriété n'est pas assignée, `SpellEntries` peut être correctement rempli mais aucune ligne visuelle n'est créée.

---

## 3. `WBP_GridSpellbookEntry` — hiérarchie officielle

```text
WBP_GridSpellbookEntry
└── Border_Entry
    └── VerticalBox_Entry
        ├── HorizontalBox_Header
        │   ├── Text_SpellName
        │   ├── Text_School
        │   ├── Text_Cost
        │   ├── Text_Range
        │   └── Text_Hotbar
        └── Text_Description
```

### Contrat des widgets

Tous les `Text_*` ci-dessous sont des `BindWidgetOptional` natifs et doivent garder exactement ces noms.

| Nom exact | Type UMG | `Is Variable` | Contenu de Designer conseillé | Valeur runtime |
|---|---|---:|---|---|
| `Text_SpellName` | Text Block | **oui** | `Nom du sort` | `DisplayName`, sinon `SpellId` |
| `Text_School` | Text Block | **oui** | `École` | nom d'affichage de `EGridSpellSchool` |
| `Text_Cost` | Text Block | **oui** | `Mana / PA` | `Mana N  |  PA N` |
| `Text_Range` | Text Block | **oui** | `Portée` | `Portée N` ou `Portée N-M` |
| `Text_Hotbar` | Text Block | **oui** | `Non assigné` | `Non assigné` ou `Raccourci N` |
| `Text_Description` | Text Block | **oui** | `Description du sort` | `Description` |

### Paramètres visuels de première passe

Ces valeurs sont des recommandations de présentation, pas des contraintes du contrat C++ :

```text
Border_Entry
    Padding = 10
    Horizontal Alignment = Fill

Text_SpellName
    Font Size = 22
    Auto Wrap Text = false

Text_School
Text_Cost
Text_Range
Text_Hotbar
    Font Size = 16

Text_Description
    Font Size = 16
    Auto Wrap Text = true
```

---

## 4. Projection des données

`FGridSpellbookEntryView` expose actuellement :

```text
SpellId
DisplayName
Description
School
ManaCost
ActionPointCost
MinRangeCells
MaxRangeCells
TargetingPolicy
bRequiresLineOfSight
bDefinitionResolved
bCanAssignToHotbar
bAssignedToHotbar
AssignedHotbarSlotIndex
CombatActionDefinition
```

`UGridSpellbookWidget::RefreshSpellbook()` :

1. lit le personnage sélectionné depuis `UGridPartyInventoryComponent` ;
2. récupère son `CharacterId` ;
3. garantit l'existence du conteneur Spellbook vide si nécessaire ;
4. lit les dix bindings hotbar MON12 ;
5. appelle `UGridSpellbookUILibrary::BuildProductionSpellbookEntries()` ;
6. reconstruit les lignes visuelles ;
7. diffuse `OnSpellbookRefreshed`.

Flux :

```text
RefreshSpellbook
    ↓
BuildProductionSpellbookEntries
    ↓
SpellEntries[]
    ↓
NotifySpellbookRefreshed
    ↓
RebuildSpellEntryWidgets
    ├── Text_EmptySpellbook Visible/Collapsed
    ├── Panel_SpellEntries.ClearChildren()
    └── CreateWidget(WBP_GridSpellbookEntry) pour chaque entrée
            ↓
        InitializeSpellEntry
            ↓
        RefreshEntryVisual
```

Aucun `For Each`, `Create Widget` ni binding gameplay parallèle ne doit être ajouté en Blueprint.

---

## 5. État vide

Si le personnage sélectionné ne connaît aucun sort :

```text
SpellEntries.Num() == 0
Text_EmptySpellbook = Visible
Panel_SpellEntries = vide
```

Le texte attendu est :

```text
Aucun sort connu
```

Ce cas a été validé visuellement en PIE pendant UI01.4.3c.

---

## 6. État non vide — commande de validation runtime

Le projet ne possède pas encore de flux gameplay de production qui enseigne automatiquement les quatre sorts MON18.5 aux personnages. Pour valider l'interface sans modifier un Blueprint ni la progression du jeu, une commande console de développement existe :

```text
Grimrock.Spellbook.SeedProduction
```

Cette commande :

- cible le Pawn du joueur 0 ;
- utilise le personnage actuellement sélectionné dans `UGridPartyInventoryComponent` ;
- crée le composant Spellbook runtime s'il n'existe pas encore ;
- garantit le conteneur du `CharacterId` sélectionné ;
- enseigne les quatre sorts de production MON18.5 ;
- réutilise `LearnSpell`, donc `OnSpellbookChanged` provoque le rafraîchissement normal de l'UI ;
- n'est pas compilée en `Shipping` ;
- ne persiste rien : l'arrêt du PIE détruit cet état runtime.

Sorts injectés pour validation :

```text
Spell_ArcaneBolt
Spell_LesserHeal
Spell_Haste
Spell_CurePoison
```

Valeurs visuelles attendues pour la première entrée :

```text
Arcane Bolt
Arcane
Mana 3  |  PA 2
Portée 1-5
Non assigné
A focused bolt of arcane force that damages the first hostile target in line.
```

---

## 7. Procédure PIE — validation non vide

1. Compiler le projet UE5.5.4.
2. Lancer PIE.
3. Ouvrir la console Unreal avec la touche configurée pour la console (`~` sur une configuration standard).
4. Saisir exactement :

```text
Grimrock.Spellbook.SeedProduction
```

5. Fermer la console.
6. Ouvrir `WBP_GrimrockMenu`.
7. Sélectionner l'onglet `Sorts`.
8. Vérifier :
   - `Aucun sort connu` a disparu ;
   - quatre lignes sont présentes ;
   - chaque ligne possède nom, école, coût, portée, statut hotbar et description ;
   - aucune erreur Blueprint Runtime n'est présente.
9. Changer de personnage dans le système de sélection existant puis revenir sur `Sorts` : le Spellbook doit suivre le `SelectedCharacterIndex` autoritaire de l'inventaire.
10. Arrêter PIE : les sorts injectés par la commande ne doivent pas survivre au redémarrage du runtime.

---

## 8. Hotbar

Le Spellbook ne possède pas sa propre barre de raccourcis.

Les fonctions natives :

```text
AssignSpellToHotbar
UnassignSpellFromHotbar
```

réutilisent les dix slots persistants MON12 via `UGridSpellbookUILibrary`.

Dans une ligne :

```text
bAssignedToHotbar = false
    → Text_Hotbar = "Non assigné"

bAssignedToHotbar = true
    → Text_Hotbar = "Raccourci N"
```

La validation interactive complète du drag/drop et de l'affectation depuis l'interface appartient à la suite de UI01.4.3 / MON18.7b.

---

## 9. Ce qui n'appartient pas à cette UI

Le widget ne doit pas :

- apprendre des sorts en gameplay normal ;
- décider de la progression magique ;
- payer mana ou PA ;
- lancer un sort ;
- résoudre le ciblage ;
- appliquer des effets ;
- créer une seconde hotbar ;
- persister `KnownSpellIds`.

Ces responsabilités restent dans les systèmes MON18 existants ou dans les jalons ultérieurs.
