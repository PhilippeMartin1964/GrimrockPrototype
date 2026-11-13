# ITEM-VFX01.4B — Retrait définitif de l'ancien Niagara de lumière

> **ARM-CLEAN06 — NOUVELLE AUTORITÉ (13.11.2026).** [Modèle cible approuvé](ARM_CLEAN06_ITEM_SYSTEM_NORMATIVE.md) · [Migration](ARM_CLEAN06_MIGRATION_PLAN.md). Les mécaniques et tests historiques restent pertinents ; les schémas DataAssets ou fallbacks anciens ne sont plus des prescriptions de conception. **Cible non encore implémentée ; aucun legacy dans le runtime final.**


**Statut : code livré ; validation UE 5.5.4 locale requise.**
Prérequis prouvés par les logs .4A du 3 novembre 2026 :
47 définitions d'items chargées, zéro ancien Niagara, zéro
`LiquidSparkle` mal attribué, zéro erreur de chargement.
Publication Git LFS : `f19ab6ea`.

## 1. Supprimé du runtime

`FGridLightEmitterConfig` n'est plus qu'un contrat de PointLight :
- suppression de `NiagaraSystem`,
  `NiagaraRelativeLocation`, `NiagaraRelativeRotation` ;
- `HasEmitter()` ne retourne plus vrai pour un Niagara seul ;
- aucun changement aux paramètres de lumière, de scintillement,
  au flag `bDefaultEnabled` ou à la sauvegarde des instances.

`UGridLightEmitterComponent` :
- suppression du pointeur `UNiagaraComponent` ;
- suppression des branches de spawn, refresh et destruction de Niagara ;
- remplacement du masque double canal
  `SetEmitterChannelsEnabled(bool Niagara, bool PointLight)`
  par `SetPointLightPresentationEnabled(bool)` ;
- suppression de `IsNiagaraPresentationEnabled()` ;
- conservation du PointLight, du tick de flicker, des ombres,
  de `SetLightEnabled()` et de `RuntimeConfig`.

`UGridPartyIlluminationComponent` :
- plus de suppression manuelle de champs Niagara avant de copier
  la configuration lumineuse ;
- le proxy reste un PointLight sans Niagara.

`AGrimrockPartyPawn::EquipHeldItem()` :
- masque exclusivement le PointLight physique du HeldItem ;
- les effets de flamme restent attachés au mesh via
  `UGridItemVisualEffectsComponent`.

## 2. Nettoyage editor-only

Les outils ponctuels .2, .3 et .4A (3 commandlets, 3 scripts
PowerShell) ont servi à migrer les fichiers Git LFS.
Ils sont supprimés, afin de ne pas maintenir un système qui
ne pourrait plus relire les champs hérités désormais absents.

Les trois anciennes suites de tests de migration sont remplacées
par des tests **de contrat de données de production**. Les tests
de runtime (potions empilées, torche tenue, Niagara conditionnel)
restent en place.

## 3. Tests et risques

Les nouveaux tests vérifient :
- `Grimrock.Items.VFX01_4B.PointLightOnlySchema` :
  absence des trois anciennes `UPROPERTY`, du champ de composant
  Niagara et des anciennes fonctions de présentation ;
- `Grimrock.Items.VFX01_4B.RuntimeSeparation` :
  un seul Niagara issu de `VisualEffects` et un PointLight ;
  le masque du PointLight ne masque pas les particules ;
  extinction et rallumage conservent le bon état ;
- `Grimrock.Items.VFX01_2` : potion de soin + trois flasques ;
- `Grimrock.Items.VFX01_3` : torche et première personne ;
- `Grimrock.Items.VFX01_4A` : politique `LiquidSparkle`
  sur tous les DataAssets chargés dans Unreal.

**Risque de compatibilité :** les Blueprints binaires pourraient
contenir d'anciens appels à `SetEmitterChannelsEnabled` ou
des chemins de propriétés obsolètes. Une compilation UE et des
tests C++ ne suffisent pas à démontrer que tous les graphes
Blueprint sont valides. Vérifier les Blueprints pertinents,
dont `BP_GrimrockPartyPawn`, ainsi que la présentation en PIE.

Aucune re-sauvegarde automatique des assets n'est effectuée
par ce jalon ; aucun `.uasset` n'est généré depuis l'environnement
de développement distant.

## 4. Validation Windows

Fermer Unreal Editor, puis :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master

# Build + tests spécifiques à la suppression du schéma historique
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_4B"

# Tests conservés
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_1" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_2" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_3" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.VFX01_4A" -SkipBuild

# Lumière et équipement
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT0" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Items.LIGHT_CONFIG02" -SkipBuild
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter "Grimrock.Party.LIGHT01" -SkipBuild
```

Critères d'acceptation : build sans erreur ; toutes les suites
précédentes vertes ; Blueprint du Pawn et visuels PIE fonctionnels
(tenue en main, monde, réceptacle, extinction, rallumage).

Aucune publication Git LFS n'est nécessaire pour .4B puisque
les DataAssets ont déjà été migrés et validés dans .2, .3 et .4A.
