# ARM-CLEAN06.5G.18 — Retrait du schéma historique du contenu initial

Précondition : 5G.17 validé sur e739471e0adbc307bd524981e065a2cb8547db2e, après contrôle X.58/X.48 des 18 entrées natives (17+1) sans anciens pointeurs.

- Retirer le champ réfléchi ItemDefinition de FGridReceptacleInitialItemConfig et de FGridInitialReceptacleItem, ainsi que le témoin transitoire bHasPersistedCanonicalIdentity. Les seuls champs d'initialisation restants sont l'identifiant, le niveau et la quantité. Ne pas confondre avec FGridContainedReceptacleItem dont les chemins de transfert, de restauration et de présentation font l'objet de lots distincts.
- Les 18 entrées de niveau restent contrôlées, sans normalisation automatique, avec le même ordre et le même manifest réfléchi ; les validations de quantité et de niveau et l'échec sur ID inconnu restent testés.
- X.91 et ITEMCLASS01 contrôlent par réflexion l'absence des propriétés. X.92 conserve présentation ARM/nonARM, Niagara, lumière et refus d'une entrée vide/invalide. X.163 conserve N0/N1/N20, changements d'identité et éditions invalides. X.58 refuse les rangées invalides, et vérifie la copie identique sans modifier les packages. X.2/X.4/W/X.48 valident les vrais identifiants ; le helper TD07 ne classe plus comme dette la suppression volontaire d'une ancienne référence.
- Aucun asset UE, WBP, SaveGame ou texte joueur modifié. La validation native Windows et la recette visuelle 06.6 restent obligatoires.
