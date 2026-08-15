# MON15.4 — Closure

Statut : **VALIDÉ ET CLOS** — Unreal Engine 5.5.4 — 15 août 2026.

## Résultat

MON15.4 introduit le modèle data-driven de progression propre aux classes sans ajouter de nouvel état persistant dans le personnage.

Le jalon fournit :

- `FRPGClassProgressionLevelGrant` pour les points et requirements automatiques par niveau ;
- `FRPGClassProgressionChoiceDefinition` pour les choix, coûts, niveaux minimums et prérequis ;
- `FRPGClassProgressionService` pour la comptabilité, l'évaluation des choix et la projection des requirements ;
- la validation des cycles et références invalides dans `URPGClassAsset` ;
- le raccord au contrat existant `FGridCombatActionDefinition::Requirements` ;
- la compatibilité des anciennes classes sans données MON15.4 ;
- aucune modification de `CurrentSaveVersion`, qui reste à `3`.

## Correctif de validation

Le premier passage de `DefinitionValidation` a révélé un assert `TArray::CheckAddress` dans le fixture de test. L'argument de `TArray::Add()` était une référence vers un élément du même tableau, donc potentiellement invalidée par réallocation.

Le correctif a été poussé dans :

```text
64f0571b90c30d311edfd49a25366804aada4b9f
Fix MON15.4 definition validation fixture
```

Le fixture effectue désormais une copie locale avant insertion. La campagne suivante n'a reproduit ni assert ni exception.

## Validation automatisée

Les sept tests dédiés MON15.4 sont Success :

```text
Grimrock.RPG.MON15.4.DefinitionValidation
Grimrock.RPG.MON15.4.ChoicePointAccounting
Grimrock.RPG.MON15.4.SelectionRules
Grimrock.RPG.MON15.4.RequirementProjection
Grimrock.RPG.MON15.4.CombatActionUnlockProjection
Grimrock.RPG.MON15.4.LevelUpIntegration
Grimrock.RPG.MON15.4.LegacyCompatibility
```

Les régressions MON15.1, MON15.2, MON15.3 et CharacterCreation pertinentes sont également Success.

La campagne complémentaire ActionCatalog / Hotbar est entièrement verte :

```text
Grimrock.Monsters.MON12.ActionCatalog.Contributions             Success
Grimrock.Monsters.MON12.ActionCatalog.GenericAttackLifecycle    Success
Grimrock.Monsters.MON12.8.*                                     Success
```

Elle couvre notamment l'exécution générique d'attaque, la hotbar persistante, le drag/drop, clic/clavier, mains nues, quick items, parchemins, capacités/sorts de classe, mana, ciblage cellule/zone, armes de jet d'inventaire et nettoyage des bindings consommés.

## Invariants de sortie

- progression de classe attachée à `URPGClassAsset` ;
- points dérivés du niveau, sans compteur mutable redondant ;
- requirements réutilisés comme mécanisme unique de déblocage d'action ;
- service MON15.4 pur, sans mutation personnage ;
- aucune nouvelle donnée SaveGame ;
- `CurrentSaveVersion == 3` ;
- aucune modification `.uasset`, `.umap` ou WBP ;
- aucun PIE supplémentaire requis pour ce jalon.

## Suite

MON15.5 est débloqué. Il pourra ajouter la transaction réelle de sélection de choix et l'interface Level Up, puis MON15.6 prendra en charge la persistance et la migration de ces sélections.
