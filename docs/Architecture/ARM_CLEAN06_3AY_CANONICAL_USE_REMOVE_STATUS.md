# ARM-CLEAN06.3AY — UseActions : retrait ciblé d'un statut en combat

**Parent :** ARM-CLEAN06.3AX (`4f78087`).
**Périmètre :** raccordement d'une seconde famille d'effets typés `Use` sans conversion d'assets Unreal.

## Audit de l'existant

Le moteur `GridTurnManagerPlayerActionCatalog.cpp` dispose déjà d'un
exécuteur QuickItem de type `Effect` avec `StatusRemovals`.
`FGridCombatActionCatalog` refuse les actions sans effet utile ;
`FGridCombatTargetingResolver::CollectStatusRemovalIds` filtre les
statuts par `EffectIds`, et
`UGridStatusEffectLifecycleSubsystem::RemoveCombatStatusEffectsFromPartyCharacter`
effectue la mutation à l'issue d'une action réellement acceptée.

Le contrat `FGridCanonicalActionEffect` possède `Kind`,
`Magnitude` et `StatusId` mais ne définit ni tags implicites,
ni ciblage avancé, ni coût en PA pour `Use`. On ne peut donc pas
convertir indistinctement tous les effets.

## Contrat d'implémentation

`FGridCanonicalQuickItemUseReadModel` reste le **seul adaptateur
transient** entre `DA_ItemSystem` et les exécuteurs existants.
Il admet désormais deux formes de `Use`, mutuellement distinctes :

1. **AX :** `Self / Heal(Magnitude>0)` conservé à l'identique.
2. **AY :** `Self / RemoveStatus(StatusId, Magnitude=1)` avec
   `SourceItemQuantityCost=1`. Une action concrète doit contenir
   exactement cet unique effet, et le statut doit être nommé
   explicitement. `Magnitude=1` signifie *une identité de statut
   retirée*, pas un décrément arbitraire de stacks. Les quantités 0
   ou 2+ restent refusées jusqu'à définition métier plus complète.

La projection produit `FGridCombatActionDefinition` avec :

- `ActionId`, `PresentationId`, `ManaCost`, `CooldownRounds`,
  icône, nom, description et tags du système canonique ;
- `SourcePolicy=QuickItem`, `TargetingPolicy=Self`,
  `ResolutionProfile=Effect`, `ActionPointCost=0` (aucun PA
  authoré dans `FGridCanonicalAction`) ;
- `StatusRemovals[0].EffectIds=[StatusId]`,
  `MaximumRemovals=1`, sans soin, dégâts, statut ajouté, effet
  secondaire, terrain, déplacement ni changement de cible ;
- `TargetSide=Any` **dans le filtre interne seulement**, car le
  catalogue QuickItem évalue le statut de l'acteur avec le paramètre
  `Any` par défaut ; ce n'est pas une permission de cibler un
  ennemi. L'action est `Self` et son exécuteur opère sur le
  personnage source uniquement.

**Une seule action par ItemId dans ce jalon.** Si une définition
possède plusieurs `UseActions`, même dont une seule serait
convertible, la sélection reste refusée : la barre enregistre
l'`ItemId` et l'`ActionId` d'un binding unique, mais aucun écran
de choix des usages n'est encore authoré. On ne choisit jamais
arbitrairement l'action première.

## Points d'intégration / comportement conservé

- `GridCanonicalItemInteractionReadModel` conserve l'affectation
  en hotbar canonique ; `Consume` dans le menu contextuel
  demeure réservé à un véritable soin personnel direct.
- Le `GridTurnManager` obtient l'action `RemoveStatus` depuis
  `DA_ItemSystem` à niveau réel, sans `DA_Item_*` historique ;
  son catalogue calcule la disponibilité à partir des statuts
  présents et du coût d'objet. Pas de statut éligible =
  `NoApplicableEffect`, donc pas de consommation.
- Aucun mode exploration de retrait de statut n'est inventé.
  `bExplorationCompatible` exige un vrai profil de soin et aucune
  suppression de statut. Aucun raccourci n'essaie l'exécuteur de
  soins pendant l'exploration pour un `RemoveStatus`.
- Les objets absents du registre conservent leurs mécanismes
  QuickItem historiques ; un `ItemId` enregistré ne retombe
  jamais sur ces mécanismes.
- Les autres formes `Use` (`Ally`, `ApplyStatus`, `Armor`,
  `Surface`, `Trap`, `Teleport`, etc.) sont refusées
  faute de contrat de projection approuvé.

## Validation et preuve

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseRemoveStatus.ProjectionAndRejections`
crée, dans un registre UE transitoire et validé, une définition
`UGridConsumableDefinitionAsset` N0 dont l'unique
`UseAction` est `RemoveStatus(Status_ARM063AY_Toxin,1)`.
Teste les liaisons Action/Presentation, le contrôle `IsValid`,
les coûts conservés, l'absence de `Consume` et de soin,
les niveaux invalides, un `StatusId` absent, les magnitudes
ambiguës, les cibles non Self et les effets multiples.

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalUseRemoveStatus.CombatCatalogueAndHotbar`
vérifie une vraie instance stockée dans le sac, un ancien asset
contradictoire, le menu, l'affectation du binding canonique,
la lecture de `FGridCombatActionCatalog` et le filtrage exact :
statut présent = action disponible ; aucun statut =
`NoApplicableEffect`, sans retirer d'objet.

Ces tests vérifient **la projection et la disponibilité via
l'exécuteur existant**. Ils ne prouvent pas à eux seuls
l'animation, les interactions PIE ni une transaction complète de
retrait de statut sur personnage réel en combat ; ce scénario
reste à accepter lors de la recette runtime.

## Localisation / UI

Checklist contrôlée avant et après :
`Localization/Source/UI.csv → ST_UI.uasset → WBP → Game/fr/en`
**inchangée**. Aucun texte visible neuf, aucun `FText` UI
nouveau, aucune clé ST_UI, aucune modification WBP, PO ou locres ;
le nom et la description viennent du DataAsset canonique.
Pas de matérialisation `ST_UI` nécessaire.

**06.4 n'est pas lancé.** Aucun binaire `.uasset`,
`.umap`, SaveGame ou format de sérialisation n'est modifié.
