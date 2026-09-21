# UI-ITEM01 — Item Tooltip and Equipment Comparison

Date : **21 septembre 2026**  
Statut : **AUTOMATION VALIDÉE — 21 septembre 2026 ; migration visuelle WBP_ItemTooltip à faire**

## Objectif

UI-ITEM01 transforme le tooltip d'objet en projection structurée et passive.

Le tooltip doit pouvoir afficher :

- identité de l'objet ;
- type ;
- description ;
- quantité ;
- poids unitaire et poids total d'une pile ;
- slots d'équipement compatibles ;
- capacités intrinsèques connues ;
- bonus de statistiques ;
- résistances ;
- comparaison avec l'équipement actuellement porté par le personnage sélectionné.

Il ne possède aucune autorité gameplay et ne déplace aucun item.

## Asset réutilisé

Le projet possède déjà :

~~~text
Content/GrimrockPrototype/Blueprints/UI/WBP_ItemTooltip
~~~

UI-ITEM01 ne crée pas un second tooltip.

Le WBP existant doit être migré vers le nouveau read model après validation C++.

## Read model

`UGridInventorySlotWidget::GetTooltipView()` retourne :

~~~text
FGridItemTooltipView
├── identité
├── description / type
├── quantité
├── poids unitaire / total
├── compatible slots
├── bEquippable
├── bReadable
├── bProvidesLight / bLightEnabled
├── bCanAssignToHotbar
├── StatLines[]
└── EquipmentComparisons[]
~~~

Les lignes de statistiques utilisent :

~~~text
FGridItemTooltipStatLine
├── Label
├── ItemValue
├── EquippedValue
├── Delta
├── DeltaState
├── ValueText
└── DeltaText
~~~

Le Blueprint peut donc choisir ses couleurs et sa typographie sans recalculer les valeurs.

## Statistiques projetées

UI-ITEM01 utilise uniquement les autorités déjà existantes dans `UGridItemDefinitionAsset`.

Statistiques :

~~~text
StrengthBonus
DexterityBonus
ConstitutionBonus
IntelligenceBonus
WisdomBonus
CharismaBonus
MaxHealthBonus
MaxManaBonus
CarryWeightBonus
ArmorBonus
~~~

Résistances :

~~~text
Physical
Fire
Ice
Lightning
Poison
Holy
Necrotic
Arcane
~~~

Aucune statistique inventée n'est calculée dans l'UI.

## Comparaison d'équipement

Pour chaque slot compatible réellement occupé par un autre objet :

~~~text
item survolé
-> CompatibleEquipmentSlots
-> équipement du SelectedCharacterIndex
-> définition de l'objet équipé
-> delta candidat - équipé
~~~

Exemple :

~~~text
Ceinture
Ancienne ceinture

Force +3      Δ +2
Armure +1     Δ -1
Résistance feu +8  Δ +3
~~~

Un delta positif est `Positive`, un delta négatif `Negative`, zéro `Neutral`.

La couleur reste du ressort du WBP.

Un slot compatible vide ne crée aucune comparaison : le bandeau de comparaison ne doit pas confondre « emplacement compatible » et « objet réellement comparable ».

Si l'objet survolé est déjà l'objet équipé dans ce slot exact, la comparaison avec lui-même est également omise.

## État d'utilisation

Le read model expose uniquement des capacités dérivées des autorités existantes :

- équipable ;
- lisible ;
- assignable à la hotbar depuis l'inventaire ;
- source de lumière allumée / éteinte.

La règle `Readable` est désormais partagée avec `UGridItemContextActionLibrary::IsItemReadable()` afin que le tooltip et le menu contextuel ne maintiennent pas deux définitions différentes d'un objet lisible.

Les usages dépendant d'une cible monde, par exemple clé/serrure ou gemme/réceptacle, restent résolus par le système d'actions contextuelles et ne sont pas inventés par le tooltip.

## Fallback texte

`GetTooltipText()` utilise désormais le même `FGridItemTooltipView`.

Il reste donc un fallback textuel cohérent même avant la finalisation visuelle de `WBP_ItemTooltip`.

## Automation

Filtre :

~~~text
Grimrock.UI.Item01
~~~

Tests :

~~~text
Grimrock.UI.Item01.TooltipProjection
Grimrock.UI.Item01.EquipmentComparison
~~~

Ils vérifient :

- identité / type / description ;
- quantité ;
- poids total d'une pile ;
- état équipable ;
- état lisible partagé ;
- projection des stats ;
- absence de comparaison quand le slot compatible est vide ;
- comparaison d'un Belt avec l'objet équipé ;
- delta positif et négatif ;
- résumé de comparaison.

## Passe UMG après validation

Après validation Automation, ouvrir uniquement `WBP_ItemTooltip`.

Le ticket C++ expose déjà toutes les données nécessaires ; le WBP ne doit pas recalculer les bonus ou les deltas.

La migration visuelle sera faite progressivement dans l'éditeur afin de préserver le tooltip existant.


## Validation Automation reçue

Validation locale du 21 septembre 2026 :

~~~text
Filter                 : Grimrock.UI.Item01
Succeeded              : 2
Succeeded with warnings: 0
Failed                 : 0
Not run                : 0
Process exit code       : 0
~~~

Le read model tooltip et la comparaison d'équipement sont donc validés côté C++.

Le travail restant de UI-ITEM01 est la migration visuelle de l'asset existant `WBP_ItemTooltip` vers `GetTooltipView()`, sans recréer de seconde autorité ni de second widget.
