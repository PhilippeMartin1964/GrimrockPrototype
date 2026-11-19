# ARM-CLEAN06.5E.4A — Préconditions au retrait de la réhydratation historique

Parent `47a3fad5454d1a48da8b90257568743a9b6e3cf2` : deux statuts Windows SUCCESS.

## Deux défauts observés
- Menu contextuel : construction déjà basée sur FGridCanonicalEquipmentSlotsReadModel, mais exécution Equip exigeait encore FindItemDefinition et CanEquipToSlot de l'ancien asset avant la seconde validation CanEquipItemToSlot. Un objet canonique sans ancien pointeur ou avec vieux asset contradictoire pouvait être affiché équipable et refusé au clic.
- RehydrateOwnedItemDefinitions : ResolveItem autorise des lectures famille N1 non-ARM pour compatibilité, ce qui n'est pas un niveau physique valide dans un SaveGame. La vérification doit être faite avant l'appel du resolver et le remplacement atomique de la map.
 
## Correction
Le menu et l'exécution utilisent la seule autorité métier existante de l'inventaire. Le test couvre le vrai ExecuteInventoryContextActionByIndex avec épée N20 sans ancienne définition et bouclier N1 avec ancien asset Book contradictoire, préserve les GUID/niveaux et refuse un ARM N0. Le test de réhydratation couvre la flèche N1 invalide, zéro appel au resolver, état/caches préservés et maintien de la map antérieure. Aucun changement de SaveGame v27 ou de l'alias famille ResolveItem.

## Suite et blocage architectural
Le retrait des pointeurs au chargement ne peut pas encore être généralisé sans régression Audio/VFX : UGridPlayerAttackPresentationComponent::ResolveProfile ne couvre que les non-ARM détaillés côté registre, puis lit l'ancien PlayerAttackPresentationProfile. Il faut projeter l'action/presentation canonique exacte au moment de l'attaque, ne pas déduire N1, ni relire une arme consommée ou déplacée après le debit; conserver les deux sources du dual wield. Ensuite vérifier les transferts et l'absence d'autres lecteurs, avant suppression de la recherche historique sur identité inscrite.

Contrats UI relus : aucun nouveau texte joueur, aucune nouvelle clé ou nouveau WBP, UI.csv/ST_UI/WBP/Game fr/en inchangés. Aucun package ou sauvegarde modifié, PIE reste à vérifier. Deux statuts exact-SHA avant poursuite.
