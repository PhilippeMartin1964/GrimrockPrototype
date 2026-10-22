# ARMEMENT01.10.1 — Audit de préparation des 22 archétypes N1

Date : 22 octobre 2026.

## Référence validée

Le jalon ARMEMENT01.9.7 est validé localement par le journal utilisateur
`TD04-20261022-132200` : filtre `Grimrock.RPG.ARMEMENT01.9.Kit`,
**6 réussites, 0 avertissement, 0 échec, 0 non exécuté, code retour 0**.
Les 18 recettes portent toutes `ApprovedDesign` ; elles ne sont pas
activées dans le wizard. Les 12 identifiants historiques et SaveGame restent
inchangés.

## Objectif de cette première tranche

Construire une photographie de production **depuis les sources existantes**
avant toute fabrication des 19 définitions manquantes. L'audit est non
destructif, ne modifie aucun DataAsset, ne crée pas d'arme et n'affirme pas
qu'un item soit utilisable en jeu sur le seul critère de présence de son asset.

Autorités :

- Recettes : `docs/Design/ARMEMENT01_9_6_STARTER_KIT_RECIPES.csv` (18 lignes).
- Données N1, affinités et calcul : `DA_WeaponCatalogue` issu du XLSX canonique.
- Présentation : `DA_WeaponVisualCatalogue` et ses ressources réelles.
- Exécution et slots : `UGridItemDefinitionAsset` et runtime existant.
- Localisation affichée : `Localization/Source/UI.csv` → `ST_UI`, pas les champs bruts du XLSX.

À la date du commit de préparation, le répertoire Git versionné
`Content/GrimrockPrototype/Core/DataAssets/Weapons` contient seulement
**trois définitions pilotes N1** des 22 archétypes requis :
`DA_Weapon_Dagger_01` (ARM001), `DA_Weapon_ShortBow_01` (ARM045),
`DA_Weapon_RoundShield_01` (ARM075). Les objets Shuriken et ThrowingKnife
sont **hors périmètre** et ne doivent pas être modifiés.

Les 19 autres archétypes présents dans les recettes approuvées sont :

`ARM002, ARM003, ARM013, ARM014, ARM015, ARM020, ARM025, ARM031,
ARM038, ARM048, ARM054, ARM057, ARM058, ARM061, ARM062, ARM067,
ARM069, ARM070, ARM081`.

Le nombre réel de profils visuels complets n'est **pas déduit des noms
de fichiers** : le test UE interroge le catalogue chargé et vérifie le
chargement effectif de l'icône et du WorldMesh.

## Contrôle UE ajouté

Un test Editor lit les 18 recettes, calcule l'ensemble des 22 IDs, charge
les deux catalogues, scanne les définitions dans le dossier Weapons, puis
écrit un rapport **sous Saved/** :

`Saved/ARMEMENT01/ARMEMENT01_10_ProductionReadiness.csv`

Test :

`Grimrock.RPG.ARMEMENT01.10.Readiness.Audit22N1`

Colonnes du rapport : `ArchetypeId, ItemLevel, VisualMapped, IconLoaded,
WorldMeshLoaded, ItemCount, ItemId, DefinitionValid, ActionChecked, Status`.

Interprétation des statuts :

| Statut | Interprétation |
|---|---|
| `MissingItemAndVisual` | Définition et visuels complets non disponibles |
| `MissingItem` | Visuels disponibles, définition manquante |
| `MissingVisual` | Définition présente, icône ou mesh manquant |
| `InvalidDefinition` | Définition présente mais contrat N1/catalogues invalide |
| `NeedsActionReview` | Objet présent mais action d'attaque ou statut passif non vérifié |
| `DuplicateDefinitions` | Plusieurs définitions pour un même archétype, erreur à résoudre |
| `CandidateForPlaytest` | Prérequis statiques satisfaits, **pas** une validation de gameplay ni artistique |

Le test exige que les trois pilotes restent des candidats valides,
qu'aucun doublon ne soit introduit et que les deux catalogues ne deviennent
pas dirty. Toute nouvelle définition présente mais incomplète fait échouer
le test. Les 19 absences initiales restent **informatives** : elles ne
transforment pas l'audit de couverture en échec artificiel.

**Limite explicite** : une action de tir doit encore démontrer les munitions,
un projectile le jet/récupération, les passifs leur bonus réellement
appliqué et les armes magiques leurs vrais effets. Un candidat statique
n'est pas `ProductionReady`.

## Campagne locale UE5.5.4

Fermer Unreal Editor ; contrôler l'arbre Git avant de tirer master :

```powershell
cd D:\Development\GrimrockPrototype
git status --short
git pull --ff-only origin master

.\Scripts\ValidateUE.ps1 `
    -EngineRoot D:\UE_5.5 `
    -AutomationFilter "Grimrock.RPG.ARMEMENT01.10.Readiness"

Get-Content .\Saved\ARMEMENT01\ARMEMENT01_10_ProductionReadiness.csv
```

**Attendu et non encore attesté** : compilation Development Editor OK ;
1 test réussi, 0 avertissement, 0 échec ; rapport à 22 lignes.
Le contrôle local du rapport utilisateur demeure obligatoire avant
validation de cette tranche. Ne pas supprimer les anciens tests 01.9.

## Séquence de production, à réaliser seulement après cette validation

1. **ARMEMENT01.10.2** — lot Guerrier : ARM014, ARM015, ARM025, ARM031.
2. **ARMEMENT01.10.3** — autres armes de mêlée : ARM002, ARM003, ARM013, ARM020.
3. **ARMEMENT01.10.4** — portée et jet : ARM038, ARM048, ARM054, ARM057, ARM058.
4. **ARMEMENT01.10.5** — magie et passifs : ARM061, ARM062, ARM067, ARM069, ARM070, ARM081.
5. **ARMEMENT01.10.6** — revue des cinq compléments fonctionnels,
   munitions, slots, récupération, consommables, focaliseurs et essais PIE.

Pour chaque lot : silhouettes/icônes **réellement authorées** et vérifiées
à 48×48, matériaux principaux possédés par les Material Slots du mesh,
`WeaponArchetypeId` exact, `ItemLevel=1`, profil Excel conservé, slots
et actions testés. Une silhouette peut être partagée entre niveaux et
archétypes lorsque c'est artistiquement justifié, jamais en substitution
silencieuse d'une arme non créée.

Les `.uasset` seront produits localement dans UE, contrôlés visuellement
avant toute publication Git LFS sélective. **Aucun `git add .`**, aucune
réécriture des 12 kits legacy, pas d'activation des 18 nouveaux kits
pendant la fabrication.
