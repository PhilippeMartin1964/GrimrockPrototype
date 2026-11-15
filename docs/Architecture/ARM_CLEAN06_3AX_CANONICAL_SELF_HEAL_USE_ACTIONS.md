# ARM-CLEAN06.3AX — UseActions canoniques : soins directs sur soi

**Parent :** ARM-CLEAN06.3AW (`96e02fa`).
**Portée :** projection transitoire de l'exécution, aucune conversion binaire.

## Écart découvert

Le schéma approuvé expose `UGridConsumableDefinitionAsset.UseActions` et
`FGridCanonicalAction { Kind=Use, Targeting, SourceItemQuantityCost,
ManaCost, CooldownRounds, Effects }`. L'exécuteur de combat et celui
d'exploration consomment encore `FGridCombatActionDefinition` issu
de `UGridItemDefinitionAsset::BuildQuickItemCombatActionDefinition`.

Le validateur du catalogue interdisait aussi `SourceItemQuantityCost`
à toute action non `Attack` — ce qui rendait impossible une potion
réellement consommable. Ce jalon autorise ce coût sur `Use` uniquement,
sans élargir la règle aux autres genres d'action.

## Contrat de la projection strictement supportée

`FGridCanonicalQuickItemUseReadModel` :

1. Reconnaît d'abord le `ItemId` via `HasCanonicalItem`, puis
   résout l'instance **au niveau réellement enregistré**.
2. Vérifie la liaison `UseActions.ActionId/PresentationId` via
   `ResolveBoundAction`. Aucun ancien `DA_Item_*` ni
   `DA_Weapon_*` ne peut fournir une action alternative.
3. Autorise **exactement une** action de soin personnel :
   `Kind=Use`, `Targeting=Self`, `Effects=[Heal(Magnitude > 0)]`,
   `SourceItemQuantityCost=1`, sans arme, munitions, ciblage
   à distance, dégâts, statut ou tout autre effet non transcrit.
4. Projette sans sauvegarde de copie `ActionId`, `ManaCost`,
   `CooldownRounds`, les tags, l'icône et les textes depuis le
   système canonique vers l'ancien format *transient* attendu
   par l'exécuteur QuickItem de combat.
5. **PA = 0 explicitement** : `FGridCanonicalAction` ne comporte
   pour l'instant aucun champ PA pour `Use`; une valeur arbitraire
   serait une deuxième règle métier. La décision d'ajouter un coût
   PA authoré appartient à une évolution distincte du contrat.
6. L'exploration n'utilise ce même profil de soin que lorsque
   `ManaCost=0` et `CooldownRounds=0`. Le moteur existant
   valide l'identité physique du consommable par GUID, le
   personnage, l'effet utile et le stock avant de retirer
   exactement une unité ; pas de dépense de PA hors combat.
7. Dans le catalogue de combat, une action QuickItem est
   reconstruite à partir du registre unique ; le TurnManager
   conserve ses contrôles de tour, mana, cibles, statuts, coût
   de ressource et retrait de l'unité. Le HUD d'exploration
   reconstruit la même projection depuis une instance détenue.

**Arbitrage déterministe :** si un consommable possède plusieurs
actions de soin personnel supportées, le raccourci par `ItemId`
ne peut pas choisir silencieusement « la première » :
projection refusée jusqu'à l'introduction d'une identité
de sélection par `ActionId` dans l'interface d'affectation.

## Compatibilité et limites

- Objets non enregistrés : conservés avec leurs anciens exécuteurs
  `QuickItem`, `Consume`, potion, nourriture, huiles et catalyseurs.
- ARM connus N0/N21, munitions N0, objets sans action `Use`
  supportée : aucun effet de soin fabriqué, aucun recours à
  l'ancienne définition même contradictoire.
- Effets `Damage`, `ApplyStatus`, `RemoveStatus`, `Armor`,
  `Move`, `Surface`, `Trap`, `Teleport` et cibles
  multiples : interdits pour cette projection et laissés à
  la migration typée suivante.
- Les calculs de soins complexes d'anciens QuickItem
  (pourcentages/bonus de talents, statut, résistances,
  transmutation, alchimie) **ne sont pas** implicitement
  appliqués aux nouveaux `Heal(Magnitude)` sans contrat.
- Les nouvelles données canoniques de consommation ne
  sont pas authorées dans les assets de production par ce
  jalon. Le runtime attend la conversion contrôlée 06.4
  pour y placer les potions réelles.

## Tests

`Grimrock.RPG.ARM.CLEAN06_3.CanonicalSelfHealUseActions`
crée un `UGridItemSystemAsset` transitoire en réutilisant
les catalogues valides de production, puis lui ajoute
un `UGridConsumableDefinitionAsset` N0 (MaxStack=5) et
l'action canonique `Use/Heal(8)` avec sa présentation liée.
Les assertions contrôlent :

- le validateur d'action (coût d'objet pour `Use`), le
  résolveur de liaisons et la projection QuickItem;
- la vraie matérialisation de l'inventaire, le menu
  `Consume`, l'affectation et l'identité de la hotbar ;
- la consommation exploratoire d'une seule unité avec
  restitution de 8 points de vie au maximum de vie ;
- le refus sans repli des niveaux invalides et d'effets
  `Teleport`, de soins nuls ou de cibles `Area`.

L'exécution de `UE5.5.4/LocalAuto` et de la suite
`Grimrock` sur le SHA final constitue la validation
automatisée ; elle ne prouve pas un rendu visuel ou un
flux de combat PIE avec les futurs assets convertis.

## UI, WBP et localisation

Checklist entrée/sortie :
`Localization/Source/UI.csv → ST_UI.uasset → WBP → Game/fr/en`
**inchangé** : aucune nouvelle clé, chaîne visible, image,
Widget Blueprint ou cible PO/locres. Le menu, le HUD et
le texte de refus réutilisent leurs traductions existantes.
Aucune matérialisation `ST_UI` ni édition binaire nécessaire.

**Ne pas engager 06.4 tant que les autres catégories
d'effets UseActions et les derniers lecteurs legacy
n'ont pas été qualifiés.**
