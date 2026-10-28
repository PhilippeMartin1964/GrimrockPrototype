# ARMEMENT-LOT02.8 — Focus passif et armure magique des équipements

**Contrat moteur :** `ARM070` correspond à `FocusPassive`, objet passif OffHand (sans sort, attaque, parade ou lancer). Son bonus d'armure magique est lu exclusivement depuis le `DA_WeaponCatalogue` N1. Aucun chiffre n'est inventé.

L'armure magique apportée par les équipements est distincte du réservoir magique de base. À chaque coup, le bonus restant sur l'objet est consommé avant la réserve de base ; `AbsorbedEquipmentMagicalArmor` est sauvegardé sur l'instance (sac, équipement et sauvegardes). Rééquiper ne recharge pas la protection. Restauration et fin de combat rechargent les réserves dans leur chemin autorisé, et les portraits utilisent la même valeur effective que le combat.

Le nouveau mode industriel `FocusPassive` n'utilise pas `ShieldPassive` pour fabriquer un objet non conforme. Les boucliers physiques existants restent inchangés ; le N1 ARM081 reste traité via `ShieldPassive`.

## Limites

Pas de `.uasset` fictif : ARM070 reste `UNDECLARED` en attendant son véritable mesh, ses textures, son icône et ses entrées `Planned` au manifeste. Les kits Arcaniste et Transmutateur ne sont pas encore déverrouillés. Les bénéfices spéciaux de focalisation (coût de sorts, concentrations) ne sont pas inférés des données N1.

## Validation locale regroupée

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\PlanStarterWeaponWaves.ps1 -Action Validate -EngineRoot D:\UE_5.5
```

La suite LOT02 passe de 6 à 7 tests, soit 46 au total sur les quatre filtres, sans échec attendu. Une campagne de combat PIE et d'assets artistiques sera ensuite nécessaire.
