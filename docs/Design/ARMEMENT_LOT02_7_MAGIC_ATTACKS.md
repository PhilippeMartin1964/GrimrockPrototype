# ARMEMENT-LOT02.7 — Armes magiques offensives et lance à deux mains

**Source publiée sur master ; validation UE5.5.4 locale encore nécessaire.**

Le pipeline accepte `MagicOneHand` et `MagicTwoHand`. Ces modes créent uniquement une **attaque directe d'équipement** (type `RangedAttack`, ligne de vue), avec dégâts, type de dégâts, portée et coût en PA issus du catalogue N1. Ils ne créent aucun sort, talent, soin, statut, surface ou mana artificiel.

La lance ARM038 utilise le constructeur `MeleeTwoHand` existant. ARM061, ARM062, ARM067 et ARM069 peuvent utiliser les nouveaux modes, sans préjuger des mécanismes magiques spéciaux. ARM048, ARM054, ARM057, ARM058 et ARM081 bénéficient déjà des modes existants. L'audit générique contrôle main(s), action et ligne de vue. Le N1 canonique reste l'autorité.

**ARM070 reste bloqué.** Le bonus d'armure magique de l'objet passif n'est pas projeté par le système d'équipement. Le détour par ShieldPassive est interdit.

Le fichier `docs/Design/ARMEMENT_LOT02_7_PRODUCTION_MODES.csv` liste les 11 armes, sans chemin d'art fictif et sans modifier leur manifeste.

### Validation locale groupée

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\PlanStarterWeaponWaves.ps1 -Action Validate -EngineRoot D:\UE_5.5
```

Attendu : 6 tests LOT02 + 30 tests ARMEMENT01.9 + 2 tests ARMEMENT01.11.1 + 7 tests Industrial = **45 tests**.

### Étape artistique suivante

Importer les vrais meshes, matériaux, icônes et PBR, puis enregistrer leurs chemins exacts dans le manifeste comme Planned, vérifier les tests UE et les recettes PIE avant toute activation. Aucun kit de création de personnage et aucun DataAsset binaire de production n'est modifié dans ce jalon.
