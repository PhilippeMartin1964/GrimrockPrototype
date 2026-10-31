# TRAP-EDITOR-MAP01 — Pièges configurables et symbole sur la carte

**Statut : code publié, validation UE5.5.4 locale et PIE en attente.**

## Inspecteur Grid Editor

Le type, le sous-type de dégâts, les dégâts minimum/maximum, la difficulté
de détection, la portée maximum (1 à 4 cases) et la difficulté de désamorçage
sont éditables dans l'inspecteur de piège. Ces modifications portent sur le
`UGridWorldObjectDefinitionAsset` partagé par tous ses placements, **pas**
sur une instance unique. Elles sont transactionnelles et marquent le
DataAsset modifié ; enregistrer le DataAsset dans Unreal Editor.

Le mode `OnEnterCell` seul est fonctionnel aujourd'hui. Les champs
sérialisés `bOneShot` et `bAffectsEntireParty` restent présents pour
éviter de casser d'anciennes définitions : les trois pièges provisionnés ont
ces valeurs à `true` par défaut, mais le présent jalon ne réécrit pas
les assets existants. Le sabotage est une mécanique distincte, non assimilée
au désamorçage. La surbrillance reste modifiable sous `DungeonTrap|Reveal`
(TRAP01.6), y compris le Material Instance, CustomDepth et stencil.

Les noms des **nouveaux** DataAssets provisionnés comportent désormais les
accents (« Pointes dissimulées », « Dards dissimulés », « Flammes dissimulées »).
Les DataAssets déjà créés ne sont pas écrasés : corriger leur DisplayName
dans Unreal Editor puis les enregistrer.

## Symbole sur la carte

`EGridMapSymbolKind::Trap` représente le mécanisme par un pictogramme
de pointes dessiné dans le widget Slate existant, sans nouvel asset.
Un piège apparaît **uniquement après exploration de sa case et connaissance
du piège par GUID** : détecté, déclenché, désamorcé ou désactivé.
L'exploration seule ne divulgue pas un secret. Le symbole demeure après
désamorçage. Les définitions, GUID et liens logiques ne sont pas exposés
par le read model aux widgets. La priorité d'affichage Trap/Pit restera
à spécifier lors du prochain jalon.

## Piège de sol pouvant ouvrir une fosse

**Non implémenté par ce commit.** Réutiliser les commandes et événements
existants `Activated -> Open`, `SetPitOpen` et
`TryBeginPitFallAtCell` (PIT01/PIT03). Prévoir une association
explicite vers une fosse sur la même case, validation de l'étage inférieur
et une seule chute. Aucun second système de téléportation.

## Recette

Compilation et tests UE locaux à réaliser :
`Grimrock.Editor.TRAP01.5`, `Grimrock.Map.MON21_6_12`,
`Grimrock.Map.TRAP_MAP01`, `Grimrock.TRAP01`.
PIE : vérifier les trois pièges, le partage des DataAssets, leurs dégâts
et résistances, l'absence de divulgation des pièges cachés et l'affichage
du symbole après détection/désamorçage.
