# TRAP-INSTANCE01 — Paramètres individuels des pièges et perception silencieuse

**Code préparé sur master ; compilation Unreal Engine 5.5.4 et recette PIE à valider localement.**

## Autorité des données

`UGridWorldObjectDefinitionAsset` = mesh, apparence, comportement par défaut.
`FGridWorldObjectInstance` = un piège réellement placé, avec son propre GUID.
`FGridWorldObjectInstanceConfig::DungeonTrap` = propriétés locales par champ
(option d'override + valeur). Le LevelAsset conserve ces overrides ; le
DataAsset **n'est jamais modifié** depuis les champs de l'inspecteur du Grid Editor.

L'inspecteur montre les valeurs effectives. À la première édition d'un champ,
son indicateur d'override est activé. Les propriétés sans override continuent
d'hériter des valeurs du modèle. Un bouton « Rétablir les valeurs du DataAsset
pour ce piège » efface uniquement les overrides du piège sélectionné.

La résolution unique `GridDungeonTrapInstance::Resolve` est utilisée par la
détection passive, la validation d'état des pièges et les dégâts.
`ResolveDisarmDifficulty` gouverne le test de désamorçage.
La sauvegarde des états dynamiques (révélation, activation, désamorçage) reste
dans le runtime SaveGame et ne doit pas être confondue avec le placement.

## Aucun message lorsqu'aucun danger n'est repéré

La détection passive se poursuit normalement. Si elle découvre réellement un
piège, le message de découverte est conservé. Sinon, aucun message HUD.
Le paramètre obsolète `bShowPassivePerceptionFeedback` est retiré.
Un futur bouton de recherche volontaire est un autre jalon à concevoir.

## Attention aux anciens DataAssets déjà modifiés

Le code antérieur de l'inspecteur a **modifié les valeurs partagées** des
DataAssets existants. Ce jalon ne peut pas deviner les valeurs que l'utilisateur
souhaitait remettre par défaut. Aucune migration destructive : vérifier chaque
DA de piège dans Unreal Editor et restaurer volontairement ses réglages
communs. Les overrides locaux nouvellement édités résident dans le
`DA_GridLevel_XX` correspondant, jamais dans le DA du modèle.

## Recette locale obligatoire

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.TRAP_INSTANCE01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.TRAP01"
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter "Grimrock.Editor.TRAP01.5"
```

En PIE : placer deux pièges depuis la même définition, changer la difficulté,
la portée et les dégâts du premier seulement, sauver le niveau, le recharger,
contrôler la différence entre placements et les jets réels ; vérifier qu'aucun
« aucun danger repéré » ne paraît au HUD.

**À ne pas déclarer validé avant ces preuves.**
