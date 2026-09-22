# UI-WEIGHT01 — Inventory Weight Feedback

Date : **22 septembre 2026**  
Statut : **CIBLE CANONIQUE — validation locale requise après UI-WEIGHT01-CLEAN01**

## Décision fonctionnelle

Le poids/encombrement est présenté **uniquement dans l'Inventaire**, jamais dans la feuille de personnage.

Surface canonique :

```text
WBP_InventoryBag
├── Text_InventoryBagWeight
└── ProgressBar_InventoryBagWeight
```

`WBP_CharacterSheet` ne possède aucun texte ni aucune progress bar de poids.

## Autorité

Le calcul reste dans `UGridPartyInventoryComponent::GetCharacterSummary()` :

```text
CurrentWeight
MaxWeight
WeightState
```

`WeightState` est un read model dérivé et non persistant :

```text
Normal      CurrentWeight < 80 % de MaxWeight
Heavy       CurrentWeight >= 80 % de MaxWeight et <= MaxWeight
Overloaded  CurrentWeight > MaxWeight
```

Cas limite : capacité nulle + poids nul = `Normal`; capacité nulle + poids positif = `Overloaded`.

## Présentation UMG

Le C++ renseigne le texte et le pourcentage de la jauge. Le Blueprint ne recalcule aucun seuil.

`PresentInventoryWeightState` sert uniquement au feedback visuel de la jauge :

```text
Normal      -> couleur normale
Heavy       -> couleur d'avertissement
Overloaded  -> couleur de surcharge
```

Le texte peut conserver sa couleur habituelle. Il n'est pas nécessaire de dupliquer la couleur de la jauge dans un `SlateColor`.

## Évolutions futures explicites

Ces éléments sont voulus mais ne font pas partie du ticket actuel :

1. **Icône d'encombrement sur le portrait** du personnage concerné.
2. **Handicap gameplay de déplacement** lorsque la surcharge l'exige, par exemple davantage de temps pour passer d'une cellule à l'autre.
3. Polish visuel de la jauge d'inventaire.

Le handicap de déplacement devra rester une règle gameplay C++, l'icône n'étant qu'une projection de l'état.

Le seuil exact et la sévérité du handicap ne sont pas encore décidés et ne doivent pas être inventés dans l'UI.

## UI-FILTER01

Le prochain gros ticket UI doit regrouper les `EGridItemType` existants en catégories de présentation cohérentes.

Types canoniques actuels :

```text
None
Torch
Weapon
Shield
Armor
Jewelry
Key
Gem
Potion
Scroll
Book
Food
Component
Quest
Misc
```

Les catégories visibles pourront être plus larges, mais elles devront être une projection de ces types, sans créer une seconde taxonomie gameplay.

## Validation

Filtre :

```text
Grimrock.UI.Weight01
```

Le ticket doit vérifier :

- états `Normal / Heavy / Overloaded` ;
- seuil à 80 % ;
- surcharge stricte au-dessus de `MaxWeight` ;
- cas de capacité nulle ;
- présence des bindings de poids dans l'Inventaire ;
- absence des anciens bindings de poids dans la feuille personnage ;
- hook de présentation Blueprint disponible.

Après validation Automation, vérifier en PIE que la jauge de `WBP_InventoryBag` suit le personnage sélectionné et change correctement d'état visuel.
