# ARMEMENT-LOT02.13.4.1 — Correction conversion en pickup et diagnostic de palette

## Contexte
Validation Windows du 29.10.2026, après le commit `61369032` :
- `Grimrock.Monsters.MON11.Presentation` : **8/8** tests réussis.
- `Grimrock.RPG.ARMEMENT.LOT02` : échec de `RecoverableRangedAmmunitionProjectile` (pickup absent) et de `ArrowPaletteDirectCollectible` (validation de palette et binding direct non conformes).

## Correction runtime
`AGridThrownItemActor::ConvertToWorldPickupAtImpact()` transmet désormais `GetItemDefinitionAsset()` à toutes les variantes de `TryDropItemInstanceAtCell`. Le monde n'a donc pas besoin de résoudre à nouveau la définition de la munition à partir de l'ID lorsque le projectile dispose déjà de la référence canonique. Le GUID, le niveau, la quantité et les autres propriétés de l'instance restent inchangés.

Le test transitoire vérifie également que le pickup récupéré référence exactement la définition source.

## Palette : ne pas modifier silencieusement les assets locaux
La règle de `FGridObjectPaletteEntry` n'est **pas** assouplie :
- `DefaultItemDefinition = DA_Item_Arrows_01` ;
- `DefaultWorldObjectDefinition = None` ;
- `Icon = None` dans l'entrée de palette (l'icône vient de `DA_Item_Arrows_01.Icon`) ;
- `PaletteCategory = Items`.

Le test `ArrowPaletteDirectCollectible` détaille séparément la présence d'une icône dupliquée, une éventuelle WorldObjectDefinition et les messages de validation complète. Ces diagnostics sont nécessaires parce que `DA_ObjectPalette_Default.uasset` est modifié localement, et pourrait contenir d'autres changements qu'il faut préserver.

## Validation locale
Fermer Unreal Editor et exécuter :

```powershell
cd D:\Development\GrimrockPrototype
git fetch origin master
git diff --name-only HEAD..origin/master
git pull --ff-only origin master
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.RPG.ARMEMENT.LOT02'
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.Monsters.MON11.Presentation'
git status --short
```

Si le test palette demeure en échec, inspecter dans Unreal `DA_ObjectPalette_Default > Entries > Item_Arrows_01 > Icon` et supprimer exclusivement l'éventuel doublon, sans retirer l'icône du `DA_Item_Arrows_01`. Vérifier les autres messages d'erreurs dans `Automation.log` avant toute autre modification.

Un test automatisé en monde transitoire ne remplace pas la recette PIE avec projectile visible, tir touché/manqué, ramassage, sauvegarde/rechargement.
