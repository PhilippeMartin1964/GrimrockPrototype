# RPG03.8 — Batch / Target Filters / Targetability (C8)

Date : **5 octobre 2026**  
Parent : `RPG_Talents_Mechanics_v0_1.md`  
Dépendance : **RPG03.7 validé 8/8**  
Statut : **IMPLÉMENTÉ — VALIDATION UE UTILISATEUR REQUISE**

## Objectif

RPG03.8 ferme le dernier contrat générique C8 requis avant l'authoring des 90 Talents.

Le principe reste celui des jalons RPG03 précédents :

- aucune logique de production ne branche sur un `TalentId` ;
- le catalogue et `UGridTurnManagerComponent` restent les voies d'exécution ;
- MON16 reste l'autorité du lifecycle des Status Effects ;
- `AGridLevelRuntimeActor` reste l'autorité des surfaces ;
- C3 reste l'autorité des mutations d'armure ;
- C7 reste l'autorité des modificateurs QuickItem/Alchemy.

## 1. Targeting policies C8

`EGridCombatTargetingPolicy` ajoute :

- `Hostile` : une cible ennemie explicite ou la cible suggérée déterministe ;
- `Party` : tous les personnages vivants ;
- `FrontRowParty` : les personnages vivants des slots 0..2.

`Ally` conserve une sélection explicite et peut recevoir plusieurs indices lorsqu'un effet secondaire, tel que Diffusion, l'autorise.

Les nouvelles APIs du TurnManager sont :

`RequestCharacterCombatActionOnPartyTargets(...)`

`RequestCharacterCombatActionOnMonsterTarget(...)`

Le chemin standard `RequestCharacterCombatAction(...)` sait en outre :

- résoudre automatiquement `Party` et `FrontRowParty` ;
- utiliser `SuggestedTargetId` pour un `Hostile Effect` ;
- conserver le comportement Spellbook existant.

## 2. Batch déterministe

`FGridCombatTargetingResolver` centralise :

- sélection Self/Ally/Party/FrontRowParty ;
- suppression des doublons de cibles explicites ;
- ordre stable des membres du groupe ;
- filtres de cibles ;
- sélection de statuts à retirer ;
- règle `PrimaryTargetOnly`.

Pour les zones Cell/Area, les cellules sont ordonnées :

1. distance Manhattan depuis la cellule primaire ;
2. Y ;
3. X.

`MaximumResolvedTargets = 0` signifie aucune limite ; une valeur positive tronque après cet ordre déterministe.

## 3. Résolutions répétées

`FGridCombatActionDefinition` ajoute :

- `ResolutionCount` ;
- `SubsequentResolutionAccuracyModifier`.

Les coûts AP/Mana/objet et le cooldown sont payés une seule fois, puis chaque résolution d'attaque est indépendante.

Cela couvre notamment Tir rapide :

`ResolutionCount = 2`

`SubsequentResolutionAccuracyModifier = -1`

Chaque attaque produit son propre roll, ses propres dégâts, ArmorGate et réactions, mais appartient à la même action logique.

## 4. Target filters

`FGridCombatTargetFilterProfile` fournit des filtres data-driven :

- `AllowedMonsterCategoryIds` ;
- `RequiredStatusEffectIds` ;
- `RequiredStatusTags` ;
- `bRequiredStatusesFromSource`.

Champs vides = wildcard.

Exemples :

- `Predator Strike` peut exiger `Status_MarkedByRanger` posé par le Ranger agissant ;
- Holy Dispel / Exorcism peuvent filtrer les catégories de monstres sans test de TalentId ;
- un futur effet peut cibler tout monstre portant un tag de Status particulier.

## 5. Targetability

`FGridStatusEffectControlProfile` ajoute :

`bBlockDirectHostileTargeting`

Il s'agit d'une capacité générique, pas d'une sémantique codée sur `Hidden` ou `Sanctuary`.

Conséquences :

- la sélection de cible des monstres ignore les membres du groupe non ciblables directement ;
- les attaques directes et Hostile Effects du joueur refusent une cible non ciblable ;
- les attaques `Area`, dégâts périodiques et surfaces restent capables d'affecter cette cible.

Ainsi Hidden/Sanctuary sont des configurations de Status Effect, pas des branches de code.

## 6. Filtres et retrait de Status Effects

`UGridStatusEffectDefinitionAsset` ajoute :

`StatusTags`

Exemples attendus à l'authoring RPG03.9 :

- `Toxin` ;
- `Purifiable`.

`FGridCombatStatusRemovalProfile` permet :

- liste exacte d'`EffectIds` ;
- `AnyStatusTags` ;
- `AllowedDispositions` ;
- `MaximumRemovals`.

Le matching identité/tag utilise OR ; `AllowedDispositions` restreint ensuite le résultat.

Les candidats sont ordonnés par `EffectId`, ce qui rend Dispel/Purification/Antidote déterministes.

Les retraits runtime passent exclusivement par `UGridStatusEffectLifecycleSubsystem`, donc feedback, initiative et lifecycle MON16 restent uniques.

## 7. Portée des Status Applications

`FGridCombatStatusApplicationProfile` ajoute :

`TargetScope = AllResolvedTargets | PrimaryTargetOnly`

Cela permet par exemple :

- Sweep/Volley : effets éventuels sur chaque cible ;
- Ravage : dégâts sur tous mais KnockedDown seulement sur la cible primaire, si telle est l'authoring retenue ;
- un effet secondaire ne doit pas être dupliqué sur chaque cible du batch.

## 8. Cell / Area + surfaces

Les actions `ResolutionProfile=Effect` et `TargetingPolicy=Cell|Area` peuvent maintenant exécuter directement leurs `SurfaceEffects`.

Une action `ResolutionProfile=Attack` peut également porter des `SurfaceEffects` :

1. résolution des cibles hostiles ;
2. résolution des dégâts ;
3. application C1/C3 ;
4. création des surfaces sur les cellules affectées.

Le stockage et le tick restent ceux de RPG03.6.

## 9. Friendly fire explicite

`bAffectsAlliesInArea` est opt-in et valide uniquement pour une attaque `Area`.

Lorsque la cellule du groupe est couverte :

- chaque membre vivant reçoit une résolution séparée ;
- les résistances, armures et C2 entrants sont respectés ;
- `FriendlyDirectDamagePercentModifier` de C7 est appliqué.

La présence d'un tag `Bomb` ne force jamais implicitement le friendly fire. Le comportement appartient à la définition de l'action.

Cela permet à Precise Charge d'appliquer son `-50 %` sans introduire un switch sur le Talent.

## 10. Effets Hostile directs d'armure

C3 est étendu sans nouvelle autorité avec :

- `WouldAnyDirectDamage(...)` ;
- `ApplyDirectDamageEffects(...)`.

Ils acceptent les profils `Damage + AfterResolution`, modifient uniquement PhysicalArmor/MagicalArmor et n'ont aucun overflow HP.

Exemple cible RPG03.9 :

`Acid Flask = PhysicalArmor - (6 + 2 × Alchemy), jamais de dégâts HP`.

## 11. Diffusion / sélection secondaire QuickItem

Le projection C7 `FGridQuickItemSecondaryEffectProjection` est maintenant consommée par le batch Ally :

- cible primaire à 100 % ;
- jusqu'à `TargetCount` cibles secondaires explicites ;
- magnitude secondaire via `MagnitudePercent` ;
- durée secondaire via `DurationPercent`.

Les ressources et l'objet ne sont consommés qu'une fois.

## 12. Ce que RPG03.8 ne fait pas

RPG03.8 n'authorise pas encore les 90 Talents.

Il ne crée pas non plus :

- de nouveaux `.uasset` de Status/Action ;
- de seconde économie de Talents ;
- de second moteur de ciblage ;
- de moteur de surface parallèle ;
- de branche `switch(TalentId)`.

L'authoring concret des 90 talents appartient à **RPG03.9**.

## 13. Automation ajoutée

Filtre :

`Grimrock.RPG.RPG03.8`

Tests :

1. `Targetability`
2. `PartyBatchSelection`
3. `StatusRemovalFilter`
4. `TargetFilter`
5. `PrimaryTargetScope`
6. `ActionValidation`
7. `DirectArmorDamage`
8. `MonsterPartyTargetability`

Ces tests sont ajoutés au dépôt mais **ne sont pas déclarés passants tant que la sortie TD04.2 utilisateur n'a pas été fournie**.
