# ITEM-THROW-MIG01 — Migration de l'autorité de lancer du Shuriken

Date : 16 septembre 2026

Statut : migration terminée par le commit `33f05524037374d2cd4b31efc47a9679d246151f`. Le champ historique `bThrowable` et les outils one-shot décrits ci-dessous ont ensuite été supprimés par `ITEM-THROW-CLEAN01`.

## Pourquoi cette migration précède ITEM-THROW-CLEAN01

Au début de cette migration, `bThrowable` était encore sérialisé dans `DA_Weapon_Shuriken`. Le dernier commit ayant modifié cet asset avant l'introduction de `HandUsage` et `bCombatThrowWeapon` était la réparation TD07.3.5.2 du 28 août 2026.

`HandUsage` et `bCombatThrowWeapon` avaient ensuite été introduits par la généralisation du lancer physique. Le runtime conservait alors ce fallback :

```text
bThrowable
  -> Auto => OneHanded
  -> IsCombatThrowable()
```

Supprimer directement `bThrowable` aurait fait perdre au Shuriken son intention de projectile de combat si le DataAsset n'avait pas d'abord été réparé.

## Cible

Le DataAsset de production doit explicitement porter :

```text
DA_Weapon_Shuriken
  HandUsage          = OneHanded
  bCombatThrowWeapon = true
```

`CompatibleEquipmentSlots = MainHand` reste inchangé.

Le champ legacy `bThrowable` n'a volontairement pas été effacé par ce ticket : il est resté sérialisé jusqu'à ce que `ITEM-THROW-CLEAN01` supprime physiquement le champ C++ après migration. Cela a évité une fenêtre intermédiaire où les régressions historiques dépendant encore du champ legacy seraient devenues rouges.

## Outil one-shot historique

Le test et le script suivants ont servi uniquement à la migration puis ont été supprimés par `ITEM-THROW-CLEAN01`.

Automation :

```text
Grimrock.Items.ITEM_THROW_MIG01.ShurikenAuthorityRepair
```

Elle charge le vrai :

```text
/Game/GrimrockPrototype/Core/DataAssets/Weapons/DA_Weapon_Shuriken
```

puis, si nécessaire :

```text
HandUsage = OneHanded
bCombatThrowWeapon = true
```

et sauvegarde le package via Unreal.

Le test vérifie ensuite que le Shuriken reste :

- physiquement lançable ;
- projectile de combat ;
- équipable en MainHand ;
- une définition valide.

## Script local

```powershell
.\Scripts\RepairItemThrowShurikenAuthority.ps1 -EngineRoot D:\UE_5.5
```

Le script :

1. exige `master` ;
2. exige Git LFS ;
3. refuse un `DA_Weapon_Shuriken.uasset` déjà modifié localement ;
4. lance l'Automation ;
5. stage uniquement le Shuriken ;
6. crée uniquement le commit asset ;
7. pousse `origin/master`.

Aucun autre `.uasset` n'est touché.

## Suite

Après migration et publication du vrai asset, `ITEM-THROW-CLEAN01` a supprimé sans fallback :

```text
UGridItemDefinitionAsset::bThrowable
bThrowable -> OneHanded
bThrowable -> IsCombatThrowable
```

Les autorités finales deviennent alors :

```text
HandUsage          = sémantique physique de main / lancer
bCombatThrowWeapon = sémantique de projectile de combat
CombatActions      = définition de l'action de combat
```
