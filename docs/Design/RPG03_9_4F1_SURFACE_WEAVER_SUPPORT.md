# RPG03.9.4F1 — socle générique du Tisseur de surfaces

Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

F1 ajoute uniquement les primitives génériques qui manquaient pour authorer les cinq talents du Tisseur de surfaces en F2.

Aucun Talent Mage, ChoiceId, ActionId ou asset de production n'est authoré dans F1.

## 1. Imprégnation : réaction d'attaque d'arme + dégâts secondaires

C4 gagne un filtre générique `bRequireWeaponAttack`.

Un événement `AttackHit` sait désormais distinguer :

- une attaque réellement résolue depuis une arme équipée ;
- une attaque non armée ou un autre type d'action offensive.

Une réaction peut produire un paquet de dégâts directs secondaire avec :

- magnitude plate ;
- DamageType / PhysicalSubtype indépendant du coup principal ;
- scaling optionnel par modificateur d'attribut ;
- résolution non critique par `ResolveDirectDamage`.

Cela permet en F2 :

```text
Status_ElementalImbuement
AttackHit + WeaponAttack
-> 3 + INT mod du Mage source
-> DamageType de l'affinité
-> consommation du Status
```

### Buff posé sur un allié

Le Status conserve déjà son `SourceId`. F1 n'ajoute donc aucun payload Mage persistant.

Les réactions de Status peuvent déclarer `RequiredStatusSourceRequirementIds`. Lors de la projection, ces Requirements sont reconstruits depuis la progression autoritaire du personnage identifié par `SourceId`.

Conséquences :

- un Mage Fire peut imprégner l'arme d'un allié ;
- l'élément reste celui du Mage source ;
- le scaling INT utilise le Mage source ;
- un seul `Status_ElementalImbuement` suffit ;
- aucune copie Fire/Frost/Air/Earth du Status n'est nécessaire.

## 2. Conversion élémentaire

C6 gagne `FGridCombatSurfaceConversionProfile` :

- liste de surfaces d'entrée ;
- option cellule neutre ;
- surface de sortie ;
- durée initiale pour une création sur cellule neutre.

La conversion utilise le même `TMap<FIntPoint, FGridCombatSurfaceState>` déjà autoritaire dans `AGridLevelRuntimeActor`.

Une conversion valide :

- remplace le type de surface ;
- conserve la durée existante comme base ;
- applique les modifiers de durée de la source ;
- réattribue `SourceCombatantId` et `SourceActionId` au convertisseur ;
- remet à zéro les dégâts périodiques/status périodiques, car la conversion n'invente aucune magnitude non authorée.

Une conversion invalide est un **no-op** et ne fait pas échouer l'action.

Le type `Blood` est ajouté en fin de `EGridCombatSurfaceType` afin de préserver les valeurs sérialisées des types déjà existants.

## 3. Projection d'affinité des actions Surface

`FGridCombatActionOwnerVariantProfile` peut maintenant ajouter :

- `SurfaceEffects` ;
- `SurfaceConversions`.

Le catalogue continue à projeter exactement une variante à partir des Requirements du propriétaire puis supprime `OwnerVariants` de la copie runtime.

Cela permet à un seul ActionId de représenter les variantes Fire/Frost/Air/Earth de Conversion élémentaire et Architecte du terrain.

## 4. Conduction : contexte environnemental de cible

C2 gagne `AnyTargetEnvironmentTags`.

Le contexte cible expose désormais :

- les EffectIds actifs ;
- les StatusTags actifs ;
- la surface de cellule sous forme `Surface.<Type>`.

Exemples :

```text
Status_Burning
Elemental.Fire
Surface.Oil
Surface.Water
Surface.ElectrifiedWater
```

Un modifier peut donc exprimer le +20 % de Conduction par données, sans switch Mage dans le TurnManager.

Les applications C1 déjà authorées sur une attaque restent exécutées après dégâts avec leur ArmorGate normal. Ainsi une attaque dont le bonus de Conduction épuise MagicalArmor peut immédiatement appliquer son contrôle associé sans nouvelle exception runtime.

## 5. Surface persistante

Aucune nouvelle primitive n'est nécessaire.

C6 validé possède déjà :

```text
SurfaceDurationRoundsModifier
SurfacePeriodicDamagePercentModifier
```

La surface créée stocke son auteur et snapshotte les modifiers lors de sa création. F2 pourra donc authorer :

- durée +2, cap 6 ;
- dégâts périodiques +15 % ;
- uniquement sur les surfaces créées/converties par le Mage.

## Couverture Automation F1

Filtre :

```text
Grimrock.RPG.RPG03.9.4F1
```

Tests :

1. `ReactionSecondaryDamage` ;
2. `SurfaceConversion` ;
3. `AffinitySurfaceProjection` ;
4. `TargetEnvironmentContext`.

La validation n'est acquise qu'après réception de la sortie TD04.2 de l'utilisateur.
