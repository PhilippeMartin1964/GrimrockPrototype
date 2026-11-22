# ARM-CLEAN06.5 — Conversion des readiness et des niveaux de l'objet

Quatre fichiers de tests sont convertis sans altérer les noms Automation.
Les anciennes définitions DA_Weapon_* et les catalogues
DA_WeaponCatalogue / DA_WeaponVisualCatalogue ne sont plus chargés
par ces tests.

- ARMEMENT01.10 : 18 recettes, 22 armes N1, profils physiques et visuels
  complets, trois pilotes intacts, rapport CSV sous Saved ;
- ARMEMENT01.11 : quatre armes Rogue/Priest de mêlée approuvées,
  permissions MainHand/OffHand, affinités, actions et CSV ;
- ARMEMENT01.9 : six classes, 18 branches et 22 archétypes N1,
  soutiens de départ et politique canonique des deux mains ;
- ARMEMENT01.5 / CLEAN02 : sept régressions de niveaux d'objets,
  N6/N7/N9/N11, propriété ItemLevel non transiente, identité GUID,
  attaque liée sans copie de dégâts, equip/unequip, focus passive,
  armure N1/N20 et refus des niveaux N0/N21 pour les armes.

La sauvegarde de l'objet est exprimée par des métadonnées de
FGridItemInstance, jamais par une copie de chiffres du catalogue.

Aucun .uasset, .sav ou WBP n'est modifié. Les 53 candidats, les deux
Readables enregistrés dans les sept saves v27 et le redirector
ThrowingKnife demeurent protégés.
