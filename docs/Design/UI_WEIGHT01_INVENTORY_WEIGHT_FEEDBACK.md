# UI-WEIGHT01 — Inventory Weight Feedback

Date : **22 septembre 2026**  
Statut : **VALIDÉ — 22 septembre 2026**

## Décision fonctionnelle

Le poids/encombrement est présenté **uniquement dans l'Inventaire**, jamais dans la feuille de personnage.

Surface canonique :

```text
WBP_InventoryBag
└── Text_InventoryBagWeight
```

`WBP_CharacterSheet` ne possède aucun affichage de poids. La progress bar d'inventaire a été supprimée par UI-INVENTORY02.3.

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

Le C++ renseigne uniquement :

```text
Poids : CurrentWeight / MaxWeight
```

`WeightState` reste un état dérivé gameplay utilisé par l'alerte de surcharge du portrait. Il n'existe plus de hook Blueprint `PresentInventoryWeightState` ni de jauge à colorer.

## Évolutions futures explicites

Ces éléments sont voulus mais ne font pas partie du ticket actuel :

1. **Icône d'encombrement sur le portrait** du personnage concerné.
2. **Implémenté par PARTY-WEIGHT01** : dès qu'un personnage actif est `Overloaded`, toute translation du groupe est interdite jusqu'à ce que sa charge redevienne `<= MaxWeight`. La rotation sur place reste autorisée.
3. Polish visuel du résumé de poids.

La règle de déplacement est désormais portée exclusivement par le C++ gameplay via `UGridPartyInventoryComponent::IsAnyActiveCharacterOverloaded()` et `AGrimrockPartyPawn::TryStartMove()`. L'icône reste une simple projection. Le seuil canonique reste strict : `CurrentWeight > MaxWeight`.

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

Après validation Automation, vérifier en PIE que le texte de poids suit le personnage sélectionné et que l'alerte de surcharge du portrait reste correcte.


## Validation reçue — 22 septembre 2026

Validation locale après UI-WEIGHT01-CLEAN01 :

```text
Filter                 : Grimrock.UI.Weight01
Succeeded              : 3
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

Le build Development Editor est vert et le contrat inventory-only est validé.

Validation complémentaire de non-régression de la feuille personnage :

```text
Filter                 : Grimrock.UI.Character02
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
```

UI-WEIGHT01 est clos. Le prochain gros ticket UI est UI-FILTER01.


## UI-FEEDBACK01.1 — évolution implémentée

L'évolution prévue « icône d'encombrement sur le portrait » est désormais engagée via le binding optionnel `Image_WeightAlert` de `WBP_PartyMember`.

Le C++ affiche cette icône uniquement lorsque `WeightState == Overloaded`. Le seuil `Heavy` reste disponible dans le read model mais n'allume pas l'alerte portrait.

Le verrou de déplacement lié à la surcharge est désormais implémenté par `PARTY-WEIGHT01`; UI-FEEDBACK01.1 reste uniquement responsable de son indication visuelle.
