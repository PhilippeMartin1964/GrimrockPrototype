# ITEM-VFX01.4A — Audit clos et politique LiquidSparkle

**Statut : validé le 3 novembre 2026.** Le code de ce jalon a été
publié sur `master`, suivi de trois fichiers de flasques via Git LFS
(commit `f19ab6ea`).

## Décision sur les effets visuels

`NS_LiquideSparkle` est réservé à la potion de soin
`DA_Item_Potion_Healing_01` :
- `DA_Item_Flask_Oil` : pas de `LiquidSparkle` ;
- `DA_Item_Flask_Acid` : pas de `LiquidSparkle` ;
- `DA_Item_Flask_CorrosiveCloud` : pas de `LiquidSparkle`.

La flasque d'huile a été corrigée manuellement avant la migration
.4A ; l'outil a conservé sa configuration. La migration a sauvegardé
les deux autres flasques. Les autres effets éventuels des flasques
et les paramètres PointLight sont conservés.

## Audit UE effectivement exécuté

L'outil `GridItemVfx014Audit` (désormais supprimé après usage) a
chargé les DataAssets de type `UGridItemDefinitionAsset` avec
l'AssetRegistry, lors d'un dry-run et après la sauvegarde :

```text
SCAN items=47 legacy=0 invalidSparkle=0 loadFailure=0
AUDIT OK
```

La compilation UE 5.5.4 et les **17 tests** des suites .1,
.2, .3, .4A et LIGHT ont réussi, sans échec ni avertissement.
Les trois DataAssets ont été publiés dans le commit `f19ab6ea`.

## Suite ITEM-VFX01.4B

Cet audit était la précondition à la suppression du
`UPROPERTY` Niagara historique :
- `FGridLightEmitterConfig::NiagaraSystem`
- `FGridLightEmitterConfig::NiagaraRelativeLocation`
- `FGridLightEmitterConfig::NiagaraRelativeRotation`

La migration .4B archive les résultats, retire les commandlets et
scripts à usage unique et conserve des tests permanents sur les assets
canoniques. L'ancien `-Apply` n'a plus de sens après la suppression
du schéma source.

Voir `ITEM_VFX01_DATA_DRIVEN_ITEM_NIAGARA.md` et
`ITEM_VFX014B_REMOVE_LEGACY_LIGHT_NIAGARA.md`.
