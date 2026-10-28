# ARMEMENT-LOT02.11.1 — 11 modèles Meshy déclarés `Planned` dans le manifeste

**Source d'observation** : journal utilisateur du 28 octobre 2026,
`AuditStarterWeaponArt.ps1 -NoThrow` : **11/11 lots de 6 packages, 66/66 fichiers, Missing=0**.
Cela prouve uniquement la présence locale. Le chargement Unreal des `.uasset`,
les dimensions d'icônes, les LOD et les paramètres PBR ne sont PAS encore validés.

## Portée du commit

- Onze nouvelles lignes `Planned` dans `Config/Armement/WeaponProductionManifest.csv`,
  avec leurs identifiants d'item, d'attaque/visuel et les vrais chemins fournis par
  `ARMEMENT_LOT02_11_ART_INTAKE.csv`.
- Modes tirés de `ARMEMENT_LOT02_6_MISSING_WEAPON_WAVES.csv` (supportés par le C++).
- `ARM054` conserve volontairement `MI_ThrowingKnife_02` et ses trois textures
  `T_ThrowingKnife_02_*`, alors que le mesh, l'icône et le nouvel Item portent `_01`.
- `ARM070` est un `FocusPassive` en `OffHand`, sans `AttackId` ;
  `ARM081` un `ShieldPassive` en `OffHand`, sans `AttackId`.
- Pour les autres armes : attaque canonique directe ou projectile physique
  selon le BuildKind ; aucun soin/poison/statut/surface imaginaire.
- `ClassId` reflète la classe des recettes approuvées, à l'exception du
  focaliseur ARM070 partagé par Mage et Alchemist (affinité non limitée à une classe).

## Contrôle local UE5.5.4 sans écriture métier

Fermer Unreal Editor avant les commandes ci-dessous.

    cd D:\Development\GrimrockPrototype
    git pull --ff-only origin master
    .\Scripts\AuditStarterWeaponArt.ps1
    .\Scripts\WeaponPipeline.ps1 -EngineRoot D:\UE_5.5 -Action Plan -All
    Import-Csv .\Saved\ARMEMENT-INDUS\INDUS02_Plan_All.csv |
        Where-Object { $_.ArchetypeId -in @('ARM038','ARM048','ARM054','ARM057','ARM058','ARM061','ARM062','ARM067','ARM069','ARM070','ARM081') } |
        Format-Table ArchetypeId,ProductionState,Art,Status,Detail -AutoSize

### Interprétation

Attendu pour chaque nouvelle arme : `ProductionState=Planned`, `Art=READY` et
`Status=VISUAL_MISSING` (visual non encore enregistré), **sans** `CONFLICT`.
Le résultat ne sera confirmé qu'avec le rapport local UE ;
en cas de `ART_MISSING` ou `CONFLICT`, corriger exactement le `Detail` sans forcer `-Apply`.

Le pipeline vérifie le chargement de l'icône, du mesh et de la Material Instance ;
icône carrée >= 256 px, bounds/LOD du mesh, MI assignée à un Material Slot,
textures PBR existantes et sRGB conforme (BC true, N/ORM false).

### Régression facultative en lecture seule

    .\Scripts\ValidateUE.ps1 -EngineRoot D:\UE_5.5 -AutomationFilter 'Grimrock.RPG.ARMEMENT.Industrial'

Les onze nouvelles lignes restent `Planned` et n'ajoutent aucun item au runtime :
l'étape `Visual`, puis `Item`, puis `Palette`, reste explicitement séparée.
L'ancien `DA_Weapon_ThrowingKnife` est préservé : sa suppression nécessitera
la migration des kits `Warrior_Skirmisher` et `Rogue_Shadow`, des tests
`LegacyThrowingItemsPreserved` et la revue des références de sauvegarde/Blueprint.

**Hors périmètre** : modification des 66 binaires `.uasset` non versionnés,
du XLSX modifié localement, du guide Meshy local non suivi, des kits 12/18,
des palettes partagées ou des sauvegardes.
