# ARMEMENT-LOT02.12 — Fabrication groupée des 11 armes Meshy

Prévalidation utilisateur le 28.10.2026 : 66/66 fichiers présents,
les 11 art sets `Art=READY` et `Status=VISUAL_MISSING`, aucun conflit ;
les onze armes de production existantes restent `COMPLETE`.

Le nouveau lanceur PowerShell réutilise exclusivement le commandlet C++
`GridWeaponPipeline` et ne crée pas un second pipeline d'assets.
Il effectue un seul build Editor, un preflight global, les 33 étapes
Visual/Item/Palette des onze armes, un audit global, puis **une seule**
campagne `Grimrock.RPG.ARMEMENT.Industrial` après les modifications.

## Exécution

Fermer Unreal Editor et rester sur master. Le lanceur ne publie rien dans Git.

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\ProduceStarterWeaponWave.ps1 -EngineRoot D:\UE_5.5

Cette commande contrôle sans créer d'assets. Pour écrire les `.uasset` :

    .\Scripts\ProduceStarterWeaponWave.ps1 -EngineRoot D:\UE_5.5 -Apply

Sorties de chaque commandelet dans `Saved/ARMEMENT-LOT02/LOT02_12_*.csv`
et `.log`. Le batch s'arrête au premier échec, ne masque pas un conflit,
et ne supprime pas des fichiers partiellement produits.

## Invariants

- Les onze entrées restent `Planned` (activation et PIE ultérieurs).
- Aucun des 18 kits, 12 presets historiques ou sauvegardes n'est modifié.
- Ancien `DA_Weapon_ThrowingKnife` inchangé jusqu'à la migration validée.
- La modification locale du XLSX et le guide des 44 vues ne sont pas touchés.
- Aucun `git add`, commit ni push de `.uasset` ; publication sélective après
  audit local validé et décision explicite.
- Aucune compétence spéciale n'est inventée : poison/soin/surface restent
  des contrats à traiter indépendamment de la définition d'attaque N1.

En cas de production partielle, le C++ standard sait reconnaître les
étapes déjà `PRESENT_SAME`. Analyser les logs avant toute relance.
Le statut COMPLETE ne prouve pas les animations, gameplay PIE ou qualité artistique.
