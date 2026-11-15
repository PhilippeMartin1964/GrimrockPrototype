# ARM-CLEAN06.3BA — Consommables canoniques : ciblage explicite Ally

**Parent :** ARM-CLEAN06.3AZ, commit `d486a2e`.
**Portée :** cible `Ally` pour les trois effets déjà supportés sur soi
(`Heal`, `RemoveStatus`, `ApplyStatus`), en combat seulement.
Pas d'asset UE binaire ni de modification SaveGame.

## Problème démontré dans le code existant

`UGridTurnManagerComponent::RequestCharacterCombatActionOnPartyTargets`
et `RequestCharacterBatchPartyEffect` savent déjà appliquer des effets
`QuickItem` à une liste de personnages explicitement sélectionnés.
Ils recalculent l'effet sur la **cible** avant de payer PA, mana ou objet.

Cependant :

1. `FGridCanonicalQuickItemUseReadModel` refusait toute valeur autre
   que `Targeting=Self` ;
2. `FGridCombatActionCatalog::EvaluateMON126Availability` calculait
   l'utilité d'un soin avec les PV et statuts **du lanceur** :
   un lanceur à pleine santé pouvait désactiver une potion destinée
   à son allié blessé avant que celui-ci soit choisi ;
3. `UGridCombatHudWidget` permettait le ciblage case/zone mais
   aucune sélection de portrait pour un `Ally`, puis
   `RequestCharacterCombatAction` refusait `TargetRequired`.

## Contrat de projection et exécution

`FGridCanonicalQuickItemUseReadModel` autorise désormais
`Targeting=Self|Ally` pour chacun de ces cas **strictement uniques** :

- `Use / Heal(Magnitude>0)` : soin numérique direct ;
- `Use / RemoveStatus(StatusId, Magnitude=1)` : retrait d'un ID
  précis sans suppression implicite de plusieurs stacks ;
- `Use / ApplyStatus(StatusId, Magnitude=1)` : application d'une
  charge selon le DataAsset de statut (durée, puissance, cumul).

Les contraintes AX/AY/AZ restent obligatoires : un seul `UseAction`
lié au vrai `DA_ItemSystem`, objet canonique valide au niveau exact,
`SourceItemQuantityCost=1`, `ActionId/PresentationId` authentiques,
pas de mélange d'effets, ni ligne de vue / projectile / dégâts inventés.
La projection donne `MaximumResolvedTargets=1`. Elle ne permet
pas `Party`, `AllyOrHostile`, `Hostile`, `Area` ou `Cell`.
Coût PA projeté = 0 : le champ PA des `UseActions` n'est toujours
pas authoré. Les coûts en mana et les cooldowns restent autoritaires.

L'évaluation globale du catalogue ne rejette plus un `QuickItem`
de type `Ally` simplement parce que **le lanceur** n'a pas besoin
de l'effet. Elle continue de vérifier le combat actif, le tour,
les ressources, les cooldowns et la validité du profil.
**Le `GridTurnManager` constitue le préflight final**, sur le
personnage réellement sélectionné : une cible morte, absente
ou un effet inutile sont refusés avant paiement.

Pour les `ItemId` canoniques de type `Ally`, le batch refuse
explicitement toute requête avec zéro ou plusieurs index de cible.
Cette garde ne modifie pas la règle historique des autres actions
batch. L'identité `ItemId+ItemLevel` et l'action sont re-résolues
par le catalogue avant l'exécution ; pas de fallback `DA_Item_*`.

## Parcours HUD

Une action de hotbar `TargetingPolicy=Ally` active
`bCombatActionPartyTargetingActive`, **sans exécution immédiate**.
Le joueur clique ensuite sur l'un des **panneaux de personnage
existants** (`UGridCombatActionPanelWidget`) :

1. Les panneaux deviennent hit-testables seulement durant cette
   sélection, sans modifier leur composition UMG ;
2. Le clic gauche transmet l'index du personnage et n'est pas
   propagé au monde (y compris en cas de cible refusée) ;
3. Le HUD récupère une action **fraîche**, portant le même
   `ActionId`, `SourcePolicy`, `SourceDefinitionId` et main,
   puis appelle `RequestCharacterCombatActionOnPartyTargets`
   avec exactement un index ;
4. L'action n'est payée que par l'exécuteur ; sur succès,
   la sélection est fermée. Sur refus, elle reste disponible,
   sous réserve des règles du combat et de l'action ;
5. Échap et fin de tour annulent la sélection. Les modes de
   ciblage `Cell/Area` et `Ally` sont mutuellement exclus.

**Limite de vérification :** le parcours C++ de clic sur
`UGridCombatActionPanelWidget::NativeOnMouseButtonDown`
nécessite une recette PIE du WBP réel pour confirmer que sa
racine est hit-testable et que l'input UMG a priorité sur le
clic souris monde. Aucun widget, hiérarchie, style, texte
ou `.uasset` n'est recréé par C++ pour contourner le Designer.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalAllyUse.Heal`,
`.ApplyStatus`, `.RemoveStatus` :

- construisent chacun un `UGridItemSystemAsset` transitoire
  (catalogues de production clonés) et une action `Ally`
  avec l'effet typé correct ;
- vérifient le `ItemId` connu N0, le refus N21,
  l'authenticité de la liaison action/présentation,
  la quantité dépensée, `MaximumResolvedTargets=1`,
  et l'impossibilité de revenir à `Consume` via
  une vieille définition `Potion` contradictoire ;
- vérifient un groupe de deux personnages dont le
  **lanceur a 30/30 PV** et la **cible 10/30 PV** :
  le catalogue doit permettre la sélection `Ally`
  malgré les PV complets du lanceur ;
- testent `CollectPartyTargets` : l'index 1 est choisi
  explicitement, aucun index implicite n'est ajouté ;
- contrôlent les profils `Heal(8)`, l'application
  de statut (définition de statut séparée et valide)
  et le filtre de retrait sur le personnage cible ;
- garantissent que l'affectation hotbar et la prévisualisation
  ne consomment rien, ni chez le lanceur ni chez la cible.

**Frontière des tests :** le test headless valide
les résolveurs et le contrat de ciblage. Il ne simule pas un
clic GPU dans le WBP réel ni la séquence de combat PIE
avec animation, SFX et panneaux réels.

## Localisation et assets

Checklist de début et fin :
`Localization/Source/UI.csv → ST_UI.uasset → WBP → Game/fr/en`
**inchangée**. Aucun texte visible nouveau, aucune nouvelle
clé, aucune modification des polices ou du Designer.
Tous les noms et descriptions demeurent les `FText`
des DataAssets. Le HUD ne crée aucun TextBlock.
Aucune matérialisation de String Table ni fichier binaire
n'est nécessaire à cette étape.

**Dette suivante :** recette PIE souris et sélection sur le
vrai WBP ; autorisations séparées pour cible Party,
soins d'exploration sur allié, effets composés et autres
familles d'effets. La conversion 06.4 reste différée.

## Adaptation des tests de régression AY/AZ

La suite `Grimrock` exige également une mise à jour **explicite**
des tests historiques qui rejetaient `Targeting=Ally` en AY et AZ.
Ces assertions sont actualisées pour vérifier que BA autorise
désormais `Ally` avec une seule cible, tout en refusant
`Party` (AY/AZ) et `AllyOrHostile` (AZ). Elles ne
doivent pas rester de faux échecs après une évolution approuvée
du contrat. Les anciens rapports restent historiques.
