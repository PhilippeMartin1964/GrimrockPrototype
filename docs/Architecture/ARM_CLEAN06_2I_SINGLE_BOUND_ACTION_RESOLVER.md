# ARM-CLEAN06.2I — Résolution unique des actions liées par ID

## Résultat

Le `UGridItemSubsystem` propose maintenant
`ResolveBoundAction(ItemId, ItemLevel, ActionId, OutAction)` sans
accès direct aux anciennes fiches `DA_Weapon_*`. Il résout
l'`ItemId` à travers le registre unique, les
`ItemActionBinding` de l'archétype (ou du consommable), puis
`ActionId` dans `DA_ActionCatalogue` et `PresentationId`
dans `DA_ActionPresentationCatalogue`.

Le résultat `FGridResolvedBoundAction` est **transient** et
contient les mécanismes et la présentation, sans ajout de
propriétés persistantes aux armes. Les échecs sont explicites :
registre absent, ItemId/niveau invalide, action non liée à
l'objet, définition d'action absente, présentation absente.
Aucun raccourci vers les catalogues historiques, aucun
`ActionId` ni `PresentationId` imaginé, aucune fuite d'une
ancienne réponse réussie après un nouvel échec.

## Automation

`Grimrock.RPG.ARM.CLEAN06_2.Canonical.SingleBoundActionResolver`
utilise les catalogues canoniques transitoires existants et
teste épée N20, bouclier sans attaque, munitions, action
inconnue, niveau invalide, ItemId inconnu et consommable à
liaison explicite. Le `GameInstance` et son sous-système
conservent la durée de vie correcte des objets des fixtures.

## Statut d'activation

Ce nouveau point d'entrée est utilisable uniquement lorsque
le `DA_ItemSystem` validé existe et est configuré. Le
runtime historique continue de fournir tout le gameplay
jusqu'à la conversion vérifiée des assets. Le nouveau
résolveur ne se connecte à aucun WBP/acteur/SaveGame et ne
crée **aucune** action standard de lui-même.

Aucun fichier `.uasset`, `.umap` ou SaveGame n'est
touché. Un succès nécessite sur le SHA exact compilation UE
5.5.4 et Automation `Grimrock`, pas seulement la validation
de la structure C++.
