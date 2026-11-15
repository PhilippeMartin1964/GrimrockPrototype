# ARM-CLEAN06.3AZ — UseActions : application d'un statut sur soi, en combat

**Parent :** ARM-CLEAN06.3AY (`b12ad68`).
**Portée :** projection transitoire `ApplyStatus` sur le moteur QuickItem actuel.
**Sans** modification des DataAssets binaires, des sauvegardes ou de la localisation.

## Audit des autorités

Le schéma canonique possède déjà `FGridCanonicalActionEffect
{Kind, Magnitude, StatusId}`. Les règles précises de statut
(`StackPolicy`, `DurationUnit`, `DefaultDuration`, `DefaultPotency`,
résistances et contrôles) restent dans
`UGridStatusEffectDefinitionAsset` et non dans `DA_ItemSystem`.

Le moteur existant sait déjà :
- résoudre les définitions de statuts avec
  `FGridCombatStatusApplicationResolver::ResolveDefinition` ;
- décider sans mutation si un statut serait effectivement appliqué
  avec `WouldAnyMutate` et la collection `TryApply` ;
- refuser l'action QuickItem avec `NoApplicableEffect`
  avant dépense lorsque le statut ne peut rien changer ;
- appliquer les statuts après résolution via
  `UGridStatusEffectLifecycleSubsystem` et la transaction QuickItem.

Il n'est donc pas nécessaire de créer un second exécuteur.

## Contrat limité du jalon

`FGridCanonicalQuickItemUseReadModel` accepte à présent une
troisième forme, strictement indépendante d'AX/AY :

`Use / Targeting=Self / Effects=[ApplyStatus(StatusId, Magnitude=1)]`

Contraintes obligatoires :

1. **Instance canonique validée** à son vrai `ItemId + ItemLevel`,
   sans réouverture du `DA_Item_*` historique. Exactement une
   liaison `UseActions` pour ce consommable.
2. Action réellement liée via
   `UGridItemSubsystem::ResolveBoundAction` à la présentation
   et à l'`ActionId` du catalogue unique.
3. Exactement **un effet** `ApplyStatus` avec un `StatusId`
   non vide, `Magnitude=1`, `SourceItemQuantityCost=1`,
   sans dégâts, statistiques supplémentaires, zone, tir,
   ligne de vue, effet imbriqué ou cible autre que `Self`.
4. **Magnitude=1 est projetée en une seule charge initiale** ;
   `DurationOverride=INDEX_NONE`,
   `PotencyOverride=INDEX_NONE` et
   `ArmorGate=None`. Les valeurs de durée, puissance,
   politique de cumul et éligibilité restent sous l'autorité
   du DataAsset de statut.
5. `Trigger=AfterResolution`,
   `ResolutionProfile=Effect`,
   `SourcePolicy=QuickItem`,
   `ActionPointCost=0` **explicitement**, car `Use`
   ne dispose pas encore d'un coût en PA authoré.
   `ManaCost`, `CooldownRounds`, `ActionId`,
   `PresentationId`, textes et icône proviennent
   exclusivement du registre canonique.

**Aucun effet inventé :** les magnitudes 0 ou 2+, les IDs
vides, les effets combinés et les cibles `Ally`, `Party`,
`Hostile`, `Cell` et `Area` sont rejetés.

## Intégration et ciblage

- Le consommable est affectable à la barre de combat
  avec le vrai `ActionId` canonique ; l'ancienne propriété
  `ItemType=Potion` ne peut pas provoquer un `Consume`
  hors combat.
- L'effet cible **exclusivement son utilisateur**.
  Le menu et le HUD d'exploration ne possèdent pas un
  exécuteur transactionnel de statut : cette action
  n'est donc pas `bExplorationCompatible`.
- Le catalogue de combat conserve son calcul
  `WouldAnyMutate`. Avec `NoStack`, le statut
  présent rend immédiatement la nouvelle action
  `NoApplicableEffect`; une définition de statut
  introuvable ne crée pas une fausse permission.
- L'exécution effective appartient exclusivement au
  `GridTurnManager` et au `GridStatusEffectLifecycleSubsystem`
  déjà en place, non à un nouvel exécuteur AY/AZ.
- Les objets encore hors registre gardent leurs
  QuickItem historiques ; les identités enregistrées
  refusent tout fallback et tout niveau invalide.

**Ciblage d'allié différé :** bien que le moteur possède
`RequestCharacterBatchPartyEffect`, son interface
nécessite un parcours explicite de sélection et de
prévisualisation de la cible, ainsi qu'une recette de la
transaction d'objet et des effets secondaires. La politique
`Self` est donc la seule autorisée par AZ.

## Validation

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseApplyStatus.ProjectionAndRejections`

- Registre Unreal transitoire clonant les catalogues
  réels ; ajout en mémoire d'un consommable N0 et de
  son action `Use / ApplyStatus` ;
- contrat du binding `ActionId / PresentationId`,
  champ `InitialStackCount=1`, durées/potence non
  fabriquées, aucune action parasite ;
- refuse les niveaux invalides, magnitudes ambiguës,
  les cibles `Ally/Party/Hostile`, les IDs vides,
  les usages multiples et les effets non correspondants.

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseApplyStatus.CombatCatalogueAndHotbar`

- stockage dans un vrai inventaire et raccourci
  `QuickItem` sans ancien DataAsset ;
- définition de statut transitoire valide et
  résolue par le moteur existant ;
- catalogue : un premier usage `NoStack` disponible,
  puis le même usage devient `NoApplicableEffect`
  avec ce statut déjà présent ;
- le contrôle de disponibilité ne consomme ni ne
  duplique l'objet.

Les scénarios AY et AX restent dans la suite `Grimrock`.
Les tests `-NullRHI` valident la projection et les portes
d'entrée du moteur, **pas** l'animation GPU ni une recette
PIE complète de transaction de statut et ciblage.

## Checklist UI / traduction

Début et fin de jalon :
`Localization/Source/UI.csv → ST_UI.uasset → WBP → Game/fr/en` :
**inchangé sur les cinq étapes**.

Aucun nouveau texte joueur, aucune clé, aucun `TextBlock`,
aucun WBP, aucune ressource PO/locres ou String Table
n'est modifié. Le nom et la description sont des `FText`
métiers authorés dans le DataAsset canonique. Aucune
nouvelle matérialisation `ST_UI.uasset` n'est requise.

**À faire après AZ :** évaluer la première famille de
`UseActions` à cible `Ally` dans un jalon séparé, puis
continuer l'audit des lecteurs historiques avant 06.4.
