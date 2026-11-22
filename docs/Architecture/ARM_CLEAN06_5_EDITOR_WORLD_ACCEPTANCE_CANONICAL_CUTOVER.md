# ARM-CLEAN06.5 — Recette Editor canonique des objets dans le monde

## Conversion groupée

Les suites historiques d'acceptation de monde (`LOT02.13.2`),
dague d'assassin (`ARMEMENT01.11.2.2`) et audit d'inventaire
(`ARMEMENT01.15.4`) sont conservées sous leurs noms Automation.

Leurs références et comportements proviennent désormais de
`DA_ItemSystem` via `UGridItemSubsystem`, dans un vrai `GameInstance`
d'un monde de test. Aucun chargement d'anciens `DA_Weapon_*` ou
`DA_Item_*` ne constitue une condition de succès.

- Les 11 armes ARM de l'ancienne matrice de monde subissent encore
  le cycle ramassage, équipement, extraction, redépose, nouveau
  ramassage et tentative de projectile pour les armes concernées.
  Les niveaux N1 et les GUID restent les mêmes. Les sources
  historiques ne sont plus enregistrées dans l'inventaire.
- Les arcs ARM045/ARM048 et les flèches N0 vérifient toujours
  zéro flèche refusé, deux -> une -> zéro, puis refus.
- ARM002 (Assassin Dagger) utilise l'unique identité, son
  archetype N1, son action de main, et les visuels chargés
  depuis le registre, sans catalogues `DA_WeaponCatalogue`
  ou `DA_WeaponVisualCatalogue`.
- L'audit produit toujours sous `Saved/Automation/ARMEMENT01`
  le fichier `ARMEMENT01_15_ActiveItemAudit.csv`, désormais
  avec les **48** identités du registre canonique. La potion
  conserve des assertions strictes sur son icône, son mesh,
  son action et son aptitude à la consommation en exploration.
  Les items incomplets restent consignés en `Review`, pas
  arbitrairement inventés.
- Le décompte et le rendu du rapport ne modifient aucun .uasset.
  Les régressions de gameplay couvertes restent distinctes
  d'une validation PIE ou d'un Cook GPU.

## Garde et reste à faire

Ce lot ne retire aucun des 53 packages restants, ne modifie
aucune sauvegarde v27 et ne modifie aucune donnée auteur. Les
imports de migration obsolètes et les anciennes suites qui y sont
encore attachées nécessitent un autre regroupement avant tout
retrait Editor natif. Les deux Readables cités par les sauvegardes
restent intouchables. Exiger les deux statuts Windows sur le SHA
exact avant une autre transaction; la recette finale comporte
encore AssetRegistry natif, Save/Continue, PIE, Cook et Shipping.
