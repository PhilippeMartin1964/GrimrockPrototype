# ARMEMENT-LOT02.6 — Vague industrielle des armes de jet

**Code à valider sous UE5.5.4.** Le nouveau mode `ThrowingOneHand` prend en charge ARM054, ARM057 et ARM058 au niveau du constructeur industriel, sans modifier leurs données N1. Le constructeur affecte une attaque `RangedAttack` avec ligne de vue, consomme exactement une arme physique, active `bCombatThrowWeapon`, et autorise le lancer depuis le sac via le système QuickItem existant. Les Shuriken et couteaux de jet historiques restent inchangés.

Un nouveau test `Grimrock.RPG.ARMEMENT.LOT02.ThrowingWeaponN1Batch` contrôle les trois profils canon N1, les actions, la physique et l'absence de mutation. Il ne garantit ni animations ni recette PIE.

Le CSV `docs/Design/ARMEMENT_LOT02_6_MISSING_WEAPON_WAVES.csv` couvre les 11 armes encore nécessaires aux 18 kits. Les modes proposés ne sont **pas** ajoutés automatiquement au manifeste officiel ; aucun chemin artistique ni gameplay spécial (poison, magie, focus) n'est inventé.

**Validation consolidée :**
```powershell
git pull --ff-only origin master
.\Scripts\PlanStarterWeaponWaves.ps1 -Action Validate -EngineRoot D:\UE_5.5
```

Le rapport `Saved/ARMEMENT-LOT02/LOT02_6_WeaponWaves.csv` indique les dépendances et les familles de production. Après import de véritables meshes/icons/PBR et revue des modes, les lignes manifest `Planned` pourront être autorisées et la vague d'armes appliquée avec le pipeline existant. Aucun kit du wizard ni DataAsset de production n'est modifié à cette étape.
