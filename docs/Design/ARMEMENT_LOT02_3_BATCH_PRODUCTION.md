# ARMEMENT-LOT02.3 — Piloter les 18 kits et fabriquer les armes par lots

**28 octobre 2026 — outil publié côté dépôt, validation Windows PowerShell et UE5.5.4 à confirmer localement.**

## Objectif

Une **commande unique** produit une vue globale des **18 kits N1** approuvés et des **22 archétypes d'armes** qu'ils utilisent, sans remplacer ni activer les 12 kits actuellement affichés dans le wizard. La production future des armes importées peut ensuite être exécutée pour **plusieurs archétypes dans une seule commande**, avec les garde-fous de `WeaponPipeline.ps1` et un commit par arme. Il ne s'agit **pas** d'un mécanisme qui génère de l'art 3D ou des fonctionnalités de combat manquantes.

### Sources d'autorité

- `docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv` : les 18 recettes `ApprovedDesign` ;
- `Config/Armement/WeaponProductionManifest.csv` : états `Active`/`Planned`, chemins effectifs, identité des assets ;
- `WeaponPipeline.ps1` et son commandlet C++ : contrôle réel de l'art, des items, du catalogue et de la palette ;
- DataAssets, gameplay et PIE : arbitres finaux de la jouabilité.

Aucun champ de dégâts, affinité, talent ou poids n'est recopié dans le nouveau script.

## État initial vérifié sur `master`

Onze archétypes sur les vingt-deux sont déclarés `Active` : `ARM001`, `ARM002`, `ARM003`, `ARM013`, `ARM014`, `ARM015`, `ARM020`, `ARM025`, `ARM031`, `ARM045` et `ARM075`.

Onze archétypes restent **non déclarés au manifeste** et ont besoin d'un lot d'assets réellement importés, puis d'un `Planned` avec chemins confirmés : `ARM038`, `ARM048`, `ARM054`, `ARM057`, `ARM058`, `ARM061`, `ARM062`, `ARM067`, `ARM069`, `ARM070`, `ARM081`.

Cinq recettes approuvées peuvent être candidates PIE :
`Kit_Warrior_Guardian`, `Kit_Warrior_Breaker`, `Kit_Warrior_WeaponMaster`, `Kit_Rogue_Shadow`, `Kit_Priest_Protection`.
Les treize autres sont bloquées par des armes, une dépendance fonctionnelle ou les munitions. La disponibilité **au disque** n'atteste pas qu'un DataAsset est valide : le script conserve volontairement le statut `CANDIDATE_FOR_PIE` et délègue l'autorité à UE.

### Problèmes non armement

- **Cinq compléments à produire** : outil de désamorçage, catalyseur de surfaces, consommable de soin, potion/composants d'apothicaire, composants de transmutation. Aucun ID ni effet inventé.
- **Rôdeur Tireur** : `ARM045` (arc court) et `ARM001` (dague) sont actifs, mais aucun `DA_Item_Arrow`/jeu de flèches n'est actuellement versionné. Le code d'attaque du joueur dispose d'une consommation d'**objet source** (arme jetable ou quick item), pas d'une consommation distincte de flèches à partir d'un arc équipé. La recette reste `AMMO_NOT_VERIFIED`, et **aucune munition n'est attribuée fictivement**.
- **Objets existants** : torche et bombe de feu doivent être présents pour leurs recettes respectives ; l'existence du package ne prouve pas à elle seule l'action de combat ou de lumière en PIE.
- La main-gauche a une attaque OffHand validée, mais sa parade spéciale n'est pas implémentée.

## Usage : une commande pour le lot entier

Depuis `D:\Development\GrimrockPrototype` :

```powershell
# Rapport consolidé : sans UE ni mutation des assets.
.\Scripts\StarterKitProductionBatch.ps1 -Action Plan

# Les 18 kits, les 22 armes et les dépendances sont affichés dans Saved/.
Import-Csv .\Saved\ARMEMENT-LOT02\LOT02_3_All22WeaponBacklog.csv |
    Where-Object { $_.Issue -ne 'NONE_REQUIRES_UE_VALIDATION' } |
    Sort-Object KitsAffected -Descending |
    Format-Table ArchetypeId,ManifestState,KitsAffected,KitIds -AutoSize

# **Un seul lancement** pour les quatre filtres de non-régression.
.\Scripts\StarterKitProductionBatch.ps1 -Action Validate -EngineRoot D:\UE_5.5
```

Sorties exclusivement sous `Saved/ARMEMENT-LOT02/` :

- `LOT02_3_All18KitMatrix.csv` : chacune des 18 recettes, **tous** ses blocages au lieu du premier seulement ;
- `LOT02_3_All22WeaponBacklog.csv` : un enregistrement par archétype, état et kits concernés ;
- `LOT02_3_NonWeaponDependencies.csv` : supports existants, compléments à créer et absence de preuve des munitions.

Les quatre filtres UE de `-Action Validate` sont : `Grimrock.RPG.ARMEMENT.LOT02`, `Grimrock.RPG.ARMEMENT01.9`, `Grimrock.RPG.ARMEMENT01.11.1`, `Grimrock.RPG.ARMEMENT.Industrial`. Le script arrête la séquence sur échec et ne masque aucun warning.

## Fabrication future par lots sans répétition manuelle

**Préconditions pour chaque arme** : véritable art importé (`Mesh`, `MI`, `Icon`, `BC`, `N`, `ORM`) ; manifeste amendé en `Planned` avec **chemins réels et commit préliminaire** ; UE Editor fermé ; branche `master` ; index Git vide ; catalogues partagés propres. Aucune ligne `Planned` ne doit être ajoutée depuis un nom d'asset supposé.

```powershell
# Exemple seulement si ARM054 et ARM057 ont été réellement importés et déclarés Planned.
.\Scripts\StarterKitProductionBatch.ps1 -Action Produce `
    -EngineRoot D:\UE_5.5 `
    -Archetypes ARM054,ARM057 `
    -Apply -Push
```

Pour **toute** la sélection, le batch vérifie d'abord les fichiers d'art puis lance `WeaponPipeline -Action Plan -Archetype` et exige `Art=READY`, **avant la première mutation**. Il exécute ensuite, arme par arme : `Visual -Apply`, `Item -Apply`, `Palette -Apply`, `Audit`, puis vérifie `COMPLETE`. Il indexe **uniquement** les sept packages propres à l'arme (six assets artistiques et Item) plus les deux catalogues partagés, effectue un commit Git atomique par arme et, si demandé avec `-Push`, pousse `master` après chacun. Les modifications sans lien (dont `L_MainMenu.umap`) restent hors de l'index. Aucun `git add .` ou `git reset --hard`.

En cas d'échec, le batch **s'arrête et ne fait aucune réparation ou rollback silencieux** : inspecter les log UE et `git status` avant reprise. Si vous souhaitez uniquement préparer le plan, n'utilisez ni `-Action Produce` ni `-Apply`.

Après la fabrication, il reste l'étape distincte `Planned → Active` + narratif APPROVED + tests + recette PIE de chaque arme. Ce lot ne bascule pas l'état de production automatiquement et ne prétend pas qu'une définition `COMPLETE` est un kit de départ actif.

## Clôture et suite

`ARMEMENT-LOT02.3` sera clôturé lorsque le rapport local et les quatre filtres de validation auront été contrôlés. Pour la fabrication proprement dite, produire **par vague d'assets**, pas par modification manuelle de chaque fichier C++. L'activation des 18 kits dans le wizard attend toujours les 11 armes manquantes et les dépendances fonctionnelles, avec non-régression des anciennes sauvegardes.
