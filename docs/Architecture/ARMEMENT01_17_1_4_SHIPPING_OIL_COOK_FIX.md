# ARMEMENT01.17.1.4.SHIPPING01 — Flaque d'huile absente en Shipping

## Observation opérateur — 21 novembre 2026

L'exécutable `TD04-Shipping-20261121-160515` est validé pour New Game, combat/inventaire, interactions et Save/Continue. La flaque d'huile n'est pourtant pas visible lorsqu'une flasque d'huile est lancée, contrairement à PIE UE 5.5.4. L'archive d'origine est **techniquement réussie** (Build, Cook, Stage, Package, Archive, UAT ExitCode 0) ; la validation de l'apparence est distincte.

## Diagnostic étayé et limite de preuve

`AGridLevelRuntimeActor::OilSurfaceMaterial` est initialisé dans le constructeur C++ via un chemin `FSoftObjectPath` vers `/Game/GrimrockPrototype/Art/Materials/Surfaces/MI_Surface_Oil`. `RefreshCombatOilSurfaceVisuals` et `PlayCombatOilSplashAtCell` appellent `LoadSynchronous()` et ne créent pas de décalque si le matériau manque. `RefreshCombatOilSurfaceVisuals` journalise ce cas.

Les deux assets sources existent dans `Content/GrimrockPrototype/Art/Materials/Surfaces` :
- `M_Surface_Oil.uasset` : matériau maître en domaine Deferred Decal ;
- `MI_Surface_Oil.uasset` : instance utilisée en runtime.

Le précédent `DefaultGame.ini` n'incluait pas explicitement ce dossier parmi les `DirectoriesToAlwaysCook`. Des dépendances Blueprint peuvent parfois suffire ; le journal de Cook précédent ne prouve **ni l'inclusion ni l'absence** de chaque asset. Hypothèse principale : ressource exclue du Cook. Hypothèses alternatives : réception de decal par le sol, RHI, options de rendu. **Ne pas déclarer la cause prouvée avant la vérification native.**

## Correctif limité

1. `Config/DefaultGame.ini` ajoute `+DirectoriesToAlwaysCook=(Path="/Game/GrimrockPrototype/Art/Materials/Surfaces")`. Cela force l'inclusion des deux assets de production et des dépendances du matériau.
2. `Scripts/ValidatePackage.ps1` échoue si `M_Surface_Oil.uasset` ou `MI_Surface_Oil.uasset` manque dans `Saved/Cooked/Windows/GrimrockPrototype/Content/GrimrockPrototype/Art/Materials/Surfaces`. UAT ExitCode 0 seul ne suffit plus.
3. `Scripts/ValidateUE.ps1` vérifie dès le préflight que la règle et les deux sources sont présents, de sorte qu'une régression de configuration ne puisse être publiée avec `LocalAuto=SUCCESS`.

Aucune modification de C++, de matériau natif, de sauvegarde, de carte ou du gameplay. Pas de suppression d'asset dans ce lot.

## Prochain contrôle

Attendre `UE5.5.4/SilentTask=SUCCESS` et `UE5.5.4/LocalAuto=SUCCESS` sur le SHA publié. Puis dans le checkout de développement :

```powershell
cd D:\Development\GrimrockPrototype
git pull --ff-only origin master
.\Scripts\ValidatePackage.ps1 -EngineRoot D:\UE_5.5 -Configuration Shipping
```

Le **nouveau** journal doit comporter `Shipping oil decal cooked asset validation` avec deux `[OK]`. Exécuter uniquement le **nouvel exécutable Shipping** avec un `-UserDir` isolé ; lancer une flasque d'huile et vérifier la flaque durable et l'impact visuel. Si les ressources sont présentes mais la flaque reste invisible, comparer les logs Shipping, options `r.Decals`, supports de decals des matériaux du sol et RHI GPU. Ne pas inventer de résultat visuel à partir des tests `-NullRHI`.
