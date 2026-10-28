# ARMEMENT-LOT02.13.1 — Lier les flèches au véritable arc ARM048

Le test local LOT02.13 (28.10.2026) a validé 8/8 tests, 11/11 armes
et 220/220 tooltips. Le seul blocage signalé est
`ARM048: BLOCKED_MISSING_ARROWS_LINK`.

## Contrat réutilisé

`ARM045` consomme déjà `Item_Arrows_01` avec
`FGridRangedAmmunitionPolicy` : refus sans munitions, puis consommation
d'une flèche après acceptation de l'action. Ne pas inventer de second type de flèche.

Dans `GridWeaponPipelineCommandlet.cpp`, une nouvelle production de
`ARM048` renseigne désormais `RequiredAmmunitionItemDefinitionId`.
La comparaison industrielle refuse un `ARM048` dont cette liaison manque.
Pour migrer le **DataAsset ARM048 déjà committé**, le commandlet existant
`GridMarksmanAmmunitionAuthoring` accepte désormais
`-Archetype=ARM048` et modifie seulement le champ de munitions.
Sans `-Archetype`, son ancien comportement ARM045 reste inchangé.

## Procédure UE5.5.4 locale

Fermer Unreal Editor et conserver tous les changements utilisateurs hors périmètre.

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\RepairRecurveBowAmmo.ps1 -EngineRoot D:\UE_5.5
.\Scripts\RepairRecurveBowAmmo.ps1 -EngineRoot D:\UE_5.5 -Apply
```

Le script valide le seul `DA_Weapon_RecurveBow_01` modifiable,
garantit que l'arc court ARM045 et les flèches sont inchangés,
fait l'audit LOT02 et Industrial, puis contrôle que le CSV affiche
`ARM048: ARROWS_LINKED_PIE_REQUIRED`.

Le cas `PRESENT_SAME` est idempotent. Tout conflit arrête l'exécution.
Aucun `git add`, `commit`, `push` ni suppression n'est automatisé.
Les assets de l'ancien `ThrowingKnife`, le XLSX local, le guide Meshy,
les 18 kits et les sauvegardes restent inchangés.

## Après validation

Versionner **uniquement** `DA_Weapon_RecurveBow_01.uasset` dans
un commit séparé, après lecture de `git status`, et pousser master.
La liaison aux flèches est un **prérequis**, pas un test PIE de tir,
de consommation lors d'une véritable attaque ou de rendu de projectile.
ARM048 reste Planned jusqu'à recette PIE explicite.