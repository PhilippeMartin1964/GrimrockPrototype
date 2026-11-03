# ITEM-VFX01.4A — Audit Niagara hérité et politique LiquidSparkle

**Phase préparatoire au nettoyage structurel (ITEM-VFX01.4B).**
Le retrait des trois champs Niagara dans `FGridLightEmitterConfig` est
**volontairement différé** tant qu'un audit natif UE de tous les
`UGridItemDefinitionAsset` n'a pas confirmé l'absence de références
encore nécessaires. Aucun `UPROPERTY` n'est supprimé durant cette phase.

## 1. Source de vérité fonctionnelle

Décision du 3 novembre 2026 : pour l'instant,
`NS_LiquideSparkle` est utilisé **exclusivement sur la potion de soin**.
La flasque d'huile a déjà été modifiée manuellement dans l'éditeur
par le propriétaire du projet. L'outil doit respecter cette décision
et ne jamais recréer cet effet sur les flasques.

| Asset | LiquidSparkle | Autres VFX | PointLight |
|---|---|---|---|
| `DA_Item_Potion_Healing_01` | Oui, `Always` | Conservés | Conservé |
| `DA_Item_Flask_Oil` | Non | Conservés | Conservé |
| `DA_Item_Flask_Acid` | Non | Conservés | Conservé |
| `DA_Item_Flask_CorrosiveCloud` | Non | Conservés | Conservé |
| `DA_Item_Torch` | Non (`TorchFlame` conditionnelle) | Conservés | Conservé |

La politique ne supprime pas les autres VFX de flasques. Il n'y a
pas d'ajout automatique d'un Niagara lors de l'initialisation
d'un item, d'un empilement, d'un drop ou d'un réceptacle.

## 2. Modifications C++ de cette phase

- `FGridItemLiquidVfxMigration::Run` a été limité à la **seule potion
  de soin**, pour éviter qu'une réexécution de l'outil ITEM-VFX01.2
  ne réintroduise `LiquidSparkle` sur les flasques.
- La vérification de production `VFX01_2.MigratedProductionAssets`
  concerne désormais la potion de soin ; le test recette de
  migration demeure utilisable sur des objets transitoires.
- `UGridItemVfx014AuditCommandlet` vérifie les flasques ciblées
  et inspecte les définitions d'items via l'`AssetRegistry` UE.
- La migration `-Apply` enlève **uniquement** l'entrée
  `EffectId=LiquidSparkle` liée à `NS_LiquideSparkle` dans les
  trois flasques. Si cette entrée est déjà absente (cas de la
  flasque d'huile), le DataAsset n'est pas réenregistré.
- Un effet avec un identifiant `LiquidSparkle` mais un autre
  Niagara, ou le Niagara canonique avec un autre identifiant,
  déclenche un refus explicite au lieu d'une suppression ambiguë.
- Le code de migration ne touche ni aux données lumineuses
  ni à la définition `DA_Item_Potion_Healing_01` ou `DA_Item_Torch`.

## 3. Audit de compatibilité avant suppression des UPROPERTY

L'audit charge toutes les définitions identifiées par
`AssetRegistry.GetAssetsByClass(UGridItemDefinitionAsset, subclasses)`
et recherche :

- les références encore utilisées dans
  `LightEmitter.NiagaraSystem` ;
- les anciens offsets non nuls de
  `NiagaraRelativeLocation` et `NiagaraRelativeRotation` ;
- un `LiquidSparkle` mal placé, ambigu ou absent de la potion ;
- un échec de chargement ou un scan de zéro définitions.

Les trois flasques sont autorisées à contenir un
`LiquidSparkle` **temporairement pendant le dry-run seulement**,
parce que l'outil propose de le retirer. Après
`-Apply`, l'audit est relancé sans cette tolérance.

Les journaux portent le préfixe `[ITEM-VFX01.4A]` et des
messages explicites `BLOCKER_...`, `READY_TO_REMOVE_SPARKLE`,
`ALREADY_CLEAN`, `SCAN` et `AUDIT OK`.

**Limite :** l'AssetRegistry couvre les DataAssets UE de cette
classe et ses sous-classes enregistrées. Des effets stockés hors
`UGridItemDefinitionAsset`, des Blueprints ou des données chargées
depuis un format externe nécessitent un audit séparé avant toute
suppression générale. Cette étape évite de supposer que les
pointeurs Git LFS correspondent à une configuration vide.

## 4. Ordre de validation locale — UE 5.5.4

Ne pas supprimer ou écraser les modifications manuelles de
`DA_Item_Flask_Oil.uasset` avant cette procédure. Le script de
publication prend également en compte le fichier local modifié.

Avec Unreal Editor fermé :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

# Compilation et test de référence
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_1"

# Audit read-only (aucun .uasset modifié)
.\Scripts\ItemVFX014-AuditAndNormalize.ps1

# Normalisation de 3 flasques au maximum + contrôle global strict
.\Scripts\ItemVFX014-AuditAndNormalize.ps1 -Apply

# Tests de la politique actuelle
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_4A" -SkipBuild

# Régressions : potion, torche, éclairage
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_2" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_3" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT0" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT_CONFIG02" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Party.LIGHT01" -SkipBuild

# Après succès de l'audit et de tous les tests uniquement :
.\Scripts\ItemVFX014-AuditAndNormalize.ps1 -Publish
```

La commande `-Publish` ne prend que les trois flasques
autorisées, dont l'huile déjà modifiée manuellement ; elle n'ajoute
aucun autre fichier à l'index Git. Si les tests échouent,
ne pas publier les assets.

## 5. Critères pour ITEM-VFX01.4B

1. Audit global sans aucun `BLOCKER`, avec
   `legacy=0`, `invalidSparkle=0`, `loadFailure=0`.
2. Compilation et suites de tests ci-dessus vertes.
3. Les trois flasques normalisées et les éventuels changements
   manuels de l'huile publiés par Git LFS sur `master`.
4. Vérification des références C++ et Blueprint au vieux canal
   `NiagaraSystem` de `FGridLightEmitterConfig`.

Alors seulement retirer la partie Niagara de
`FGridLightEmitterConfig` et de
`UGridLightEmitterComponent`, simplifier
`UGridPartyIlluminationComponent`, et archiver ou retirer les
outils historiques de migration incompatibles.

**Statut actuel :** code et documentation à valider sur Windows.
