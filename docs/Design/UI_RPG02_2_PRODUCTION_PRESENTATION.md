# UI-RPG02.2 — Minimal Production Presentation Catalog

Date : **7 octobre 2026**  
Parent : **UI-RPG02 — Visual Language & Presentation Data**  
État : **VALIDÉ — catalogue matérialisé, `ProductionPresentation` 3/3 et `PresentationData` 5/5**

## Objectif

Créer le premier catalogue de présentation réellement consommable par le futur `WBP_GridSkills`, sans attendre les textures finales.

Asset canonique :

```text
/Game/GrimrockPrototype/Core/DataAssets/UI/RPG/DA_RPGTalentPresentation
```

Il contient **6 classes / 18 branches**, l'ordre gauche-centre-droite, les labels français et des palettes provisoires issues de la direction UI-RPG02 validée.

Les portraits, emblèmes et motifs restent volontairement vides. Ils pourront être renseignés ensuite sans changer l'architecture ni retarder UI-RPG03.

## Ordres canoniques

```text
Guerrier   : Guardian / Breaker / WeaponMaster
Voleur     : Assassin / Shadow / Saboteur
Rôdeur     : Marksman / Hunter / Scout
Mage       : Evoker / Arcanist / SurfaceWeaver
Prêtre     : Restoration / Protection / Exorcism
Alchimiste : Grenadier / Apothecary / Transmuter
```

## Matérialisation

```powershell
cd D:\Development\GrimrockPrototype
.\Scripts\AuthorUIRPGPresentation.ps1 -EngineRoot D:\UE_5.5
```

Le script exige `master` propre, compile l'Editor, exécute `UIRPGPresentationAuthoring`, exige exactement un DataAsset créé/modifié, puis lance :

```text
Grimrock.UI.RPG02.ProductionPresentation
Grimrock.UI.RPG02.PresentationData
```

## Validation production

`ProductionPresentation` vérifie le chargement du catalogue, les 6 classes / 18 branches, la concordance avec les `TalentBranchId` des six `DA_Class_*`, l'ordre visuel et la présence des labels.

## Suite immédiate

Après matérialisation et commit de ce DataAsset :

```text
UI-RPG03.1 — REAL WBP_GridSkills Talent Tree shell
```

Ce sera le premier ticket qui remplace visiblement l'écran texte provisoire actuel.

## Clôture

```text
UI-RPG02.2A : 572caa5485735e7c4535a402a8bdbe3295f20ebe
UI-RPG02.2B : 4780d4a640c314ab493efb2c404946e60ec9ec93

ProductionPresentation : 3/3
PresentationData       : 5/5
Warnings Automation    : 0
Failures                : 0
```

Le DataAsset `DA_RPGTalentPresentation` est désormais versionné et UI-RPG03 peut construire le vrai écran UMG.
