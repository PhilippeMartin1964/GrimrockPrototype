# ARM-CLEAN06.5 — Gardes LOT02 et Warrior sur ItemSystem

Six sources de tests Editor restent identifiées par leurs noms Automation
existants, mais utilisent uniquement le registre final DA_ItemSystem.
Les anciennes classes UGridWeaponCatalogueAsset et
UGridWeaponVisualCatalogueAsset ne sont plus chargées dans ces suites.

- LOT02.6 : trois projectiles récupérables ARM054/057/058, vitesse,
  consommation 1, N1/N20 et liaison des attaques physiques.
- LOT02.7 : cinq armes magiques et la ShortSpear, attaques à distance
  ou mêlée, coût d'action, deux mains et identités stables.
- LOT02.8 : Orb FocusPassive ARM070, équipement OffHand réel, armure
  magique dépensée et conservation du GUID/de la dette dans le snapshot.
- ARMEMENT01.10 : quatre armes Warrior, leurs trois recettes approuvées,
  bindings main/attaque, profils N1 et association visuelle.
- ARMEMENT01.10.2.1 / 01.11.2 : quatre visuels Warrior et le poignard
  Assassin, maillages, icônes, slots Materials et PBR BC/N/ORM.

Aucun auteur ancien n'est recréé. Aucun package Unreal, blueprint ou
sauvegarde privée n'est modifié. Les protections des 53 candidats et
des deux Readables v27 restent inchangées.
