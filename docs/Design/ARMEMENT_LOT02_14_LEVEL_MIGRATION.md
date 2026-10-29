# ARMEMENT-LOT02.14 — Démantèlement Throwing Knife : migration du niveau

## Source du blocage, confirmée dans UE5.5.4

- `DA_GridLevel_00` est l'unique package externe qui référence encore l'ancien `DA_Weapon_ThrowingKnife`.
- Le vieux mesh `SM_ThrowingKnife`, l'icône, la MI et les trois textures PBR 01 ne sont référencés que par cette chaîne legacy.
- Le couteau de remplacement est `DA_Weapon_ThrowingKnife_01` (ARM054, état `Planned`).
- Les deux kits `Warrior_Skirmisher` et `Rogue_Shadow` sont déjà migrés en C++.
- Shuriken et animations GoblinThrower sont hors périmètre.

## Opération automatique limitée à une seule ressource

Lancer UE Editor **fermé**, après copie du présent dossier dans la racine du dépôt :

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\MigrateLegacyThrowingKnife.ps1 -EngineRoot D:\UE_5.5
.\Scripts\MigrateLegacyThrowingKnife.ps1 -EngineRoot D:\UE_5.5 -Apply -SkipBuild
```

Le `-Apply` :

1. vérifie le seul referencer externe autorisé, `DA_GridLevel_00` ;
2. crée une **copie binaire SHA256 de la version locale** sous `Saved/ARMEMENT-LOT02/Backups/` ;
3. remplace les pointeurs dans les placements d'items, les générateurs et les configurations d'objets contenant des références d'items ;
4. ajuste les AcceptedKeyIds correspondants le cas échéant ;
5. sauvegarde **uniquement** `DA_GridLevel_00.uasset` ;
6. effectue un second audit dans un **nouveau** processus UE pour éviter un faux résultat de cache.

Le contenu hors de ces champs, les identifiants `FGuid`, la position, les quantités, les niveaux d'item, les autres assets localement modifiés et les sauvegardes ne sont pas modifiés. Pas de commit automatique. La sauvegarde locale est conservée en cas d'échec.

## Suppression finale (UE Editor)

Une fois la migration vérifiée, relancer :

```powershell
.\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -SkipBuild -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'
Import-Csv '.\Saved\ARMEMENT-LOT02\LOT02_14_LegacyThrowingKnifeReferences.csv' | Select-Object Label,ReferencerCount,Referencers,Decision | Format-List
```

Le test industriel `LegacyThrowingItemsPreserved` exige pour l'instant la présence de l'ancien couteau. Il doit être adapté **dans le même jalon de suppression**, sinon les tests industriels échoueront après effacement.

Pour les sept packages, confirmer dans le Reference Viewer UE l'absence de dépendances actives non legacy. Supprimer ensuite par Content Browser et `Fix Up Redirectors`, de l'ancien DataAsset vers ses ressources. Ne supprimer **ni** `MI_ThrowingKnife_02` / PBR 02 (ARM054) **ni** le Shuriken.

La validation PIE des deux kits et Save/Continue sera nécessaire avant la clôture. Les anciennes sauvegardes ne sont pas silencieusement remappées entre deux ItemDefinitionId.

## Limites

Le code du présent lot n'a pas été compilé dans UE5.5.4 dans l'environnement de livraison. Si un contrôle refuse la migration, **ne jamais forcer** : examiner le log et le backup. Aucun asset binaire n'est livré par ce dossier.
